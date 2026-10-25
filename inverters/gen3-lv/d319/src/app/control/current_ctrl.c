// clang-format off
/* The battery converter's current controller, run from RAM inside control_isr: the EPWM5
   buck/boost between the DC/DC high side (hv_v, ADC c6) and the DC link (bus_v, c5). It regulates
   the high-side current hv_i (ADC c9), not the battery current (docs/names/C-notes.md, finding 4).
   Entry point: current_controller(&dcdc_cc), called by converter_mode (converter_ctrl.c), which
   copies the references and limits into the block at 0xC240 first.
   Data: cc_i_work (0xCCBE), the working current, a variable of this file (see match: below).
   Loop structure: the outer loops run only on ctrl_slot 4 (one sample in eight, 2.4 kHz); the
   inner current PI runs every sample (19.2 kHz).
   Units: bus and battery voltages in 0.1 V; currents in 0.1 A on the high side (hv_i is
   meas_fast.hv_i x 0.1). Positive current is charge (B's sign convention for the power command);
   that the measured hv_i is negative when charging is an inference from pi_i.e = i_ref + hv_i.
   Neighbours: converter_ctrl.c (converter_mode, dcdc_ctrl_init, dcdc_ctrl_reset), pi_ctrl.c,
   power_limits.c (task 2 writes the references: isr_limits at 0xC208.., the bounds at 0xC212..).
   name: descriptive (was cur_ctrl.c; map/filenames.tsv).
   at: current_controller 0x3F1C54 (load address; it runs at 0x8000)
   match: the limit at 0xCCBE is a variable of this file, which the compiler reaches as its own:
   one data page load per run, reloaded after each join. The flag at 0xC268 says the output sits at
   its upper bound. */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

float cc_i_work = 0;
#define CC_I_WORK cc_i_work
/* clamp v to [min, max] through a copy of v */
#define SAT_TERNARY(v, max, min)                                                                   \
    {                                                                                              \
        float t_ = (v);                                                                            \
        (v) = (t_ > (max)) ? (max) : __fmax(t_, (min));                                            \
    }
#define SAT_IFELSE(v, max, min)                                                                    \
    if ((v) > (max))                                                                               \
        (v) = (max);                                                                               \
    else                                                                                           \
        (v) = __fmax((v), (min));

#pragma CODE_SECTION(current_controller, "ramfuncs");
/* One step of the battery converter's current control. On slot 4:
   - Start: until start_ok, pi_bus_sync works on the bus error (clamped to +-5 = 0.5 V while the
     error is 8.0 V or more); start_ok is set once |bus_v_ref - bus_v| < 80 (8.0 V).
   - mode 1 (converter modes 5 and 6, grid): the command i_cmd from task 2 is slewed by at most
     1.0 (0.1 A) per slot and limited to +-i_cmd_max (isr_limits.hv_current_max, 15.0 A). Two band
     PIs act only outside bus_v_ref +- bus_band (10.0 V): pi_bus_hi (output -30..0 A) when the bus
     is too high, pi_bus_lo (0..30 A) when it is too low.
   - mode 2 (converter modes 7 and 8, EPS): i_cmd is 0 and pi_bus regulates the bus voltage (error
     clamped to +-50.0 V). In BMS mode with the battery full (CF19 bit 8) a negative output, which
     would charge, is reset to 0.
   - The sum i_cmd - pi_bus.u - pi_bus_sync.u, limited to +-i_cmd_max, less the band PIs, is
     limited to +-i_limit (30.0 A).
   - Not in BMS mode (battery_type, HR54, = 0): battery voltage limits. While the current charges
     (> 0), pi_v_max on batt_v_max - batt_v caps it; otherwise pi_v_min on batt_v_min - batt_v
     floors it. batt_v_max/min are 0xC208/0xC20A from task 2 (ctl_voltage_limits_set): 56.0/46.0 V,
     or 50.0/46.0 V (aging_test_mode 1) and 54.0/46.0 V (mode 2). In BMS mode these PIs are skipped,
     so this controller has no battery voltage ceiling then (docs/subsystems/control.md).
   - Then the charge and discharge bounds (chg_bound, dis_bound at 0xC212/0xC214, task 2) and the
     full-battery trim i_offset; at_chg_bound is set at the charge bound, cleared 1.0 A below it.
   Every sample: pi_i on i_ref + hv_i (kp 0.3, ki 0.04, +-2343) plus the feed-forward duty_ff
   (pwm_period x hv_v / bus_v, meas_avg8) is the EPWM5 duty. */
void current_controller(dcdc_cc_t *s) {
    if (s->slot == 4) {
        if (s->start_ok == 1) {
            s->pi_bus_sync.e1 = 0;
            s->pi_bus_sync.i = 0;
            s->pi_bus_sync.u = 0;
            if (s->mode == 1) {
                SAT_IFELSE(s->i_cmd, s->i_cmd_prev + 1.0f, s->i_cmd_prev - 1.0f);
                SAT_IFELSE(s->i_cmd, s->i_cmd_max, -s->i_cmd_max);
                s->i_cmd_prev = s->i_cmd;
                s->pi_bus_hi.e = s->bus_v_ref + s->bus_band - s->bus_v;
                pi_step(&s->pi_bus_hi);
                s->pi_bus_lo.e = s->bus_v_ref - s->bus_band - s->bus_v;
                pi_step(&s->pi_bus_lo);
            } else {
                s->i_cmd = 0;
                s->i_cmd_prev = 0;
                s->pi_bus_hi.e1 = 0;
                s->pi_bus_hi.i = 0;
                s->pi_bus_hi.u = 0;
                s->pi_bus_lo.e1 = 0;
                s->pi_bus_lo.i = 0;
                s->pi_bus_lo.u = 0;
            }
            if (s->mode == 2) {
                s->i_cmd = 0;
                s->pi_bus.e = s->bus_v_ref - s->bus_v;
                SAT_TERNARY(s->pi_bus.e, 500.0f, -500.0f);
                pi_step(&s->pi_bus);
                if (s->bms_mode != 0 && s->batt_full == 1 && s->pi_bus.u < 0.0f) {
                    s->pi_bus.e1 = 0;
                    s->pi_bus.i = 0;
                    s->pi_bus.u = 0;
                }
            } else {
                s->pi_bus.e1 = 0;
                s->pi_bus.i = 0;
                s->pi_bus.u = 0;
            }
        } else {
            s->pi_bus_sync.e = s->bus_v_ref - s->bus_v;
            if (s->start_ok == 0) {
                if (s->pi_bus_sync.e > -80.0f && s->pi_bus_sync.e < 80.0f) {
                    s->start_ok = 1;
                } else {
                    SAT_TERNARY(s->pi_bus_sync.e, 5.0f, -5.0f);
                }
            }
            pi_step(&s->pi_bus_sync);
        }
        {
            float t = s->i_cmd - s->pi_bus.u - s->pi_bus_sync.u;
            CC_I_WORK = t;
            CC_I_WORK = t > s->i_cmd_max ? s->i_cmd_max : __fmax(t, -s->i_cmd_max);
        }
        CC_I_WORK += -s->pi_bus_hi.u - s->pi_bus_lo.u;
        CC_I_WORK = CC_I_WORK > s->i_limit ? s->i_limit : __fmax(CC_I_WORK, -s->i_limit);
        if (s->bms_mode == 0) {
            if (CC_I_WORK > 0.0f) {
                s->pi_v_max.e = s->batt_v_max - s->batt_v;
                pi_step(&s->pi_v_max);
            } else {
                s->pi_v_min.e = s->batt_v_min - s->batt_v;
                pi_step(&s->pi_v_min);
            }
            CC_I_WORK =
                CC_I_WORK > s->pi_v_max.u ? s->pi_v_max.u : __fmax(CC_I_WORK, s->pi_v_min.u);
        }
        CC_I_WORK = CC_I_WORK > s->chg_bound ? s->chg_bound : __fmax(CC_I_WORK, s->dis_bound);
        if (CC_I_WORK >= s->chg_bound) {
            s->at_chg_bound = 1;
        } else if (CC_I_WORK <= s->chg_bound - 10.0f) {
            s->at_chg_bound = 0;
        }
        CC_I_WORK = -s->i_offset + CC_I_WORK;
        s->i_ref = CC_I_WORK;
    }
    s->pi_i.e = s->i_ref + s->hv_i;
    s->duty = pi_step(&s->pi_i) + s->duty_ff;
}
