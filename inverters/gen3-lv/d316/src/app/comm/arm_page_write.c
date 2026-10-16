// clang-format off
/* arm_page_write.c: the handlers for settings pages 6 and 7 of the ARM's 0x43 frame. Like
   arm_page0..arm_page5 (arm_link.c), each is called once for every byte index 1..42 of a good
   frame whose byte 7 names its page, and stores the field that starts at that index, clamped,
   in the DSP's settings and limits.

   Entry points: arm_page6 and arm_page7, through arm_page_handlers[6] and [7] (arm_link.c).
   Data: none of its own; it writes the settings words at 0xD600 .. 0xD8FF (arm_bms_request,
   power_ramp_rate, charge_target_soc and the rest of arm_link.h's "defined by no source" list),
   batt_power_rating, grid_func_flags and eps_cfg_bits.
   Neighbours: arm_link.c (the frame and the dispatch), battery/bms_link.c (sends
   arm_bms_request to the BMS; the reply goes back on page 5), power/ (the ramp rates and battery
   limits), state/ (op_state).
   name: descriptive (map/filenames.tsv: was arm_page_b.c, range B's page 6 and 7 handlers).
   at: arm_page6 0x3EBAB2; arm_page7 0x3EBB98 (switch table sw_arm_page7 0x3F3F14) */
// clang-format on
#define OWN_eps_cfg_bits /* match: this file sees 0xD7C0 as one bit word; the header has the three-word struct eps_cfg_bits_t (0xD7C0 .. 0xD7C2) the C files use */
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "control.h"
#include "grid.h"
#include "state.h"
#include "power_limits.h"

extern union bits16_u eps_cfg_bits;

/* one object: the code uses one data page for both, here and in current_limit_tapers */

/* Settings page 6 (bytes 8-42; the A316 field in brackets, its offset is byte - 7):
     8-11   not used (calibration gains and offsets 6-8)
     12-13  cal_power, 870..1178               16-17 cal_batt_v (not clamped)
     20     1: copy bytes 21-26 into arm_bms_request[0..5] and set arm_bms_request_pending.
            This is the ARM's own BMS request (device, function 4, start, count); bms_link.c sends
            it and the reply comes back to the ARM in 20-byte pieces on page 5.
     28-29  cal_d819, 870..1178
     30-31  ofr_delay (HR300)                   32-33 q_pt1_tau (HR301)
     34-35  power_ramp_rate_reconnect, 10..30000 (HR302)
     36-37  ofr_recover_delay (HR303)
     38     grid_func_flags bit 0, only 0 or 1 (HR178)
     39     grid_func_flags bit 2, only 0 or 1 (HR179)
     40     pv_mode_cfg, only 0..4 (HR305)
   Bytes 21-26 come back here as i = 21..26 too, but no case takes them. */
void arm_page6(unsigned int reg) {
    unsigned int t;
    switch (reg) {
    case 0x0C:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        cal_power = t;
        limitu(&cal_power, 1178, 870);
        break;
    case 0x10:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        cal_batt_v = t;
        break;
    case 0x14:
        if ((arm_rx_frame[reg] & 0xFF) == 1) {
            arm_bms_request_pending = 1;
            arm_bms_request[0] = arm_rx_frame[0x15] & 0xFF;
            arm_bms_request[1] = arm_rx_frame[0x16] & 0xFF;
            arm_bms_request[2] = arm_rx_frame[0x17] & 0xFF;
            arm_bms_request[3] = arm_rx_frame[0x18] & 0xFF;
            arm_bms_request[4] = arm_rx_frame[0x19] & 0xFF;
            arm_bms_request[5] = arm_rx_frame[0x1A] & 0xFF;
        }
        break;
    case 0x1C:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        cal_d819 = t;
        limitu(&cal_d819, 1178, 870);
        break;
    case 0x1E:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        ofr_delay = t;
        break;
    case 0x20:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        q_pt1_tau = t;
        break;
    case 0x22:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        power_ramp_rate_reconnect = t;
        limitu(&power_ramp_rate_reconnect, 30000, 10);
        break;
    case 0x24:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        ofr_recover_delay = t;
        break;
    case 0x26:
        t = arm_rx_frame[reg] & 0xFF;
        if (t <= 1)
            grid_func_flags.bit.b0 = t;
        break;
    case 0x27:
        t = arm_rx_frame[reg] & 0xFF;
        if (t <= 1)
            grid_func_flags.bit.b2 = t;
        break;
    case 0x28:
        t = arm_rx_frame[reg] & 0xFF;
        if (t <= 4)
            pv_mode_cfg = t;
        break;
    }
}

/* Settings page 7 (the A316 field in brackets):
     8-9    power_ramp_rate, 10..30000 (HR306)
     10-11  grid_v_nominal, only when 2500 or less and below OP_UPDATE (HR307)
     12-13  batt_power_rating.chg_max = word x 10, 10000..60000; dis_max = chg_max + 3000 (HR308)
     14-15  batt_max_charge_current = word x 100, 1000..8000 (HR309)
     16     charge_target_soc_now, 20..100
     17     grid_import_amps, 1..255 (HR101)
     18-19  ufr_delay (HR312)
     20     force_charge_pct, 0..100 (HR313)   21 force_discharge_pct, 0..100 (HR314)
     22-23  fast_trip_v_low_set (HR315)        24-25 fast_trip_v_high_set (HR316)
     26-27  HR102: bit 6 -> eps_cfg_bits bit 6; bit 0 -> bit 9, but 0 while batt_cal_request is
            1, soc_floor_latch bit 3 or run_status_bits bit 6 is set; bits 12 and 14 are set each
            time (with interrupts off)
     28     sched_window (sched_window_out)    29-30 hr4000_value (HR4000)
     31     enable_charge_target, 0 while batt_cal_stage is set (HR20)
     32     charge_target_soc, at most 100 (HR116)
   The products word x 10 and word x 100 are 16-bit, so a word above 6553 or 655 wraps before
   the clamp (docs/findings-comm.md). */
void arm_page7(unsigned int reg) {
    unsigned int t = 0;
    unsigned int c;
    switch (reg) {
    case 0x08:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        power_ramp_rate = t;
        limitu(&power_ramp_rate, 30000, 10);
        break;
    case 0x0C:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        batt_power_rating.chg_max = (unsigned int)(t * 10);
        limitf(&batt_power_rating.chg_max, 60000.0f, 10000.0f);
        batt_power_rating.dis_max = batt_power_rating.chg_max + 3000.0f;
        break;
    case 0x0E:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        batt_max_charge_current = (unsigned int)(t * 100);
        limitf(&batt_max_charge_current, 8000.0f, 1000.0f);
        break;
    case 0x10:
        t = arm_rx_frame[reg] & 0xFF;
        charge_target_soc_now = t;
        limitu(&charge_target_soc_now, 100, 20);
        break;
    case 0x11:
        t = arm_rx_frame[reg] & 0xFF;
        limitu(&t, 0xFF, 1);
        grid_import_amps = t;
        break;
    case 0x12:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        ufr_delay = t;
        break;
    case 0x14:
        t = arm_rx_frame[reg] & 0xFF;
        limitu(&t, 100, 0);
        force_charge_pct = t;
        break;
    case 0x15:
        t = arm_rx_frame[reg] & 0xFF;
        limitu(&t, 100, 0);
        force_discharge_pct = t;
        break;
    case 0x16:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        fast_trip_v_low_set = t;
        break;
    case 0x18:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        fast_trip_v_high_set = t;
        break;
    case 0x1A:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        eps_cfg_bits.bit.b6 = t >> 6;
        if ((t & 1) && batt_cal_request != 1 && !soc_floor_latch.bit.b3 && !run_status_bits.bit.b6)
            c = 1;
        else
            c = 0;
        DINT;
        eps_cfg_bits.bit.b14 = 1;
        eps_cfg_bits.bit.b9 = c;

        eps_cfg_bits.bit.b12 = 1;
        EINT;
        break;
    case 0x1C:
        t = arm_rx_frame[reg] & 0xFF;
        sched_window = t;
        break;
    case 0x1D:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        hr4000_value = t;
        break;
    case 0x0A:
        t = arm_rx_frame[reg] << 8;
        reg++;
        t |= arm_rx_frame[reg] & 0xFF;
        if (t <= 2500 && op_state < OP_UPDATE)
            grid_v_nominal = t;
        break;
    case 0x1F:
        t = arm_rx_frame[reg] & 0xFF;
        if (batt_cal_stage != 0)
            t = 0;
        enable_charge_target = t;
        break;
    case 0x20:
        t = arm_rx_frame[reg] & 0xFF;
        if (t > 100)
            t = 100;
        charge_target_soc = t;
        break;
    }
}
