/* The measurements: ADC scaling, averages, filters and the measurement task (task 3), with the AC
   measurements of task 1 declared in rating.h. Files: src/app/meas/ (adc_scale.c, meas_avg8.c,
   avg_win.c, dc_avg.c, grid_rms_accum.c, eps_rms_accum.c run in the control interrupt;
   dc_measure.c and meas_task.c are task 3; ac_measure.c is task 1).
   Units: voltages 0.1 V, PV currents 0.01 A, powers 0.1 W, temperatures 0.1 degree C;
   calibration gains are counts over 1024. docs/subsystems/meas.md has the details. */
#ifndef MEAS_H
#define MEAS_H

#include "F2806x_Device.h"
#include "util.h"

/* the per-cycle DC sums (dc_avg.c): the count n, its inverse (task 3 sets it) and the sums of the
 * battery voltage, high-side current and voltage, PV voltages and currents, bus voltage,
 * insulation channel and slow residual current (aux). dc_acc at 0xD180 holds two of them (a is
 * the task copy; b is only cleared and counted by meas_init); dc_acc_isr at 0xD19C is the
 * interrupt's running sum. 28 words each. */
typedef struct dc_acc_t {
    Uint16 n;
    Uint16 rsvd0;
    float inv_n, batt_v, hv_i, hv_v, pv1_v, pv2_v, pv1_i, pv2_i, bus_v, iso, aux, acc_18,
        acc_1A; /* by word offset */
} dc_acc_t;

/* the ADC offsets measured at start-up: each an average and the sum it came from */
struct adc_offset_t {
    int16 offset;
    int32 sum;
};

/* 0xD140..0xD178 are the measurement file's own variables (src/app/meas/dc_measure.c) */
/* 0xD176..0xD178, cleared together; the third word is the MPPT flags: bits 0/1 PV power over its
 * limit, bits 4/5 PV1/PV2 tracking started (mppt.c), bit 6 set after a rescan, bit 14 PV restart
 * in op_state 6 */
union mppt_flags_t {
    unsigned int w[3];
    struct {
        unsigned int w0;
        unsigned int w1;
        union bits16_u flags;
    } s;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_Device.h"
#include "control.h"

/* adc_scale.c */
void adc_scale(void);

/* meas_avg8.c */
void meas_avg8(void);

/* avg_win.c */
void avg_win_step(AVG_WIN *p);
extern Uint16 win_clear_idx;
extern Uint16 win_len;
extern Uint16 win_mode_prev;
extern Uint16 win_restart;
extern Uint16 win_settle;

/* rcmu_fast_check.c */
void rcmu_fast_check(void);

/* dc_avg.c */
void dc_acc_step(void);

/* dc_measure.c */
void bus_voltage_check(void);
void bus_voltage_filter(void);
void eps_bus_voltage_check(void);
void iso_fault_clear(void);
void meas_aux_f16(void);
void meas_batt_dcdc(void);
void meas_bus_voltage(void);
void meas_init(void);
void meas_iso(void);
void meas_pv(void);
void meas_pv_power(void);
extern union mppt_flags_t mppt_flags_blk;
void ntc_temp_check(void);
extern float pv1_ref_max;
extern float pv2_ref_max;
void pv_boost_select(void);
extern unsigned int pv_detect_done;
void pv_input_detect(void);
void pv_input_detect_run(void);
void pv_iso_check(void);
void pv_power_limit_flags(void);
void pv_power_limit_update(void);
void pv_ref_max_update(void);
void pv_short_check(void);
void pv_voltage_check(void);
extern unsigned int rescan_settle_count;
void temp_check(void);

/* defined by no source: the linker places it at its map address */
extern volatile Uint16 adc_dma_buf[32]; /* filled by DMA from the ADC result registers */
extern struct adc_offsets_t {
    struct adc_offset_t res8;
    struct adc_offset_t res15;
    struct adc_offset_t res2;
    struct adc_offset_t res3;
    struct adc_offset_t res7;
} adc_offsets;
extern float aux_f16_k;
extern float batt_temp_limit;
extern float bus_ov_limit;
extern float bus_ref_isr;
/* 0xD808: the calibration gains from the ARM (HR4097..HR4146), each a count over 1024; the
   measurement code accepts 870..1178 (0.85..1.15) */
extern struct cal_gains_t {
    unsigned int batt_v;
    unsigned int hv_i;
    unsigned int pv1_v;
    unsigned int pv1_i;
    unsigned int pv2_v;
    unsigned int pv2_i;
    unsigned int pv1_p;
    unsigned int pv2_p;
    unsigned int grid_v;
    unsigned int g811;
    unsigned int g812;
    unsigned int power;
    unsigned int g814;
    unsigned int g815;
    unsigned int g816;
    unsigned int g817;
    unsigned int bus_v;
} cal_gains; /* name: settings.c defines these words one by one from cal_batt_v (an alias row in map/data.tsv) */
extern float charger_temp_limit;
double cos(double x);
#ifndef OWN_dc_acc
extern struct dc_acc_pair_t {
    struct dc_acc_t a;
    struct dc_acc_t b;
} dc_acc;
#endif
extern dc_acc_t dc_acc_isr;
/* the battery voltage, high-side current and bus gains handed to the interrupt by task 3 */
extern struct dc_gain_copy_t {
    float batt_v;
    float hv_i;
    float bus_v;
} dc_gain_isr;
/* the DC results of task 3 (dc_measure.c): PV voltages and currents (raw and calibrated), the
   bus voltage and its reference, battery and high-side values, powers (0.1 W) and temperatures */
extern struct dc_meas_t {
    float pv1_v_raw;
    float pv2_v_raw;
    float pv1_v;
    float pv2_v;
    float pv_v_max;
    float pv_v_min;
    float bus_v_ref;
    float bus_v_ref_prev;
    float bus_v;
    float batt_v;
    float hv_v;
    float hv_i;
    float batt_power;
    float pv1_i_raw;
    float pv1_i;
    float pv2_i_raw;
    float pv2_i;
    float pv1_p_filt;
    float pv2_p_filt;
    float pv_p;
    float pv1_p;
    volatile float pv2_p;
    float heatsink_temp;
    float charger_temp;
    float batt_temp;
    float aux_f16;
    float dc_234;
    float dc_236;
    float dc_238;
    float dc_23A;
    float dc_23C;
    float dc_23E;
} dc_meas;
extern float heatsink_temp_limit;
extern struct dsp_settings_d8d5_view {
    unsigned int iso_check_enable;
    unsigned int rsvd[20];
    unsigned int run_mode_req;
} iso_check_enable;
extern float mod_gain;
extern float mod_gain_isr;
extern struct mppt_state_t {
    unsigned int rescan;
    unsigned int rsvd0;
    float pv1_ref;
    float pv2_ref;
    float pv1_voc;
    float pv2_voc;
    float pv1_sweep_min;
    float pv2_sweep_min;
    float pv1_ref_prev;
    float pv2_ref_prev;
    float pv1_p_sum;
    float pv1_p_prev;
    float pv2_p_sum;
    float pv2_p_prev;
    float p_hyst;
    float v_step;
    unsigned int pv1_sweep_steps;
    unsigned int pv2_sweep_steps;
    unsigned int pv1_p_count;
    unsigned int pv2_p_count;
} mppt;
extern struct pv_gain_copy_t {
    float pv1_v;
    float pv2_v;
} pv_gain_isr;
extern unsigned int pv_input_mode;
extern float pv_ov_limit;
extern float pv_power_limit;
double sin(double x);
/* ---- end of declarations ---- */

#endif
