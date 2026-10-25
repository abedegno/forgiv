// clang-format off
/* Start-up of the application state (called from main before the RTOS starts), and two small
   accessors of the settings block. at: app_vars_init 0x3EF6D5, app_init 0x3EF7D5, preset_none
   0x3EFB32, arm_run_mode_set 0x3EFC26, run_mode_req_get 0x3EFC2A, inv_off 0x3EFF82

   Job: put the application's variables in a known state before the tasks start: the SCI rings,
   "no battery" (battery_type 0, HR19 bits 0-1 clear, so no BMS until the ARM says so), the PV and
   DC-DC start words, then the settings and rating defaults and the RCMU.
   Entry points: app_init (main), inv_off (many: state and fault code), arm_run_mode_set
   (arm_link.c), preset_none (the rating_presets table). run_mode_req_get is not called.
   Data: none of its own. Neighbours: settings.c, rating_presets.c, rcmu.c, op_states.c.
   name: descriptive, kept from the matching (map/filenames.tsv: src/app_init.c). */
// clang-format on
#define OWN_state_flags /* match: this file reads and writes 0xCF0E as a plain word (masks); the header has the bit view union bits16_u, and this file's code is written against the word */
#include "F2806x_Device.h"
#include "sci.h"
#include "settings.h"
#include "faults.h"
#define OWN_bms_hr1_request /* startup writes the HR1 word before tasks can access it */
#include "bms.h"
extern union bits16_u bms_hr1_request;
#include "control.h"
#include "state.h"
#include "rcmu.h"
#include "util.h"

extern Uint16 state_flags;

/* Reset the SCI rings (150 bytes), the BMS status bits, the battery type (0: no BMS), the PV
   boosts, the eCAP count, the DC-DC soft-start duty (1), the PV mode and the bus reference
   (380.0 V). */
void app_vars_init(void) {
    ring_reset(0, 150);
    ring_reset(1, 150);
    bms.hr19_status.all &= ~0x0003;
    dsp_settings.battery_type = 0;
    pv1_boost_on = 0;
    pv2_boost_on = 0;
    ecap_valid_count = 0;
    dcdc_soft_duty = 1;
    bms_request_flags.all = 0;
    bms_hr1_request.all = 0;
    bms_hr15_edge.all = 0;
    dsp_settings.pv_input_mode = 0;
    bus_v_ref_prev = 3800.0f;
}

/* Start-up before the RTOS: inverter off, relay test off, variables, settings defaults,
   rating defaults (init_defaults), RCMU. */
void app_init(void) {
    inv_off();
    relay_test_abort();
    app_vars_init();
    settings_defaults();
    init_defaults();
    rcmu_init();
}

/* The empty rating preset (rating_presets entries 0 and 6 .. 8). */
void preset_none(void) {}

/* Store the run mode the ARM sent (arm_link.c). */
void arm_run_mode_set(Uint16 v) { dsp_settings.arm_run_mode = v; }

/* The requested run mode. Not called. */
Uint16 run_mode_req_get(void) { return dsp_settings.run_mode_req; }

/* Inverter bridge off: clear the bridge start bits 12 and 13 of state_flags and inv_on (0xC004). */
void inv_off(void) {
    state_flags &= 0xCFFF;
    inv_on = 0;
}
