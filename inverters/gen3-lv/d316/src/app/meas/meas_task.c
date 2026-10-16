// clang-format off
/* RTOS task 3 (every 10 ms): the measurement task.
   Entry point: task3_measure, an RTOS task.
   Data: none of its own here (dc_measure.c owns 0xD140..0xD178).
   Loop: on the slow average event (dc_acc_step, once per grid cycle) it runs the DC measurements
   and checks of dc_measure.c, the insulation test, the MPPT step (pv_mppt_step), the bus filter
   and the boost selection. On its 10 ms tick it posts fault events to task 0 (1 stop, 2 grid,
   3 PV, 5 EPS), and after an MPPT rescan it waits for the bus and then starts the bridge again
   (state_flags bit 13 in op_state 6 with the frequency and PLL locked, bit 12 in op_state 8).
   Neighbours: dc_measure.c, mppt_step.c, op_states.c (fault queries), converter_ctrl.c
   (pv_ctrl_reset), faults.h.
   name: descriptive (was task3_b.c; map/filenames.tsv).
   at: task3_measure 0x3EBD1B
   match: its own view of 0xD147 and 0xD178 is as plain variables (one data page load each), where
   the measurement code has 0xD178 as the last of three words. */
// clang-format on
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "meas.h"
#include "control.h"
#include "state.h"
#include "rating.h"
#include "mppt.h"
#include "F2806x_GlobalPrototypes.h"

/* RTOS task 3. The rescan: rescan 1 clears the PV loops (interrupts off), waits 100 us
   (DSP28x_usDelay 1798, TI's DELAY_US(100) at 90 MHz), sets MPPT flag bit 6 and moves to rescan 2; after 101 more ticks
   (about 1 s) with the bus within 25.0 V of its reference the low byte of CF12 is cleared and the bridge is
   asked to start again. */
void task3_measure(void) {
    unsigned long ev;
    unsigned int n;

    meas_init();
    for (;;) {
        ev = os_wait(0);
        if (ev & EV3_SLOW_AVG_M) {
            n = dc_acc.a.n;
            if (n != 0)
                dc_acc.a.inv_n = 1.0f / n;
            meas_pv();
            meas_pv_power();
            meas_bus_voltage();
            meas_aux_f16();
            meas_iso();
            ntc_temp_check();
            pv_input_detect_run();
            pv_ref_max_update();
            pv_voltage_check();
            pv_power_limit_flags();
            pv_short_check();
            bus_voltage_check();
            temp_check();
            meas_batt_dcdc();
            if (pv_bus_mode == 0 || dc_meas.pv1_v >= 3000.0f) {
                if (iso_check_enable.iso_check_enable == 1 && iso_check_enable.run_mode_req == 5)
                    pv_iso_check();
            } else {
                run_status_bits.bit.b5 = 1;
            }
            pv_mppt_step();
            bus_voltage_filter();
            pv_boost_select();
            pv_power_limit_update();
            eps_bus_voltage_check();
        }
        if (ev & EV_TICK_M) {
            iso_fault_clear();
            if (has_stop_fault() == 1)
                os_post(TASK_STATE, EV0_STOP_FAULT);
            if (has_grid_fault() == 1) {
                run_status_bits.bit.b2 |= fault_words[0].bit.b15 | fault_words[0].bit.b12;
                set_grid_fault_flag(1);
                os_post(TASK_STATE, EV0_GRID_FAULT);
            } else {
                set_grid_fault_flag(0);
            }
            if (has_pv_fault() == 1) {
                set_pv_fault_flag(1);
                os_post(TASK_STATE, EV0_PV_FAULT);
            } else {
                set_pv_fault_flag(0);
            }
            if (has_eps_fault() == 1)
                os_post(TASK_STATE, EV0_EPS_FAULT);
            if (mppt.rescan == 1) {
                DINT;
                pv_ctrl_reset();
                EINT;
                DSP28x_usDelay(1798);
                mppt_flags.bit.b6 = 1;
                mppt.rescan = 2;
                mppt_flags.bit.b4 = 0;
                mppt_flags.bit.b5 = 0;
            } else if (mppt.rescan == 2) {
                if (++rescan_settle_count > 100) {
                    rescan_settle_count = 0;
                    if (dc_meas.bus_v + 250.0f > dc_meas.bus_v_ref) {
                        fault_words[0].all &= 0xFF00;
                        mppt.rescan = 0;
                        if (op_state == OP_GRID_BATT) {
                            if (grid_freq_detected == 1 && pll_locked == 1)
                                state_flags.bit.b13 = 1;
                        } else if (op_state == OP_EPS_BATT) {
                            state_flags.bit.b12 = 1;
                        }
                    }
                }
            }
        }
    }
}
