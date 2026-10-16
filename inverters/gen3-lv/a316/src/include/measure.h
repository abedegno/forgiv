/* measure.h: measurements: the ADC samples and window sums (stm32f10x_it.c), RMS values, NTC
   temperatures, the CT clamp, DRM inputs, the power flow figures (control/monitor.c,
   control/measure.c) and the values the DSP reports for PV and the inverter (comm/dsp_link.c).
   Powers are in 0.1 W where the input registers divide them by 10 (inference).
   name: descriptive (readability step 1). */
#ifndef MEASURE_H
#define MEASURE_H

#include "ghidra_types.h"

/* data */
extern uchar adc_cycle_end;
extern ushort adc_cycle_n;
extern ushort adc_cycle_samples;
extern int adc_cycle_sumprod;
extern int adc_cycle_sumsq_ch0;
extern int adc_cycle_sumsq_ch1;
extern int adc_cycle_sumsq_ch5;
extern int adc_cycle_sumsq_ch7;
extern short adc_dma_buf[];
extern ushort adc_ntc_battery;
extern ushort adc_ntc_charger;
extern ushort adc_ntc_heatsink;
extern ushort adc_offset_ch0;
extern ushort adc_offset_ch1;
extern ushort adc_offset_ch5;
extern ushort adc_offset_samples;
extern uchar adc_offset_state; /* 0 measuring, 1 sums ready, 2 offsets set */
extern ushort adc_start_toggle;
extern int adc_sumprod_ch0_ch7;
extern int adc_sumsq_ch0;
extern int adc_sumsq_ch5;
extern int adc_sumsq_ch7;
extern short adc_wave_buf[];
extern ushort adc_wave_idx;
extern ushort bus_hv_voltage;
extern ushort bus_p_voltage;
extern int ct_i2_acc;
extern short ct_i_rms; /* CT current x 10 (IR58 without meters) */
extern int ct_p_acc;
extern int ct_power; /* CT power, sum(v x i) / 0x776, sign by HR42 */
extern uchar drm0_active[];
extern int drm_bits;
extern uchar drm_bits_halves;
extern uchar drm_power_limit; /* DRM power limit in %: 2, 40, 70 or 100 */
extern uchar output_enable_count;
extern uchar bms_fail_load_count;
extern uchar measure_flags[];
extern short i_grid_port;
extern ushort inv_countdown;
extern uchar inv_run_mode; /* 0 waiting, 1 normal, 2, 3 fault, 4 firmware update (IR0) */
extern int ntc_battery_acc;
extern ushort ntc_battery_cnt;
extern int ntc_charger_acc;
extern ushort ntc_charger_cnt;
extern int ntc_heatsink_acc;
extern ushort ntc_heatsink_cnt;
extern short p_battery;
extern short p_battery_dsp; /* DSP page 0 battery power, positive = discharge */
extern int p_eps_backup;
extern int p_grid_apparent;
extern int p_grid_out; /* IR30 grid power / 10 */
extern int p_inverter_dsp;
extern int p_inverter_out; /* IR24 inverter power / 10 */
extern int p_load_demand;
extern ushort pv1_current;
extern int pv1_power;
extern ushort pv1_voltage; /* DSP page 0, 0.1 V (IR1) */
extern ushort pv2_current;
#ifndef OWN_pv2_power
extern int pv2_power;
#endif
extern ushort pv2_voltage;
extern ushort pv_power_override; /* above 300 it replaces both PV powers with half of it */

/* functions */
ushort calc_checksum_xor_index(uchar *buffer_addr, int first_index, int last_index);
void drm_mode_eval();
void inv_output_enable_ctrl();
int isqrt_div(uint value, int divisor);
void ntc_battery_temp_update();
void ntc_charger_temp_update();
void ntc_heatsink_temp_update();
void ntc_stub();
void power_flow_update();
void task_measure();
void task_monitor();

#endif
