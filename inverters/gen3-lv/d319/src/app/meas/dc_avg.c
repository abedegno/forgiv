// clang-format off
/* The slow per-cycle sums, run from RAM every control interrupt: ten scaled ADC channels are summed
   into the block at 0xD19C (dc_acc_isr); when the count reaches grid_cycle_samples (one nominal
   grid cycle) the count and the sums are copied to the block at 0xD180 (dc_acc), the sums are
   cleared, and task 3 gets its slow average event (event 1).
   Entry point: dc_acc_step, called at the end of control_isr.
   Data: none of its own.
   Neighbours: meas_task.c and dc_measure.c (task 3 divides the sums by the count).
   name: descriptive (was ch_avg.c; map/filenames.tsv).
   at: dc_acc_step 0x3F2872 (load address; it runs at 0x8C1E) */
// clang-format on
#define OWN_dc_acc /* match: the task copy of the slow averages at 0xD180 as one volatile block (its stores keep their order); the header has the two-channel view */
#include "F2806x_Device.h"
#include "os.h"
#include "control.h"
#include "mppt.h"
#include "meas.h"
#include "util.h"

typedef struct {
    Uint16 n;
    Uint16 pad;
    float inv_n, batt_v, hv_i, hv_v, pv1_v, pv2_v, pv1_i, pv2_i, bus_v, iso, aux;
} dc_acc_task_t;
extern volatile dc_acc_task_t dc_acc;

#pragma CODE_SECTION(dc_acc_step, "ramfuncs");
/* Adds this sample's battery voltage, high-side current and voltage, PV voltages and currents,
   bus voltage, slow residual current and insulation channel to the sums; at the end of the cycle
   hands them over (see the header). */
void dc_acc_step(void) {
    dc_acc_isr.batt_v += meas_fast.batt_v;
    dc_acc_isr.hv_i += meas_fast.hv_i;
    dc_acc_isr.hv_v += meas_fast.hv_v;
    dc_acc_isr.pv1_v += meas_fast.pv1_v;
    dc_acc_isr.pv2_v += meas_fast.pv2_v;
    dc_acc_isr.bus_v += meas_fast.bus_v;
    dc_acc_isr.pv1_i += meas_fast.pv1_i;
    dc_acc_isr.pv2_i += meas_fast.pv2_i;
    dc_acc_isr.aux += meas_fast.leak_i_slow;
    dc_acc_isr.n++;
    dc_acc_isr.iso += meas_fast.iso_v;
    if (dc_acc_isr.n >= grid_cycle_samples) {
        dc_acc.n = dc_acc_isr.n;
        dc_acc.batt_v = dc_acc_isr.batt_v;
        dc_acc.hv_i = dc_acc_isr.hv_i;
        dc_acc.hv_v = dc_acc_isr.hv_v;
        dc_acc.pv1_v = dc_acc_isr.pv1_v;
        dc_acc.pv2_v = dc_acc_isr.pv2_v;
        dc_acc.bus_v = dc_acc_isr.bus_v;
        dc_acc.pv1_i = dc_acc_isr.pv1_i;
        dc_acc.pv2_i = dc_acc_isr.pv2_i;
        dc_acc.aux = dc_acc_isr.aux;
        dc_acc.iso = dc_acc_isr.iso;
        dc_acc_isr.n = 0;
        dc_acc_isr.batt_v = 0;
        dc_acc_isr.hv_i = 0;
        dc_acc_isr.hv_v = 0;
        dc_acc_isr.pv1_v = 0;
        dc_acc_isr.pv2_v = 0;
        dc_acc_isr.bus_v = 0;
        dc_acc_isr.pv1_i = 0;
        dc_acc_isr.pv2_i = 0;
        dc_acc_isr.aux = 0;
        dc_acc_isr.iso = 0;
        os_post_isr(TASK_MEASURE, EV3_SLOW_AVG);
    }
}
