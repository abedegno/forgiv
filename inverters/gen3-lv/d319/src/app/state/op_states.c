// clang-format off
/* The operating states that task 0 runs: each waits for events and returns the next state.
   at: state0_idle 0x3EDDCF; state1_start 0x3EDE45; state3_fault 0x3EE0AF; state4_update 0x3EE0DA; converters_stop 0x3EE109;
       state_init 0x3EE42B; state8_eps_batt 0x3EE054; state7_eps_pv 0x3EDFEE; state6_grid_batt 0x3EDF57;
       state5_grid_pv 0x3EDE87; relay_test_step 0x3EE468;
   and the fault queries and the start-up sequencer's helpers:
       has_pv_fault 0x3EE5CD; has_grid_fault 0x3EE5D9; has_stop_fault 0x3EE5EE; has_fault_cf12_low 0x3EE5FF;
       has_eps_fault 0x3EE608; has_batt_fault 0x3EE611; set_grid_fault_flag 0x3EE623; set_pv_fault_flag 0x3EE62A;
       relay_test_abort 0x3EE6E6; soft_start 0x3EE633 (the start-up sequencer)
   and the run states 5..8 and their checks: start_grid_pv 0x3EE138; start_grid_batt 0x3EE228;
       start_eps_pv 0x3EE2C0; start_eps_batt 0x3EE392; grid_in_range 0x3EE577; pv_boost_precharge 0x3EE4DB
   match: pv_boost_precharge matches only with a 16.x compiler (16.3 .. 17.3), not with 15.12.7:
   15.12.7 reaches pv_phase[1].cmp through *+XAR4[AR1] where the image computes base + 98 in XAR7.
   The tree builds every file with 16.9.6 (toolchain.toml).
   This file defines the state machine's variables 0xCF08..0xCF33. Any change to this file can
   move them: rerun python3 tools/ebss_solve.py src/app/state/op_states.c

   Job: the operating state machine's states (op_state 0xCF0D): 0 idle, 1 start, 3 fault, 4
   firmware update, 5 grid with PV, 6 grid on battery only, 7 EPS with PV, 8 EPS on battery only.
   The ARM asks for a run mode (dsp_settings.run_mode_req, 0xD8EA); state 1 runs that mode's start
   checks (start_grid_pv .. start_eps_batt), which count down connect_countdown, soft-start the
   battery converter, run the relay test and close the relays. The run states keep the converter
   mode (conv_mode) and fall back to idle on a stop, a fault or a new mode; a grid fault in 5 or 6
   switches to EPS (7 or 8). Entry points: the state functions (task0_state_machine, state_task.c),
   soft_start, the fault queries (has_*_fault, used by most tasks),
   set_grid_fault_flag/set_pv_fault_flag (meas_task.c). Data it owns: op_state, state_flags
   (0xCF0E), the soft-start words, fault_words (0xCF12 .. 0xCF19, cleared by state_init) and
   power_limits (0xCF1A ..). Neighbours: power_limits.c (start_permit, soft_start_enable, batt
   power), meas_task.c (fault events), converter_ctrl.c (conv_mode, dcdc_soft_duty), ac_measure.c
   (relays), mppt.c. Event bits and states: os.h EV0_*, state.h OP_*. One pass = 20 ms.
   name: descriptive (map/filenames.tsv: src/state_b.c, the operating states and their checks). */
// clang-format on
/* match: this file reaches the BMS page as union bms_page_u (named words and the first three
 * as an array), not as the volatile struct the other files see; with the struct view the
 * accesses would all be volatile and the code would differ. */
#define OWN_bms
#define OWN_eps_cfg_bits /* match: this file sees 0xD7C0 as one bit word; the header has the three-word struct eps_cfg_bits_t (0xD7C0 .. 0xD7C2) the C files use */
#include <string.h>
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "meas.h"
#include "power_limits.h"
#include "grid.h"
#include "rating.h"
#include "init.h"
#include "boot.h"
#include "F2806x_GlobalPrototypes.h"
#include "state.h"
#include "F2806x_Gpio.h"
#include "mppt.h"
#include "control.h"
extern union bits16_u eps_cfg_bits; /* match: own form, see OWN_eps_cfg_bits */
extern union bms_page_u bms;        /* match: see OWN_bms above */

#define DELAY_US(A)                                                                                \
    DSP28x_usDelay(((((long double)A * 1000.0L) / (long double)11.111L) - 9.0L) / 5.0L)

// clang-format off
/* .ebss begin */
union bits16_u state_flags;  /* 0xCF0E: bit 0 grid fault, 1 PV fault, 3 relay test done, 12/13 bridge start for EPS/grid */
unsigned int soft_start_state;  /* soft_start step 0..3 */
unsigned int precharge_duty = 0;  /* pv_boost_precharge compare value, to 700 */
unsigned int standby_count = 0;  /* state0_idle, modes 5/6: 500 passes */
unsigned int soft_start_step = 1;  /* duty step: 1, or 50 while the HV side tracks */
unsigned int op_state = OP_IDLE;  /* 0xCF0D */
unsigned int soft_start_timeout = 0;  /* 8000 calls = CF15 bit 5 */
unsigned int soft_start_duty = 0;
unsigned int board_id = 0;  /* GPIO32/33, see state0_idle */
unsigned int start_wait_count = 0;  /* state0_idle: waits 1500 for the battery to wake */
union bits16_u fault_words[8];  /* 0xCF12 .. 0xCF19: CF12 .. CF19 (faults.h) */
struct power_limits_t power_limits;
/* .ebss end */
// clang-format on

/* State 1: on each tick run the start check of the requested run mode (5..8) until it returns a
   state; mode 0 goes to idle and 3 to fault; other modes wait. A stop or update event leaves at
   once. */
void state1_start(void) {
    unsigned long ev;
    unsigned int next;
    epwm_trip_clear();
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV0_STOP_FAULT_M) {
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV0_STOP_M) {
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV_TICK_M) {
            switch (dsp_settings.run_mode_req) {
            case 0:
                next = 0;
                break;
            case 3:
                next = 3;
                break;
            case 5:
                next = start_grid_pv();
                break;
            case 6:
                next = start_grid_batt();
                break;
            case 7:
                next = start_eps_pv();
                break;
            case 8:
                next = start_eps_batt();
                break;
            default:
                continue;
            }
            if (next != 1) {
                op_state = next;
                return;
            }
        }
    }
}

/* State 3: stop everything and wait until the stop faults (CF17, CF18 low byte) clear and the
   ARM no longer asks for mode 3, then go idle. */
void state3_fault(void) {
    unsigned long ev;
    DINT;
    inv_off();
    pv_stop_all();
    EINT;
    eps_vctl_init();
    pv_ctrl_init();
    dcdc_ctrl_init();
    grid_ctrl_init();
    converters_stop();
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV_TICK_M) {
            if (dsp_settings.run_mode_req != 3 && has_stop_fault() == 0) {
                op_state = OP_IDLE;
                return;
            }
        }
    }
}

/* State 4: stop everything, then every 101 ticks post EV0_BOOT to itself, which runs the boot
   loader's update (boot_update) with interrupts off. It never returns. */
void state4_update(void) {
    unsigned long ev;
    unsigned long n = 0;
    DINT;
    inv_off();
    pv_stop_all();
    EINT;
    eps_vctl_init();
    pv_ctrl_init();
    dcdc_ctrl_init();
    grid_ctrl_init();
    converters_stop();
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_BOOT_M) {
            DINT;
            boot_update();
            EINT;
        }
        if (ev & EV_TICK_M) {
            if (++n > 100) {
                os_post(TASK_STATE, EV0_BOOT);
                n = 0;
            }
        }
    }
}

/* Turn the inverter bridge and the PV boosts off, abort the relay test, open relays b and c,
   and reset the converter mode, the soft start and the power factor (1.000). */
void converters_stop(void) {
    DINT;
    inv_off();
    pv_stop_all();
    relay_test_abort();
    relay_cmd.relay_b = 0;
    relay_cmd.relay_c = 0;
    EINT;
    ac.pf_x1000 = 1000;
    mppt_flags.all &= 0xFFCF;
    run_status_bits.bit.b4 = 0;
    soft_start_state = 0;
    soft_start_phase = 0;
    conv_mode = 0;
    state_flags.bit.b15 = 0;
    ac.pf_code = ac.pf_x1000 * 10 + 10000;
}

/* Called once by task 0: clear the relay, state, fault and power-limit words, then set CF18
   bit 2 (ARM link lost), CF16 bit 5 (no PV), CF13 bit 15 and CF15 bit 2 (battery under-voltage)
   until the measurements clear them, and EPS config bits 9 and 12. */
void state_init(void) {
    memset(&relay_ctl, 0, sizeof(relay_ctl));
    memset(&state_flags, 0, sizeof(state_flags));
    memset(fault_words, 0, sizeof(fault_words));
    memset(&eps_cfg_bits, 0, sizeof(eps_cfg_bits));
    memset(&grid_func_flags, 0, sizeof(grid_func_flags));
    memset(&run_status_bits, 0, sizeof(run_status_bits));
    memset(&power_limits, 0, sizeof(power_limits));
    fault_words[6].all |= FS2_ARM_LINK_LOST;
    fault_words[4].all |= 0x0020;
    fault_words[1].all |= 0x8000;
    fault_words[3].all |= FB_UNDER_V;
    cfg_d800.w1 = 0;
    eps_cfg_bits.all |= 0x1200;
    op_state = OP_IDLE;
    dsp_settings.run_mode_req = 0;
    cfg_d800.w2 = 0;
    cfg_d800.pv_bus_mode = 0;
}

/* State 8, EPS on battery only: keep the soft start going (conv_mode 8) while soft_start_enable
   is 1; leave on a stop, update or EPS fault, or when the run mode changes (7 goes to state 7). */
void state8_eps_batt(void) {
    unsigned long ev;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x51;
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x52;
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x53;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV0_EPS_FAULT_M) {
            state_trace = 0x54;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV_TICK_M) {
            if (bms.s.soft_start_enable == 1) {
                if (soft_start() == 1)
                    conv_mode = 8;
                else
                    conv_mode = 0;
            }
            if (dsp_settings.run_mode_req != 8) {
                if (dsp_settings.run_mode_req == 7) {
                    state_trace = 0x55;
                    op_state = OP_EPS_PV;
                    return;
                }
                state_trace = 0x56;
                relay_cmd.relay_c = 0;
                op_state = OP_IDLE;
                return;
            }
        }
    }
}

/* State 7, EPS with PV: as state 8 with conv_mode 7; a PV fault goes to state 8. */
void state7_eps_pv(void) {
    unsigned long ev;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x33;
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x34;
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x35;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV0_PV_FAULT_M) {
            state_trace = 0x36;
            op_state = OP_EPS_BATT;
            return;
        }
        if (ev & EV0_EPS_FAULT_M) {
            state_trace = 0x37;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV_TICK_M) {
            if (bms.s.soft_start_enable == 1) {
                if (soft_start() == 1)
                    conv_mode = 7;
                else
                    conv_mode = 0;
            }
            if (dsp_settings.run_mode_req != 7) {
                if (dsp_settings.run_mode_req == 8) {
                    state_trace = 0x38;
                    op_state = OP_EPS_BATT;
                    return;
                }
                state_trace = 0x39;
                relay_cmd.relay_c = 0;
                op_state = OP_IDLE;
                return;
            }
        }
    }
}

/* State 6, grid on battery only: keep the soft start going (conv_mode 6). On a grid fault close
   relays b and c, stop the bridge, wait one 11 ms tick, start the bridge for EPS (state_flags
   bit 12, EPS config bit 9), clear the SoC-floor EPS latch and go to state 8. */
void state6_grid_batt(void) {
    unsigned long ev;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x47;
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x48;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV0_GRID_FAULT_M) {
            state_trace = 0x49;
            relay_test_abort();
            relay_cmd.relay_b = 1;
            relay_cmd.relay_c = 1;
            DINT;
            eps_vctl_clear();
            pwm34_force_off();
            inv_off();
            EINT;
            os_tcb0.ticks = 0;
            os_tcb0.period = 11;
            os_tcb0.flags.bit.b0 = 0;
            while (os_wait(0xFFFE) == 0) {
            }
            os_tcb0.ticks = 0;
            state_flags.bit.b12 = 1;
            eps_cfg_bits.bit.b9 = 1;
            soc_floor_latch.bit.b3 = 0;
            mppt.rescan = 0;
            dsp_settings.run_mode_req = 8;
            op_state = OP_EPS_BATT;
            os_tcb0.period = 20;
            eps_v_peak = 1.4142135f * ac.eps_v_ref;
            return;
        }
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x4A;
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV_TICK_M) {
            if (bms.s.soft_start_enable == 1) {
                if (soft_start() == 1)
                    conv_mode = 6;
                else
                    conv_mode = 0;
            }
            if (dsp_settings.run_mode_req != 6) {
                state_trace = 0x4B;
                op_state = OP_IDLE;
                return;
            }
        }
    }
}

/* State 5, grid with PV: keep the soft start going (conv_mode 5; also in aging mode 1). The first
   finished soft start sets eps_start_ok (0xC269, the current controller's start_ok). A grid fault
   goes to state 7 as in state 6, but to idle with a battery fault; a PV fault resets the PV
   control; run mode 6 goes to state 6. */
void state5_grid_pv(void) {
    unsigned long ev;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x3D;
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x3E;
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x3F;
            op_state = OP_IDLE;
            return;
        }
        if (ev & EV0_GRID_FAULT_M) {
            state_trace = 0x40;
            relay_test_abort();
            if (has_batt_fault() != 0) {
                op_state = OP_IDLE;
                return;
            }
            relay_cmd.relay_b = 1;
            relay_cmd.relay_c = 1;
            DINT;
            eps_vctl_clear();
            pwm34_force_off();
            inv_off();
            EINT;
            os_tcb0.ticks = 0;
            os_tcb0.period = 11;
            os_tcb0.flags.bit.b0 = 0;
            while (os_wait(0xFFFE) == 0) {
            }
            os_tcb0.ticks = 0;
            state_flags.bit.b12 = 1;
            eps_cfg_bits.bit.b9 = 1;
            soc_floor_latch.bit.b3 = 0;
            mppt.rescan = 0;
            dsp_settings.run_mode_req = 7;
            op_state = OP_EPS_PV;
            os_tcb0.period = 20;
            eps_v_peak = 1.4142135f * ac.eps_v_ref;
            return;
        }
        if (ev & EV0_PV_FAULT_M) {
            state_trace = 0x41;
            DINT;
            pv_ctrl_reset();
            EINT;
            return;
        }
        if (ev & EV_TICK_M) {
            if (bms.s.soft_start_enable == 1 || dsp_settings.aging_test_mode == 1) {
                if (soft_start() == 1) {
                    conv_mode = 5;
                    if (eps_start_ok == 0)
                        eps_start_ok = 1;
                } else {
                    conv_mode = 0;
                }
            }
            if (dsp_settings.run_mode_req != 5) {
                if (dsp_settings.run_mode_req == 6) {
                    state_trace = 0x42;
                    DINT;
                    pv_ctrl_reset();
                    EINT;
                    op_state = OP_GRID_BATT;
                    conv_mode = 6;
                    return;
                }
                state_trace = 0x43;
                op_state = OP_IDLE;
                return;
            }
        }
    }
}

/* One pass of the relay test (relay_test_pass counts to 650): relay a on and off, with the
   test bits state_flags 4 and 5 at passes 75, 200 and 400 when relay_test_enable is 1; at pass 450
   it sets state_flags bit 3 (test done) and starts over. */
void relay_test_step(void) {
    if (dsp_settings.relay_test_pass < 650)
        dsp_settings.relay_test_pass++;
    switch (dsp_settings.relay_test_pass) {
    case 5:
        relay_cmd.relay_b = 0;
        relay_cmd.relay_c = 0;
        relay_test_abort();
        break;
    case 25:
        relay_cmd.relay_a = 1;
        break;
    case 75:
        if (dsp_settings.relay_test_enable == 1)
            state_flags.bit.b5 = 1;
        break;
    case 100:
        state_flags.bit.b5 = 0;
        relay_cmd.relay_a = 0;
        break;
    case 150:
        run_status_bits.bit.b3 = 1;
        break;
    case 200:
        if (dsp_settings.relay_test_enable == 1)
            state_flags.bit.b5 = 1;
        break;
    case 250:
        state_flags.bit.b5 = 0;
        state_flags.bit.b15 = 0;
        break;
    case 300:
        relay_cmd.relay_a = 1;
        break;
    case 301:
        state_flags.bit.b15 = 0;
        break;
    case 400:
        if (dsp_settings.relay_test_enable == 1)
            state_flags.bit.b4 = 1;
        break;
    case 450:
        state_flags.bit.b4 = 0;
        state_flags.bit.b5 = 0;
        state_flags.bit.b3 = 1;
        dsp_settings.relay_test_pass = 0;
        break;
    }
}

/* Any PV fault: CF16. */
unsigned int has_pv_fault(void) {
    return ((fault_words[4].all & 0xFF) | (fault_words[4].all >> 8)) != 0;
}

/* Any grid fault: CF12 high byte, CF13, CF14 low byte. */
unsigned int has_grid_fault(void) {
    return ((fault_words[0].all >> 8) | (fault_words[1].all & 0xFF) | (fault_words[1].all >> 8) |
            (fault_words[2].all & 0xFF)) != 0;
}

/* Any fault that stops the inverter: CF17, CF18 low byte. */
unsigned int has_stop_fault(void) {
    return ((fault_words[5].all & 0xFF) | (fault_words[5].all >> 8) |
            (fault_words[6].all & 0xFF)) != 0;
}

/* CF12 low byte. Not called. */
unsigned int has_fault_cf12_low(void) { return (fault_words[0].all & 0xFF) != 0; }

/* Any EPS fault: CF14 high byte. */
unsigned int has_eps_fault(void) { return (fault_words[2].all & 0xFF00) != 0; }

/* Any battery fault: CF15, or 0xD502 bit 11 (never written, so only CF15 counts). A battery
   fault stops the battery converter (the interrupt code's fault check) and the soft start. */
unsigned int has_batt_fault(void) {
    return ((fault_words[3].all & 0xFF) | (fault_words[3].all >> 8) | bms_status2.bit.b11) != 0;
}

/* state_flags bit 0: a grid fault is present (meas_task.c). */
void set_grid_fault_flag(unsigned int on) { state_flags.bit.b0 = on; }

/* state_flags bit 1: a PV fault is present (meas_task.c). */
void set_pv_fault_flag(unsigned int on) { state_flags.bit.b1 = on; }

/* Stop the relay test: run_status bit 3, state_flags bits 3, 4, 5 and relay a off. */
void relay_test_abort(void) {
    run_status_bits.bit.b3 = 0;
    state_flags.all &= 0xFFC7;
    relay_cmd.relay_a = 0;
}

/* State 0: stop everything, then on each tick read the board id and wait for a run mode. Modes 5
   and 6 start (state 1) after 501 passes (10 s) once the RCMU test has passed (or the RCMU is off).
   Other modes start at once, unless batt_cal_request (HR175) is 1 with the BMS answering: then it
   waits 1501 passes (30 s) for the battery to wake. Mode 4 goes to the update state.
   match: board_id adds GPIO33 twice (<< 2 and << 1), as the image does; GPIO34 was probably meant.
   In modes 7 and 8 it returns on every pass, so the stop and init at the top run again each
   tick. */
void state0_idle(void) {
    unsigned long ev;
    eps_vctl_init();
    pv_ctrl_init();
    grid_ctrl_init();
    dcdc_ctrl_init();
    converters_stop();
    run_status_bits.bit.b7 = 0;
    ac.u_d487 = 0;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            op_state = OP_UPDATE;
            return;
        }
        if (ev & EV0_STOP_FAULT_M) {
            op_state = OP_FAULT;
            return;
        }
        if (ev & EV_TICK_M) {
            board_id = GpioDataRegs.GPBDAT.bit.GPIO32 + (GpioDataRegs.GPBDAT.bit.GPIO33 << 2) +
                       (GpioDataRegs.GPBDAT.bit.GPIO33 << 1);
            if (dsp_settings.run_mode_req == 4) {
                op_state = OP_UPDATE;
                return;
            }
            if (dsp_settings.run_mode_req == 0) {
                standby_count = 0;
            } else {
                rating_apply();
                if (dsp_settings.run_mode_req == 5 || dsp_settings.run_mode_req == 6) {
                    if (++standby_count > 500) {
                        standby_count = 0;
                        if (dsp_settings.rcmu_enable != 1 || run_status_bits.bit.b7) {
                            op_state = OP_START;
                            return;
                        }
                    }
                } else {
                    standby_count = 0;
                    if (fault_words[3].bit.b9 || batt_cal_request != 1 ||
                        ++start_wait_count > 1500) {
                        start_wait_count = 0;
                        op_state = OP_START;
                    }
                    return;
                }
            }
        }
    }
}

/* Start check for mode 5 (grid with PV). It can start without a battery: with a battery fault
   at entry it precharges the bus from PV instead of soft-starting the battery. Per tick: leave if
   the mode or the battery fault state changes or connect_countdown runs out; count down while the
   grid is in range and (soft_start_enable or a battery fault); under 1000 (and the insulation test
   done) soft start (or precharge); under 750, with the bus up, grid frequency found, PLL locked and
   PV present, step the relay test; when it is done and the countdown is 1, return 5. */
unsigned int start_grid_pv(void) {
    unsigned long ev;
    dsp_settings.relay_test_pass = 0;
    dsp_settings.start_fault_entry = has_batt_fault();
    power_limits.precharge_step = 0;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x0B;
            return 4;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x0C;
            return 3;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x0D;
            return 0;
        }
        if (ev & EV0_PV_FAULT_M) {
            state_trace = 0x0E;
            return 0;
        }
        if (ev & EV0_GRID_FAULT_M) {
            state_trace = 0x0F;
            return 0;
        }
        if (ev & EV_TICK_M) {
            dsp_settings.start_fault_now = has_batt_fault();
            if (dsp_settings.run_mode_req != 5 ||
                dsp_settings.start_fault_now != dsp_settings.start_fault_entry) {
                state_trace = 0x10;
                return 0;
            }
            if (dsp_settings.connect_countdown == 0) {
                state_trace = 0x11;
                grid_freq_detected = 0;
                return 0;
            }
            if (grid_in_range() == 1 &&
                (bms.s.soft_start_enable == 1 || dsp_settings.start_fault_now == 1))
                dsp_settings.connect_countdown--;
            if (dsp_settings.connect_countdown < 1000) {
                if ((dsp_settings.iso_check_enable == 1 && run_status_bits.bit.b5) ||
                    dsp_settings.iso_check_enable == 0) {
                    if (dsp_settings.start_fault_now == 1)
                        pv_boost_precharge();
                    else if (soft_start() == 1)
                        conv_mode = 5;
                    else
                        conv_mode = 0;
                }
                if (dsp_settings.connect_countdown < 750) {
                    if ((dc_meas.bus_v + 200.0f > dc_meas.bus_v_ref &&
                         dsp_settings.start_fault_now == 1) ||
                        (conv_mode == 5 && dsp_settings.start_fault_now == 0)) {
                        if (grid_freq_detected == 1 && pll_locked == 1 && !fault_words[4].bit.b5) {
                            if ((dsp_settings.iso_check_enable == 1 && run_status_bits.bit.b5) ||
                                dsp_settings.iso_check_enable == 0) {
                                if (!state_flags.bit.b3)
                                    relay_test_step();
                            }
                            if (state_flags.bit.b3 && dsp_settings.connect_countdown == 1) {
                                mppt_flags.all |= 0x40;
                                power_limits.grid_start_done = 1;
                                rated_ac_power = grid_power_max_pv;
                                return 5;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Start check for mode 6 (grid on battery only): soft start while soft_start_enable; with the bus
   within 20.0 V of its reference count down while the grid is in range and the converter runs;
   under 750 with frequency and PLL, step the relay test; done at countdown 1: rated AC power =
   HR308 (battery rating), return 6. */
unsigned int start_grid_batt(void) {
    unsigned long ev;
    dsp_settings.relay_test_pass = 0;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x15;
            return 4;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x16;
            return 3;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x17;
            return 0;
        }
        if (ev & EV0_GRID_FAULT_M) {
            state_trace = 0x18;
            return 0;
        }
        if (ev & EV_TICK_M) {
            if (dsp_settings.run_mode_req != 6) {
                state_trace = 0x19;
                return 0;
            }
            if (bms.s.soft_start_enable == 1) {
                if (soft_start() == 1)
                    conv_mode = 6;
                else
                    conv_mode = 0;
            }
            if (dsp_settings.connect_countdown == 0) {
                state_trace = 0x1A;
                grid_freq_detected = 0;
                return 0;
            }
            if (dc_meas.bus_v + 200.0f > dc_meas.bus_v_ref) {
                if (grid_in_range() == 1 && conv_mode != 0)
                    dsp_settings.connect_countdown--;
                if (dsp_settings.connect_countdown < 750 && grid_freq_detected == 1 &&
                    pll_locked == 1) {
                    if (!state_flags.bit.b3)
                        relay_test_step();
                    if (state_flags.bit.b3 && dsp_settings.connect_countdown == 1) {
                        state_flags.bit.b13 = 1;
                        power_limits.grid_start_done = 1;
                        rated_ac_power = batt_power_rating.chg_max;
                        return 6;
                    }
                }
            }
        }
    }
}

/* Start check for mode 7 (EPS with PV). connect_countdown is 1500 (15000 with low_batt_wake,
   which also precharges the bus from PV) and counts down while the HV side reads 220.0 V or more;
   leave when it runs out. Soft start while soft_start_enable or low_batt_wake; needs a grid
   fault present (unless low_batt_wake); with no EPS or battery fault, the bus over 320.0 V,
   conv_mode 7 and eps_start_ok, close relays b and c, wait 8 ms and return 7. */
unsigned int start_eps_pv(void) {
    unsigned long ev;
    if (low_batt_wake == 1) {
        dsp_settings.connect_countdown = 15000;
        power_limits.precharge_step = 0;
    } else {
        dsp_settings.connect_countdown = 1500;
    }
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x1F;
            return 4;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x20;
            return 3;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x21;
            return 0;
        }
        if (ev & EV0_PV_FAULT_M) {
            state_trace = 0x22;
            return 0;
        }
        if (ev & EV_TICK_M) {
            if (dsp_settings.run_mode_req != 7) {
                state_trace = 0x23;
                return 0;
            }
            if (bms.s.soft_start_enable == 1 || low_batt_wake == 1) {
                if (soft_start() == 1)
                    conv_mode = 7;
                else
                    conv_mode = 0;
            }
            if (low_batt_wake == 1)
                pv_boost_precharge();
            relay_test_abort();
            if (dsp_settings.connect_countdown == 0) {
                state_trace = 0x26;
                return 0;
            }
            if (dc_meas.hv_v >= 2200.0f)
                dsp_settings.connect_countdown--;
            if (dsp_settings.connect_countdown == 0) {
                state_trace = 0x24;
                return 0;
            }
            if (!state_flags.bit.b0 && low_batt_wake != 1) {
                state_trace = 0x25;
                return 0;
            }
            if (has_eps_fault() == 0 && has_batt_fault() == 0 && dc_meas.bus_v > 3200.0f &&
                conv_mode == 7 && eps_start_ok == 1) {
                relay_cmd.relay_b = 1;
                relay_cmd.relay_c = 1;
                DELAY_US(5000L);
                DELAY_US(3000L);
                state_flags.bit.b12 = 1;
                mppt_flags.all |= 0x40;
                return 7;
            }
        }
    }
}

/* Start check for mode 8 (EPS on battery only), within 1500 passes (30 s, counted while
   soft_start_enable): needs both the grid and PV fault flags; with no EPS or battery fault, the bus
   within 20.0 V of its reference, conv_mode 8 and eps_start_ok, close relays b and c, wait 10 ms
   and return 8. */
unsigned int start_eps_batt(void) {
    unsigned long ev;
    dsp_settings.connect_countdown = 1500;
    for (;;) {
        ev = os_wait(0);
        if (ev & EV0_UPDATE_M) {
            state_trace = 0x29;
            return 4;
        }
        if (ev & EV0_STOP_FAULT_M) {
            state_trace = 0x2A;
            return 3;
        }
        if (ev & EV0_STOP_M) {
            state_trace = 0x2B;
            return 0;
        }
        if (ev & EV_TICK_M) {
            if (dsp_settings.run_mode_req != 8) {
                state_trace = 0x2C;
                return 0;
            }
            if (bms.s.soft_start_enable == 1) {
                if (soft_start() == 1)
                    conv_mode = 8;
                else
                    conv_mode = 0;
            }
            relay_test_abort();
            if (!(state_flags.bit.b1 & state_flags.bit.b0)) {
                state_trace = 0x2E;
                return 0;
            }
            if (dsp_settings.connect_countdown != 0 && bms.s.soft_start_enable == 1)
                dsp_settings.connect_countdown--;
            if (dsp_settings.connect_countdown == 0) {
                state_trace = 0x2D;
                return 0;
            }
            if (has_eps_fault() == 0 && has_batt_fault() == 0 &&
                dc_meas.bus_v + 200.0f > dc_meas.bus_v_ref && conv_mode == 8 && eps_start_ok == 1) {
                relay_cmd.relay_b = 1;
                relay_cmd.relay_c = 1;
                DELAY_US(5000L);
                DELAY_US(5000L);
                state_flags.bit.b12 = 1;
                return 8;
            }
        }
    }
}

/* For the listed grid codes (outside the auto test), 0 when the grid voltage or frequency is
   outside the reconnect window (grid_limits *_c), else 1. */
unsigned int grid_in_range(void) {
    unsigned int ok = 1;
    if (auto_test_state == 0 &&
        (grid_code == 4 || grid_code == 7 || grid_code == 1 || grid_code == 3 || grid_code == 12 ||
         grid_code == 8 || grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
         grid_code == 18 || grid_code == 28 || grid_code == 24 || grid_code == 23 ||
         grid_code == 22 || grid_code == 25 || grid_code == 26 || grid_code == 19 ||
         grid_code == 6 || grid_code == 0)) {
        if (ac.grid_v > grid_limits.v_ac_high_c || ac.grid_v < grid_limits.v_ac_low_c ||
            grid_freq > grid_limits.f_ac_high_c || grid_freq < grid_limits.f_ac_low_c)
            return 0;
    }
    return 1;
}

/* The battery DC-DC soft start, one step per call (20 ms). It runs while the start permit is set
   with no battery fault, or with low_batt_wake, or in aging mode 1; otherwise it resets and
   returns 0. Steps: 0 -> 1 -> 2, then the duty (dcdc_soft_duty) rises by 1 per call, or by 50 while
   the HV side is within 10.0 V of 5.5 x the battery voltage, to 1054; at 1054 (seen only while the
   HV side tracks, or in aging mode 1) it goes to step 3 and returns 1 from then on. 8001 calls
   without finishing (160 s) set CF15 bit 5 (FB_SOFT_START_TIMEOUT), a battery
   fault that nothing clears after start-up (docs/findings-power.md). */
unsigned int soft_start(void) {
    unsigned int done = 0;
    unsigned int t, s;
    if ((bms.s.start_permit == 1 && has_batt_fault() == 0) || low_batt_wake == 1 ||
        dsp_settings.aging_test_mode == 1) {
        s = soft_start_state;
        if (s == 0) {
            soft_start_state = 1;
            soft_start_phase = 0;
            done = 0;
            soft_start_timeout = 0;
            soft_start_duty = 0;
            soft_start_step = 1;
        } else if (s == 1) {
            soft_start_phase = 2;
            soft_start_state = 2;
        } else if (s == 2) {
            t = soft_start_duty + soft_start_step;
            soft_start_phase = 4;
            soft_start_duty = t;
            if (t >= 1054)
                soft_start_duty = 1054;
            else if (t == 0)
                soft_start_duty = 1;
            DINT;
            dcdc_soft_duty = soft_start_duty;
            EINT;
            if (dsp_settings.aging_test_mode == 1)
                goto chk;
            if (!(dc_meas.hv_v < 5.5f * dc_meas.batt_v - 100.0f ||
                  dc_meas.hv_v > 5.5f * dc_meas.batt_v + 100.0f) &&
                low_batt_wake == 0) {
                soft_start_step = 50;
                goto chk;
            }
            soft_start_step = 1;
            goto post;
        chk:
            if (dcdc_soft_duty >= 1054) {
                DINT;
                dcdc_soft_duty = 1054;
                soft_start_phase = 50;
                soft_start_state = 3;
                EINT;
            }
        } else if (s == 3) {
            soft_start_duty = 0;
            done = 1;
        }
    post:
        if (soft_start_state == 3 || low_batt_wake == 1) {
            soft_start_timeout = 0;
        } else if (++soft_start_timeout > 8000) {
            done = 0;
            fault_words[3].all |= FB_SOFT_START_TIMEOUT;
            soft_start_timeout = 0;
        }
    } else {
        soft_start_phase = 0;
        done = 0;
        soft_start_timeout = 0;
        soft_start_duty = 0;
        soft_start_state = 0;
    }
    return done;
}

/* Precharge the bus from PV without the battery (start_grid_pv with a battery fault, start_eps_pv
   with low_batt_wake): the boosts of the present inputs run while the bus is under its reference;
   step 1 raises their compare value by 2 per call to 700 while a PV input reads over 120.0 V, and
   ends when the bus is within 15.0 V of its reference. */
void pv_boost_precharge(void) {
    if (dc_meas.bus_v < dc_meas.bus_v_ref) {
        if (dsp_settings.pv_input_mode == 4 || dsp_settings.pv_input_mode == 5) {
            pv1_boost_on = 1;
            pv2_boost_on = 1;
        } else if (dsp_settings.pv_input_mode == 2) {
            pv2_boost_on = 1;
        } else if (dsp_settings.pv_input_mode == 1) {
            pv1_boost_on = 1;
        } else {
            pv1_boost_on = 0;
            pv2_boost_on = 0;
        }
    } else {
        pv1_boost_on = 0;
        pv2_boost_on = 0;
    }
    switch (power_limits.precharge_step) {
    case 0:
        precharge_duty = 0;
        if (dc_meas.bus_v < dc_meas.bus_v_ref - 150.0f)
            power_limits.precharge_step = 1;
        else
            power_limits.precharge_step = 2;
        break;
    case 1:
        if (dc_meas.pv1_v > 1200.0f || dc_meas.pv2_v > 1200.0f) {
            if (precharge_duty < 700)
                precharge_duty += 2;
            else
                precharge_duty = 700;
            if (pv1_boost_on == 1) {
                pv_phase[0].cmp = precharge_duty;
                pv_phase[0].cmp_pending = 1;
            } else {
                pv_phase[0].cmp = 0;
                pv_phase[0].cmp_pending = 0;
            }
            if (pv2_boost_on == 1) {
                pv_phase[1].cmp = precharge_duty;
                pv_phase[1].cmp_pending = 1;
            } else {
                pv_phase[1].cmp = 0;
                pv_phase[1].cmp_pending = 0;
            }
        }
        if (dc_meas.bus_v >= dc_meas.bus_v_ref - 150.0f) {
            precharge_duty = 0;
            power_limits.precharge_step = 2;
        }
        break;
    case 2:
        precharge_duty = 0;
        break;
    }
}
