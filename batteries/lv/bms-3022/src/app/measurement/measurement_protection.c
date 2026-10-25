/* at: measurement_protection_settings 0x2000156A */
extern unsigned short measurement_protection_settings[];
/* at: peer_auxiliary_gate_word 0x20000184 */
extern unsigned short peer_auxiliary_gate_word;
/* at: current_substitution_active 0x200000F0, current_substitution_retries 0x200000F1 */
/* at: afe_calibration_ready 0x200000D4, settings_initialized 0x200000D5, measurement_ready 0x200000F3 */
extern unsigned char current_substitution_active;
extern unsigned char current_substitution_retries;
extern unsigned char afe_calibration_ready;
extern unsigned char settings_initialized;
extern unsigned char measurement_ready;
#include "../../include/bms_heating.h"
/* at: battery_heating 0x20000196 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I keep filtering, current/temperature conversion and protection checks in one unit.
   protection_cycle staggers some checks and publishes the measurement snapshot.
   This unit owns its filter tables, protection counters and resistance sample history.
   match: I preserve source order, repeated reads and the paired status-byte layouts.
   name: I use a descriptive filename, not a recovered original filename. */
/* Measurement, temperature, protection and current-limit control.
   Matched as one source unit: 08017E40..0801D424 text, the 08030560 const tables,
   0x2F8 initialised bytes at 20000214 and 0x28A zero-init bytes at 20003360. */
/* at: soc_offset_mean_history_index 0x2000023F, soc_adjusted_mean_history_index 0x20000240, capacity_full_low_current_ticks 0x20000241, capacity_full_trip_ticks 0x20000242, capacity_full_refresh_ticks 0x20000243, capacity_empty_trip_ticks 0x20000244, capacity_empty_clear_ticks 0x20000245, capacity_deep_empty_ticks 0x20000246, soc_correction_day_baseline 0x20000247, soc_correction_day_count 0x20000248, soc_correction_lock_state 0x20000249, pack_undervoltage_limit_ticks 0x2000024A, pack_undervoltage_protection_ticks 0x2000024B, pack_undervoltage_normal_ticks 0x2000024C, cell_undervoltage_limit_ticks 0x2000024D, cell_undervoltage_protection_ticks 0x2000024E, cell_undervoltage_normal_ticks 0x2000024F, pack_overvoltage_limit_ticks 0x20000250, pack_overvoltage_protection_ticks 0x20000251, pack_overvoltage_normal_ticks 0x20000252, cell_overvoltage_limit_ticks 0x20000253, cell_overvoltage_protection_ticks 0x20000254, cell_overvoltage_normal_ticks 0x20000255, discharge_overcurrent_limit_ticks 0x20000256, discharge_overcurrent_protection_ticks 0x20000257, discharge_overcurrent_normal_ticks 0x20000258, charge_overcurrent_limit_ticks 0x20000259, charge_overcurrent_protection_ticks 0x2000025A, charge_overcurrent_normal_ticks 0x2000025B, charge_overtemperature_limit_ticks 0x2000025C, charge_overtemperature_protection_ticks 0x2000025D, charge_overtemperature_normal_ticks 0x2000025E, discharge_overtemperature_limit_ticks 0x2000025F, discharge_overtemperature_protection_ticks 0x20000260, discharge_overtemperature_normal_ticks 0x20000261, charge_undertemperature_limit_ticks 0x20000262, charge_undertemperature_protection_ticks 0x20000263, charge_undertemperature_normal_ticks 0x20000264, discharge_undertemperature_limit_ticks 0x20000265, discharge_undertemperature_protection_ticks 0x20000266, discharge_undertemperature_normal_ticks 0x20000267, mosfet_overtemperature_limit_ticks 0x20000268, mosfet_overtemperature_protection_ticks 0x20000269, mosfet_overtemperature_normal_ticks 0x2000026A, ambient_overtemperature_limit_ticks 0x2000026B, ambient_overtemperature_protection_ticks 0x2000026C, ambient_overtemperature_normal_ticks 0x2000026D, ambient_undertemperature_limit_ticks 0x2000026E, ambient_undertemperature_protection_ticks 0x2000026F, ambient_undertemperature_normal_ticks 0x20000270, voltage_consistency_fault_ticks 0x20000271, temperature_sensor_fault_ticks 0x20000272, positive_current_activity_latch 0x20000273, resistance_sample_index 0x20000274, resistance_high_ticks 0x20000275, resistance_normal_ticks 0x20000276, resistance_current_range_ticks 0x20000277, protection_cycle_phase 0x20000278, voltage_current_protection_flags 0x20000279, temperature_protection_flags 0x2000027A, capacity_low_voltage_hold_ticks 0x200002BE, capacity_hold_expiry_ticks 0x200002C0, soc_correction_interval 0x200002C2, soc_rest_correction_ticks 0x200002C4, soc_full_capacity_refresh_ticks 0x200002C6, soc_update_divider 0x200002C8, soc_near_full_ticks 0x200002CA, soc_high_load_ticks 0x200002CC, protection_cycle_aux_reserved_word 0x200002CE, positive_current_activity_ticks 0x200002D0, cold_low_soc_divider 0x200002D2, cold_low_soc_trip_ticks 0x200002D4, cold_low_soc_clear_ticks 0x200002D6, field_half_2CC 0x200002D4, field_half_2CE 0x200002D6, soc_correction_elapsed 0x200002F0, soc_correction_day_ticks 0x200002F4, cell_undervoltage_shutdown_state 0x200002F8, field_timer_2F0 0x200002F8, field_timer_2F2 0x200002FA, field_timer_2F4 0x200002FC, field_timer_2F6 0x200002FE, protection_debounce_settings 0x20000304, width_byte_237 0x2000023F, width_byte_238 0x20000240, width_byte_239 0x20000241, width_byte_23A 0x20000242, width_byte_23B 0x20000243, width_byte_23C 0x20000244, width_byte_23D 0x20000245, width_byte_23E 0x20000246, width_byte_23F 0x20000247, width_byte_240 0x20000248, width_byte_241 0x20000249, width_byte_242 0x2000024A, width_byte_243 0x2000024B, width_byte_244 0x2000024C, width_byte_245 0x2000024D, width_byte_246 0x2000024E, width_byte_247 0x2000024F, width_byte_248 0x20000250, width_byte_249 0x20000251, width_byte_24A 0x20000252, width_byte_24B 0x20000253, width_byte_24C 0x20000254, width_byte_24D 0x20000255, width_byte_24E 0x20000256, width_byte_24F 0x20000257, width_byte_250 0x20000258, width_byte_251 0x20000259, width_byte_252 0x2000025A, width_byte_253 0x2000025B, width_byte_254 0x2000025C, width_byte_255 0x2000025D, width_byte_256 0x2000025E, width_byte_257 0x2000025F, width_byte_258 0x20000260, width_byte_259 0x20000261, width_byte_25A 0x20000262, width_byte_25B 0x20000263, width_byte_25C 0x20000264, width_byte_25D 0x20000265, width_byte_25E 0x20000266, width_byte_25F 0x20000267, width_byte_260 0x20000268, width_byte_261 0x20000269, width_byte_262 0x2000026A, width_byte_263 0x2000026B, width_byte_264 0x2000026C, width_byte_265 0x2000026D, width_byte_266 0x2000026E, width_byte_267 0x2000026F, width_byte_268 0x20000270, width_byte_269 0x20000271, width_byte_26A 0x20000272, width_byte_26B 0x20000273, width_byte_26C 0x20000274, width_byte_26D 0x20000275, width_byte_26E 0x20000276, width_byte_26F 0x20000277, width_byte_270 0x20000278, width_byte_271 0x20000279, width_byte_272 0x2000027A, width_half_2B2 0x200002BA, width_half_2B4 0x200002BC, width_half_2B6 0x200002BE, width_half_2B8 0x200002C0, width_half_2BA 0x200002C2, width_half_2BC 0x200002C4, width_half_2BE 0x200002C6, width_half_2C0 0x200002C8, width_field_half_2C2 0x200002CA, width_half_2C4 0x200002CC, width_half_2C6 0x200002CE, width_half_2C8 0x200002D0, width_half_2CA 0x200002D2, width_half_2CC 0x200002D4, width_half_2CE 0x200002D6, discharge_maximum_temperature_hold_ticks 0x200002A0, discharge_minimum_temperature_hold_ticks 0x200002A2, full_voltage_release_ticks 0x200002A4, charge_cell_voltage_held 0x200002A6, charge_cell_voltage_hold_ticks 0x200002A8, charge_maximum_temperature_held 0x200002AA, charge_minimum_temperature_held 0x200002AC, charge_maximum_temperature_hold_ticks 0x200002AE, charge_minimum_temperature_hold_ticks 0x200002B0, peer_charge_ramp_ticks 0x200002B2, peer_charge_excess_ticks 0x200002B4, peer_discharge_ramp_ticks 0x200002B6, peer_discharge_excess_ticks 0x200002B8, peer_charge_limit_previous 0x200002BA, peer_discharge_limit_previous 0x200002BC, work_scalar_reserved_09E 0x200002BA, positive_compensation_current_floor 0x200002DC, negative_compensation_current_floor 0x200002E0, capacity_integrated_ticks 0x200002E4, capacity_display_integrated_ticks 0x200002E8, soc_rest_integrated_current 0x200002EC, work_scalar_reserved_0D0 0x200002EC, measurement_input_fault_flags 0x2000021C, primary_temperature_sensor_faults 0x2000021D, auxiliary_temperature_sensor_faults 0x2000021E, scalar_reserved_003 0x2000021F, capacity_full_voltage_seen 0x20000221, soc_mean_voltage_offset 0x20000222, soc_correction_state 0x20000223, scalar_reserved_008 0x20000224, positive_voltage_history_ready 0x20000227, negative_voltage_history_ready 0x20000228, positive_voltage_history_index 0x20000229, negative_voltage_history_index 0x2000022A, capacity_empty_voltage_seen 0x2000022B, soc_high_cell_history_index 0x2000022C, soc_low_cell_history_index 0x2000022D, soc_rest_history_index 0x2000022E, soc_rest_positive_bin 0x2000022F, soc_rest_near_zero_bin 0x20000230, soc_rest_negative1000_bin 0x20000231, soc_rest_negative2000_bin 0x20000232, soc_rest_negative3000_bin 0x20000233, discharge_maximum_temperature_band 0x20000234, discharge_minimum_temperature_band 0x20000235, discharge_cell_voltage_band 0x20000236, discharge_voltage_recovery_seen 0x20000237, charge_maximum_temperature_band 0x20000238, charge_minimum_temperature_band 0x20000239, charge_cell_voltage_band 0x2000023A, charge_soc_difference_latch 0x2000023B, peer_charge_excess_latch 0x2000023C, peer_discharge_excess_latch 0x2000023D, peer_limit_startup_count 0x2000023E, scalar_reserved_023 0x2000023F, power_output_status_flags 0x2000027B, scalar_reserved_060 0x2000027C, capacity_latch_flags 0x2000027E, voltage_current_limit_flags 0x2000027F, scalar_reserved_064 0x20000280, extended_limit_flags 0x20000281, protection_cycle_reserved_word 0x20000282, cell_voltage_step_trip_ticks 0x20000284, cell_voltage_step_clear_ticks 0x20000286, secondary_current_zero_estimate 0x20000288, primary_current_zero_estimate 0x2000028A, soc_correction_tick_divider 0x20000290, soc_rest_ticks 0x20000292, soc_high_positive_current_ticks 0x20000294, ambient_temperature_held 0x20000296, discharge_cell_voltage_held 0x20000298, discharge_cell_voltage_hold_ticks 0x2000029A, discharge_maximum_temperature_held 0x2000029C, discharge_minimum_temperature_held 0x2000029E, positive_voltage_resistance_history 0x20003378, negative_voltage_resistance_history 0x20003440, measurement_work294 0x2000029C, soc_high_cell_curve 0x200003D4, measurement_compensate_cell_voltages 0x08017E48, measurement_update_voltage_summary 0x0801846E, measurement_convert_primary_thermistor 0x08018614, measurement_convert_secondary_thermistor 0x080187F2, measurement_update_auxiliary_temperatures 0x080188A2, measurement_update_temperature_summary 0x0801891E, measurement_detect_cell_voltage_steps 0x080189C2, measurement_update_voltage_and_temperature 0x08018A3C, measurement_update_current 0x08018A4C, capacity_integrate_current 0x08018EC4, capacity_update_cycle_count 0x080191BE, capacity_update_operating_status 0x0801927C, capacity_latch_full_voltage 0x080192AA, capacity_latch_empty_voltage 0x08019432, soc_select_model_curve 0x080194A8, capacity_get_full_capacity 0x0801952A, soc_lookup_high_cell 0x08019530, soc_average_high_cell 0x0801986A, soc_lookup_low_cell 0x080198E8, soc_average_low_cell 0x08019BC4, soc_correction_tick 0x08019C40, soc_evaluate_rest_correction 0x08019C6A, current_limit_from_ambient_temperature 0x08019F20, discharge_temperature_band 0x08019F76, discharge_voltage_band 0x08019FF0, discharge_limit_from_cell_conditions 0x0801A03A, discharge_limit_from_capacity 0x0801A1EE, discharge_select_limit 0x0801A202, discharge_update_current_limit 0x0801A23A, charge_temperature_band 0x0801A38C, charge_voltage_band 0x0801A414, charge_limit_from_cell_conditions 0x0801A45A, charge_limit_from_capacity 0x0801A5F6, charge_select_limit 0x0801A60A, charge_update_current_limit 0x0801A642, pack_aggregate_current_limits 0x0801A7BA, soc_lookup_adjusted_mean_voltage 0x0801AB9C, soc_lookup_offset_mean_voltage 0x0801ADE0, measurement_current 0x2000014C, measurement_cell_count 0x20000166, cell_adc_reference_scale 0x20000096, sampled_cell_voltages 0x20001164, compensated_cell_voltages 0x200010AC, averaged_cell_adc_codes 0x200010CC, cell_adc_accumulators 0x20001124, positive_cell_resistance 0x20001184, negative_cell_resistance 0x20001198, measurement_current_mode 0x2000009F, cell_voltage_corrections 0x200011AC, measurement_rated_capacity 0x20000186, measurement_state_anchor 0x2000021C, control_20003360 0x20003378, adc_reference_scale 0x20000094, independent_voltage_adc 0x20000124, independent_pack_voltage 0x20000120, auxiliary_voltage_adc 0x2000012C, auxiliary_voltage_reading 0x20000128, measurement_maximum_cell_voltage 0x20000110, measurement_minimum_cell_voltage 0x20000112, sorted_cell_voltages 0x200010EC, maximum_selected_cell_voltage 0x20000118, sorted_selected_cell_voltages 0x2000110C, minimum_selected_cell_voltage 0x2000011A, maximum_reference_cell_voltage 0x2000011C, minimum_reference_cell_voltage 0x2000011E, pack_voltage_scale 0x20000158, measurement_pack_voltage 0x20000114, mean_cell_voltage 0x20000116, raw_temperature_samples 0x200011C0, temperature_readings 0x200011B6, measurement_maximum_temperature 0x2000012E, measurement_minimum_temperature 0x20000130, mean_temperature 0x20000132, cell_voltage_step_detected 0x200000F8, current_adc_gain 0x200000C8, positive_current_scale 0x20000154, negative_current_scale 0x20000150, current_adc_options 0x200000FC, current_zero_calibrated 0x2000009C, primary_current_samples 0x200011CA, secondary_current_samples 0x2000135A, secondary_current_zero 0x2000015C, primary_current_zero 0x2000015E, secondary_current_reading 0x20000160, integrated_current 0x20000148, control_20000294 0x2000029C, remaining_capacity 0x20000170, primary_soc 0x20000188, displayed_soc 0x20000189, soc_correction_gain 0x200000A4, displayed_capacity 0x2000017C, capacity_voltage_hold_active 0x200000AA, capacity_learning_active 0x2000018B, full_capacity 0x20000168, cycle_count_flags 0x200000A9, cycle_count 0x200000A2, cycle_integrated_capacity 0x2000016C, capacity_operating_status 0x200000A0, full_capacity_recorded 0x200000A8, model_code 0x20000077, capacity_full_latch 0x200000AB, full_capacity_snapshot 0x20000174, capacity_learning_ticks 0x200000B0, soc_model_curve_be_bk 0x080309A4, protection_cycle 0x0801D4DE, device_registers 0x20000FB4, soc_model_curve_bj_dx 0x08030A08, soc_model_mean_voltage_curve 0x20000370, soc_high_cell_history 0x20003508, soc_low_cell_curve 0x20000428, soc_low_cell_history 0x20003512, soc_rest_current_history 0x2000351C, ambient_rate_limits 0x08030B98, discharge_rate_limits 0x08030B14, measurement_limit_settings 0x2000153A, upper_voltage_current_hold 0x2000013D, upper_voltage_hold_latched 0x2000013E, lower_voltage_current_hold 0x2000013C, capacity_empty_latch 0x200000AC, discharge_current_limit 0x20000138, charge_rate_limits 0x08030A6C, charge_current_limit 0x20000134, model_current_limit_steps 0x20000090, measurement_peer_records 0x20003D6A, peer_current_excess_threshold 0x2000009A, combined_discharge_current_limit 0x20000144, combined_charge_current_limit 0x20000142, peer_mode_one_summary 0x20000140, peer_mode_two_summary 0x20000141, peer_auxiliary_state_present 0x2000013F, soc_fixed_mean_voltage_curve 0x2000030C, __aeabi_memclr8 0x0802F6BE, __aeabi_f2iz 0x0803042C, __aeabi_ui2f 0x080304D4, __ARM_scalbnf 0x080308C2, __aeabi_f2uiz 0x08030464, __aeabi_i2f 0x080304A4, __aeabi_f2d 0x08030168, __aeabi_d2f 0x0802F788, eeprom_write_word 0x080225D6, __aeabi_ui2d 0x0802FC8C */
#include "bms_measurement_types.h"
#include "../../include/bms_soc_state.h"
#include "../../include/bms_control.h"
#include "../../include/bms_measurements.h"
#include "../../include/bms_protection.h"
#include "../../include/bms_thresholds.h"
#include <stdlib.h>
const ushort soc_model_curve_be_bk[10][5] = {
    {0, 2739, 2890, 3193, 3206},  {5, 3109, 3149, 3207, 3213},  {10, 3204, 3205, 3218, 3231},
    {15, 3213, 3220, 3241, 3248}, {20, 3240, 3246, 3257, 3261}, {25, 3257, 3263, 3268, 3270},
    {30, 3273, 3278, 3276, 3276}, {35, 3289, 3286, 3281, 3279}, {40, 3294, 3288, 3282, 3280},
    {45, 3295, 3289, 3283, 3281}};

const ushort soc_model_curve_bj_dx[10][5] = {
    {0, 2765, 2909, 3200, 3206},  {5, 3135, 3162, 3207, 3220},  {10, 3205, 3206, 3228, 3243},
    {15, 3220, 3225, 3251, 3261}, {20, 3247, 3250, 3267, 3273}, {25, 3262, 3267, 3278, 3280},
    {30, 3279, 3284, 3282, 3281}, {35, 3294, 3288, 3283, 3282}, {40, 3295, 3289, 3284, 3282},
    {45, 3296, 3290, 3284, 3283}};

const ushort charge_rate_limits[7][12] = {{0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0},
                                          {0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 0},
                                          {0, 33, 33, 50, 100, 100, 80, 60, 40, 20, 10, 0},
                                          {0, 10, 20, 20, 20, 20, 20, 20, 20, 20, 10, 0},
                                          {0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0},
                                          {0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},
                                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

const ushort discharge_rate_limits[6][11] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 20, 20, 10, 10, 10, 10, 10, 10, 10, 0},   {0, 40, 40, 40, 40, 40, 40, 40, 30, 10, 0},
    {0, 40, 50, 50, 50, 50, 50, 50, 30, 10, 0}, {0, 50, 50, 80, 115, 100, 80, 50, 30, 10, 0}};

const ushort ambient_rate_limits[10] = {90, 80, 70, 60, 50, 40, 30, 20, 10, 5};
/* Candidate initialized data view; source ownership remains unproved. */
uchar measurement_input_fault_flags = 0;
uchar primary_temperature_sensor_faults = 0;
uchar auxiliary_temperature_sensor_faults = 0;
uchar charge_overcurrent_retry_count = 0;
uchar discharge_overcurrent_retry_count = 0;
volatile uchar capacity_full_voltage_seen = 0;
uchar soc_mean_voltage_offset = 0;
uchar soc_correction_state = BMS_SOC_CORRECTION_IDLE;
signed char soc_correction_target = 0;
uchar positive_current_output_fault_ticks = 0;
uchar negative_current_output_fault_ticks = 0;
uchar positive_voltage_history_ready = 0;
uchar negative_voltage_history_ready = 0;
uchar positive_voltage_history_index = 0;
uchar negative_voltage_history_index = 0;
volatile uchar capacity_empty_voltage_seen = 0;
uchar soc_high_cell_history_index = 0;
uchar soc_low_cell_history_index = 0;
uchar soc_rest_history_index = 0;
uchar soc_rest_positive_bin = 0;
uchar soc_rest_near_zero_bin = 0;
uchar soc_rest_negative1000_bin = 0;
uchar soc_rest_negative2000_bin = 0;
uchar soc_rest_negative3000_bin = 0;
uchar discharge_maximum_temperature_band = 0;
uchar discharge_minimum_temperature_band = 0;
uchar discharge_cell_voltage_band = 0;
uchar discharge_voltage_recovery_seen = 1;
uchar charge_maximum_temperature_band = 0;
uchar charge_minimum_temperature_band = 0;
uchar charge_cell_voltage_band = 0;
uchar charge_soc_difference_latch = 0;
uchar peer_charge_excess_latch = 0;
uchar peer_discharge_excess_latch = 0;
uchar peer_limit_startup_count = 0;
uchar soc_offset_mean_history_index = 0;
uchar soc_adjusted_mean_history_index = 0;
uchar capacity_full_low_current_ticks = 0;
uchar capacity_full_trip_ticks = 0;
uchar capacity_full_refresh_ticks = 0;
uchar capacity_empty_trip_ticks = 0;
uchar capacity_empty_clear_ticks = 0;
uchar capacity_deep_empty_ticks = 0;
uchar soc_correction_day_baseline = 0;
uchar soc_correction_day_count = 0;
uchar soc_correction_lock_state = 0;
uchar pack_undervoltage_limit_ticks = 0;
uchar pack_undervoltage_protection_ticks = 0;
uchar pack_undervoltage_normal_ticks = 0;
uchar cell_undervoltage_limit_ticks = 0;
uchar cell_undervoltage_protection_ticks = 0;
uchar cell_undervoltage_normal_ticks = 0;
uchar pack_overvoltage_limit_ticks = 0;
uchar pack_overvoltage_protection_ticks = 0;
uchar pack_overvoltage_normal_ticks = 0;
uchar cell_overvoltage_limit_ticks = 0;
uchar cell_overvoltage_protection_ticks = 0;
uchar cell_overvoltage_normal_ticks = 0;
uchar discharge_overcurrent_limit_ticks = 0;
uchar discharge_overcurrent_protection_ticks = 0;
uchar discharge_overcurrent_normal_ticks = 0;
uchar charge_overcurrent_limit_ticks = 0;
uchar charge_overcurrent_protection_ticks = 0;
uchar charge_overcurrent_normal_ticks = 0;
uchar charge_overtemperature_limit_ticks = 0;
uchar charge_overtemperature_protection_ticks = 0;
uchar charge_overtemperature_normal_ticks = 0;
uchar discharge_overtemperature_limit_ticks = 0;
uchar discharge_overtemperature_protection_ticks = 0;
uchar discharge_overtemperature_normal_ticks = 0;
uchar charge_undertemperature_limit_ticks = 0;
uchar charge_undertemperature_protection_ticks = 0;
uchar charge_undertemperature_normal_ticks = 0;
uchar discharge_undertemperature_limit_ticks = 0;
uchar discharge_undertemperature_protection_ticks = 0;
uchar discharge_undertemperature_normal_ticks = 0;
uchar mosfet_overtemperature_limit_ticks = 0;
uchar mosfet_overtemperature_protection_ticks = 0;
uchar mosfet_overtemperature_normal_ticks = 0;
uchar ambient_overtemperature_limit_ticks = 0;
uchar ambient_overtemperature_protection_ticks = 0;
uchar ambient_overtemperature_normal_ticks = 0;
uchar ambient_undertemperature_limit_ticks = 0;
uchar ambient_undertemperature_protection_ticks = 0;
uchar ambient_undertemperature_normal_ticks = 0;
uchar voltage_consistency_fault_ticks = 0;
uchar temperature_sensor_fault_ticks = 0;
uchar positive_current_activity_latch = 0;
uchar resistance_sample_index = 0;
uchar resistance_high_ticks = 0;
uchar resistance_normal_ticks = 0;
uchar resistance_current_range_ticks = 0;
uchar protection_cycle_phase = 0;
volatile uchar voltage_current_protection_flags = 0;
volatile uchar temperature_protection_flags = 0;
volatile uchar power_output_status_flags = 0;
volatile uchar hardware_fault_flags = 0;
volatile uchar protection_inhibit_flags = 0;
volatile uchar capacity_latch_flags = 0;
volatile uchar voltage_current_limit_flags = 0;
volatile uchar temperature_limit_flags = 0;
volatile uchar extended_limit_flags = 0;
ushort protection_cycle_reserved_word = 0;
ushort cell_voltage_step_trip_ticks = 0;
ushort cell_voltage_step_clear_ticks = 0;
ushort secondary_current_zero_estimate = 0;
ushort primary_current_zero_estimate = 0;
/* New current-substitution duration and retry timers. */
/* at: current_substitution_total_ticks 0x2000028C, current_substitution_retry_ticks 0x2000028E */
ushort current_substitution_total_ticks = 0;
ushort current_substitution_retry_ticks = 0;
ushort soc_correction_tick_divider = 0;
ushort soc_rest_ticks = 0;
ushort soc_high_positive_current_ticks = 0;
ushort ambient_temperature_held = 0;
ushort discharge_cell_voltage_held = 4000;
ushort discharge_cell_voltage_hold_ticks = 0;
ushort discharge_maximum_temperature_held = 0;
ushort discharge_minimum_temperature_held = 0;
ushort discharge_maximum_temperature_hold_ticks = 0;
ushort discharge_minimum_temperature_hold_ticks = 0;
ushort full_voltage_release_ticks = 0;
ushort charge_cell_voltage_held = 0;
short charge_cell_voltage_hold_ticks = 0;
ushort charge_maximum_temperature_held = 0;
ushort charge_minimum_temperature_held = 0;
ushort charge_maximum_temperature_hold_ticks = 0;
ushort charge_minimum_temperature_hold_ticks = 0;
ushort peer_charge_ramp_ticks = 600;
ushort peer_charge_excess_ticks = 0;
ushort peer_discharge_ramp_ticks = 600;
ushort peer_discharge_excess_ticks = 0;
ushort peer_charge_limit_previous = 0;
ushort peer_discharge_limit_previous = 0;
ushort capacity_low_voltage_hold_ticks = 0;
ushort capacity_hold_expiry_ticks = 0;
ushort soc_correction_interval = 1800;
ushort soc_rest_correction_ticks = 0;
ushort soc_full_capacity_refresh_ticks = 0;
ushort soc_update_divider = 0;
ushort soc_near_full_ticks = 0;
ushort soc_high_load_ticks = 0;
ushort protection_cycle_aux_reserved_word = 0;
ushort positive_current_activity_ticks = 0;
ushort cold_low_soc_divider = 0;
short cold_low_soc_trip_ticks = 0;
short cold_low_soc_clear_ticks = 0;
ushort resistance_estimates[2] = {0, 0};
float positive_compensation_current_floor = 0.0f;
float negative_compensation_current_floor = 0.0f;
int capacity_integrated_ticks = 0;
int capacity_display_integrated_ticks = 0;
int soc_rest_integrated_current = 0;
uint soc_correction_elapsed = 0;
uint soc_correction_day_ticks = 0;
uint cell_undervoltage_shutdown_state = 0;
ushort soc_rate_duration_ticks[4] = {0, 0, 0, 0};
uchar protection_debounce_settings[8] = {0};
ushort soc_fixed_mean_voltage_curve[10][5] = {
    {0, 2719, 2695, 2813, 2807},  {5, 3159, 3166, 3189, 3195},  {10, 3219, 3221, 3224, 3231},
    {15, 3237, 3239, 3246, 3253}, {20, 3267, 3270, 3275, 3280}, {25, 3288, 3291, 3298, 3303},
    {30, 3289, 3302, 3306, 3308}, {35, 3305, 3304, 3307, 3309}, {40, 3306, 3304, 3307, 3310},
    {45, 3307, 3305, 3308, 3311}};

ushort soc_model_mean_voltage_curve[10][5] = {
    {0, 2745, 2868, 2868, 2868},  {5, 3113, 3130, 3118, 3185},  {10, 3203, 3204, 3204, 3199},
    {15, 3211, 3215, 3217, 3210}, {20, 3237, 3241, 3243, 3234}, {25, 3255, 3259, 3259, 3254},
    {30, 3270, 3276, 3275, 3264}, {35, 3289, 3286, 3280, 3277}, {40, 3293, 3288, 3282, 3279},
    {45, 3294, 3288, 3282, 3280}};
ushort soc_high_cell_curve[7][6] = {
    {70, 3370, 3380, 3420, 3441, 3434}, {75, 3380, 3390, 3424, 3448, 3442},
    {80, 3390, 3400, 3480, 3487, 3453}, {85, 3430, 3450, 3482, 3497, 3490},
    {90, 3440, 3470, 3498, 3500, 3503}, {95, 3460, 3530, 3535, 3543, 3550},
    {100, 3650, 3650, 3650, 3650, 3650}};
ushort soc_low_cell_curve[7][6] = {
    {0, 2700, 2700, 2700, 2700, 2700},  {5, 3000, 2990, 2940, 2870, 2850},
    {10, 3100, 3070, 3030, 2990, 2940}, {15, 3130, 3110, 3050, 3030, 3010},
    {20, 3170, 3150, 3100, 3070, 3060}, {25, 3200, 3170, 3155, 3140, 3125},
    {30, 3230, 3220, 3205, 3190, 3160}};
#define compensated_cell_voltages_offset_8_view                                                    \
    (*(short *)(((uchar *)compensated_cell_voltages) + 8))
#define compensated_cell_voltages_offset_14_view                                                   \
    (*(short *)(((uchar *)compensated_cell_voltages) + 14))
#define compensated_cell_voltages_offset_24_view                                                   \
    (*(short *)(((uchar *)compensated_cell_voltages) + 24))
#define compensated_cell_voltages_offset_30_view                                                   \
    (*(short *)(((uchar *)compensated_cell_voltages) + 30))

#include <string.h>
/* Candidate history layout; ownership awaits the complete unit proof. */
int positive_voltage_resistance_history[5][10];
int negative_voltage_resistance_history[5][10];
uchar soc_high_cell_history[10];
uchar soc_low_cell_history[10];
short soc_rest_current_history[100];
/* Scales cell channels and applies five current-dependent voltage corrections.
   match: cell_value is first an index, then the reference voltage; sample_index also holds a count.
   Splitting either role changes native bytes. I keep the unused stride assignments and the
   negative-history branch that resets the positive-history index. */
void measurement_compensate_cell_voltages(void)

{
    int raw_sample;
    ushort cell_value;
    float sample_as_float;
    float scaled_sample;
    uint converted_sample;
    uint sample_index;
    int current_magnitude;
    int current_work;
    int resistance_sums[5];
    int current_threshold;
    ushort result;

    memset(resistance_sums, 0, sizeof resistance_sums);
    current_work = (int)measurement_current / 1000;
    cell_value = 0;
    for (; cell_value < measurement_cell_count; cell_value = cell_value + 1 & 0xff) {
        (&averaged_cell_adc_codes[0])[cell_value] =
            (short)((uint)(&cell_adc_accumulators[0])[cell_value] / 0x32);
        {
            uint address_check;
            address_check = (uint)((const void *)(&measurement_cell_count) !=
                                   (const void *)(&averaged_cell_adc_codes[cell_value + 1]));
        }
    }
    sample_index = measurement_cell_count;
    for (cell_value = 0; cell_value < sample_index; cell_value = cell_value + 1 & 0xff) {
        raw_sample = (&averaged_cell_adc_codes[0])[cell_value];
        scaled_sample = ((float)(uint)((uint)cell_adc_reference_scale));
        scaled_sample = ((scaled_sample) / 4096.0f);
        {
            uint address_check;
            address_check = ((const void *)(averaged_cell_adc_codes) !=
                             (const void *)(&measurement_state_anchor));
        }
        sample_as_float = ((float)(uint)((uint)raw_sample));
        scaled_sample = ((sample_as_float * scaled_sample) * 2.0f);
        converted_sample = ((uint)(scaled_sample));
        (&sampled_cell_voltages[0])[cell_value] = (short)converted_sample;
    }
    cell_value = (ushort)((&sampled_cell_voltages[0])[1] + (&sampled_cell_voltages[0])[2] +
                          (&sampled_cell_voltages[0])[3] + (&sampled_cell_voltages[0])[5] +
                          (&sampled_cell_voltages[0])[6] + (&sampled_cell_voltages[0])[8] +
                          (&sampled_cell_voltages[0])[9] + (&sampled_cell_voltages[0])[10] +
                          (&sampled_cell_voltages[0])[11] + (&sampled_cell_voltages[0])[13] +
                          (&sampled_cell_voltages[0])[14]) /
                 11;
    if ((measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) || (measurement_current > 250.0f)) {
        current_magnitude = (int)measurement_current;
        current_magnitude = abs(current_magnitude);
        current_threshold = (int)(((uint)measurement_rated_capacity * 0xf) / 10);
        if (current_magnitude > current_threshold) {
            current_magnitude = (int)measurement_current;
            if (current_magnitude < 0) {
                current_magnitude = -current_magnitude;
            }
            scaled_sample = ((float)(int)(current_magnitude));
            if ((scaled_sample >= positive_compensation_current_floor) ||
                (positive_voltage_history_ready == '\0')) {
                positive_voltage_resistance_history[0][positive_voltage_history_index] =
                    (int)((sampled_cell_voltages[0] - cell_value) * 1000) / current_work;
                positive_voltage_resistance_history[1][positive_voltage_history_index] =
                    (int)((sampled_cell_voltages[4] - cell_value) * 1000) / current_work;
                positive_voltage_resistance_history[2][positive_voltage_history_index] =
                    (int)((sampled_cell_voltages[7] - cell_value) * 1000) / current_work;
                positive_voltage_resistance_history[3][positive_voltage_history_index] =
                    (int)((sampled_cell_voltages[12] - cell_value) * 1000) / current_work;
                positive_voltage_resistance_history[4][positive_voltage_history_index] =
                    (int)((sampled_cell_voltages[15] - cell_value) * 1000) / current_work;
                current_magnitude = ((int)(measurement_current));
                current_magnitude = abs(current_magnitude);
                scaled_sample = ((float)(int)(current_magnitude));
                if ((scaled_sample >= positive_compensation_current_floor) &&
                    (positive_voltage_history_ready == '\x01')) {
                    positive_voltage_history_index = 0;
                    positive_voltage_history_ready = '\0';
                }
                {
                    uint address_check;
                    address_check = (uint)((const void *)(sampled_cell_voltages) !=
                                           (const void *)(&measurement_state_anchor));
                }
                positive_voltage_history_index = positive_voltage_history_index + 1;
                if (positive_voltage_history_index >= 10) {
                    positive_voltage_history_index = 0;
                    positive_voltage_history_ready = '\x01';
                }
                sample_index = 1;
                do {
                    resistance_sums[0] =
                        resistance_sums[0] + positive_voltage_resistance_history[0][sample_index];
                    current_magnitude = sample_index * 4;
                    resistance_sums[1] =
                        resistance_sums[1] + positive_voltage_resistance_history[1][sample_index];
                    resistance_sums[2] =
                        resistance_sums[2] + positive_voltage_resistance_history[2][sample_index];
                    resistance_sums[3] =
                        resistance_sums[3] + positive_voltage_resistance_history[3][sample_index];
                    resistance_sums[4] =
                        resistance_sums[4] + positive_voltage_resistance_history[4][sample_index];
                    sample_index = sample_index + 1 & 0xff;
                } while (sample_index < 9);
                positive_cell_resistance[0] = resistance_sums[0] / 8;
                positive_cell_resistance[1] = resistance_sums[1] / 8;
                positive_cell_resistance[2] = resistance_sums[2] / 8;
                positive_cell_resistance[3] = resistance_sums[3] / 8;
                positive_cell_resistance[4] = resistance_sums[4] / 8;
            }
        }
        sample_index = 0;
        do {
            if ((&positive_cell_resistance[0])[sample_index] > 0) {
                (&cell_voltage_corrections[0])[sample_index] =
                    (short)(((&positive_cell_resistance[0])[sample_index] * abs(current_work)) /
                            1000);
            } else {
                (&cell_voltage_corrections[0])[sample_index] = 0;
            }
            sample_index = sample_index + 1 & 0xff;
        } while (sample_index < 5);
        current_work = (int)measurement_current;
        current_work = current_work < 0 ? -current_work : current_work;
        scaled_sample = ((float)(int)(current_work));
        if (scaled_sample > positive_compensation_current_floor) {
            current_work = (int)measurement_current;
            if (current_work < 0) {
                current_work = -current_work;
            }
            positive_compensation_current_floor = ((float)(int)(current_work + -2000));
        }
    } else if ((measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) ||
               (measurement_current < -250.0f)) {
        current_magnitude = (int)measurement_current;
        current_magnitude = current_magnitude < 0 ? -current_magnitude : current_magnitude;
        current_threshold = (int)(((uint)measurement_rated_capacity * 0xf) / 10);
        if (current_magnitude > current_threshold) {
            current_magnitude = (int)measurement_current;
            if (current_magnitude < 0) {
                current_magnitude = -current_magnitude;
            }
            scaled_sample = ((float)(int)(current_magnitude));
            if ((scaled_sample >= negative_compensation_current_floor) ||
                (negative_voltage_history_ready == '\0')) {
                negative_voltage_resistance_history[0][negative_voltage_history_index] =
                    (int)((sampled_cell_voltages[0] - cell_value) * 1000) / current_work;
                negative_voltage_resistance_history[1][negative_voltage_history_index] =
                    (int)((sampled_cell_voltages[4] - cell_value) * 1000) / current_work;
                negative_voltage_resistance_history[2][negative_voltage_history_index] =
                    (int)((sampled_cell_voltages[7] - cell_value) * 1000) / current_work;
                negative_voltage_resistance_history[3][negative_voltage_history_index] =
                    (int)((sampled_cell_voltages[12] - cell_value) * 1000) / current_work;
                negative_voltage_resistance_history[4][negative_voltage_history_index] =
                    (int)((sampled_cell_voltages[15] - cell_value) * 1000) / current_work;
                current_magnitude = ((int)(measurement_current));
                if (current_magnitude < 0) {
                    current_magnitude = -current_magnitude;
                }
                scaled_sample = ((float)(int)(current_magnitude));
                if ((scaled_sample >= negative_compensation_current_floor) &&
                    (negative_voltage_history_ready == '\x01')) {
                    positive_voltage_history_index = 0;
                    negative_voltage_history_ready = '\0';
                }
                negative_voltage_history_index = negative_voltage_history_index + 1;
                if (negative_voltage_history_index >= 10) {
                    negative_voltage_history_index = 0;
                    negative_voltage_history_ready = '\x01';
                }
                {
                    uint address_check;
                    address_check = (uint)((const void *)(&sampled_cell_voltages[2]) !=
                                           (const void *)(&measurement_current));
                }
                sample_index = 1;
                do {
                    resistance_sums[0] =
                        resistance_sums[0] + negative_voltage_resistance_history[0][sample_index];
                    current_magnitude = sample_index * 4;
                    resistance_sums[1] =
                        resistance_sums[1] + negative_voltage_resistance_history[1][sample_index];
                    resistance_sums[2] =
                        resistance_sums[2] + negative_voltage_resistance_history[2][sample_index];
                    resistance_sums[3] =
                        resistance_sums[3] + negative_voltage_resistance_history[3][sample_index];
                    resistance_sums[4] =
                        resistance_sums[4] + negative_voltage_resistance_history[4][sample_index];
                    sample_index = sample_index + 1 & 0xff;
                } while (sample_index < 9);
                negative_cell_resistance[0] = resistance_sums[0] / 8;
                negative_cell_resistance[1] = resistance_sums[1] / 8;
                negative_cell_resistance[2] = resistance_sums[2] / 8;
                negative_cell_resistance[3] = resistance_sums[3] / 8;
                negative_cell_resistance[4] = resistance_sums[4] / 8;
            }
        }
        sample_index = 0;
        do {
            if ((&negative_cell_resistance[0])[sample_index] > 0) {
                (&cell_voltage_corrections[0])[sample_index] =
                    (short)(((&negative_cell_resistance[0])[sample_index] * abs(current_work)) /
                            1000);
            } else {
                (&cell_voltage_corrections[0])[sample_index] = 0;
            }
            sample_index = sample_index + 1 & 0xff;
        } while (sample_index < 5);
        current_work = abs(((int)(measurement_current)));
        scaled_sample = ((float)(int)(current_work));
        if (scaled_sample > negative_compensation_current_floor) {
            current_work = abs(((int)(measurement_current)));
            negative_compensation_current_floor = ((float)(int)(current_work + -2000));
        }
    } else {
        sample_index = 0;
        do {
            (&cell_voltage_corrections[0])[sample_index] = 0;
            sample_index = sample_index + 1 & 0xff;
        } while (sample_index < 5);
    }
    sample_index = 0;
    for (; sample_index < measurement_cell_count; sample_index = sample_index + 1 & 0xff) {
        (&compensated_cell_voltages[0])[sample_index] = (&sampled_cell_voltages[0])[sample_index];
    }
    if ((measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) || (measurement_current > 250.0f)) {
        result = (uint)sampled_cell_voltages[0] - (uint)cell_voltage_corrections[0];
        if ((result < cell_value) && (cell_voltage_corrections[0] != 0)) {
            compensated_cell_voltages[0] = (short)cell_value;
        } else {
            compensated_cell_voltages[0] = (short)result;
        }
        result = (uint)sampled_cell_voltages[4] - (uint)cell_voltage_corrections[1];
        if ((result < cell_value) && (cell_voltage_corrections[1] != 0)) {
            compensated_cell_voltages_offset_8_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_8_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[7] - (uint)cell_voltage_corrections[2];
        if ((result < cell_value) && (cell_voltage_corrections[2] != 0)) {
            compensated_cell_voltages_offset_14_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_14_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[12] - (uint)cell_voltage_corrections[3];
        if ((result < cell_value) && (cell_voltage_corrections[3] != 0)) {
            compensated_cell_voltages_offset_24_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_24_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[15] - (uint)cell_voltage_corrections[4];
        if ((result < cell_value) && (cell_voltage_corrections[4] != 0)) {
            compensated_cell_voltages_offset_30_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_30_view = (short)result;
        }
    } else if ((measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) ||
               (measurement_current < -250.0f)) {
        result = (uint)sampled_cell_voltages[0] + (uint)cell_voltage_corrections[0];
        if ((result > cell_value) && (cell_voltage_corrections[0] != 0)) {
            compensated_cell_voltages[0] = (short)cell_value;
        } else {
            compensated_cell_voltages[0] = (short)result;
        }
        result = (uint)sampled_cell_voltages[4] + (uint)cell_voltage_corrections[1];
        if ((result > cell_value) && (cell_voltage_corrections[1] != 0)) {
            compensated_cell_voltages_offset_8_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_8_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[7] + (uint)cell_voltage_corrections[2];
        if ((result > cell_value) && (cell_voltage_corrections[2] != 0)) {
            compensated_cell_voltages_offset_14_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_14_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[12] + (uint)cell_voltage_corrections[3];
        if ((result > cell_value) && (cell_voltage_corrections[3] != 0)) {
            compensated_cell_voltages_offset_24_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_24_view = (short)result;
        }
        result = (uint)sampled_cell_voltages[15] + (uint)cell_voltage_corrections[4];
        if ((result > cell_value) && (cell_voltage_corrections[4] != 0)) {
            compensated_cell_voltages_offset_30_view = (short)cell_value;
        } else {
            compensated_cell_voltages_offset_30_view = (short)result;
        }
    } else {
        compensated_cell_voltages[0] = sampled_cell_voltages[0] + cell_voltage_corrections[0];
        compensated_cell_voltages_offset_8_view =
            sampled_cell_voltages[4] + cell_voltage_corrections[1];
        compensated_cell_voltages_offset_14_view =
            sampled_cell_voltages[7] + cell_voltage_corrections[2];
        compensated_cell_voltages_offset_24_view =
            sampled_cell_voltages[12] + cell_voltage_corrections[3];
        compensated_cell_voltages_offset_30_view =
            sampled_cell_voltages[15] + cell_voltage_corrections[4];
    }
    return;
}

#undef compensated_cell_voltages_offset_8_view
#undef compensated_cell_voltages_offset_14_view
#undef compensated_cell_voltages_offset_24_view
#undef compensated_cell_voltages_offset_30_view

/* Filters cell samples and publishes voltage extrema, mean and pack sum. */
void measurement_update_voltage_summary(void) {
    ushort maximum_all = 0, maximum_selected = 0, maximum_reference = 0;
    ushort voltage_sum = 0;
    ushort minimum_all = 9999, minimum_selected = 9999, minimum_reference = 9999;
    uchar cell_index, sort_index;
    ushort swap_value;
    uint pack_voltage;
    float reference_scale = (float)adc_reference_scale / 4096.0f;
    independent_pack_voltage = (int)((float)independent_voltage_adc * reference_scale * 20.0f);
    auxiliary_voltage_reading =
        (int)(((float)auxiliary_voltage_adc * reference_scale * 998.0f) / 27.0f);
    if (current_substitution_active == 0) measurement_compensate_cell_voltages();
    for (cell_index = 0; cell_index < measurement_cell_count; cell_index++) {
        ushort value = compensated_cell_voltages[cell_index];
        sorted_cell_voltages[cell_index] = value;
        voltage_sum += value;
        if (value > maximum_all)
            maximum_all = value;
        if (value < minimum_all)
            minimum_all = value;
        if (cell_index != 0 && cell_index != 4 && cell_index != 7 && cell_index != 12 &&
            cell_index != 15) {
            if (value > maximum_selected)
                maximum_selected = value;
            if (value < minimum_selected)
                minimum_selected = value;
        } else {
            if (sampled_cell_voltages[cell_index] > maximum_reference)
                maximum_reference = sampled_cell_voltages[cell_index];
            if (sampled_cell_voltages[cell_index] < minimum_reference)
                minimum_reference = sampled_cell_voltages[cell_index];
        }
    }
    measurement_maximum_cell_voltage = maximum_all;
    measurement_minimum_cell_voltage = minimum_all;
    maximum_selected_cell_voltage = maximum_selected;
    minimum_selected_cell_voltage = minimum_selected;
    maximum_reference_cell_voltage = maximum_reference;
    minimum_reference_cell_voltage = minimum_reference;
    for (cell_index = 0; cell_index < measurement_cell_count; cell_index++) {
        for (sort_index = 0; sort_index < measurement_cell_count - cell_index - 1; sort_index++) {
            if (sorted_cell_voltages[sort_index] < sorted_cell_voltages[sort_index + 1]) {
                swap_value = sorted_cell_voltages[sort_index];
                sorted_cell_voltages[sort_index] = sorted_cell_voltages[sort_index + 1];
                sorted_cell_voltages[sort_index + 1] = swap_value;
            }
        }
    }
    sorted_selected_cell_voltages[0] = compensated_cell_voltages[1];
    sorted_selected_cell_voltages[1] = compensated_cell_voltages[2];
    sorted_selected_cell_voltages[2] = compensated_cell_voltages[3];
    sorted_selected_cell_voltages[3] = compensated_cell_voltages[5];
    sorted_selected_cell_voltages[4] = compensated_cell_voltages[6];
    sorted_selected_cell_voltages[5] = compensated_cell_voltages[8];
    sorted_selected_cell_voltages[6] = compensated_cell_voltages[9];
    sorted_selected_cell_voltages[7] = compensated_cell_voltages[10];
    sorted_selected_cell_voltages[8] = compensated_cell_voltages[11];
    sorted_selected_cell_voltages[9] = compensated_cell_voltages[13];
    sorted_selected_cell_voltages[10] = compensated_cell_voltages[14];
    for (cell_index = 0; cell_index < 11; cell_index++) {
        for (sort_index = 0; sort_index < 10 - cell_index; sort_index++) {
            if (sorted_selected_cell_voltages[sort_index] <
                sorted_selected_cell_voltages[sort_index + 1]) {
                swap_value = sorted_selected_cell_voltages[sort_index];
                sorted_selected_cell_voltages[sort_index] =
                    sorted_selected_cell_voltages[sort_index + 1];
                sorted_selected_cell_voltages[sort_index + 1] = swap_value;
            }
        }
    }
    pack_voltage = (uint)((float)voltage_sum * pack_voltage_scale);
    measurement_pack_voltage = (ushort)pack_voltage;
    mean_cell_voltage = (ushort)pack_voltage / measurement_cell_count;
}

/* Piecewise ADC-to-temperature conversion used by the temperature channels. */
int measurement_convert_primary_thermistor(uint adc_sample)

{
    if (adc_sample < 0x170) {
        return (int)(short)-((short)((uint)adc_sample * 0xf7a0 >> 0x10) + -0x54c);
    }
    if (adc_sample < 0x1db) {
        return (int)(short)-((short)((uint)adc_sample * 0xf1a0 >> 0x10) + -0x544);
    }
    if (adc_sample < 0x266) {
        return (int)(short)-((short)((uint)adc_sample * 0xb850 >> 0x10) + -0x4da);
    }
    if (adc_sample < 0x31c) {
        return (int)(short)-((short)((uint)adc_sample * 0x8c90 >> 0x10) + -0x471);
    }
    if (adc_sample < 0x3ff) {
        return (int)(short)-(((uint)adc_sample * 0x707 >> 0xc) + -0x419);
    }
    if (adc_sample < 0x510) {
        return (int)(short)-(((uint)adc_sample * 0x5dd >> 0xc) + -0x3cf);
    }
    if (adc_sample < 0x642) {
        return (int)(short)-(((uint)adc_sample * 0x53d >> 0xc) + -0x39d);
    }
    if (adc_sample < 0x77c) {
        return (int)(short)-(((uint)adc_sample * 0x518 >> 0xc) + -0x38e);
    }
    if (adc_sample < 0x8a7) {
        return (int)(short)(0x3ac - ((uint)adc_sample * 0x558 >> 0xc));
    }
    if (adc_sample < 0x9ae) {
        return (int)(short)-(((uint)adc_sample * 0x615 >> 0xc) + -0x412);
    }
    if (adc_sample < 0xa85) {
        return (int)(short)-(((uint)adc_sample * 0x776 >> 0xc) + -0x4e7);
    }
    if (adc_sample < 0xb1c) {
        return (int)(short)(0x6f0 - (short)((uint)adc_sample * 0xa8e0 >> 0x10));
    }
    if (adc_sample < 0xb87) {
        return (int)(short)-((short)((uint)adc_sample * 0xf040 >> 0x10) + -0xa09);
    }
    return (int)(short)(0x1072 - (short)((uint)adc_sample * 0x17ea0 >> 0x10));
}

/* Second piecewise ADC-to-temperature transfer curve. */
int measurement_convert_secondary_thermistor(uint adc_sample)

{
    if (adc_sample < 0x139) {
        return (int)(short)-((short)((uint)adc_sample * 0x18c10 >> 0x10) + -0x569);
    }
    if (adc_sample < 0x20d) {
        return (int)(short)-((short)((uint)adc_sample * 0xf1a0 >> 0x10) + -0x4ac);
    }
    if (adc_sample < 0x357) {
        return (int)(short)(0x3f8 - (short)((uint)adc_sample * 0x9a90 >> 0x10));
    }
    if (adc_sample < 0x4f0) {
        return (int)(short)-(((uint)adc_sample * 0x7d4 >> 0xc) + -0x396);
    }
    if (adc_sample < 0x664) {
        return (int)(short)-((short)((uint)adc_sample * 0x89f0 >> 0x10) + -0x3d5);
    }
    if (adc_sample < 0x755) {
        return (int)(short)-((short)((uint)adc_sample * 0xd3e0 >> 0x10) + -0x5ae);
    }
    return (int)(short)-((short)((uint)adc_sample * 0x1bf40 >> 0x10) + -0xc6c);
}

/* Converts MOS/ambient channels and updates their separate fault bits. */
void measurement_update_auxiliary_temperatures(void)

{
    short converted_temperature, raw_input;
    int temperature_celsius;

    raw_input = raw_temperature_samples[3];
    temperature_celsius = measurement_convert_primary_thermistor((ushort)raw_input);
    temperature_readings[3] = (short)temperature_celsius + 0xaaa;
    if (temperature_readings[3] < 0x91a) {
        temperature_readings[3] = 0x91a;
    }
    if ((int)temperature_readings[3] - 0x97fU <= 0x3b4) {
        auxiliary_temperature_sensor_faults = auxiliary_temperature_sensor_faults & 0xfe;
    } else {
        auxiliary_temperature_sensor_faults = auxiliary_temperature_sensor_faults | 1;
    }
    raw_input = raw_temperature_samples[4];
    temperature_celsius = measurement_convert_primary_thermistor((ushort)raw_input);
    converted_temperature = (short)temperature_celsius + 0xaaa;
    *(short *)(((int)temperature_readings) + 8) = converted_temperature;
    if (converted_temperature < 0x91a) {
        *(ushort *)(((int)temperature_readings) + 8) = 0x91a;
    }
    if ((int)*(short *)(((int)temperature_readings) + 8) - 0x97fU < 0x545) {
        auxiliary_temperature_sensor_faults = auxiliary_temperature_sensor_faults & 0xfd;
        return;
    }
    auxiliary_temperature_sensor_faults = auxiliary_temperature_sensor_faults | 2;
    return;
}

#define temperature_readings_view (*(ushort *)(((uchar *)temperature_readings) + 0))

static __forceinline void update_temperature_fault(int reading, uint index) {
    if (reading - 0x97fU <= 0x3b4) {
        primary_temperature_sensor_faults &= ~(1U << index);
    } else {
        primary_temperature_sensor_faults |= 1U << index;
    }
}
/* Converts four channels and publishes maximum, minimum and mean. */
void measurement_update_temperature_summary(void)

{
    short converted_temperature, raw_input;
    int temperature_reading;
    uint sensor_index;
    ushort sum = 0;

    measurement_maximum_temperature = 0;
    measurement_minimum_temperature = 9999;
    sensor_index = 0;
    do {
        raw_input = (&raw_temperature_samples[0])[sensor_index];
        temperature_reading = measurement_convert_primary_thermistor((ushort)raw_input);
        converted_temperature = (short)temperature_reading + 0xaaa;
        (&temperature_readings_view)[sensor_index] = converted_temperature;
        if (converted_temperature < 0x91a) {
            (&temperature_readings_view)[sensor_index] = 0x91a;
        }
        temperature_reading = (int)(short)(&temperature_readings_view)[sensor_index];
        update_temperature_fault(temperature_reading, sensor_index);
        sum = temperature_reading + sum;
        if (temperature_reading > (int)(uint)measurement_maximum_temperature) {
            measurement_maximum_temperature = (&temperature_readings_view)[sensor_index];
        }
        if (temperature_reading < (int)(uint)measurement_minimum_temperature) {
            measurement_minimum_temperature = (&temperature_readings_view)[sensor_index];
        }
        sensor_index = sensor_index + 1 & 0xff;
    } while (sensor_index < 4);
    mean_temperature = (short)(sum / 4);
    return;
}

#undef temperature_readings_view
#define cell_voltage_step_detected_view (*(uchar *)(((uchar *)&cell_voltage_step_detected) + 0))

/* Debounces large adjacent-cell differences with asymmetric recovery. */
void measurement_detect_cell_voltage_steps(void) {
    uchar i;
    ushort max = 0;
    ushort diff[16];
    for (i = 0; i < measurement_cell_count - 1; i++) {
        diff[i] =
            abs((short)compensated_cell_voltages[i] - (short)compensated_cell_voltages[i + 1]);
        if (diff[i] > max)
            max = diff[i];
    }
    if (max >= 800) {
        cell_voltage_step_clear_ticks = 0;
        if (cell_voltage_step_trip_ticks++ >= 5) {
            cell_voltage_step_trip_ticks = 0;
            cell_voltage_step_detected = 1;
        }
    } else {
        cell_voltage_step_trip_ticks = 0;
        if (cell_voltage_step_clear_ticks++ >= 300) {
            cell_voltage_step_clear_ticks = 0;
            cell_voltage_step_detected = 0;
        }
    }
}

#undef cell_voltage_step_detected_view

/* Runs voltage summary, temperature summary and cell-step detection. */
void measurement_update_voltage_and_temperature(void)

{
    measurement_update_voltage_summary();
    measurement_update_temperature_summary();
    measurement_detect_cell_voltage_steps();
    return;
}


/* Trims two 200-sample ADC histories and applies current calibration.
   match: The first-channel zero estimate subtracts the second-channel extrema.
   I retain that expression and the double-precision literal spellings. */
void measurement_update_current(void) {
    int max1, min1;
    int max2;
    int min2;
    int sum1;
    int sum2;
    uchar i;
    float current;
    int substitution_threshold;
    ushort cached_total_ticks;
    bool substitute_current = false;

    max1 = -9999;
    min1 = 9999;
    sum1 = 0;
    i = 0;
    do {
        sum1 += primary_current_samples[i];
        if (primary_current_samples[i] > max1) {
            max1 = primary_current_samples[i];
        }
        if (primary_current_samples[i] < min1) {
            min1 = primary_current_samples[i];
        }
        i++;
    } while (i < 200);
    sum1 -= max1 + min1;
    if (sum1 > 0) {
        if (current_adc_options & 1) current = (float)sum1 / 198.0f * ((float)adc_reference_scale / 4096.0f) * 1150.0f * 2.0f /
                   current_adc_gain * positive_current_scale;
        else current = (float)sum1 / 198.0f * ((float)adc_reference_scale / 4096.0f) * 1150.0f /
               current_adc_gain * positive_current_scale;
    } else {
        if (current_adc_options & 1) current = (float)sum1 / 198.0f * ((float)adc_reference_scale / 4096.0f) * 1150.0f * 2.0f /
               current_adc_gain * negative_current_scale;
        else current = (float)sum1 / 198.0f * ((float)adc_reference_scale / 4096.0f) * 1150.0f /
           current_adc_gain * negative_current_scale;
    }

    max2 = -9999;
    min2 = 9999;
    sum2 = 0;
    i = 0;
    do {
        sum2 += secondary_current_samples[i];
        if (secondary_current_samples[i] > max2) {
            max2 = secondary_current_samples[i];
        }
        if (secondary_current_samples[i] < min2) {
            min2 = secondary_current_samples[i];
        }
        i++;
    } while (i < 200);
    if (current_zero_calibrated == 0) {
        if ((capacity_latch_flags & 8) && (capacity_latch_flags & 0x10)) {
            secondary_current_zero_estimate = (sum2 - max2 - min2) / 198;
            primary_current_zero_estimate = (sum1 - max2 - min2) / 198;
        } else {
            current_zero_calibrated = 1;
            secondary_current_zero = secondary_current_zero_estimate;
            primary_current_zero = primary_current_zero_estimate;
        }
    }
    sum2 -= max2 + min2;
    secondary_current_reading =
        (sum2 > 0 ? (current_adc_options & 1
                         ? ((float)sum2 / 198.0f * ((float)adc_reference_scale / 4096.0f)) * 1010.0f * 2.0f / 21.0f * positive_current_scale
                         : ((float)sum2 / 198.0f * ((float)adc_reference_scale / 4096.0f)) * 970.0f / 13.0f * positive_current_scale)
                  : (current_adc_options & 1
                         ? ((float)sum2 / 198.0f * ((float)adc_reference_scale / 4096.0f)) * 1010.0f * 2.0f / 21.0f * negative_current_scale
                         : ((float)sum2 / 198.0f * ((float)adc_reference_scale / 4096.0f)) * 970.0f / 13.0f * negative_current_scale));
    if (current < 0.0f) {
        if (current_adc_options & 1) {
            measurement_current = current * 0.95 / 0.96;
        } else {
            measurement_current = current * 0.962;
        }
    }
    if (current > 0.0f) {
        if (current_adc_options & 1) {
            measurement_current = current * 0.93 / 0.95;
        } else {
            measurement_current = current * 0.961;
        }
    }
    if (afe_calibration_ready == 1 && settings_initialized == 1 && measurement_ready == 1) {
        substitution_threshold = measurement_protection_settings[4] * 1000;
        if ((float)substitution_threshold < measurement_current &&
            secondary_current_reading < 10000.0f) {
            if ((measurement_maximum_cell_voltage < 3500 && measurement_pack_voltage < 56000) ||
                current_substitution_retries == 0) {
                substitute_current = true;
            }
            current_substitution_retry_ticks = 0;
        }
        if (current_substitution_active != 0 || substitute_current == 1) {
            cached_total_ticks = current_substitution_total_ticks;
            if (cached_total_ticks < 36000 && current_substitution_retry_ticks < 6000) {
                if (substitute_current) current_substitution_active = 1;
                current_substitution_total_ticks = cached_total_ticks + 1;
                current_substitution_retry_ticks++;
            } else {
                if (current_substitution_retries < 200) current_substitution_retries++;
                current_substitution_active = 0;
                current_substitution_total_ticks = 0;
                current_substitution_retry_ticks = 0;
            }
        } else {
            current_substitution_total_ticks = 0;
        }
    } else {
        current_substitution_active = 0;
        current_substitution_total_ticks = 0;
    }
    if (substitute_current) measurement_current = secondary_current_reading;
    integrated_current = measurement_current - 30.0f;
}

#define capacity_learning_active_view (*(char *)(((uchar *)&capacity_learning_active) + 0))

/* Integrates current into remaining and displayed capacity with mode-dependent correction.
   match: I keep the unsigned qualifying test, wrap/cast forms and negated floating comparisons. */
void capacity_integrate_current(void) {
    uint cur;
    int integrated;
    float f;
    uint delta; /* default-mode rate, zero unless the current qualifies */
    uint step;  /* charge step added to the tick counter */

    delta = 0;
    integrated = (int)integrated_current;
    cur = (uint)integrated;
    if ((uint)integrated >= 201) {
        capacity_integrated_ticks = capacity_integrated_ticks + (uint)integrated;
    }
    if (capacity_integrated_ticks >= 0x15f900) {
        remaining_capacity = remaining_capacity + 1.0f;
        capacity_integrated_ticks = capacity_integrated_ticks - 0x15f900;
    } else if (capacity_integrated_ticks <= -0x15f900) {
        remaining_capacity = remaining_capacity - 1.0f;
        capacity_integrated_ticks = capacity_integrated_ticks + 0x15f900;
    }
    if (primary_soc > displayed_soc) {
        switch (measurement_current_mode) {
        case BMS_CURRENT_MODE_POSITIVE:
            if ((int)cur < 5000) {
                if ((int)cur < 500) {
                    step = (measurement_rated_capacity * 10) / 10;
                } else if ((int)cur < 1000) {
                    step = (measurement_rated_capacity * 9) / 10;
                } else if ((int)cur < 2000) {
                    step = (measurement_rated_capacity * 8) / 10;
                } else if ((int)cur < 3000) {
                    step = (measurement_rated_capacity * 7) / 10;
                } else {
                    step = (measurement_rated_capacity * 6) / 10;
                }
                step = (int)(cur * step) / 1000;
            } else {
                f = (float)(int)(measurement_rated_capacity + cur);
                step = (int)(f * soc_correction_gain);
            }
            if (capacity_voltage_hold_active != 0 && displayed_soc <= 92) {
                step = cur;
            }
            break;
        case BMS_CURRENT_MODE_NEGATIVE:
            step = (int)cur / 3;
            break;
        default:
            if (0 < (int)cur) {
                delta = (int)(measurement_rated_capacity * cur) / 1000;
            }
            step = delta;
            break;
        }
        {
            int ticks = capacity_display_integrated_ticks + step;
            capacity_display_integrated_ticks = ticks;
            if (ticks >= 0x15f900) {
                displayed_capacity = displayed_capacity + 1.0f;
                capacity_display_integrated_ticks = ticks - 0x15f900;
            } else if (ticks <= -0x15f900) {
                displayed_capacity = displayed_capacity - 1.0f;
                capacity_display_integrated_ticks = ticks + 0x15f900;
            }
        }
    } else if (primary_soc < displayed_soc) {
        if (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) {
            step = (int)cur / 3;
        } else if (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
            if ((int)cur < -5000) {
                f = (float)(int)(cur - (measurement_rated_capacity >> 2));
                step = (int)(f * soc_correction_gain);
            } else {
                if ((int)cur < -3000) {
                    step = (measurement_rated_capacity * 3) / 10;
                } else if ((int)cur < -2000) {
                    step = (measurement_rated_capacity * 5) / 10;
                } else if ((int)cur < -1000) {
                    step = (measurement_rated_capacity * 7) / 10;
                } else if ((int)cur < -500) {
                    step = (measurement_rated_capacity * 8) / 10;
                } else {
                    step = (measurement_rated_capacity * 10) / 10;
                }
                step = (int)(step * cur) / 1000;
            }
        } else {
            if ((int)cur < 0) {
                delta = (int)(cur * (measurement_rated_capacity << 1)) / 1000;
            }
            step = delta;
        }
        {
            int ticks = capacity_display_integrated_ticks + step;
            capacity_display_integrated_ticks = ticks;
            if (ticks >= 0x15f900) {
                displayed_capacity = displayed_capacity + 1.0f;
                capacity_display_integrated_ticks = ticks - 0x15f900;
            } else if (ticks <= -0x15f900) {
                displayed_capacity = displayed_capacity - 1.0f;
                capacity_display_integrated_ticks = ticks + 0x15f900;
            }
        }
    } else {
        displayed_capacity = remaining_capacity;
        capacity_display_integrated_ticks = capacity_integrated_ticks;
    }
    if (!(remaining_capacity >= 0.0f)) {
        remaining_capacity = 0.0f;
        return;
    }
    if (capacity_learning_active == 0 && !(remaining_capacity <= full_capacity)) {
        remaining_capacity = full_capacity;
        return;
    }
    if (remaining_capacity > 65535.0f) {
        remaining_capacity = 65535.0f;
    }
}

#undef capacity_learning_active_view

/* Accumulates negative current and persists cycle completion state. */
void capacity_update_cycle_count(void)

{

    if ((cycle_count_flags & 1) != 0) {
        cycle_count_flags = 0;
        cycle_count = cycle_count + 1;
        eeprom_write_word(10, cycle_count);
        eeprom_write_word(0x10, (ushort)cycle_count_flags);
    }
    if ((integrated_current < 0.0f) && ((cycle_count_flags & 1) == 0)) {
        cycle_integrated_capacity = cycle_integrated_capacity - integrated_current / 360000.0f;
        if ((double)cycle_integrated_capacity > full_capacity * 0.8) {
            if ((double)cycle_integrated_capacity > measurement_rated_capacity * 0.2) {
                cycle_integrated_capacity = 0.0f;
                cycle_count_flags = cycle_count_flags | 1;
                eeprom_write_word(0xe, (ushort)cycle_integrated_capacity);
                eeprom_write_word(0x10, (ushort)cycle_count_flags);
                return;
            }
        }
    }
    return;
}

/* Publishes an operating status from output and full-capacity latch bits. */
void capacity_update_operating_status(void)

{
    if (((int)((uint)power_output_status_flags << 0x1e) < 0) &&
        ((int)((uint)power_output_status_flags << 0x1d) < 0)) {
        capacity_operating_status = 1;
        return;
    }
    if ((capacity_latch_flags & 1) != 0) {
        capacity_operating_status = 2;
        return;
    }
    capacity_operating_status = 0;
    return;
}

#define cell_voltage_step_detected_view (*(char *)(((uchar *)&cell_voltage_step_detected) + 0))
#define capacity_learning_active_view (*(uchar *)(((uchar *)&capacity_learning_active) + 0))
#define full_capacity_recorded_view (*(char *)(((uchar *)&full_capacity_recorded) + 0))
#define model_code_view (*(char *)(((uchar *)model_code) + 0))
#define model_code_offset_1_view (*(char *)(((uchar *)model_code) + 1))
#define capacity_full_latch_view (*(uchar *)(((uchar *)&capacity_full_latch) + 0))
#define capacity_learning_ticks_view (*(uint *)(((uchar *)&capacity_learning_ticks) + 0))

/* Latches upper-voltage limits, updates learned capacity and persists full state. */
void capacity_latch_full_voltage(void)

{
    uchar flags = voltage_current_limit_flags;
    float rated_capacity;

    if ((flags & 5) == 0) {
        capacity_full_voltage_seen = '\0';
    } else if ((cell_voltage_step_detected_view == '\0') && (capacity_full_voltage_seen == '\0')) {
        if (capacity_learning_active_view == 0) {
            remaining_capacity = full_capacity;
            displayed_capacity = full_capacity;
        }
        if (full_capacity_recorded_view == '\0') {
            full_capacity_recorded_view = '\x01';
            eeprom_write_word(4, (ushort)full_capacity_recorded_view);
            if (capacity_learning_active_view != 0) {
                rated_capacity = ((float)(uint)((uint)measurement_rated_capacity));
                if (rated_capacity >= remaining_capacity) {
                    full_capacity = remaining_capacity;
                } else {
                    if (((model_code_view == 'B') && (model_code_offset_1_view == 'G')) ||
                        ((model_code_view == 'D' && ((model_code_offset_1_view == 'G' ||
                                                      (model_code_offset_1_view == 'Y')))))) {
                        if (remaining_capacity > 17300.0f) {
                            full_capacity = 17300.0f;
                        } else {
                            full_capacity = remaining_capacity;
                        }
                    } else if ((model_code_view == 'D') && ((((model_code_offset_1_view == 'X' ||
                                                               (model_code_offset_1_view == 'Z')) ||
                                                              (model_code_offset_1_view == 'W')) ||
                                                             (model_code_offset_1_view == 'F')))) {
                        if (remaining_capacity > 20000.0f) {
                            full_capacity = 20000.0f;
                        } else {
                            full_capacity = remaining_capacity;
                        }
                    } else {
                        full_capacity = ((float)(uint)((uint)measurement_rated_capacity));
                    }
                }
                capacity_full_latch_view = 1;
                capacity_latch_flags = capacity_latch_flags | 1;
                rated_capacity =
                    ((float)(uint)((uint)(((uint)measurement_rated_capacity * 8U) / 10)));
                if (rated_capacity > full_capacity) {
                    full_capacity =
                        ((float)(uint)((uint)(((uint)measurement_rated_capacity * 8U) / 10)));
                }
                full_capacity_snapshot = full_capacity;
                capacity_learning_active_view = 0;
                capacity_learning_ticks_view = 0;
                if ((int)((uint)cycle_count_flags << 0x1e) < 0) {
                    ushort count;
                    cycle_count_flags = 0;
                    count = (ushort)(cycle_count + 1);
                    cycle_count = count;
                    cycle_integrated_capacity = 0.0f;
                    eeprom_write_word(10, count);
                    eeprom_write_word(0xe, (ushort)cycle_integrated_capacity);
                    eeprom_write_word(0x10, (ushort)cycle_count_flags);
                }
                eeprom_write_word(2, (ushort)capacity_learning_active_view);
                eeprom_write_word(6, (ushort)remaining_capacity);
                eeprom_write_word(8, (ushort)full_capacity);
            }
        }
        capacity_full_voltage_seen = '\x01';
    }
    return;
}

#undef cell_voltage_step_detected_view
#undef capacity_learning_active_view
#undef full_capacity_recorded_view
#undef model_code_view
#undef model_code_offset_1_view
#undef capacity_full_latch_view
#undef capacity_learning_ticks_view
#define cell_voltage_step_detected_view (*(char *)(((uchar *)&cell_voltage_step_detected) + 0))
#define capacity_learning_active_view (*(char *)(((uchar *)&capacity_learning_active) + 0))
#define full_capacity_recorded_view (*(uchar *)(((uchar *)&full_capacity_recorded) + 0))
#define capacity_learning_ticks_view (*(uint *)(((uchar *)&capacity_learning_ticks) + 0))

/* Latches lower-voltage limits, zeroes remaining capacity and persists empty state. */
void capacity_latch_empty_voltage(void)

{
    uchar flags = voltage_current_limit_flags;

    if ((flags & 10) == 0) {
        capacity_empty_voltage_seen = '\0';
    } else if ((cell_voltage_step_detected_view == '\0') && (capacity_empty_voltage_seen == '\0')) {
        capacity_empty_voltage_seen = '\x01';
        if (remaining_capacity != 0.0f) {
            remaining_capacity = 0;
        }
        if (-1 < (int)((uint)cycle_count_flags << 0x1e)) {
            cycle_count_flags |= 2;
            eeprom_write_word(0x10, (ushort)cycle_count_flags);
        }
        if (capacity_learning_active_view == '\0') {
            capacity_learning_active_view = 1;
            full_capacity_recorded_view = 0;
            capacity_learning_ticks_view = 0;
            eeprom_write_word(2, (ushort)capacity_learning_active_view);
            eeprom_write_word(4, (ushort)full_capacity_recorded_view);
            eeprom_write_word(6, 0);
            return;
        }
    }
    return;
}

#undef cell_voltage_step_detected_view
#undef capacity_learning_active_view
#undef full_capacity_recorded_view
#undef capacity_learning_ticks_view
#define model_code_view (*(char *)(((uchar *)model_code) + 0))
#define model_code_offset_1_view (*(char *)(((uchar *)model_code) + 1))

/* Copies one model-specific low-SOC voltage curve into the working table. */
void soc_select_model_curve(void) {
    uchar row;
    uchar column;

    row = 0;
    if ((model_code[0] == 'B' && model_code[1] == 'J') ||
        (model_code[0] == 'D' && model_code[1] == 'X')) {
        do {
            column = 0;
            do {
                soc_model_mean_voltage_curve[row][column] = soc_model_curve_bj_dx[row][column];
                column++;
            } while (column < 5);
            row++;
        } while (row < 10);
    } else if (model_code[0] == 'B' && (model_code[1] == 'E' || model_code[1] == 'K')) {
        do {
            column = 0;
            do {
                soc_model_mean_voltage_curve[row][column] = soc_model_curve_be_bk[row][column];
                column++;
            } while (column < 5);
            row++;
        } while (row < 10);
    }
}

#undef model_code_view
#undef model_code_offset_1_view

/* Returns the full-capacity scalar. */
float capacity_get_full_capacity(void)

{
    return full_capacity;
}

/* Interpolates SOC from the second-highest selected-cell voltage and minimum temperature. */
uchar soc_lookup_high_cell(void) {
    uchar found_columns = 0;
    uchar row;
    short column_soc[6] = {100, 100, 100, 100, 100, 100};
    ushort temperature;
    ushort cell_voltage;

    temperature = measurement_minimum_temperature;
    cell_voltage = sorted_selected_cell_voltages[1];
    row = 1;
    do {
        if ((found_columns & 1) == 0 && soc_high_cell_curve[row][1] >= cell_voltage) {
            found_columns |= 1;
            column_soc[1] =
                (cell_voltage - soc_high_cell_curve[row - 1][1]) * 1000 /
                    (int)((float)(soc_high_cell_curve[row][1] - soc_high_cell_curve[row - 1][1]) *
                          1000.0f / 5.0f) +
                soc_high_cell_curve[row - 1][0];
        }
        if ((found_columns & 2) == 0 && soc_high_cell_curve[row][2] >= cell_voltage) {
            found_columns |= 2;
            column_soc[2] =
                (cell_voltage - soc_high_cell_curve[row - 1][2]) * 1000 /
                    (int)((float)(soc_high_cell_curve[row][2] - soc_high_cell_curve[row - 1][2]) *
                          1000.0f / 5.0f) +
                soc_high_cell_curve[row - 1][0];
        }
        if ((found_columns & 4) == 0 && soc_high_cell_curve[row][3] >= cell_voltage) {
            found_columns |= 4;
            column_soc[3] =
                (cell_voltage - soc_high_cell_curve[row - 1][3]) * 1000 /
                    (int)((float)(soc_high_cell_curve[row][3] - soc_high_cell_curve[row - 1][3]) *
                          1000.0f / 5.0f) +
                soc_high_cell_curve[row - 1][0];
        }
        if ((found_columns & 8) == 0 && soc_high_cell_curve[row][4] >= cell_voltage) {
            found_columns |= 8;
            column_soc[4] =
                (cell_voltage - soc_high_cell_curve[row - 1][4]) * 1000 /
                    (int)((float)(soc_high_cell_curve[row][4] - soc_high_cell_curve[row - 1][4]) *
                          1000.0f / 5.0f) +
                soc_high_cell_curve[row - 1][0];
        }
        if ((found_columns & 16) == 0 && soc_high_cell_curve[row][5] >= cell_voltage) {
            found_columns |= 16;
            column_soc[5] =
                (cell_voltage - soc_high_cell_curve[row - 1][5]) * 1000 /
                    (int)((float)(soc_high_cell_curve[row][5] - soc_high_cell_curve[row - 1][5]) *
                          1000.0f / 5.0f) +
                soc_high_cell_curve[row - 1][0];
        }
        row++;
    } while (row <= 6);
    if (temperature >= 3180) {
        column_soc[0] = column_soc[1];
    } else if (temperature >= 2980) {
        column_soc[0] =
            column_soc[1] + (temperature - 3180) * (column_soc[1] - column_soc[2]) / 200;
    } else if (temperature >= 2930) {
        column_soc[0] = column_soc[2] + (temperature - 2980) * (column_soc[2] - column_soc[3]) / 50;
    } else if (temperature >= 2830) {
        column_soc[0] =
            column_soc[3] + (temperature - 2930) * (column_soc[3] - column_soc[4]) / 100;
    } else if (temperature >= 2780) {
        column_soc[0] = column_soc[4] + (temperature - 2830) * (column_soc[4] - column_soc[5]) / 50;
    } else if (temperature >= 2730) {
        column_soc[0] = column_soc[5] + (temperature - 2780) * (column_soc[4] - column_soc[5]) / 50;
    } else {
        column_soc[0] = column_soc[5];
    }
    if (column_soc[0] < 1) {
        return 1;
    }
    return column_soc[0];
}

/* Trimmed ten-sample average of the high-cell SOC lookup; zero input resets.
   A disabled or incomplete history returns zero. I retain the reset and trimmed-mean order. */
uint soc_average_high_cell(int enabled)

{
    uchar new_sample;
    uint sample;
    uint maximum_sample;
    uint minimum_sample;
    uint sample_index;
    uint average;
    ushort sample_sum;

    sample_sum = 0;
    sample_index = 0;
    if (enabled == 0) {
        if (soc_high_cell_history[0] != 0) {
            do {
                (&soc_high_cell_history[0])[sample_index] = 0;
                sample_index = sample_index + 1 & 0xff;
            } while (sample_index < 10);
        }
        average = 0;
    } else {
        if (soc_high_cell_history_index < 10) {
            new_sample = soc_lookup_high_cell();
            (&soc_high_cell_history[0])[soc_high_cell_history_index] = new_sample;
            soc_high_cell_history_index++;
        } else {
            soc_high_cell_history_index = 0;
            new_sample = soc_lookup_high_cell();
            (&soc_high_cell_history[0])[soc_high_cell_history_index] = new_sample;
            soc_high_cell_history_index++;
        }
        maximum_sample = (uint)soc_high_cell_history[0];
        minimum_sample = maximum_sample;
        do {
            sample = (uint)(&soc_high_cell_history[0])[sample_index];
            if (sample == 0) {
                return 0;
            }
            if (sample > maximum_sample) {
                maximum_sample = sample;
            }
            if (sample < minimum_sample) {
                minimum_sample = sample;
            }
            sample_sum = sample + sample_sum & 0xffff;
            sample_index = sample_index + 1 & 0xff;
        } while (sample_index < 10);
        sample_sum -= maximum_sample;
        sample_sum -= minimum_sample;
        average = sample_sum / 8;
        if (100 < average) {
            average = 100;
        }
        average = average & 0xff;
    }
    return average;
}

/* Interpolates SOC from selected-cell sorted index9 and minimum temperature. */
uchar soc_lookup_low_cell(void) {
    uchar found_columns = 0;
    uchar row;
    short column_soc[6] = {100, 100, 100, 100, 100, 100};
    ushort temperature;
    ushort cell_voltage;

    temperature = measurement_minimum_temperature;
    cell_voltage = sorted_selected_cell_voltages[9];
    row = 1;
    do {
        if ((found_columns & 1) == 0 && soc_low_cell_curve[row][1] >= cell_voltage) {
            found_columns |= 1;
            column_soc[1] =
                (cell_voltage - soc_low_cell_curve[row - 1][1]) * 1000 /
                    (int)((float)(soc_low_cell_curve[row][1] - soc_low_cell_curve[row - 1][1]) *
                          1000.0f / 5.0f) +
                soc_low_cell_curve[row - 1][0];
        }
        if ((found_columns & 2) == 0 && soc_low_cell_curve[row][2] >= cell_voltage) {
            found_columns |= 2;
            column_soc[2] =
                (cell_voltage - soc_low_cell_curve[row - 1][2]) * 1000 /
                    (int)((float)(soc_low_cell_curve[row][2] - soc_low_cell_curve[row - 1][2]) *
                          1000.0f / 5.0f) +
                soc_low_cell_curve[row - 1][0];
        }
        if ((found_columns & 4) == 0 && soc_low_cell_curve[row][3] >= cell_voltage) {
            found_columns |= 4;
            column_soc[3] =
                (cell_voltage - soc_low_cell_curve[row - 1][3]) * 1000 /
                    (int)((float)(soc_low_cell_curve[row][3] - soc_low_cell_curve[row - 1][3]) *
                          1000.0f / 5.0f) +
                soc_low_cell_curve[row - 1][0];
        }
        if ((found_columns & 8) == 0 && soc_low_cell_curve[row][4] >= cell_voltage) {
            found_columns |= 8;
            column_soc[4] =
                (cell_voltage - soc_low_cell_curve[row - 1][4]) * 1000 /
                    (int)((float)(soc_low_cell_curve[row][4] - soc_low_cell_curve[row - 1][4]) *
                          1000.0f / 5.0f) +
                soc_low_cell_curve[row - 1][0];
        }
        if ((found_columns & 16) == 0 && soc_low_cell_curve[row][5] >= cell_voltage) {
            found_columns |= 16;
            column_soc[5] =
                (cell_voltage - soc_low_cell_curve[row - 1][5]) * 1000 /
                    (int)((float)(soc_low_cell_curve[row][5] - soc_low_cell_curve[row - 1][5]) *
                          1000.0f / 5.0f) +
                soc_low_cell_curve[row - 1][0];
        }
        row++;
    } while (row <= 6);
    if (temperature >= 3180) {
        column_soc[0] = column_soc[1];
    } else if (temperature >= 2980) {
        column_soc[0] =
            column_soc[1] + (temperature - 3180) * (column_soc[1] - column_soc[2]) / 200;
    } else if (temperature >= 2930) {
        column_soc[0] = column_soc[2] + (temperature - 2980) * (column_soc[2] - column_soc[3]) / 50;
    } else if (temperature >= 2830) {
        column_soc[0] =
            column_soc[3] + (temperature - 2930) * (column_soc[3] - column_soc[4]) / 100;
    } else if (temperature >= 2780) {
        column_soc[0] = column_soc[4] + (temperature - 2830) * (column_soc[4] - column_soc[5]) / 50;
    } else if (temperature >= 2730) {
        column_soc[0] = column_soc[5] + (temperature - 2780) * (column_soc[4] - column_soc[5]) / 50;
    } else {
        column_soc[0] = column_soc[5];
    }
    if (column_soc[0] < 1) {
        return 1;
    }
    return column_soc[0];
}

/* Trimmed ten-sample average of the low-cell lookup; incomplete history returns100.
   A disabled or incomplete history returns100, unlike the high-cell helper. */
uint soc_average_low_cell(int enabled)

{
    uchar new_sample;
    uint sample;
    uint maximum_sample;
    uint minimum_sample;
    uint sample_index;
    uint average;
    ushort sample_sum;

    sample_sum = 0;
    sample_index = 0;
    if (enabled == 0) {
        if (soc_low_cell_history[0] != 0) {
            do {
                (&soc_low_cell_history[0])[sample_index] = 0;
                sample_index = sample_index + 1 & 0xff;
            } while (sample_index < 10);
        }
        average = 100;
    } else {
        if (soc_low_cell_history_index < 10) {
            new_sample = soc_lookup_low_cell();
            (&soc_low_cell_history[0])[soc_low_cell_history_index] = new_sample;
            soc_low_cell_history_index++;
        } else {
            soc_low_cell_history_index = 0;
            new_sample = soc_lookup_low_cell();
            (&soc_low_cell_history[0])[soc_low_cell_history_index] = new_sample;
            soc_low_cell_history_index++;
        }
        maximum_sample = (uint)soc_low_cell_history[0];
        minimum_sample = maximum_sample;
        do {
            sample = (uint)(&soc_low_cell_history[0])[sample_index];
            if (sample == 0) {
                return 100;
            }
            if (sample > maximum_sample) {
                maximum_sample = sample;
            }
            if (sample < minimum_sample) {
                minimum_sample = sample;
            }
            sample_sum = sample + sample_sum & 0xffff;
            sample_index = sample_index + 1 & 0xff;
        } while (sample_index < 10);
        sample_sum -= maximum_sample;
        sample_sum -= minimum_sample;
        average = sample_sum / 8;
        if (100 < average) {
            average = 100;
        }
        average = average & 0xff;
    }
    return average;
}

/* Returns an alternating correction tick unless the correction state is active. */
uint soc_correction_tick(void) {
    uint result = 0;
    if (soc_correction_tick_divider++ != 0) {
        soc_correction_tick_divider = 0;
        if (battery_heating.enabled == 0 && soc_correction_state != BMS_SOC_CORRECTION_ACTIVE)
            result = 1;
    }
    return result;
}

/* Classifies sustained low-current history and selects a mean-voltage correction. */
uint soc_evaluate_rest_correction(void) {
    uchar ret = 0;

    if (soc_rest_ticks >= 1800) {
        if (soc_correction_state >= BMS_SOC_CORRECTION_ACTIVE &&
            soc_correction_state <= BMS_SOC_CORRECTION_SPECIAL_LAST) {
            soc_rest_positive_bin = 0;
            soc_rest_near_zero_bin = 0;
            soc_rest_negative1000_bin = 0;
            soc_rest_negative2000_bin = 0;
            soc_rest_negative3000_bin = 0;
        } else if (soc_rest_integrated_current >= 180000) {
            if (measurement_current < -200.0f) {
                soc_rest_ticks = 0;
            } else {
                ret = 2;
            }
        } else {
            ret = soc_rest_near_zero_bin;
            if (ret < soc_rest_negative1000_bin)
                ret = soc_rest_negative1000_bin;
            if (ret < soc_rest_negative2000_bin)
                ret = soc_rest_negative2000_bin;
            if (ret < soc_rest_negative3000_bin)
                ret = soc_rest_negative3000_bin;
            if (ret < soc_rest_positive_bin)
                ret = soc_rest_positive_bin;
            if (soc_rest_negative3000_bin == ret || soc_rest_integrated_current < -1800000) {
                if (measurement_rated_capacity <= 6000) {
                    soc_mean_voltage_offset = 38;
                } else if (measurement_rated_capacity <= 11000) {
                    soc_mean_voltage_offset = 30;
                } else {
                    soc_mean_voltage_offset = 20;
                }
                ret = 1;
            } else if (soc_rest_negative2000_bin == ret || soc_rest_integrated_current < -1200000) {
                if (measurement_rated_capacity <= 6000) {
                    soc_mean_voltage_offset = 28;
                } else if (measurement_rated_capacity <= 11000) {
                    soc_mean_voltage_offset = 24;
                } else {
                    soc_mean_voltage_offset = 16;
                }
                ret = 1;
            } else if (soc_rest_negative1000_bin == ret || soc_rest_integrated_current < -600000) {
                {
                    uchar idle_ties_peak;
                    idle_ties_peak = (soc_rest_near_zero_bin == ret);
                }
                if (measurement_rated_capacity <= 6000) {
                    soc_mean_voltage_offset = 20;
                } else if (measurement_rated_capacity <= 11000) {
                    soc_mean_voltage_offset = 18;
                } else {
                    soc_mean_voltage_offset = 12;
                }
                ret = 1;
            } else {
                if (soc_rest_integrated_current < -180000) {
                    if (measurement_rated_capacity <= 6000) {
                        soc_mean_voltage_offset = 7;
                    } else if (measurement_rated_capacity <= 11000) {
                        soc_mean_voltage_offset = 5;
                    } else {
                        soc_mean_voltage_offset = 3;
                    }
                } else {
                    soc_mean_voltage_offset = 0;
                }
                ret = 1;
            }
        }
        if (measurement_current < -2000.0f || measurement_current > 2000.0f ||
            displayed_soc >= 80) {
            soc_rest_ticks = 0;
            soc_rest_integrated_current = 0;
            ret = 0;
        }
    } else {
        if (measurement_current > -2000.0f && measurement_current < 2000.0f && displayed_soc < 80) {
            if (soc_rest_ticks >= 1200) {
                if (soc_rest_ticks % 10 == 0) {
                    soc_rest_current_history[soc_rest_history_index] = (int)measurement_current;
                    if (soc_rest_current_history[soc_rest_history_index] < -3000) {
                        soc_rest_negative3000_bin++;
                    } else if (soc_rest_current_history[soc_rest_history_index] < -2000) {
                        soc_rest_negative2000_bin++;
                    } else if (soc_rest_current_history[soc_rest_history_index] < -1000) {
                        soc_rest_negative1000_bin++;
                    } else if (soc_rest_current_history[soc_rest_history_index] < 300) {
                        soc_rest_near_zero_bin++;
                    } else if (soc_rest_current_history[soc_rest_history_index] < 1000) {
                        soc_rest_positive_bin++;
                    }
                    soc_rest_history_index++;
                }
                soc_rest_integrated_current += (int)measurement_current;
            } else {
                soc_rest_history_index = 0;
            }
            soc_rest_ticks++;
        } else {
            soc_rest_ticks = 0;
            soc_rest_integrated_current = 0;
            soc_rest_positive_bin = 0;
            soc_rest_near_zero_bin = 0;
            soc_rest_negative1000_bin = 0;
            soc_rest_negative2000_bin = 0;
            soc_rest_negative3000_bin = 0;
            if (measurement_current > 1000.0f && displayed_soc >= 90) {
                if (soc_high_positive_current_ticks <= 1800) {
                    soc_high_positive_current_ticks++;
                }
            } else {
                soc_high_positive_current_ticks = 0;
            }
        }
    }
    if (soc_correction_state >= BMS_SOC_CORRECTION_ACTIVE &&
        soc_correction_state <= BMS_SOC_CORRECTION_SPECIAL_LAST) {
        soc_rest_ticks = 0;
        soc_high_positive_current_ticks = 0;
        soc_rest_integrated_current = 0;
        ret = 0;
        soc_rest_positive_bin = 0;
        soc_rest_near_zero_bin = 0;
        soc_rest_negative1000_bin = 0;
        soc_rest_negative2000_bin = 0;
        soc_rest_negative3000_bin = 0;
    }
    return ret;
}

#define ambient_temperature_view (*(ushort *)(((uchar *)temperature_readings) + 8))

/* Applies hysteresis and a derating table to the ambient temperature channel. */
ushort current_limit_from_ambient_temperature(void) {
    short temperature_reading;
    ushort held_temperature;
    uchar table_index;

    if ((temperature_reading = temperature_readings[4]) <
        (held_temperature = ambient_temperature_held)) {
        if (temperature_reading < held_temperature - 30) {
            held_temperature = temperature_readings[4];
            ambient_temperature_held = held_temperature;
        }
    } else {
        held_temperature = temperature_readings[4];
        ambient_temperature_held = held_temperature;
    }
    if (held_temperature > 3730) {
        return 5;
    }
    if (held_temperature >= 3640) {
        table_index = (held_temperature - 3630) / 10 - 1;
        return ambient_rate_limits[table_index];
    }
    return 115;
}

#undef ambient_temperature_view

/* Maps temperature to the eleven discharge-current lookup columns. */
uint discharge_temperature_band(uint temperature) {
    uint result = 0;
    if (temperature > 0x9e2) {
        if (temperature <= 0xa46)
            result = 1;
        else if (temperature <= 0xaaa)
            result = 2;
        else if (temperature <= 0xb72)
            result = 3;
        else if (temperature < 0xc3a)
            result = 4;
        else if (temperature < 0xc58)
            result = 5;
        else if (temperature < 0xc76)
            result = 6;
        else if (temperature < 0xc94)
            result = 7;
        else if (temperature < 0xcb2)
            result = 8;
        else if (temperature < 0xcd0)
            result = 9;
        else
            result = 10;
    }
    return result;
}

/* Maps cell voltage to the six discharge-current lookup rows. */
uint discharge_voltage_band(uint cell_voltage) {
    uint result = 0;
    if (cell_voltage > 0x7d0) {
        if (cell_voltage <= 0xb04)
            result = 1;
        else if (cell_voltage <= 0xb54)
            result = 2;
        else if (cell_voltage <= 0xbb8)
            result = 3;
        else if (cell_voltage <= 0xc1c)
            result = 4;
        else if (cell_voltage <= 0xe74)
            result = 5;
        else
            result = 5;
    }
    return result;
}

/* Combines held minimum/maximum temperature and minimum-cell voltage bands. */
ushort discharge_limit_from_cell_conditions(void) {
    ushort limits[3] = {0};
    /* match: This unread comparison preserves the held-value read before the
       band-address load in ARMCC. Its spelling is a reconstruction witness. */
    uint address_order;
    uint max_cache;
    uint minimum, min_cache, voltage, voltage_cache;
    uint max_level, min_level, voltage_level;
    uint held;
    if (measurement_maximum_temperature >= 2980) {
        max_cache = discharge_maximum_temperature_held;
        address_order = max_cache != (uint)&discharge_maximum_temperature_band;
        if (measurement_maximum_temperature < discharge_maximum_temperature_held) {
            discharge_maximum_temperature_hold_ticks++;
            if ((int)measurement_maximum_temperature <
                (int)discharge_maximum_temperature_held - 30) {
                max_cache = measurement_maximum_temperature;
                discharge_maximum_temperature_held = measurement_maximum_temperature;
            }
            if (discharge_temperature_band(measurement_maximum_temperature) ==
                discharge_maximum_temperature_band) {
                max_cache = measurement_maximum_temperature;
                discharge_maximum_temperature_held = measurement_maximum_temperature;
            }
        } else {
            discharge_maximum_temperature_hold_ticks = 0;
            discharge_maximum_temperature_held = measurement_maximum_temperature;
            max_cache = measurement_maximum_temperature;
        }
    } else {
        max_cache = discharge_maximum_temperature_held;
        address_order = max_cache != (uint)&discharge_maximum_temperature_band;
        if (measurement_maximum_temperature < discharge_maximum_temperature_held) {
            discharge_maximum_temperature_hold_ticks = 0;
            discharge_maximum_temperature_held = measurement_maximum_temperature;
            max_cache = measurement_maximum_temperature;
        } else {
            discharge_maximum_temperature_hold_ticks++;
            if (measurement_maximum_temperature > discharge_maximum_temperature_held + 10 ||
                discharge_maximum_temperature_hold_ticks >= 600) {
                max_cache = measurement_maximum_temperature;
                discharge_maximum_temperature_held = measurement_maximum_temperature;
            }
            if (discharge_temperature_band(measurement_maximum_temperature) ==
                discharge_maximum_temperature_band) {
                max_cache = measurement_maximum_temperature;
                discharge_maximum_temperature_held = max_cache;
            }
        }
    }
    minimum = measurement_minimum_temperature;
    if (minimum >= 2980) {
        held = discharge_minimum_temperature_held;
        if (minimum < held) {
            discharge_minimum_temperature_hold_ticks++;
            if ((int)minimum < (int)held - 30) {
                min_cache = minimum;
                discharge_minimum_temperature_held = minimum;
            } else
                min_cache = held;
            if (discharge_temperature_band(minimum) == discharge_minimum_temperature_band) {
                min_cache = measurement_minimum_temperature;
                discharge_minimum_temperature_held = measurement_minimum_temperature;
            }
        } else {
            discharge_minimum_temperature_hold_ticks = 0;
            discharge_minimum_temperature_held = minimum;
            min_cache = minimum;
        }
    } else {
        held = discharge_minimum_temperature_held;
        if (minimum < held) {
            discharge_minimum_temperature_hold_ticks = 0;
            discharge_minimum_temperature_held = minimum;
            min_cache = minimum;
        } else {
            discharge_minimum_temperature_hold_ticks++;
            if (minimum > held + 10 || discharge_minimum_temperature_hold_ticks >= 600) {
                min_cache = minimum;
                discharge_minimum_temperature_held = minimum;
            } else
                min_cache = held;
            if (discharge_temperature_band(minimum) == discharge_minimum_temperature_band) {
                min_cache = measurement_minimum_temperature;
                discharge_minimum_temperature_held = measurement_minimum_temperature;
            }
        }
    }
    voltage = minimum_selected_cell_voltage;
    voltage_cache = discharge_cell_voltage_held;
    if (voltage < voltage_cache) {
        discharge_cell_voltage_hold_ticks = 0;
        discharge_cell_voltage_held = voltage;
        voltage_cache = voltage;
        discharge_voltage_recovery_seen = 0;
    } else {
        if (discharge_cell_voltage_hold_ticks <= 1200)
            discharge_cell_voltage_hold_ticks++;
        if (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE)
            discharge_voltage_recovery_seen = measurement_current_mode;
        if ((voltage > voltage_cache + 150 || discharge_cell_voltage_hold_ticks >= 1200) &&
            discharge_voltage_recovery_seen == 1) {
            voltage_cache = voltage;
            discharge_cell_voltage_held = voltage;
        }
        if (discharge_voltage_band(voltage) == discharge_cell_voltage_band) {
            voltage_cache = voltage;
            discharge_cell_voltage_held = voltage;
        }
    }
    min_level = discharge_temperature_band(min_cache);
    max_level = discharge_temperature_band(max_cache);
    voltage_level = discharge_voltage_band(voltage_cache);
    discharge_maximum_temperature_band = max_level;
    discharge_minimum_temperature_band = min_level;
    discharge_cell_voltage_band = voltage_level;
    limits[1] = discharge_rate_limits[voltage_level][min_level];
    limits[2] = discharge_rate_limits[voltage_level][max_level];
    if (limits[1] > limits[2])
        limits[0] = limits[2];
    else
        limits[0] = limits[1];
    return limits[0];
}

/* Selects the discharge rate ceiling from rated capacity. */
uint discharge_limit_from_capacity(void)

{
    if (measurement_rated_capacity >= 17000) {
        return 0x2d;
    }
    return 0x32;
}

/* Selects the minimum ambient, cell-condition and capacity discharge rate. */
static uint discharge_select_limit(void) {
    ushort limits[3] = {0};
    uint capacity;
    limits[0] = current_limit_from_ambient_temperature();
    limits[1] = discharge_limit_from_cell_conditions();
    capacity = discharge_limit_from_capacity();
    limits[2] = (ushort)capacity;
    if (limits[0] > limits[1]) {
        if (limits[1] <= capacity)
            return limits[1];
    } else if (limits[0] <= capacity)
        return limits[0];
    return capacity;
}

#define limit_cell_overvoltage BMS_LIMIT_FIELD(measurement_limit_settings, cell_overvoltage)
#define limit_discharge_overcurrent                                                                \
    BMS_LIMIT_FIELD(measurement_limit_settings, discharge_overcurrent)

/* Scales the selected rate and caps it using the discharge-overcurrent setting. */
void discharge_update_current_limit(void) {
    uint current_limit;
    uint rated_capacity;
    int configured_limit;

    current_limit = discharge_select_limit();
    rated_capacity = measurement_rated_capacity;
    if (rated_capacity >= 18600) {
        rated_capacity = 18600;
    }
    current_limit = current_limit * rated_capacity * 100 / 1000;
    if (voltage_current_limit_flags & 5) {
        upper_voltage_current_hold = 1;
        upper_voltage_hold_latched = 1;
        lower_voltage_current_hold = 0;
    } else {
        uchar was_latched = upper_voltage_hold_latched;
        if (was_latched == 1) {
        if (measurement_limit_settings[0] - 150 > measurement_maximum_cell_voltage) {
            upper_voltage_current_hold = 0;
        }
        if (measurement_minimum_cell_voltage < 3340) {
            if (full_voltage_release_ticks++ >= 60) {
                full_voltage_release_ticks = 0;
                upper_voltage_current_hold = 0;
                upper_voltage_hold_latched = 0;
            }
        } else {
            full_voltage_release_ticks = 0;
        }
        if (measurement_limit_settings[0] - 130 < measurement_maximum_cell_voltage &&
            upper_voltage_current_hold == 0) {
            upper_voltage_current_hold = 1;
            full_voltage_release_ticks = 0;
        }
    } else if (upper_voltage_current_hold != 0 &&
        measurement_limit_settings[0] - 150 > measurement_maximum_cell_voltage &&
        measurement_minimum_cell_voltage < 3340) {
        upper_voltage_current_hold = 0;
        upper_voltage_hold_latched = 0;
    }
    }
    if ((extended_limit_flags & 1) && rated_capacity * 10 / 10 < current_limit) {
        current_limit = rated_capacity * 10 / 10;
    }
    if (capacity_empty_latch != 0) {
        current_limit = 0;
    }
    configured_limit = abs(*(short *)((uchar *)measurement_limit_settings + 10)) * 1000;
    if (current_limit > configured_limit) {
        current_limit = configured_limit;
    }
    if (current_substitution_active == 1) current_limit = 0;
    discharge_current_limit = current_limit;
}

#undef limit_cell_overvoltage
#undef limit_discharge_overcurrent

/* Maps temperature to the twelve charge-current lookup columns. */
uint charge_temperature_band(uint temperature) {
    uint result = 0;
    if (temperature > 0xaaa) {
        if (temperature <= 0xadc)
            result = 1;
        else if (temperature <= 0xb0e)
            result = 2;
        else if (temperature <= 0xb72)
            result = 3;
        else if (temperature <= 0xbd6)
            result = 4;
        else if (temperature <= 0xc3a)
            result = 5;
        else if (temperature < 0xc4e)
            result = 6;
        else if (temperature < 0xc62)
            result = 7;
        else if (temperature < 0xc76)
            result = 8;
        else if (temperature < 0xc8a)
            result = 9;
        else if (temperature < 0xc9e)
            result = 10;
        else
            result = 11;
    }
    return result;
}

/* Maps cell voltage to the seven charge-current lookup rows. */
uint charge_voltage_band(uint cell_voltage) {
    uint result = 0;
    if (cell_voltage > 0x7d0) {
        if (cell_voltage <= 0xaf0)
            result = 1;
        else if (cell_voltage <= 0xdac)
            result = 2;
        else if (cell_voltage <= 0xdde)
            result = 3;
        else if (cell_voltage <= 0xe10)
            result = 4;
        else if (cell_voltage <= 0xe42)
            result = 5;
        else
            result = 6;
    }
    return result;
}

/* Combines held temperature extrema and maximum-cell voltage bands.
   match: The voltage-hysteresis branch calls the temperature-band helper.
   I retain that call before the final voltage-band lookup. */
ushort charge_limit_from_cell_conditions(void) {
    ushort limits[3] = {0};
    uint max_cache;
    uint minimum, min_cache, voltage, voltage_cache;
    uint min_level, max_level, voltage_level;
    uint held;
    if (measurement_maximum_temperature >= 2980) {
        max_cache = charge_maximum_temperature_held;
        if (measurement_maximum_temperature < charge_maximum_temperature_held) {
            charge_maximum_temperature_hold_ticks++;
            if ((int)measurement_maximum_temperature < (int)charge_maximum_temperature_held - 20) {
                max_cache = measurement_maximum_temperature;
                charge_maximum_temperature_held = measurement_maximum_temperature;
            }
            if (charge_temperature_band(measurement_maximum_temperature) ==
                charge_maximum_temperature_band) {
                max_cache = measurement_maximum_temperature;
                charge_maximum_temperature_held = measurement_maximum_temperature;
            }
        } else {
            charge_maximum_temperature_hold_ticks = 0;
            charge_maximum_temperature_held = measurement_maximum_temperature;
            max_cache = measurement_maximum_temperature;
        }
    } else {
        max_cache = charge_maximum_temperature_held;
        if (measurement_maximum_temperature < max_cache) {
            charge_maximum_temperature_hold_ticks = 0;
            charge_maximum_temperature_held = measurement_maximum_temperature;
            max_cache = measurement_maximum_temperature;
        } else {
            charge_maximum_temperature_hold_ticks++;
            if (measurement_maximum_temperature > charge_maximum_temperature_held + 10 ||
                charge_maximum_temperature_hold_ticks >= 600) {
                max_cache = measurement_maximum_temperature;
                charge_maximum_temperature_held = measurement_maximum_temperature;
            }
            if (charge_temperature_band(measurement_maximum_temperature) ==
                charge_maximum_temperature_band) {
                max_cache = measurement_maximum_temperature;
                charge_maximum_temperature_held = measurement_maximum_temperature;
            }
        }
    }
    minimum = measurement_minimum_temperature;
    if (minimum >= 2980) {
        held = charge_minimum_temperature_held;
        min_cache = held;
        if (minimum < held) {
            charge_minimum_temperature_hold_ticks++;
            if ((int)minimum < (int)held - 20) {
                min_cache = minimum;
                charge_minimum_temperature_held = minimum;
            }
            if (charge_temperature_band(minimum) == charge_minimum_temperature_band) {
                min_cache = measurement_minimum_temperature;
                charge_minimum_temperature_held = measurement_minimum_temperature;
            }
        } else {
            charge_minimum_temperature_hold_ticks = 0;
            charge_minimum_temperature_held = minimum;
            min_cache = minimum;
        }
    } else {
        held = charge_minimum_temperature_held;
        if (minimum < held) {
            charge_minimum_temperature_hold_ticks = 0;
            charge_minimum_temperature_held = minimum;
            min_cache = minimum;
        } else {
            charge_minimum_temperature_hold_ticks++;
            if (minimum > held + 10 || charge_minimum_temperature_hold_ticks >= 600) {
                min_cache = minimum;
                charge_minimum_temperature_held = minimum;
            } else
                min_cache = held;
            if (charge_temperature_band(minimum) == charge_minimum_temperature_band) {
                min_cache = measurement_minimum_temperature;
                charge_minimum_temperature_held = measurement_minimum_temperature;
            }
        }
    }
    voltage = maximum_selected_cell_voltage;
    held = charge_cell_voltage_held;
    if (voltage < held) {
        charge_cell_voltage_hold_ticks++;
        if ((int)voltage < (int)held - 150 && voltage <= 3400) {
            voltage_cache = voltage;
            charge_cell_voltage_held = voltage;
        } else
            voltage_cache = held;
        if (charge_temperature_band(voltage) == charge_cell_voltage_band) {
            voltage_cache = voltage;
            charge_cell_voltage_held = voltage;
        }
    } else {
        charge_cell_voltage_hold_ticks = 0;
        charge_cell_voltage_held = voltage;
        voltage_cache = voltage;
    }
    min_level = charge_temperature_band(min_cache);
    max_level = charge_temperature_band(max_cache);
    voltage_level = charge_voltage_band(voltage_cache);
    charge_maximum_temperature_band = max_level;
    charge_minimum_temperature_band = min_level;
    charge_cell_voltage_band = voltage_level;
    limits[1] = charge_rate_limits[voltage_level][min_level];
    limits[2] = charge_rate_limits[voltage_level][max_level];
    if (limits[1] > limits[2])
        limits[0] = limits[2];
    else
        limits[0] = limits[1];
    return limits[0];
}

/* Selects the charge rate ceiling from rated capacity. */
uint charge_limit_from_capacity(void)

{
    if (measurement_rated_capacity >= 17000) {
        return 0x2d;
    }
    return 0x32;
}

/* Selects the minimum ambient, cell-condition and capacity charge rate. */
uint charge_select_limit(void) {
    ushort limits[3] = {0};
    uint capacity;
    limits[0] = current_limit_from_ambient_temperature();
    limits[1] = charge_limit_from_cell_conditions();
    capacity = charge_limit_from_capacity();
    limits[2] = (ushort)capacity;
    if (limits[0] > limits[1]) {
        if (limits[1] <= capacity)
            return limits[1];
    } else if (limits[0] <= capacity)
        return limits[0];
    return capacity;
}

#define capacity_full_latch_view (*(char *)(((uchar *)&capacity_full_latch) + 0))
#define model_code_view (*(char *)(((uchar *)model_code) + 0))
#define limit_charge_overcurrent                                                                   \
    ((ushort)BMS_LIMIT_FIELD(measurement_limit_settings, charge_overcurrent))

/* Scales the selected rate and caps it using the charge-overcurrent setting. */
void charge_update_current_limit(void) {
    uint limit;
    uint rated_capacity;
    uint configured_limit;

    limit = charge_select_limit();
    rated_capacity = measurement_rated_capacity;
    if (rated_capacity >= 18600) {
        rated_capacity = 18600;
    }
    limit = limit * rated_capacity * 100 / 1000;
    if ((voltage_current_limit_flags & 2) || (voltage_current_limit_flags & 8)) {
        upper_voltage_current_hold = 0;
        lower_voltage_current_hold = 1;
    } else {
        lower_voltage_current_hold = 0;
    }
    if ((extended_limit_flags & 1) && rated_capacity * 10 / 10 < limit) {
        limit = rated_capacity * 10 / 10;
    }
    if (primary_soc - displayed_soc + 1U > 2) {
        if ((primary_soc > displayed_soc && primary_soc > 50) || charge_soc_difference_latch == 1) {
            charge_soc_difference_latch = 1;
            if (rated_capacity * 33 / 10 < limit) {
                limit = rated_capacity * 33 / 10;
            }
            if (primary_soc - displayed_soc + 2U <= 4 &&
                measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
                charge_soc_difference_latch = 0;
            }
        }
    } else {
        if (charge_soc_difference_latch == 1) {
            if (rated_capacity * 33 / 10 < limit) {
                limit = rated_capacity * 33 / 10;
            }
            if (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
                charge_soc_difference_latch = 0;
            }
        } else {
            charge_soc_difference_latch = 0;
        }
    }
    if (capacity_full_latch == 1) {
        limit = 0;
    }
    if (model_code[0] == 'B') {
        if (model_current_limit_steps > 6) {
            model_current_limit_steps = 0;
        }
        configured_limit = measurement_limit_settings[4] * 1000;
        if (configured_limit > model_current_limit_steps * 5000) {
            configured_limit -= model_current_limit_steps * 5000;
        }
    } else {
        configured_limit = measurement_limit_settings[4] * 1000;
    }
    if (limit > configured_limit) {
        limit = configured_limit;
    }
    if (current_substitution_active == 1) limit = 0;
    charge_current_limit = limit;
}

#undef capacity_full_latch_view
#undef model_code_view
#undef limit_charge_overcurrent
#define first_peer_participation_state_view (*(uchar *)(((uchar *)&measurement_peer_records) + 141))

/* Aggregates six peer limits, excess currents and participation flags. */
void pack_aggregate_current_limits(void) {
    uchar peer_index = 0;
    uchar participating_count = 0;
    uchar minimum_charge_peer = 0;
    uchar minimum_discharge_peer = 0;
    ushort charge_excess = 0;
    ushort discharge_excess = 0;
    ushort maximum_pack_voltage = 0;
    ushort minimum_pack_voltage = 0;
    ushort minimum_charge_limit;
    ushort maximum_charge_limit;
    ushort minimum_discharge_limit;
    ushort maximum_discharge_limit;
    ushort excess_current;
    int peer_current;
    ushort capacity_limit;
    ushort combined_limit;
    ushort heating_limit;

    maximum_charge_limit = measurement_peer_records[0].charge_current_limit;
    do {
        if (measurement_peer_records[peer_index].participation_state == 1) {
            minimum_charge_limit = measurement_peer_records[peer_index].charge_current_limit;
            break;
        }
        minimum_charge_limit = maximum_charge_limit;
        peer_index++;
    } while (peer_index < 6);
    minimum_discharge_limit = measurement_peer_records[0].discharge_current_limit;
    maximum_discharge_limit = minimum_discharge_limit;
    for (peer_index = 0; peer_index < 6; peer_index++) {
        excess_current = 0;
        if (measurement_peer_records[peer_index].participation_state == 1) {
            if (measurement_peer_records[peer_index].charge_current_limit < minimum_charge_limit) {
                minimum_charge_limit = measurement_peer_records[peer_index].charge_current_limit;
                minimum_charge_peer = peer_index;
            }
            if (measurement_peer_records[peer_index].discharge_current_limit <
                minimum_discharge_limit) {
                minimum_discharge_limit =
                    measurement_peer_records[peer_index].discharge_current_limit;
                minimum_discharge_peer = peer_index;
            }
            if (measurement_peer_records[peer_index].charge_current_limit > maximum_charge_limit) {
                maximum_charge_limit = measurement_peer_records[peer_index].charge_current_limit;
            }
            if (measurement_peer_records[peer_index].discharge_current_limit >
                maximum_discharge_limit) {
                maximum_discharge_limit =
                    measurement_peer_records[peer_index].discharge_current_limit;
            }
            participating_count++;
            peer_current = measurement_peer_records[peer_index].pack_current / 10;
            if (peer_current >= 0) {
                if (measurement_peer_records[peer_index].charge_current_limit < peer_current) {
                    excess_current = abs(peer_current -
                                         measurement_peer_records[peer_index].charge_current_limit);
                }
                charge_excess += excess_current;
            } else {
                if (abs(peer_current) >
                    measurement_peer_records[peer_index].discharge_current_limit) {
                    excess_current =
                        abs(abs(peer_current) -
                            measurement_peer_records[peer_index].discharge_current_limit);
                }
                discharge_excess += excess_current;
            }
            if (participating_count == 1) {
                maximum_pack_voltage = measurement_peer_records[peer_index].pack_voltage;
                minimum_pack_voltage = measurement_peer_records[peer_index].pack_voltage;
            }
            if (measurement_peer_records[peer_index].pack_voltage > maximum_pack_voltage) {
                maximum_pack_voltage = measurement_peer_records[peer_index].pack_voltage;
            }
            if (measurement_peer_records[peer_index].pack_voltage < minimum_pack_voltage) {
                minimum_pack_voltage = measurement_peer_records[peer_index].pack_voltage;
            }
        }
    }
    if (participating_count == 0) {
        participating_count = 1;
    }
    capacity_limit = ((int)measurement_peer_records[minimum_charge_peer].rated_capacity) * 40 / 100;
    combined_limit = minimum_charge_limit * participating_count;
    if (combined_limit < maximum_charge_limit && minimum_charge_limit > capacity_limit) {
        combined_limit = maximum_charge_limit;
    }
    if (combined_limit > charge_excess) {
        if (charge_excess >= peer_current_excess_threshold && peer_charge_excess_latch == 0) {
            peer_charge_excess_ticks++;
            if (peer_charge_excess_ticks > 1) {
                peer_charge_ramp_ticks = 0;
                peer_charge_excess_ticks = 0;
                peer_charge_excess_latch = 1;
                combined_charge_current_limit = peer_charge_limit_previous - charge_excess;
            }
        } else {
            peer_charge_excess_ticks = 0;
            if (peer_charge_ramp_ticks < 300) {
                peer_charge_ramp_ticks++;
                if (peer_charge_ramp_ticks >= 10) {
                    peer_charge_excess_latch = 0;
                }
                combined_charge_current_limit = peer_charge_limit_previous;
            } else {
                peer_charge_ramp_ticks = 290;
                if (peer_limit_startup_count <= 5) {
                    combined_charge_current_limit = combined_limit;
                    peer_charge_limit_previous = combined_limit;
                }
                if (peer_charge_limit_previous < combined_limit) {
                    combined_charge_current_limit = peer_charge_limit_previous + 1000;
                } else {
                    combined_charge_current_limit = combined_limit;
                }
            }
        }
    } else {
        peer_charge_excess_ticks = 0;
        peer_charge_ramp_ticks = 600;
        combined_charge_current_limit = minimum_charge_limit;
    }
    if (combined_charge_current_limit > combined_limit) {
        combined_charge_current_limit = combined_limit;
    }
    peer_charge_limit_previous = combined_charge_current_limit;
    capacity_limit =
        ((int)measurement_peer_records[minimum_discharge_peer].rated_capacity) * 40 / 100;
    combined_limit = minimum_discharge_limit * participating_count;
    if (combined_limit < maximum_discharge_limit && minimum_discharge_limit > capacity_limit) {
        combined_limit = maximum_discharge_limit;
    }
    if (combined_limit > discharge_excess) {
        if (discharge_excess >= peer_current_excess_threshold && peer_discharge_excess_latch == 0) {
            peer_discharge_excess_ticks++;
            if (peer_discharge_excess_ticks > 1) {
                peer_discharge_ramp_ticks = 0;
                peer_discharge_excess_ticks = 0;
                peer_discharge_excess_latch = 1;
                combined_discharge_current_limit = peer_discharge_limit_previous - discharge_excess;
            }
        } else {
            peer_discharge_excess_ticks = 0;
            if (peer_discharge_ramp_ticks < 300) {
                peer_discharge_ramp_ticks++;
                if (peer_discharge_ramp_ticks >= 10) {
                    peer_discharge_excess_latch = 0;
                }
                combined_discharge_current_limit = peer_discharge_limit_previous;
            } else {
                peer_discharge_ramp_ticks = 290;
                if (peer_limit_startup_count <= 5) {
                    combined_discharge_current_limit = combined_limit;
                    peer_discharge_limit_previous = combined_limit;
                }
                if (peer_discharge_limit_previous < combined_limit) {
                    combined_discharge_current_limit = peer_discharge_limit_previous + 1000;
                } else {
                    combined_discharge_current_limit = combined_limit;
                }
            }
        }
    } else {
        peer_discharge_excess_ticks = 0;
        peer_discharge_ramp_ticks = 600;
        combined_discharge_current_limit = minimum_discharge_limit;
    }
    if (combined_discharge_current_limit > combined_limit) {
        combined_discharge_current_limit = combined_limit;
    }
    peer_discharge_limit_previous = combined_discharge_current_limit;
    if (battery_heating.positive_current_stage != 0) {
        heating_limit = (uint)measurement_rated_capacity * 30 / 100;
        if (combined_charge_current_limit > heating_limit) {
            combined_charge_current_limit = heating_limit;
        }
    } else if (battery_heating.negative_current_stage != 0) {
        combined_discharge_current_limit = (uint)measurement_rated_capacity * 40 / 100;
    }
    if (peer_limit_startup_count <= 5) {
        peer_limit_startup_count++;
    }
    participating_count = 0;
    for (peer_index = 0; peer_index < 6; peer_index++) {
        if (measurement_peer_records[peer_index].voltage_hold_mode == 1) {
            participating_count++;
        } else if (measurement_peer_records[peer_index].voltage_hold_mode == 2) {
            participating_count = 0;
        }
    }
    if (participating_count != 0) {
        peer_mode_one_summary = 1;
    } else {
        peer_mode_one_summary = 0;
    }
    participating_count = 0;
    for (peer_index = 0; peer_index < 6; peer_index++) {
        if (measurement_peer_records[peer_index].voltage_hold_mode == 2) {
            participating_count++;
        } else if (measurement_peer_records[peer_index].voltage_hold_mode == 1) {
            participating_count = 0;
        }
    }
    if (participating_count != 0) {
        peer_mode_two_summary = 1;
    } else {
        peer_mode_two_summary = 0;
    }
    participating_count = 0;
    for (peer_index = 0; peer_index < 6; peer_index++) {
        if (measurement_peer_records[peer_index].soc_voltage_hold == 1) {
            participating_count++;
        }
    }
    if (participating_count != 0) {
        if (peer_auxiliary_gate_word >= 100) {
            peer_auxiliary_state_present = 1;
        } else {
            peer_auxiliary_state_present = 0;
        }
    } else {
        peer_auxiliary_state_present = 0;
    }
}

#undef first_peer_participation_state_view

/* Interpolates the model curve using the mean cell voltage plus a learned offset.
   match: I retain the ushort casts on both ternary arms before interpolation. */
uchar soc_lookup_adjusted_mean_voltage(void) {
    uchar found_columns = 0;
    uchar row;
    short column_soc[5] = {25, 25, 25, 25, 25};
    ushort temperature;
    ushort adjusted_cell_voltage;

    adjusted_cell_voltage = soc_mean_voltage_offset == 0
                                ? (ushort)(mean_cell_voltage - 5)
                                : (ushort)(mean_cell_voltage + soc_mean_voltage_offset);
    temperature = measurement_minimum_temperature;
    row = 1;
    do {
        if ((found_columns & 1) == 0 &&
            soc_model_mean_voltage_curve[row][1] >= adjusted_cell_voltage) {
            found_columns |= 1;
            column_soc[1] = (adjusted_cell_voltage - soc_model_mean_voltage_curve[row - 1][1]) *
                                1000 /
                                (int)((float)(soc_model_mean_voltage_curve[row][1] -
                                              soc_model_mean_voltage_curve[row - 1][1]) *
                                      1000.0f / 5.0f) +
                            soc_model_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 2) == 0 &&
            soc_model_mean_voltage_curve[row][2] >= adjusted_cell_voltage) {
            found_columns |= 2;
            column_soc[2] = (adjusted_cell_voltage - soc_model_mean_voltage_curve[row - 1][2]) *
                                1000 /
                                (int)((float)(soc_model_mean_voltage_curve[row][2] -
                                              soc_model_mean_voltage_curve[row - 1][2]) *
                                      1000.0f / 5.0f) +
                            soc_model_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 4) == 0 &&
            soc_model_mean_voltage_curve[row][3] >= adjusted_cell_voltage) {
            found_columns |= 4;
            column_soc[3] = (adjusted_cell_voltage - soc_model_mean_voltage_curve[row - 1][3]) *
                                1000 /
                                (int)((float)(soc_model_mean_voltage_curve[row][3] -
                                              soc_model_mean_voltage_curve[row - 1][3]) *
                                      1000.0f / 5.0f) +
                            soc_model_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 8) == 0 &&
            soc_model_mean_voltage_curve[row][4] >= adjusted_cell_voltage) {
            found_columns |= 8;
            column_soc[4] = (adjusted_cell_voltage - soc_model_mean_voltage_curve[row - 1][4]) *
                                1000 /
                                (int)((float)(soc_model_mean_voltage_curve[row][4] -
                                              soc_model_mean_voltage_curve[row - 1][4]) *
                                      1000.0f / 5.0f) +
                            soc_model_mean_voltage_curve[row - 1][0];
        }
        row++;
    } while (row <= 6);
    if (temperature >= 2980) {
        column_soc[0] =
            column_soc[1] + (temperature - 3180) * (column_soc[1] - column_soc[2]) / 200;
    } else if (temperature >= 2830) {
        column_soc[0] =
            column_soc[2] + (temperature - 2980) * (column_soc[2] - column_soc[3]) / 150;
    } else if (temperature >= 2780) {
        column_soc[0] = column_soc[3] + (temperature - 2830) * (column_soc[3] - column_soc[4]) / 50;
    } else if (temperature >= 2730) {
        column_soc[0] = column_soc[4] + (temperature - 2780) * (column_soc[3] - column_soc[4]) / 50;
    } else {
        column_soc[0] = column_soc[4];
    }
    if (column_soc[0] < 1) {
        return 1;
    }
    return column_soc[0];
}

/* Interpolates the fixed low-SOC curve using mean cell voltage plus15. */
uchar soc_lookup_offset_mean_voltage(void) {
    uchar found_columns = 0;
    uchar row;
    short column_soc[5] = {25, 25, 25, 25, 25};
    ushort temperature;
    ushort adjusted_cell_voltage;

    temperature = measurement_minimum_temperature;
    adjusted_cell_voltage = mean_cell_voltage + 15;
    row = 1;
    do {
        if ((found_columns & 1) == 0 &&
            soc_fixed_mean_voltage_curve[row][1] >= adjusted_cell_voltage) {
            found_columns |= 1;
            column_soc[1] = (adjusted_cell_voltage - soc_fixed_mean_voltage_curve[row - 1][1]) *
                                1000 /
                                (int)((float)(soc_fixed_mean_voltage_curve[row][1] -
                                              soc_fixed_mean_voltage_curve[row - 1][1]) *
                                      1000.0f / 5.0f) +
                            soc_fixed_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 2) == 0 &&
            soc_fixed_mean_voltage_curve[row][2] >= adjusted_cell_voltage) {
            found_columns |= 2;
            column_soc[2] = (adjusted_cell_voltage - soc_fixed_mean_voltage_curve[row - 1][2]) *
                                1000 /
                                (int)((float)(soc_fixed_mean_voltage_curve[row][2] -
                                              soc_fixed_mean_voltage_curve[row - 1][2]) *
                                      1000.0f / 5.0f) +
                            soc_fixed_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 4) == 0 &&
            soc_fixed_mean_voltage_curve[row][3] >= adjusted_cell_voltage) {
            found_columns |= 4;
            column_soc[3] = (adjusted_cell_voltage - soc_fixed_mean_voltage_curve[row - 1][3]) *
                                1000 /
                                (int)((float)(soc_fixed_mean_voltage_curve[row][3] -
                                              soc_fixed_mean_voltage_curve[row - 1][3]) *
                                      1000.0f / 5.0f) +
                            soc_fixed_mean_voltage_curve[row - 1][0];
        }
        if ((found_columns & 8) == 0 &&
            soc_fixed_mean_voltage_curve[row][4] >= adjusted_cell_voltage) {
            ushort upper_voltage = soc_fixed_mean_voltage_curve[row][4];
            found_columns |= 8;
            column_soc[4] =
                (adjusted_cell_voltage - soc_fixed_mean_voltage_curve[row - 1][4]) * 1000 /
                    (int)((float)(upper_voltage - soc_fixed_mean_voltage_curve[row - 1][4]) *
                          1000.0f / 5.0f) +
                soc_fixed_mean_voltage_curve[row - 1][0];
        }
        row++;
    } while (row <= 6);
    if (temperature >= 2980) {
        column_soc[0] =
            column_soc[1] + (temperature - 3180) * (column_soc[1] - column_soc[2]) / 200;
    } else if (temperature >= 2830) {
        column_soc[0] =
            column_soc[2] + (temperature - 2980) * (column_soc[2] - column_soc[3]) / 150;
    } else if (temperature >= 2780) {
        column_soc[0] = column_soc[3] + (temperature - 2830) * (column_soc[3] - column_soc[4]) / 50;
    } else if (temperature >= 2730) {
        column_soc[0] = column_soc[4] + (temperature - 2780) * (column_soc[3] - column_soc[4]) / 50;
    } else {
        column_soc[0] = column_soc[4];
    }
    if (column_soc[0] < 1) {
        return 1;
    }
    return column_soc[0];
}

/* ---- protection ---- */
/* at: capacity_hold_inhibit 0x20000167, soc_voltage_hold 0x200000AD, measurement_protection_settings 0x2000156A, protection_event_counts 0x20001586, peer_command_bytes 0x20003D58 */
extern uchar capacity_hold_inhibit;
extern char soc_voltage_hold;
extern ushort measurement_protection_settings[];
extern ushort protection_event_counts[];
extern uchar peer_command_bytes[];
#define true 1
#define false 0
int abs(int);
/* at: soc_average_offset_mean_voltage 0x0801B034, soc_average_adjusted_mean_voltage 0x0801B0A2, pack_update_peer_participation_flags 0x0801B10E, capacity_update_full_voltage_latches 0x0801B18A, soc_update_voltage_hold 0x0801B3C4, capacity_update_empty_voltage_latches 0x0801B730, soc_reconcile_capacity 0x0801B7F6, protection_update_cold_low_soc 0x0801C176, capacity_update_soc 0x0801C1F2, protection_update_pack_undervoltage 0x0801C328, protection_update_cell_undervoltage 0x0801C4A0, protection_update_pack_overvoltage 0x0801C608, protection_update_cell_overvoltage 0x0801C6F2, protection_update_voltage 0x0801C854, protection_update_discharge_overcurrent 0x0801C868, protection_update_charge_overcurrent 0x0801C99A, protection_update_current 0x0801CAC0, protection_update_charge_overtemperature 0x0801CACC, protection_update_discharge_overtemperature 0x0801CBC2, protection_update_charge_undertemperature 0x0801CCD6, protection_update_discharge_undertemperature 0x0801CDC4, protection_update_charge_discharge_temperature 0x0801CEAC, protection_update_mosfet_overtemperature 0x0801CEC0, protection_update_ambient_overtemperature 0x0801CF70, protection_update_ambient_undertemperature 0x0801D020, protection_update_ambient_temperature 0x0801D0F2, protection_update_temperature 0x0801D0FE, protection_check_voltage_consistency 0x0801D124, protection_check_current_output_consistency 0x0801D1AE, protection_check_temperature_sensor_faults 0x0801D222, protection_get_voltage_current_flags 0x0801D25C, protection_track_positive_current 0x0801D264, measurement_estimate_cell_resistance 0x0801D2C2 */
/* at: g_2000009F 0x2000009F, g_200000A4 0x200000A4, g_200000AA 0x200000AA, g_200000AB 0x200000AB, g_200000AC 0x200000AC, g_200000AD 0x200000AD, g_200000B0 0x200000B0, g_200000F6 0x200000F8, g_2000010C 0x20000110, g_2000010E 0x20000112, g_20000110 0x20000114, g_20000112 0x20000116, g_20000114 0x20000118, g_20000118 0x2000011C, g_2000011A 0x2000011E, g_2000011C 0x20000120, g_2000012A 0x2000012E, g_2000012C 0x20000130, g_2000012E 0x20000132, g_20000139 0x2000013D, g_2000013A 0x2000013E, g_20000148 0x2000014C, g_20000162 0x20000166, g_20000163 0x20000167, g_20000164 0x20000168, g_2000016C 0x20000170, g_20000170 0x20000174, g_20000178 0x2000017C, g_20000182 0x20000186, g_20000184 0x20000188, g_20000185 0x20000189, g_20000187 0x2000018B, g_20000214 0x2000021C, g_20000294 0x2000029C, g_200002F0 0x200002F8, g_200002F8 0x20000304, resistance_cell_voltage_history 0x200004E8, g_20000F9C 0x20000FB4, g_200010D4 0x200010EC, g_200010F4 0x2000110C, g_2000119E 0x200011B6, limit_cell_overvoltage 0x2000153A, g_20001552 0x2000156A, g_2000156E 0x20001586, g_200035CC 0x200035E4, g_200035E0 0x200035F8, g_20003D40 0x20003D58, g_20003D52 0x20003D6A */
uchar soc_lookup_offset_mean_voltage(void);
uchar soc_lookup_adjusted_mean_voltage(void);
float capacity_get_full_capacity(void);
uint soc_average_high_cell(int);
uint soc_average_low_cell(int);
uint soc_correction_tick(void);
uint soc_evaluate_rest_correction(void);
void discharge_update_current_limit(void);
void charge_update_current_limit(void);
void pack_aggregate_current_limits(void);
uint soc_update_voltage_average(uint, uint, int);
uint soc_classify_current_rate(uint, uint);
void capacity_update_operating_status(void);
void capacity_latch_full_voltage(void);
void capacity_latch_empty_voltage(void);
void runtime_timer_start(int, uint);
void coordination_record_event(int);
void GPIO_ResetBits(void *, ushort);
void capacity_update_empty_voltage_latches();
void pack_update_peer_participation_flags();
void capacity_update_full_voltage_latches();
void capacity_update_soc();
void soc_update_voltage_hold();
void measurement_estimate_cell_resistance();
void protection_update_cold_low_soc();
void protection_check_current_output_consistency();
void protection_track_positive_current();
uint soc_average_offset_mean_voltage();
void soc_reconcile_capacity();
uint soc_average_adjusted_mean_voltage();
void protection_check_temperature_sensor_faults();
void protection_check_voltage_consistency();

/* Provisional RAM tables inferred from references; ownership needs full unit proof. */
/* at: soc_offset_mean_history 0x200035E4, soc_adjusted_mean_history 0x200035EE, soc_voltage_hold_ticks 0x200035F8 */
uchar soc_offset_mean_history[10];
uchar soc_adjusted_mean_history[10];
ushort soc_voltage_hold_ticks[5];

/* Trimmed ten-sample average of the fixed mean-voltage SOC lookup. */
uint soc_average_offset_mean_voltage() {
    uint sample;
    uint maximum_sample;
    uint minimum_sample;
    uint sample_index;
    uint average;
    ushort sample_sum;

    sample_sum = 0;
    sample_index = 0;
    if (soc_offset_mean_history_index < 10) {
        (&soc_offset_mean_history[0])[soc_offset_mean_history_index] =
            (soc_lookup_offset_mean_voltage());
        soc_offset_mean_history_index++;
    } else {
        soc_offset_mean_history_index = 0;
        (&soc_offset_mean_history[0])[soc_offset_mean_history_index] =
            (soc_lookup_offset_mean_voltage());
        soc_offset_mean_history_index++;
    }
    maximum_sample = (uint)soc_offset_mean_history[0];
    minimum_sample = maximum_sample;
    do {
        sample = (uint)(&soc_offset_mean_history[0])[sample_index];
        if (sample == 0) {
            return 100;
        }
        if (sample > maximum_sample) {
            maximum_sample = sample;
        }
        if (sample < minimum_sample) {
            minimum_sample = sample;
        }
        sample_sum = sample + sample_sum & 0xffff;
        sample_index = sample_index + 1 & 0xff;
    } while (sample_index < 10);
    sample_sum = (sample_sum - maximum_sample) - minimum_sample;
    average = sample_sum / 8;
    if (average > 100) {
        average = 100;
    }
    return average & 0xff;
}

/* Trimmed ten-sample average of the adjusted mean-voltage SOC lookup. */
uint soc_average_adjusted_mean_voltage() {
    uint sample;
    uint maximum_sample;
    uint minimum_sample;
    uint sample_index;
    uint average;
    ushort sample_sum;

    sample_sum = 0;
    sample_index = 0;
    if (soc_adjusted_mean_history_index < 10) {
        (&soc_adjusted_mean_history[0])[soc_adjusted_mean_history_index] =
            (soc_lookup_adjusted_mean_voltage());
        soc_adjusted_mean_history_index++;
    } else {
        soc_adjusted_mean_history_index = 0;
        (&soc_adjusted_mean_history[0])[soc_adjusted_mean_history_index] =
            (soc_lookup_adjusted_mean_voltage());
        soc_adjusted_mean_history_index++;
    }
    maximum_sample = (uint)soc_adjusted_mean_history[0];
    minimum_sample = maximum_sample;
    do {
        sample = (uint)(&soc_adjusted_mean_history[0])[sample_index];
        if (sample == 0) {
            return 100;
        }
        if (sample > maximum_sample) {
            maximum_sample = sample;
        }
        if (sample < minimum_sample) {
            minimum_sample = sample;
        }
        sample_sum = sample + sample_sum & 0xffff;
        sample_index = sample_index + 1 & 0xff;
    } while (sample_index < 10);
    average = ((sample_sum - maximum_sample) - minimum_sample & 0x7ff) >> 3;
    if (average > 100) {
        average = 100;
    }
    return average;
}

#define first_peer_condition_flags_view (*(uchar *)((uchar *)measurement_peer_records + 76))
#define first_peer_participation_state_view (*(uchar *)((uchar *)measurement_peer_records + 141))

typedef union {
    uchar value;
    struct {
        uchar b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1, b7 : 1;
    } bit;
} BatteryFlags;
/* Counts participating peers and updates per-peer coordination bits. */
void pack_update_peer_participation_flags() {
    uint peer_index;
    uint participating_count;
    uint standby_count;

    peer_index = 0;
    participating_count = 0;
    standby_count = 0;
    do {
        if ((&first_peer_participation_state_view)[peer_index * 0x91] != '\0') {
            participating_count = participating_count + 1 & 0xff;
        }
        if ((&first_peer_participation_state_view)[peer_index * 0x91] == '\x02') {
            standby_count = standby_count + 1 & 0xff;
        }
        peer_index = peer_index + 1 & 0xff;
    } while (peer_index < 6);
    peer_index = 0;
    do {
        if ((((&first_peer_condition_flags_view)[peer_index * 145] & 1) != 0) &&
            ((&first_peer_participation_state_view)[peer_index * 145] == 1) &&
            ((participating_count - standby_count) + 1 > 2)) {
            ((BatteryFlags *)&(&peer_command_bytes[0])[peer_index * 3])->bit.b7 = 1;
            ((BatteryFlags *)&(&peer_command_bytes[0])[peer_index * 3])->bit.b3 = 1;
        } else {
            ((BatteryFlags *)&(&peer_command_bytes[0])[peer_index * 3])->bit.b7 = 0;
        }
        peer_index = (peer_index + 1) & 255;
    } while (peer_index < 6);
}

#undef first_peer_condition_flags_view
#undef first_peer_participation_state_view

#define capacity_full_latch_view capacity_full_latch
#define capacity_learning_active_view capacity_learning_active

/* Debounces high-cell voltage and current conditions for full-capacity state. */
void capacity_update_full_voltage_latches(void) {
    ushort release_voltage;
    int current;
    ushort cell_voltage;
    uint capacity_step;

    release_voltage = 3450;
    current = measurement_current;
    cell_voltage = measurement_maximum_cell_voltage;
    capacity_step = measurement_rated_capacity / 10;
    if (capacity_learning_active == 0 && cell_voltage > 3610 && abs(current) < capacity_step * 8) {
        if (capacity_full_low_current_ticks++ >= 100) {
            capacity_full_low_current_ticks = 0;
            if (remaining_capacity < full_capacity) {
                remaining_capacity = full_capacity;
                primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
            }
            if (capacity_full_latch == 0) {
                capacity_full_latch = 0x10;
            }
        }
    } else {
        capacity_full_low_current_ticks = 0;
    }
    if (capacity_learning_active == 0 &&
        ((cell_voltage > 3650 && abs(current) < capacity_step * 50) || cell_voltage > 3700)) {
        if (capacity_full_trip_ticks++ >= 10) {
            capacity_full_trip_ticks = 0;
            if (remaining_capacity < full_capacity) {
                remaining_capacity = full_capacity;
                primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
            }
            capacity_full_latch = 1;
            capacity_latch_flags |= 1;
        }
    } else {
        capacity_full_trip_ticks = 0;
    }
    if (primary_soc <= 98 && cell_voltage <= 3350) {
        capacity_latch_flags &= 0xfe;
        capacity_latch_flags &= 0xfd;
        capacity_full_latch = 0;
    }
    if (upper_voltage_hold_latched == 1 && capacity_full_latch == 1 && cell_voltage > 3360) {
        if (capacity_full_refresh_ticks++ >= 10) {
            capacity_full_refresh_ticks = 0;
            remaining_capacity = full_capacity;
            primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
        }
    } else {
        capacity_full_refresh_ticks = 0;
    }
    if (capacity_hold_inhibit != 0) {
        return;
    }
    if (measurement_maximum_temperature >= 3030) {
        release_voltage = 3430;
    }
    if (cell_voltage < 3450 && abs(current) > capacity_step * 15 &&
        measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) {
        capacity_low_voltage_hold_ticks++;
    } else {
        capacity_low_voltage_hold_ticks = 0;
        if (capacity_voltage_hold_active != 0 && cell_voltage > release_voltage) {
            capacity_voltage_hold_active = 0;
        }
    }
    if (capacity_low_voltage_hold_ticks > 3000) {
        capacity_low_voltage_hold_ticks = 0;
        capacity_voltage_hold_active = 1;
    }
    if (capacity_voltage_hold_active != 0) {
        if (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) {
            if (capacity_hold_expiry_ticks++ >= 36000) {
                capacity_voltage_hold_active = 0;
                capacity_hold_expiry_ticks = 0;
            }
        }
    } else {
        capacity_hold_expiry_ticks = 0;
    }
}
#undef capacity_full_latch_view
#undef capacity_learning_active_view

/* at: soc_voltage_limits 0x2000047C */
ushort soc_voltage_limits[5][6] = {{0, 3330, 3330, 3330, 3330, 3330},
                                   {1, 3375, 3380, 3385, 3400, 3410},
                                   {2, 3380, 3385, 3390, 3430, 3450},
                                   {3, 3385, 3390, 3410, 3470, 3520},
                                   {4, 3405, 3415, 3440, 3510, 3550}};
#define PABS(x) (uint) abs((int)x)
/* Uses current-duration and temperature bands to latch a voltage-hold state. */
void soc_update_voltage_hold() {
    ushort temperature_or_voltage;
    uint current_or_index;
    int temperature_band;
    int current_duration_band;
    ushort release_voltage;
    uint capacity_step;

    current_duration_band = 0;
    release_voltage = 0xd48;
    if (mean_temperature >= 2980) {
        temperature_or_voltage = measurement_maximum_temperature;
    } else {
        temperature_or_voltage = measurement_minimum_temperature;
    }
    capacity_step = measurement_rated_capacity / 10;
    current_or_index = ((int)(measurement_current));
    if ((measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) || (-500 < (int)current_or_index)) {
        if (PABS(current_or_index) < (capacity_step << 2)) {
            soc_voltage_hold_ticks[0]++;
            soc_voltage_hold_ticks[1] = 0;
            soc_voltage_hold_ticks[2] = 0;
            soc_voltage_hold_ticks[3] = 0;
            soc_voltage_hold_ticks[4] = 0;
        } else if (PABS(current_or_index) < (capacity_step * 10)) {
            soc_voltage_hold_ticks[0]++;
            soc_voltage_hold_ticks[1]++;
            soc_voltage_hold_ticks[2] = 0;
            soc_voltage_hold_ticks[3] = 0;
            soc_voltage_hold_ticks[4] = 0;
        } else if (PABS(current_or_index) < (capacity_step * 20)) {
            soc_voltage_hold_ticks[0]++;
            soc_voltage_hold_ticks[1]++;
            soc_voltage_hold_ticks[2]++;
            soc_voltage_hold_ticks[3] = 0;
            soc_voltage_hold_ticks[4] = 0;
        } else if (PABS(current_or_index) < (capacity_step * 33)) {
            soc_voltage_hold_ticks[0]++;
            soc_voltage_hold_ticks[1]++;
            soc_voltage_hold_ticks[2]++;
            soc_voltage_hold_ticks[3]++;
            soc_voltage_hold_ticks[4] = 0;
        } else if (PABS(current_or_index) < (capacity_step * 50)) {
            soc_voltage_hold_ticks[0]++;
            soc_voltage_hold_ticks[1]++;
            soc_voltage_hold_ticks[2]++;
            soc_voltage_hold_ticks[3]++;
            soc_voltage_hold_ticks[4]++;
        } else {
            soc_voltage_hold_ticks[0] = 0;
            soc_voltage_hold_ticks[1] = 0;
            soc_voltage_hold_ticks[2] = 0;
            soc_voltage_hold_ticks[3] = 0;
            soc_voltage_hold_ticks[4] = 0;
        }
        if (soc_voltage_hold_ticks[4] >= 600) {
            current_duration_band = 4;
            if (soc_voltage_hold_ticks[0] >= 3600)
                soc_voltage_hold_ticks[0] = 3600;
            current_or_index = 1;
            do {
                if ((&soc_voltage_hold_ticks[0])[current_or_index] >= 600)
                    (&soc_voltage_hold_ticks[0])[current_or_index] = 600;
                current_or_index = (current_or_index + 1) & 255;
            } while (current_or_index < 5);
        } else if (soc_voltage_hold_ticks[3] >= 600) {
            current_duration_band = 3;
            if (soc_voltage_hold_ticks[0] >= 3600)
                soc_voltage_hold_ticks[0] = 3600;
            current_or_index = 1;
            do {
                if ((&soc_voltage_hold_ticks[0])[current_or_index] >= 600)
                    (&soc_voltage_hold_ticks[0])[current_or_index] = 600;
                current_or_index = (current_or_index + 1) & 255;
            } while (current_or_index < 5);
        } else if (soc_voltage_hold_ticks[2] >= 600) {
            current_duration_band = 2;
            if (soc_voltage_hold_ticks[0] >= 3600)
                soc_voltage_hold_ticks[0] = 3600;
            current_or_index = 1;
            do {
                if ((&soc_voltage_hold_ticks[0])[current_or_index] >= 600)
                    (&soc_voltage_hold_ticks[0])[current_or_index] = 600;
                current_or_index = (current_or_index + 1) & 255;
            } while (current_or_index < 5);
        } else if (soc_voltage_hold_ticks[1] >= 600) {
            current_duration_band = 1;
            if (soc_voltage_hold_ticks[0] >= 3600)
                soc_voltage_hold_ticks[0] = 3600;
            current_or_index = 1;
            do {
                if ((&soc_voltage_hold_ticks[0])[current_or_index] >= 600)
                    (&soc_voltage_hold_ticks[0])[current_or_index] = 600;
                current_or_index = (current_or_index + 1) & 255;
            } while (current_or_index < 5);
        } else if (soc_voltage_hold_ticks[0] >= 3600) {
            current_duration_band = 5;
            if (soc_voltage_hold_ticks[0] >= 3600)
                soc_voltage_hold_ticks[0] = 3600;
            current_or_index = 1;
            do {
                if ((&soc_voltage_hold_ticks[0])[current_or_index] >= 600)
                    (&soc_voltage_hold_ticks[0])[current_or_index] = 600;
                current_or_index = (current_or_index + 1) & 255;
            } while (current_or_index < 5);
        }

    } else {
        soc_voltage_hold_ticks[0] = 0;
        soc_voltage_hold_ticks[1] = 0;
        soc_voltage_hold_ticks[2] = 0;
        soc_voltage_hold_ticks[3] = 0;
        soc_voltage_hold_ticks[4] = 0;
    }
    if (temperature_or_voltage > 3180)
        temperature_band = 1;
    else if (temperature_or_voltage > 3080)
        temperature_band = 2;
    else if (temperature_or_voltage > 2980)
        temperature_band = 3;
    else if (temperature_or_voltage > 2830)
        temperature_band = 4;
    else if (temperature_or_voltage > 2780)
        temperature_band = 5;
    else if (temperature_or_voltage > 2730)
        temperature_band = 5;
    else
        temperature_band = 0;
    if ((temperature_band - 1U <= 4) && (current_duration_band - 1U <= 3)) {
        temperature_or_voltage = soc_voltage_limits[current_duration_band][temperature_band];
        release_voltage = temperature_or_voltage + 0x14;
    } else if (current_duration_band == 5) {
        temperature_or_voltage = soc_voltage_limits[0][temperature_band];
        release_voltage = 0xd2a;
    } else {
        temperature_or_voltage = 0;
    }
    if ((maximum_selected_cell_voltage < temperature_or_voltage) ||
        (capacity_voltage_hold_active == '\x01')) {
        soc_voltage_hold = '\x01';
    } else if ((maximum_selected_cell_voltage >= release_voltage) && (soc_voltage_hold == '\x01')) {
        soc_voltage_hold = '\0';
        soc_voltage_hold_ticks[0] = 0;
        soc_voltage_hold_ticks[1] = 0;
        soc_voltage_hold_ticks[2] = 0;
        soc_voltage_hold_ticks[3] = 0;
        soc_voltage_hold_ticks[4] = 0;
    }
    if ((voltage_current_limit_flags & 1) != 0 || (voltage_current_limit_flags & 4) != 0 ||
        (temperature_limit_flags & 1) != 0 || (voltage_current_protection_flags & 1) != 0 ||
        (voltage_current_protection_flags & 4) != 0 || (temperature_protection_flags & 1) != 0 ||
        upper_voltage_current_hold != 0) {
        soc_voltage_hold = '\0';
        soc_voltage_hold_ticks[0] = 0;
        soc_voltage_hold_ticks[1] = 0;
        soc_voltage_hold_ticks[2] = 0;
        soc_voltage_hold_ticks[3] = 0;
        soc_voltage_hold_ticks[4] = 0;
    }
    return;
}

#undef PABS

/* Debounces low cell/pack voltage and clears capacity at the deeper threshold. */
void capacity_update_empty_voltage_latches(void) {
    ushort cell_voltage = measurement_minimum_cell_voltage;

    if (cell_voltage < 2800 || measurement_pack_voltage < 45200) {
        capacity_empty_clear_ticks = 0;
        if (capacity_empty_trip_ticks++ >= 10) {
            capacity_empty_trip_ticks = 0;
            capacity_empty_latch = 1;
        }
    } else if (cell_voltage > 3100 || measurement_pack_voltage > 50000) {
        capacity_empty_trip_ticks = 0;
        if (capacity_empty_clear_ticks++ >= 10) {
            capacity_empty_clear_ticks = 0;
            capacity_empty_latch = 0;
        }
    } else {
        capacity_empty_clear_ticks = 0;
        capacity_empty_trip_ticks = 0;
    }
    if (((measurement_minimum_cell_voltage < 2580 &&
          sorted_cell_voltages[measurement_cell_count - 2] < 2600) ||
         measurement_minimum_cell_voltage < 2390 || measurement_pack_voltage < 44800) &&
        capacity_empty_latch != 2) {
        if (capacity_deep_empty_ticks++ >= 10) {
            capacity_deep_empty_ticks = 0;
            capacity_empty_latch = 2;
            remaining_capacity = 0;
            displayed_capacity = 0;
            primary_soc = 0;
            displayed_soc = 0;
        }
    } else {
        capacity_deep_empty_ticks = 0;
    }
}

#define capacity_learning_active_view capacity_learning_active

/* Reconciles integrated capacity with high, low and mean-voltage SOC estimates.
   match: I retain the initialized address diagnostics, staged updates and correction-state order. */
void soc_reconcile_capacity(void) {
    uint soc_estimate;
    uchar correction_override;
    uchar low_soc_ready;
    int correction_due;
    int soc_difference_limit;
    int reset_voltage;
    uint rest_correction;
    uint capacity_step;
    int current;

    soc_estimate = 0;
    correction_override = 0;
    low_soc_ready = 0;
    reset_voltage = 0;
    capacity_update_full_voltage_latches();
    capacity_update_empty_voltage_latches();
    if (++soc_update_divider < 10) {
        return;
    }
    soc_update_divider = 0;
    capacity_step = measurement_rated_capacity / 10;
    charge_update_current_limit();
    discharge_update_current_limit();
    soc_update_voltage_hold();
    correction_due = soc_correction_tick();
    rest_correction = soc_evaluate_rest_correction();
    if (rest_correction != 0 && rest_correction != 0x20 && displayed_soc < 80) {
        soc_rest_correction_ticks++;
        if (measurement_minimum_temperature <= 2730) {
            soc_correction_state = BMS_SOC_CORRECTION_BLOCKED;
            soc_rest_correction_ticks = 0;
        }
        if (rest_correction == 2) {
            soc_correction_state = BMS_SOC_CORRECTION_BLOCKED;
        } else if (rest_correction == 1 || rest_correction >= 16) {
            soc_estimate = soc_average_adjusted_mean_voltage();
            if (rest_correction == 16 && soc_correction_lock_state != 0) {
                correction_override = 1;
            }
        }
        if (soc_rest_correction_ticks >= 20) {
            soc_correction_target = soc_estimate;
            if (!((soc_estimate <= 15 || primary_soc <= 15) && soc_estimate < 100 &&
                  (soc_correction_lock_state != 1 || correction_override))) {
                soc_correction_state = BMS_SOC_CORRECTION_BLOCKED;
                soc_rest_correction_ticks = 0;
            } else {
                if (soc_estimate > 30) {
                    soc_estimate = 30;
                    soc_correction_lock_state = 2;
                } else {
                    soc_correction_lock_state = 0;
                }
                if (primary_soc - soc_estimate + 3 <= 6 ||
                    soc_correction_state == BMS_SOC_CORRECTION_ACTIVE) {
                    soc_correction_state = BMS_SOC_CORRECTION_SETTLED;
                    soc_rest_correction_ticks = 0;
                    soc_correction_lock_state = 0;
                } else {
                    soc_correction_interval = 1800;
                    if (soc_estimate > 15) {
                        soc_estimate = 15;
                    }
                    soc_correction_target = soc_estimate;
                    full_capacity_snapshot = capacity_get_full_capacity();
                    remaining_capacity =
                        (float)(soc_correction_target + 0.5) * full_capacity / 100.0f;
                    primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                    soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                    soc_rest_correction_ticks = 0;
                    soc_correction_gain = 2.0f;
                }
            }
        }
    } else if (rest_correction == 0x20) {
        if (measurement_minimum_temperature <= 2730) {
            soc_correction_state = BMS_SOC_CORRECTION_BLOCKED;
        } else if (mean_cell_voltage <= 3300 && primary_soc >= 90) {
            remaining_capacity = full_capacity * 80.5f / 100.0f;
            primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
            soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
            soc_correction_interval = 120;
            soc_correction_gain = 2.0f;
        } else if (mean_cell_voltage > 3320) {
            soc_correction_state = BMS_SOC_CORRECTION_SETTLED;
        }
    } else if (correction_due != 0) {
        if (measurement_minimum_temperature <= 2730 ||
            abs((int)measurement_current) >= capacity_step * 60) {
            soc_estimate = soc_average_high_cell(0);
        } else {
            soc_estimate = soc_average_high_cell(1);
        }
        if (primary_soc >= 90) {
            soc_difference_limit = 2;
        } else {
            soc_difference_limit = 2;
        }
        if (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) {
            if (primary_soc >= 90 && soc_estimate < 75) {
                if (abs((int)measurement_current) > capacity_step * 33) {
                    soc_high_load_ticks++;
                } else {
                    soc_high_load_ticks = 0;
                }
                if (soc_high_load_ticks > 600) {
                    remaining_capacity = full_capacity * 85.5f / 100.0f;
                    primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                    soc_correction_elapsed = 0;
                    soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                    soc_correction_gain = 2.0f;
                    soc_correction_interval = 600;
                }
            } else {
                soc_high_load_ticks = 0;
            }
        }
        if (!(abs(primary_soc - soc_estimate) < soc_difference_limit ||
              primary_soc >= soc_estimate || soc_estimate < 85 ||
              measurement_pack_voltage <= 54500 || capacity_learning_active != 0)) {
            if (!(extended_limit_flags & 0x20) ||
                ((extended_limit_flags & 0x20) && soc_estimate >= 95)) {
                soc_near_full_ticks = 0;
                remaining_capacity = (float)(soc_estimate + 0.5) * full_capacity / 100.0f;
                primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                soc_correction_elapsed = 0;
                soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                if (soc_estimate >= 95) {
                    soc_correction_gain = 4.0f;
                } else {
                    soc_correction_gain = 3.0f;
                }
                soc_correction_interval = 120;
                return;
            }
        } else {
            if (measurement_pack_voltage > 54100 && measurement_minimum_cell_voltage > 3350 &&
                abs((int)measurement_current) < 1500 && primary_soc < 95) {
                if (300 <= soc_near_full_ticks++) {
                    soc_near_full_ticks = 0;
                    remaining_capacity = full_capacity * 98.0f / 100.0f;
                    primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                    soc_correction_elapsed = 0;
                    soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                    soc_correction_gain = 3.0f;
                    soc_correction_interval = 120;
                    return;
                }
            } else {
                soc_near_full_ticks = 0;
            }
        }
        if (measurement_minimum_temperature <= 2730 ||
            abs((int)measurement_current) >= capacity_step * 60) {
            soc_estimate = soc_average_low_cell(0);
        } else {
            soc_estimate = soc_average_low_cell(1);
        }
        if (primary_soc - soc_estimate + 2 > 4 && primary_soc > soc_estimate &&
            soc_estimate <= 15 && measurement_pack_voltage < 50000) {
            if (!(extended_limit_flags & 0x20) ||
                ((extended_limit_flags & 0x20) && soc_estimate <= 5)) {
                remaining_capacity = (float)(soc_estimate + 0.5) * full_capacity / 100.0f;
                primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                soc_correction_elapsed = 0;
                soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                if (soc_estimate <= 6) {
                    soc_correction_gain = 3.0f;
                } else {
                    soc_correction_gain = 2.0f;
                }
                if ((uint)(primary_soc - displayed_soc + 9) > 18) {
                    soc_correction_gain = 2.0f;
                }
                soc_correction_interval = 120;
                return;
            }
        } else if (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
            current = measurement_current;
            if (abs(current) < capacity_step * 20) {
                soc_rate_duration_ticks[0]++;
                soc_rate_duration_ticks[1]++;
                soc_rate_duration_ticks[2]++;
                soc_rate_duration_ticks[3]++;
            } else if (abs(current) < capacity_step * 33) {
                soc_rate_duration_ticks[0] = 0;
                soc_rate_duration_ticks[1]++;
                soc_rate_duration_ticks[2]++;
                soc_rate_duration_ticks[3]++;
            } else if (abs(current) < capacity_step * 40) {
                soc_rate_duration_ticks[0] = 0;
                soc_rate_duration_ticks[1] = 0;
                soc_rate_duration_ticks[2]++;
                soc_rate_duration_ticks[3]++;
            } else if (abs(current) < capacity_step * 60) {
                soc_rate_duration_ticks[0] = 0;
                soc_rate_duration_ticks[1] = 0;
                soc_rate_duration_ticks[2] = 0;
                soc_rate_duration_ticks[3]++;
            } else {
                soc_rate_duration_ticks[0] = 0;
                soc_rate_duration_ticks[1] = 0;
                soc_rate_duration_ticks[2] = 0;
                soc_rate_duration_ticks[3] = 0;
            }
            if (soc_estimate < 20 && soc_rate_duration_ticks[2] >= 300) {
                low_soc_ready = 1;
                soc_rate_duration_ticks[2] = 200;
            } else if (soc_estimate < 25 && soc_rate_duration_ticks[1] >= 300) {
                low_soc_ready = 1;
                soc_rate_duration_ticks[1] = 200;
            } else if (soc_estimate < 30 && soc_rate_duration_ticks[0] >= 300) {
                low_soc_ready = 1;
                soc_rate_duration_ticks[0] = 200;
            }
            if (low_soc_ready && primary_soc - soc_estimate + 2 > 4 && primary_soc > soc_estimate) {
                if (!(extended_limit_flags & 0x20) ||
                    ((extended_limit_flags & 0x20) && soc_estimate <= 5)) {
                    remaining_capacity = (float)(soc_estimate + 0.5) * full_capacity / 100.0f;
                    {
                        uint address_check;
                        address_check =
                            ((const void *)(&primary_soc) != (const void *)(&soc_correction_state));
                    }
                    primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                    soc_correction_elapsed = 0;
                    soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                    soc_correction_gain = 2.0f;
                    if ((uint)(primary_soc - displayed_soc + 9) > 18) {
                        soc_correction_gain = 3.0f;
                    }
                    soc_correction_interval = 120;
                    return;
                }
            }
        } else {
            soc_rate_duration_ticks[0] = 0;
            soc_rate_duration_ticks[1] = 0;
            soc_rate_duration_ticks[2] = 0;
            soc_rate_duration_ticks[3] = 0;
        }
        if (measurement_minimum_temperature <= 2630) {
            soc_estimate = 0xff;
        } else {
            soc_estimate = soc_classify_current_rate(measurement_minimum_cell_voltage,
                                                     (int)measurement_current);
        }
        if (soc_estimate <= 10) {
            soc_difference_limit = sorted_selected_cell_voltages[9];
            soc_correction_target = soc_estimate = soc_update_voltage_average(
                soc_difference_limit, measurement_minimum_temperature, soc_estimate);
            if (soc_estimate >= 40) {
                if (soc_correction_lock_state != 0) {
                    correction_override = 1;
                }
                soc_estimate = 30;
            }
            if (soc_estimate <= 30 && soc_estimate - primary_soc + 4 > 8 &&
                soc_estimate > primary_soc && soc_correction_state != BMS_SOC_CORRECTION_ACTIVE &&
                (soc_correction_lock_state == 0 || correction_override == 1)) {
                remaining_capacity = (float)(soc_estimate + 0.5) * full_capacity / 100.0f;
                {
                    uint address_check;
                    address_check =
                        ((const void *)(&primary_soc) != (const void *)(&soc_correction_state));
                }
                primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
                soc_correction_elapsed = 0;
                soc_correction_state = BMS_SOC_CORRECTION_ACTIVE;
                soc_correction_gain = 2.0f;
                if ((uint)(primary_soc - displayed_soc + 9) > 18) {
                    soc_correction_gain = 3.0f;
                }
                soc_correction_interval = 10;
                return;
            }
        } else {
            soc_update_voltage_average(reset_voltage, measurement_minimum_temperature,
                                       soc_estimate);
        }
    } else {
        soc_rest_correction_ticks = 0;
        if (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE) {
            if (soc_correction_state != BMS_SOC_CORRECTION_ACTIVE) {
                soc_correction_elapsed = 0;
                soc_correction_state = BMS_SOC_CORRECTION_POSITIVE_CURRENT;
            }
        } else if (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
            if (soc_correction_state != BMS_SOC_CORRECTION_ACTIVE) {
                soc_correction_elapsed = 0;
                soc_correction_state = BMS_SOC_CORRECTION_NEGATIVE_CURRENT;
            }
        } else if (soc_correction_state != BMS_SOC_CORRECTION_ACTIVE) {
            soc_correction_elapsed = 0;
            soc_correction_state = BMS_SOC_CORRECTION_IDLE;
        }
        if (soc_correction_state == BMS_SOC_CORRECTION_ACTIVE || displayed_soc != primary_soc) {
            if (soc_correction_elapsed++ >= soc_correction_interval ||
                displayed_soc == primary_soc) {
                soc_correction_state = BMS_SOC_CORRECTION_SETTLED;
                soc_correction_elapsed = 0;
            }
        } else {
            soc_correction_elapsed = 0;
        }
    }
    if (++soc_full_capacity_refresh_ticks >= 10) {
        soc_full_capacity_refresh_ticks = 0;
        full_capacity_snapshot = capacity_get_full_capacity();
    }
    pack_aggregate_current_limits();
    if (soc_correction_state == BMS_SOC_CORRECTION_ACTIVE && soc_correction_target < 90) {
        if (soc_correction_lock_state == 0) {
            soc_correction_lock_state = 1;
        }
        soc_correction_day_baseline = 0;
        soc_correction_day_count = 0;
        soc_correction_day_ticks = 0;
    }
    if (soc_correction_day_baseline != soc_correction_day_count) {
        soc_correction_day_baseline = 0;
        soc_correction_day_count = 0;
        if (soc_correction_lock_state != 0) {
            soc_correction_lock_state = 0;
        }
    }
    if (soc_correction_day_ticks++ >= 86400) {
        soc_correction_day_ticks = 0;
        soc_correction_day_count++;
    }
}

#undef capacity_learning_active_view

/* Debounces the combined low-SOC and sub-freezing-temperature flag. */
void protection_update_cold_low_soc(void) {
    ushort minimum_temperature;
    uchar flags;

    if (++cold_low_soc_divider >= 10) {
        cold_low_soc_divider = 0;
        minimum_temperature = measurement_minimum_temperature;
        flags = extended_limit_flags;
        if (flags & 0x10) {
            if ((primary_soc > 6 || minimum_temperature >= 2740) &&
                ++cold_low_soc_clear_ticks >= 4) {
                cold_low_soc_clear_ticks = 0;
                extended_limit_flags &= 0xef;
            }
            cold_low_soc_trip_ticks = 0;
        } else {
            cold_low_soc_clear_ticks = 0;
            if (primary_soc <= 4 && minimum_temperature < 2730) {
                if (++cold_low_soc_trip_ticks >= 20) {
                    cold_low_soc_trip_ticks = 0;
                    extended_limit_flags |= 0x10;
                }
            } else {
                cold_low_soc_trip_ticks = 0;
            }
        }
    }
}

#define capacity_full_latch_view capacity_full_latch
#define capacity_learning_ticks_view capacity_learning_ticks
#define capacity_learning_active_view capacity_learning_active

/* Runs capacity latches and reconciliation, then publishes primary and displayed SOC. */
void capacity_update_soc(void) {
    if (++capacity_learning_ticks >= 1728000) {
        capacity_learning_ticks = 0;
        if (capacity_learning_active != 0) {
            capacity_learning_active = 0;
            eeprom_write_word(2, capacity_learning_active);
        }
    }
    capacity_latch_full_voltage();
    capacity_latch_empty_voltage();
    soc_reconcile_capacity();
    capacity_update_operating_status();
    protection_update_cold_low_soc();
    if (remaining_capacity >= full_capacity) {
        primary_soc = 100;
        if (capacity_full_latch == 1) {
            displayed_soc = 100;
        } else if (displayed_soc >= 100) {
            displayed_soc = 100;
        } else {
            displayed_soc = displayed_capacity / full_capacity * 100.0f + 0.5;
        }
        if (measurement_rated_capacity >= remaining_capacity &&
            remaining_capacity >= full_capacity) {
            full_capacity = remaining_capacity;
        }
        if (capacity_voltage_hold_active != 0 && measurement_rated_capacity < remaining_capacity &&
            remaining_capacity > full_capacity) {
            if (measurement_rated_capacity < full_capacity) {
                remaining_capacity = full_capacity;
            } else {
                remaining_capacity = measurement_rated_capacity;
            }
        }
    } else {
        primary_soc = remaining_capacity / full_capacity * 100.0f + 0.5;
        displayed_soc = displayed_capacity / full_capacity * 100.0f + 0.5;
    }
}
#undef capacity_full_latch_view
#undef capacity_learning_ticks_view
#undef capacity_learning_active_view

extern uchar g_20000F9C[];
#define undervoltage_shutdown_option (*(volatile uchar *)(g_20000F9C + 104))
extern uchar limit_cell_overvoltage[];
#define limit_pack_undervoltage BMS_LIMIT_FIELD(measurement_limit_settings, pack_undervoltage)
#define protection_pack_undervoltage                                                               \
    BMS_PROTECTION_FIELD(measurement_protection_settings, pack_undervoltage)

void protection_update_pack_undervoltage() {

    if (((int)((uint)protection_inhibit_flags << 0x1c) < 0) || (measurement_current > 300.0f)) {
        pack_undervoltage_limit_ticks = 0;
        pack_undervoltage_protection_ticks = 0;
        pack_undervoltage_normal_ticks = 0;
        if ((int)((uint)protection_inhibit_flags << 0x1c) < 0) {
            voltage_current_limit_flags =
                voltage_current_limit_flags & BMS_LIMIT_PACK_UNDERVOLTAGE_CLEAR_MASK;
        }
        return;
    }
    {
        uchar debounce_limit;
        if (measurement_pack_voltage < limit_pack_undervoltage) {
            pack_undervoltage_normal_ticks = 0;
            if (-1 < (int)((uint)voltage_current_limit_flags
                           << BMS_LIMIT_PACK_UNDERVOLTAGE_SIGN_SHIFT)) {
                pack_undervoltage_limit_ticks = pack_undervoltage_limit_ticks + 1;
                debounce_limit = protection_debounce_settings[3];
                if ((pack_undervoltage_limit_ticks >= 250) ||
                    (debounce_limit <= pack_undervoltage_limit_ticks)) {
                    pack_undervoltage_limit_ticks = 0;
                    voltage_current_limit_flags =
                        voltage_current_limit_flags | BMS_LIMIT_PACK_UNDERVOLTAGE;
                }
            }
        } else {
            pack_undervoltage_limit_ticks = 0;
            if (((int)((uint)voltage_current_limit_flags
                       << BMS_LIMIT_PACK_UNDERVOLTAGE_SIGN_SHIFT) < 0) &&
                (pack_undervoltage_normal_ticks = pack_undervoltage_normal_ticks + 1,
                 0xfa < pack_undervoltage_normal_ticks)) {
                pack_undervoltage_normal_ticks = 0;
            }
        }
        if ((int)((uint)voltage_current_protection_flags
                  << BMS_PROTECTION_PACK_UNDERVOLTAGE_SIGN_SHIFT) < 0) {
            pack_undervoltage_protection_ticks = 0;
        } else {
            if (protection_pack_undervoltage > measurement_pack_voltage) {
                pack_undervoltage_protection_ticks = pack_undervoltage_protection_ticks + 1;
                if ((pack_undervoltage_protection_ticks >= 250) ||
                    (protection_debounce_settings[3] <= pack_undervoltage_protection_ticks)) {
                    pack_undervoltage_protection_ticks = 0;
                    voltage_current_protection_flags =
                        voltage_current_protection_flags | BMS_PROTECTION_PACK_UNDERVOLTAGE;
                    if (undervoltage_shutdown_option == '\0') {
                        if (measurement_pack_voltage < 15000) {
                            runtime_timer_start(0xd, 72000);
                        } else {
                            runtime_timer_start(0xd, 0x23280);
                        }
                    }
                    protection_event_counts[3] = protection_event_counts[3] + 1;
                    eeprom_write_word(0x66, protection_event_counts[3]);
                    coordination_record_event(5);
                    return;
                }
            } else {
                pack_undervoltage_protection_ticks = 0;
            }
        }
        return;
    }
}
#undef undervoltage_shutdown_option
#undef limit_pack_undervoltage
#undef protection_pack_undervoltage

#define gpio_a_byte_view (*(volatile uchar *)0x40010800)
#define cell_voltage_step_detected_view cell_voltage_step_detected
extern uchar g_20000F9C[];
#define undervoltage_shutdown_option (*(volatile uchar *)(g_20000F9C + 104))
extern uchar limit_cell_overvoltage[];
#define limit_cell_undervoltage BMS_LIMIT_FIELD(measurement_limit_settings, cell_undervoltage)
#define protection_cell_undervoltage                                                               \
    BMS_PROTECTION_FIELD(measurement_protection_settings, cell_undervoltage)

void protection_update_cell_undervoltage(void) {
    uchar flag = protection_inhibit_flags;

    if ((flag & 2) || measurement_current > 300.0f) {
        cell_undervoltage_limit_ticks = 0;
        cell_undervoltage_protection_ticks = 0;
        cell_undervoltage_normal_ticks = 0;
        if (protection_inhibit_flags & 2) {
            voltage_current_limit_flags &= BMS_LIMIT_CELL_UNDERVOLTAGE_CLEAR_MASK;
        }
        return;
    }
    if ((measurement_minimum_cell_voltage < limit_cell_undervoltage &&
         sorted_cell_voltages[measurement_cell_count - 2] <= limit_cell_undervoltage) ||
        (measurement_minimum_cell_voltage < 2400 && cell_voltage_step_detected_view == 0) ||
        minimum_reference_cell_voltage <= 2350) {
        cell_undervoltage_normal_ticks = 0;
        if (!(voltage_current_limit_flags & BMS_LIMIT_CELL_UNDERVOLTAGE)) {
            cell_undervoltage_limit_ticks++;
            if (cell_undervoltage_limit_ticks >= 250 ||
                cell_undervoltage_limit_ticks >= protection_debounce_settings[1]) {
                cell_undervoltage_limit_ticks = 0;
                voltage_current_limit_flags |= BMS_LIMIT_CELL_UNDERVOLTAGE;
            }
        }
    } else {
        cell_undervoltage_limit_ticks = 0;
        if (voltage_current_limit_flags & BMS_LIMIT_CELL_UNDERVOLTAGE) {
            cell_undervoltage_normal_ticks++;
            if (cell_undervoltage_normal_ticks > 250) {
                cell_undervoltage_normal_ticks = 0;
            }
        }
    }
    if (voltage_current_protection_flags & BMS_PROTECTION_CELL_UNDERVOLTAGE) {
        cell_undervoltage_protection_ticks = 0;
        return;
    }
    if ((measurement_minimum_cell_voltage < protection_cell_undervoltage &&
         sorted_cell_voltages[measurement_cell_count - 2] <= protection_cell_undervoltage) ||
        measurement_minimum_cell_voltage < 2000 || minimum_reference_cell_voltage <= 2000) {
        cell_undervoltage_protection_ticks++;
        if (cell_undervoltage_protection_ticks >= 250 ||
            cell_undervoltage_protection_ticks >= protection_debounce_settings[1]) {
            cell_undervoltage_protection_ticks = 0;
            voltage_current_protection_flags |= BMS_PROTECTION_CELL_UNDERVOLTAGE;
            if (undervoltage_shutdown_option == 0) {
                if (measurement_minimum_cell_voltage < 1500) {
                    runtime_timer_start(13, 72000);
                } else {
                    runtime_timer_start(13, 144000);
                    GPIO_ResetBits(&gpio_a_byte_view, 0x8000);
                }
            }
            cell_undervoltage_shutdown_state = 1;
            protection_event_counts[3]++;
            eeprom_write_word(0x66, protection_event_counts[3]);
            coordination_record_event(5);
        }
    } else {
        cell_undervoltage_protection_ticks = 0;
    }
}
#undef gpio_a_byte_view
#undef cell_voltage_step_detected_view
#undef undervoltage_shutdown_option
#undef limit_cell_undervoltage
#undef protection_cell_undervoltage

extern uchar limit_cell_overvoltage[];
#define limit_pack_overvoltage BMS_LIMIT_FIELD(measurement_limit_settings, pack_overvoltage)
#define protection_pack_overvoltage                                                                \
    BMS_PROTECTION_FIELD(measurement_protection_settings, pack_overvoltage)

void protection_update_pack_overvoltage(void) {
    if (protection_inhibit_flags & 4) {
        pack_overvoltage_limit_ticks = 0;
        pack_overvoltage_protection_ticks = 0;
        pack_overvoltage_normal_ticks = 0;
        voltage_current_limit_flags &= BMS_LIMIT_PACK_OVERVOLTAGE_CLEAR_MASK;
        return;
    }
    if (limit_pack_overvoltage < measurement_pack_voltage) {
        pack_overvoltage_normal_ticks = 0;
        if (!(voltage_current_limit_flags & BMS_LIMIT_PACK_OVERVOLTAGE)) {
            pack_overvoltage_limit_ticks++;
            if (pack_overvoltage_limit_ticks >= 250 ||
                pack_overvoltage_limit_ticks >= protection_debounce_settings[2]) {
                pack_overvoltage_limit_ticks = 0;
                voltage_current_limit_flags |= BMS_LIMIT_PACK_OVERVOLTAGE;
            }
        }
    } else {
        pack_overvoltage_limit_ticks = 0;
        if (voltage_current_limit_flags & BMS_LIMIT_PACK_OVERVOLTAGE) {
            pack_overvoltage_normal_ticks++;
            if (pack_overvoltage_normal_ticks > 250) {
                pack_overvoltage_normal_ticks = 0;
            }
        }
    }
    if (voltage_current_protection_flags & BMS_PROTECTION_PACK_OVERVOLTAGE) {
        pack_overvoltage_protection_ticks = 0;
        return;
    }
    if (measurement_pack_voltage > protection_pack_overvoltage ||
        measurement_pack_voltage >= 59000) {
        pack_overvoltage_protection_ticks++;
        if (pack_overvoltage_protection_ticks >= 250 ||
            pack_overvoltage_protection_ticks >= protection_debounce_settings[2]) {
            pack_overvoltage_protection_ticks = 0;
            voltage_current_protection_flags |= BMS_PROTECTION_PACK_OVERVOLTAGE;
            protection_event_counts[4]++;
            eeprom_write_word(100, protection_event_counts[4]);
            coordination_record_event(2);
        }
    } else {
        pack_overvoltage_protection_ticks = 0;
    }
}
#undef limit_pack_overvoltage
#undef protection_pack_overvoltage

#define cell_voltage_step_detected_view cell_voltage_step_detected
extern uchar limit_cell_overvoltage[];
#define limit_cell_overvoltage BMS_LIMIT_FIELD(measurement_limit_settings, cell_overvoltage)
#define protection_cell_overvoltage                                                                \
    BMS_PROTECTION_FIELD(measurement_protection_settings, cell_overvoltage)

void protection_update_cell_overvoltage() {

    if ((protection_inhibit_flags & 1) == 0) {
        if ((((limit_cell_overvoltage < measurement_maximum_cell_voltage) &&
              (sorted_cell_voltages[1] >= limit_cell_overvoltage)) ||
             ((0xe74 < measurement_maximum_cell_voltage &&
               (cell_voltage_step_detected_view == '\0')))) ||
            (0xea6 < maximum_reference_cell_voltage)) {
            cell_overvoltage_normal_ticks = 0;
            if (((voltage_current_limit_flags & BMS_LIMIT_CELL_OVERVOLTAGE) == 0) &&
                ((cell_overvoltage_limit_ticks = cell_overvoltage_limit_ticks + 1,
                  cell_overvoltage_limit_ticks >= 250 ||
                      (protection_debounce_settings[0] <= cell_overvoltage_limit_ticks)))) {
                cell_overvoltage_limit_ticks = 0;
                voltage_current_limit_flags =
                    voltage_current_limit_flags | BMS_LIMIT_CELL_OVERVOLTAGE;
            }
        } else {
            cell_overvoltage_limit_ticks = 0;
            if (((voltage_current_limit_flags & BMS_LIMIT_CELL_OVERVOLTAGE) != 0) &&
                (++cell_overvoltage_normal_ticks > 250)) {
                cell_overvoltage_normal_ticks = 0;
            }
        }
        if ((voltage_current_protection_flags & BMS_PROTECTION_CELL_OVERVOLTAGE) == 0) {
            if ((((protection_cell_overvoltage < measurement_maximum_cell_voltage) &&
                  (sorted_cell_voltages[1] >= protection_cell_overvoltage)) ||
                 (measurement_maximum_cell_voltage > 3750)) ||
                (0xed8 < maximum_reference_cell_voltage)) {
                cell_overvoltage_protection_ticks = cell_overvoltage_protection_ticks + 1;
                if ((cell_overvoltage_protection_ticks >= 250) ||
                    (protection_debounce_settings[0] <= cell_overvoltage_protection_ticks)) {
                    cell_overvoltage_protection_ticks = 0;
                    voltage_current_protection_flags =
                        voltage_current_protection_flags | BMS_PROTECTION_CELL_OVERVOLTAGE;
                    protection_event_counts[4] = protection_event_counts[4] + 1;
                    eeprom_write_word(100, protection_event_counts[4]);
                    coordination_record_event(2);
                    return;
                }
            } else {
                cell_overvoltage_protection_ticks = 0;
            }
        } else {
            cell_overvoltage_protection_ticks = 0;
        }
    } else {
        cell_overvoltage_limit_ticks = 0;
        cell_overvoltage_protection_ticks = 0;
        cell_overvoltage_normal_ticks = 0;
        voltage_current_limit_flags =
            voltage_current_limit_flags & BMS_LIMIT_CELL_OVERVOLTAGE_CLEAR_MASK;
    }
    return;
}
#undef cell_voltage_step_detected_view
#undef limit_cell_overvoltage
#undef protection_cell_overvoltage

/* Runs four cell/pack over/undervoltage checks. */
void protection_update_voltage() {

    protection_update_cell_overvoltage();
    protection_update_cell_undervoltage();
    protection_update_pack_overvoltage();
    protection_update_pack_undervoltage();
    return;
}

extern uchar g_20000F9C[];
#define discharge_overcurrent_retry_option (*(sbyte *)(g_20000F9C + 89))
extern uchar limit_cell_overvoltage[];
#define limit_discharge_overcurrent                                                                \
    BMS_LIMIT_FIELD(measurement_limit_settings, discharge_overcurrent)
#define protection_discharge_overcurrent                                                           \
    BMS_PROTECTION_FIELD(measurement_protection_settings, discharge_overcurrent)

void protection_update_discharge_overcurrent() {

    if ((int)((uint)protection_inhibit_flags << 0x1a) < 0) {
        discharge_overcurrent_limit_ticks = 0;
        discharge_overcurrent_protection_ticks = 0;
        discharge_overcurrent_normal_ticks = 0;
        voltage_current_limit_flags =
            voltage_current_limit_flags & BMS_LIMIT_DISCHARGE_OVERCURRENT_CLEAR_MASK;
    } else {

        if (measurement_current < (float)limit_discharge_overcurrent * 1000.0f) {
            discharge_overcurrent_normal_ticks = 0;
            if ((-1 < (int)((uint)voltage_current_limit_flags
                            << BMS_LIMIT_DISCHARGE_OVERCURRENT_SIGN_SHIFT)) &&
                ((discharge_overcurrent_limit_ticks = discharge_overcurrent_limit_ticks + 1,
                  discharge_overcurrent_limit_ticks >= 250 ||
                      (protection_debounce_settings[5] < discharge_overcurrent_limit_ticks)))) {
                discharge_overcurrent_limit_ticks = 0;
                voltage_current_limit_flags =
                    voltage_current_limit_flags | BMS_LIMIT_DISCHARGE_OVERCURRENT;
            }
        } else {
            discharge_overcurrent_limit_ticks = 0;
            if (((int)((uint)voltage_current_limit_flags
                       << BMS_LIMIT_DISCHARGE_OVERCURRENT_SIGN_SHIFT) < 0) &&
                (++discharge_overcurrent_normal_ticks > 100)) {
                discharge_overcurrent_normal_ticks = 0;
                voltage_current_limit_flags =
                    voltage_current_limit_flags & BMS_LIMIT_DISCHARGE_OVERCURRENT_CLEAR_MASK;
            }
        }
        if ((int)((uint)voltage_current_protection_flags
                  << BMS_PROTECTION_DISCHARGE_OVERCURRENT_SIGN_SHIFT) < 0) {
            discharge_overcurrent_protection_ticks = 0;
        } else {

            if (measurement_current < (float)protection_discharge_overcurrent * 1000.0f) {
                discharge_overcurrent_protection_ticks = discharge_overcurrent_protection_ticks + 1;
                if ((discharge_overcurrent_protection_ticks >= 250) ||
                    (protection_debounce_settings[5] < discharge_overcurrent_protection_ticks)) {
                    discharge_overcurrent_protection_ticks = 0;
                    voltage_current_protection_flags =
                        voltage_current_protection_flags | BMS_PROTECTION_DISCHARGE_OVERCURRENT;
                    if (discharge_overcurrent_retry_option != 0) {
                        discharge_overcurrent_retry_count = 0;
                    } else {
                        discharge_overcurrent_retry_count++;
                    }
                    runtime_timer_start(0xb, 0x960);
                    protection_event_counts[2] = protection_event_counts[2] + 1;
                    eeprom_write_word(0x68, protection_event_counts[2]);
                    coordination_record_event(0);
                    return;
                }
            } else {
                discharge_overcurrent_protection_ticks = 0;
            }
        }
    }
    return;
}
#undef discharge_overcurrent_retry_option
#undef limit_discharge_overcurrent
#undef protection_discharge_overcurrent

extern uchar g_20000F9C[];
#define charge_overcurrent_retry_option (*(sbyte *)(g_20000F9C + 81))
extern uchar limit_cell_overvoltage[];
#define limit_charge_overcurrent                                                                   \
    ((ushort)BMS_LIMIT_FIELD(measurement_limit_settings, charge_overcurrent))
#define protection_charge_overcurrent                                                              \
    ((ushort)BMS_PROTECTION_FIELD(measurement_protection_settings, charge_overcurrent))

void protection_update_charge_overcurrent() {

    if ((int)((uint)protection_inhibit_flags << 0x1b) < 0) {
        charge_overcurrent_limit_ticks = 0;
        charge_overcurrent_protection_ticks = 0;
        charge_overcurrent_normal_ticks = 0;
        voltage_current_limit_flags =
            voltage_current_limit_flags & BMS_LIMIT_CHARGE_OVERCURRENT_CLEAR_MASK;
    } else {

        if (measurement_current > (float)limit_charge_overcurrent * 1000.0f) {
            charge_overcurrent_normal_ticks = 0;
            if ((-1 < (int)((uint)voltage_current_limit_flags
                            << BMS_LIMIT_CHARGE_OVERCURRENT_SIGN_SHIFT)) &&
                ((charge_overcurrent_limit_ticks = charge_overcurrent_limit_ticks + 1,
                  charge_overcurrent_limit_ticks >= 250 ||
                      (protection_debounce_settings[4] <= charge_overcurrent_limit_ticks)))) {
                charge_overcurrent_limit_ticks = 0;
                voltage_current_limit_flags =
                    voltage_current_limit_flags | BMS_LIMIT_CHARGE_OVERCURRENT;
            }
        } else {
            charge_overcurrent_limit_ticks = 0;
            if (((int)((uint)voltage_current_limit_flags
                       << BMS_LIMIT_CHARGE_OVERCURRENT_SIGN_SHIFT) < 0) &&
                (++charge_overcurrent_normal_ticks > 100)) {
                charge_overcurrent_normal_ticks = 0;
                voltage_current_limit_flags =
                    voltage_current_limit_flags & BMS_LIMIT_CHARGE_OVERCURRENT_CLEAR_MASK;
            }
        }
        if ((int)((uint)voltage_current_protection_flags
                  << BMS_PROTECTION_CHARGE_OVERCURRENT_SIGN_SHIFT) < 0) {
            charge_overcurrent_protection_ticks = 0;
        } else {

            if (measurement_current > (float)protection_charge_overcurrent * 1000.0f) {
                charge_overcurrent_protection_ticks = charge_overcurrent_protection_ticks + 1;
                if ((charge_overcurrent_protection_ticks >= 250) ||
                    (protection_debounce_settings[4] <= charge_overcurrent_protection_ticks)) {
                    charge_overcurrent_protection_ticks = 0;
                    voltage_current_protection_flags =
                        voltage_current_protection_flags | BMS_PROTECTION_CHARGE_OVERCURRENT;
                    if (charge_overcurrent_retry_option != 0) {
                        charge_overcurrent_retry_count = 0;
                    } else {
                        charge_overcurrent_retry_count++;
                    }
                    runtime_timer_start(10, 0x960);
                    protection_event_counts[2] = protection_event_counts[2] + 1;
                    eeprom_write_word(0x68, protection_event_counts[2]);
                    return;
                }
            } else {
                charge_overcurrent_protection_ticks = 0;
            }
        }
    }
    return;
}
#undef charge_overcurrent_retry_option
#undef limit_charge_overcurrent
#undef protection_charge_overcurrent

/* Runs charge and discharge overcurrent checks. */
void protection_update_current() {

    protection_update_charge_overcurrent();
    protection_update_discharge_overcurrent();
    return;
}

extern uchar limit_cell_overvoltage[];
#define limit_charge_overtemperature                                                               \
    BMS_LIMIT_FIELD(measurement_limit_settings, charge_overtemperature)
#define protection_charge_overtemperature                                                          \
    BMS_PROTECTION_FIELD(measurement_protection_settings, charge_overtemperature)

void protection_update_charge_overtemperature() {

    if (((int)((uint)protection_inhibit_flags << 0x19) < 0) ||
        (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE)) {
        charge_overtemperature_limit_ticks = 0;
        charge_overtemperature_protection_ticks = 0;
        charge_overtemperature_normal_ticks = 0;
        temperature_limit_flags =
            temperature_limit_flags & BMS_LIMIT_CHARGE_OVERTEMPERATURE_CLEAR_MASK;
        return;
    }
    if (limit_charge_overtemperature < measurement_maximum_temperature) {
        charge_overtemperature_normal_ticks = 0;
        if ((((temperature_limit_flags & BMS_LIMIT_CHARGE_OVERTEMPERATURE) == 0) &&
             (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE)) &&
            (++charge_overtemperature_limit_ticks >= 4)) {
            charge_overtemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_CHARGE_OVERTEMPERATURE;
        }
    } else {
        charge_overtemperature_limit_ticks = 0;
        if (((temperature_limit_flags & BMS_LIMIT_CHARGE_OVERTEMPERATURE) != 0) &&
            (++charge_overtemperature_normal_ticks > 4)) {
            charge_overtemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_CHARGE_OVERTEMPERATURE_CLEAR_MASK;
        }
    }
    if ((temperature_protection_flags & BMS_PROTECTION_CHARGE_OVERTEMPERATURE) != 0 ||
        measurement_current <= 0.0f) {
        charge_overtemperature_protection_ticks = 0;
        return;
    }
    if (protection_charge_overtemperature < measurement_maximum_temperature) {
        if (++charge_overtemperature_protection_ticks >= 4) {
            charge_overtemperature_protection_ticks = 0;
            temperature_protection_flags |= BMS_PROTECTION_CHARGE_OVERTEMPERATURE;
            protection_event_counts[1]++;
            eeprom_write_word(0x6a, protection_event_counts[1]);
            coordination_record_event(6);
        }
    } else {
        charge_overtemperature_protection_ticks = 0;
    }
}

#undef limit_charge_overtemperature
#undef protection_charge_overtemperature

extern uchar limit_cell_overvoltage[];
#define limit_discharge_overtemperature                                                            \
    BMS_LIMIT_FIELD(measurement_limit_settings, discharge_overtemperature)
#define protection_discharge_overtemperature                                                       \
    BMS_PROTECTION_FIELD(measurement_protection_settings, discharge_overtemperature)

void protection_update_discharge_overtemperature() {

    if (((int)((uint)protection_inhibit_flags << 0x19) < 0) ||
        (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE)) {
        discharge_overtemperature_limit_ticks = 0;
        discharge_overtemperature_protection_ticks = 0;
        discharge_overtemperature_normal_ticks = 0;
        temperature_limit_flags =
            temperature_limit_flags & BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_CLEAR_MASK;
        return;
    }
    if (limit_discharge_overtemperature < measurement_maximum_temperature) {
        discharge_overtemperature_normal_ticks = 0;
        if ((-1 < (int)((uint)temperature_limit_flags
                        << BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_SIGN_SHIFT)) &&
            (++discharge_overtemperature_limit_ticks >= 4)) {
            discharge_overtemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_DISCHARGE_OVERTEMPERATURE;
        }
    } else {
        discharge_overtemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags
                   << BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_SIGN_SHIFT) < 0) &&
            (++discharge_overtemperature_normal_ticks > 4)) {
            discharge_overtemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_CLEAR_MASK;
        }
    }
    if (((int)((uint)temperature_protection_flags
               << BMS_PROTECTION_DISCHARGE_OVERTEMPERATURE_SIGN_SHIFT) < 0) ||
        (measurement_current >= 0.0f)) {
        discharge_overtemperature_protection_ticks = 0;
    } else {
        if (protection_discharge_overtemperature < measurement_maximum_temperature) {
            if (++discharge_overtemperature_protection_ticks >= 4) {
                discharge_overtemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_DISCHARGE_OVERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                coordination_record_event(7);
                return;
            }
        } else {
            discharge_overtemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef limit_discharge_overtemperature
#undef protection_discharge_overtemperature

extern uchar limit_cell_overvoltage[];
#define limit_charge_undertemperature                                                              \
    BMS_LIMIT_FIELD(measurement_limit_settings, charge_undertemperature)
#define protection_charge_undertemperature                                                         \
    BMS_PROTECTION_FIELD(measurement_protection_settings, charge_undertemperature)

void protection_update_charge_undertemperature() {

    if (((int)((uint)protection_inhibit_flags << 0x18) < 0) ||
        (measurement_current_mode == BMS_CURRENT_MODE_NEGATIVE)) {
        charge_undertemperature_limit_ticks = 0;
        charge_undertemperature_protection_ticks = 0;
        charge_undertemperature_normal_ticks = 0;
        temperature_limit_flags =
            temperature_limit_flags & BMS_LIMIT_CHARGE_UNDERTEMPERATURE_CLEAR_MASK;
        return;
    }
    if (limit_charge_undertemperature > measurement_minimum_temperature) {
        charge_undertemperature_normal_ticks = 0;
        if (((-1 < (int)((uint)temperature_limit_flags
                         << BMS_LIMIT_CHARGE_UNDERTEMPERATURE_SIGN_SHIFT)) &&
             (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE)) &&
            (++charge_undertemperature_limit_ticks >= 4)) {
            charge_undertemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_CHARGE_UNDERTEMPERATURE;
        }
    } else {
        charge_undertemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags << BMS_LIMIT_CHARGE_UNDERTEMPERATURE_SIGN_SHIFT) <
             0) &&
            (++charge_undertemperature_normal_ticks > 4)) {
            charge_undertemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_CHARGE_UNDERTEMPERATURE_CLEAR_MASK;
        }
    }
    if (((int)((uint)temperature_protection_flags
               << BMS_PROTECTION_CHARGE_UNDERTEMPERATURE_SIGN_SHIFT) < 0) ||
        (measurement_current <= 0.0f)) {
        charge_undertemperature_protection_ticks = 0;
    } else {
        if (protection_charge_undertemperature > measurement_minimum_temperature) {
            if (++charge_undertemperature_protection_ticks >= 4) {
                charge_undertemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_CHARGE_UNDERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                coordination_record_event(8);
                return;
            }
        } else {
            charge_undertemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef limit_charge_undertemperature
#undef protection_charge_undertemperature

extern uchar limit_cell_overvoltage[];
#define limit_discharge_undertemperature                                                           \
    BMS_LIMIT_FIELD(measurement_limit_settings, discharge_undertemperature)
#define protection_discharge_undertemperature                                                      \
    BMS_PROTECTION_FIELD(measurement_protection_settings, discharge_undertemperature)

void protection_update_discharge_undertemperature() {

    if (((int)((uint)protection_inhibit_flags << 0x18) < 0) ||
        (measurement_current_mode == BMS_CURRENT_MODE_POSITIVE)) {
        discharge_undertemperature_limit_ticks = 0;
        discharge_undertemperature_protection_ticks = 0;
        discharge_undertemperature_normal_ticks = 0;
        temperature_limit_flags =
            temperature_limit_flags & BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_CLEAR_MASK;
        return;
    }
    if (limit_discharge_undertemperature > measurement_minimum_temperature) {
        discharge_undertemperature_normal_ticks = 0;
        if ((-1 < (int)((uint)temperature_limit_flags
                        << BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_SIGN_SHIFT)) &&
            (++discharge_undertemperature_limit_ticks >= 4)) {
            discharge_undertemperature_limit_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags | BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE;
        }
    } else {
        discharge_undertemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags
                   << BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_SIGN_SHIFT) < 0) &&
            (++discharge_undertemperature_normal_ticks > 4)) {
            discharge_undertemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_CLEAR_MASK;
        }
    }
    if (((int)((uint)temperature_protection_flags
               << BMS_PROTECTION_DISCHARGE_UNDERTEMPERATURE_SIGN_SHIFT) < 0) ||
        (measurement_current >= 0.0f)) {
        discharge_undertemperature_protection_ticks = 0;
    } else {
        if (protection_discharge_undertemperature > measurement_minimum_temperature) {
            if (++discharge_undertemperature_protection_ticks >= 4) {
                discharge_undertemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_DISCHARGE_UNDERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                coordination_record_event(9);
                return;
            }
        } else {
            discharge_undertemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef limit_discharge_undertemperature
#undef protection_discharge_undertemperature

/* Runs four charge/discharge temperature checks. */
void protection_update_charge_discharge_temperature() {

    protection_update_charge_overtemperature();
    protection_update_discharge_overtemperature();
    protection_update_charge_undertemperature();
    protection_update_discharge_undertemperature();
    return;
}

extern uchar temperature_readings_view[];
extern uchar limit_cell_overvoltage[];
#define limit_mosfet_overtemperature                                                               \
    BMS_LIMIT_FIELD(measurement_limit_settings, mosfet_overtemperature)
#define protection_mosfet_overtemperature                                                          \
    BMS_PROTECTION_FIELD(measurement_protection_settings, mosfet_overtemperature)

void protection_update_mosfet_overtemperature() {

    if ((int)temperature_readings[3] > (int)(uint)limit_mosfet_overtemperature) {
        mosfet_overtemperature_normal_ticks = 0;
        if ((-1 <
             (int)((uint)temperature_limit_flags << BMS_LIMIT_MOSFET_OVERTEMPERATURE_SIGN_SHIFT)) &&
            (++mosfet_overtemperature_limit_ticks >= 4)) {
            mosfet_overtemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_MOSFET_OVERTEMPERATURE;
        }
    } else {
        mosfet_overtemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags << BMS_LIMIT_MOSFET_OVERTEMPERATURE_SIGN_SHIFT) <
             0) &&
            (++mosfet_overtemperature_normal_ticks > 4)) {
            mosfet_overtemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_MOSFET_OVERTEMPERATURE_CLEAR_MASK;
        }
    }
    if ((int)((uint)temperature_protection_flags
              << BMS_PROTECTION_MOSFET_OVERTEMPERATURE_SIGN_SHIFT) < 0) {
        mosfet_overtemperature_protection_ticks = 0;
    } else {
        if ((int)temperature_readings[3] > (int)(uint)protection_mosfet_overtemperature) {
            if (++mosfet_overtemperature_protection_ticks >= 4) {
                mosfet_overtemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_MOSFET_OVERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                return;
            }
        } else {
            mosfet_overtemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef limit_mosfet_overtemperature
#undef protection_mosfet_overtemperature

extern uchar temperature_readings_view[];
#define ambient_temperature_view (*(short *)((uchar *)temperature_readings + 8))
extern uchar limit_cell_overvoltage[];
#define limit_ambient_overtemperature                                                              \
    BMS_LIMIT_FIELD(measurement_limit_settings, ambient_overtemperature)
#define protection_ambient_overtemperature                                                         \
    BMS_PROTECTION_FIELD(measurement_protection_settings, ambient_overtemperature)

void protection_update_ambient_overtemperature() {

    if ((int)ambient_temperature_view > (int)(uint)limit_ambient_overtemperature) {
        ambient_overtemperature_normal_ticks = 0;
        if ((-1 < (int)((uint)temperature_limit_flags
                        << BMS_LIMIT_AMBIENT_OVERTEMPERATURE_SIGN_SHIFT)) &&
            (++ambient_overtemperature_limit_ticks >= 4)) {
            ambient_overtemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_AMBIENT_OVERTEMPERATURE;
        }
    } else {
        ambient_overtemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags << BMS_LIMIT_AMBIENT_OVERTEMPERATURE_SIGN_SHIFT) <
             0) &&
            (++ambient_overtemperature_normal_ticks > 4)) {
            ambient_overtemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_AMBIENT_OVERTEMPERATURE_CLEAR_MASK;
        }
    }
    if ((int)((uint)temperature_protection_flags
              << BMS_PROTECTION_AMBIENT_OVERTEMPERATURE_SIGN_SHIFT) < 0) {
        ambient_overtemperature_protection_ticks = 0;
    } else {
        if ((int)ambient_temperature_view > (int)(uint)protection_ambient_overtemperature) {
            if (++ambient_overtemperature_protection_ticks >= 4) {
                ambient_overtemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_AMBIENT_OVERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                return;
            }
        } else {
            ambient_overtemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef ambient_temperature_view
#undef limit_ambient_overtemperature
#undef protection_ambient_overtemperature

extern uchar temperature_readings_view[];
#define ambient_temperature_view (*(short *)((uchar *)temperature_readings + 8))
extern uchar limit_cell_overvoltage[];
#define limit_ambient_undertemperature                                                             \
    BMS_LIMIT_FIELD(measurement_limit_settings, ambient_undertemperature)
#define protection_ambient_undertemperature                                                        \
    BMS_PROTECTION_FIELD(measurement_protection_settings, ambient_undertemperature)

void protection_update_ambient_undertemperature() {

    if ((int)ambient_temperature_view < (int)(uint)limit_ambient_undertemperature) {
        ambient_undertemperature_normal_ticks = 0;
        if ((-1 < (int)((uint)temperature_limit_flags
                        << BMS_LIMIT_AMBIENT_UNDERTEMPERATURE_SIGN_SHIFT)) &&
            (++ambient_undertemperature_limit_ticks >= 4)) {
            ambient_undertemperature_limit_ticks = 0;
            temperature_limit_flags = temperature_limit_flags | BMS_LIMIT_AMBIENT_UNDERTEMPERATURE;
        }
    } else {
        ambient_undertemperature_limit_ticks = 0;
        if (((int)((uint)temperature_limit_flags << BMS_LIMIT_AMBIENT_UNDERTEMPERATURE_SIGN_SHIFT) <
             0) &&
            (++ambient_undertemperature_normal_ticks > 4)) {
            ambient_undertemperature_normal_ticks = 0;
            temperature_limit_flags =
                temperature_limit_flags & BMS_LIMIT_AMBIENT_UNDERTEMPERATURE_CLEAR_MASK;
        }
    }
    if ((int)((uint)temperature_protection_flags
              << BMS_PROTECTION_AMBIENT_UNDERTEMPERATURE_SIGN_SHIFT) < 0) {
        ambient_undertemperature_protection_ticks = 0;
    } else {
        if ((int)ambient_temperature_view < (int)(uint)protection_ambient_undertemperature) {
            if (++ambient_undertemperature_protection_ticks >= 4) {
                ambient_undertemperature_protection_ticks = 0;
                temperature_protection_flags =
                    temperature_protection_flags | BMS_PROTECTION_AMBIENT_UNDERTEMPERATURE;
                protection_event_counts[1] = protection_event_counts[1] + 1;
                eeprom_write_word(0x6a, protection_event_counts[1]);
                return;
            }
        } else {
            ambient_undertemperature_protection_ticks = 0;
        }
    }
    return;
}
#undef ambient_temperature_view
#undef limit_ambient_undertemperature
#undef protection_ambient_undertemperature

/* Runs ambient over/undertemperature checks. */
void protection_update_ambient_temperature() {

    protection_update_ambient_overtemperature();
    protection_update_ambient_undertemperature();
    return;
}

/* Gates charge/discharge, MOS and ambient checks on sensor status.
   Sensor-status bits gate the groups independently; I keep their original test forms. */
void protection_update_temperature() {

    if (primary_temperature_sensor_faults == '\0') {
        protection_update_charge_discharge_temperature();
    }
    if ((auxiliary_temperature_sensor_faults & 1) == 0) {
        protection_update_mosfet_overtemperature();
    }
    if (-1 < (int)((uint)auxiliary_temperature_sensor_faults << 0x1e)) {
        protection_update_ambient_temperature();
        return;
    }
    return;
}

int abs(int);
/* Compares cell spread and pack voltage against the independent voltage reading.
   The input fault becomes persistent only after twenty failed checks. */
void protection_check_voltage_consistency(void) {
    if (!(hardware_fault_flags & 0x10)) {
        if (measurement_maximum_cell_voltage > measurement_minimum_cell_voltage + 1000 ||
            measurement_minimum_cell_voltage < 500 || measurement_pack_voltage < 10000 ||
            abs(independent_pack_voltage) + 5000 < measurement_pack_voltage ||
            independent_pack_voltage > measurement_pack_voltage + 5000) {
            measurement_input_fault_flags |= 1;
            if (++voltage_consistency_fault_ticks >= 20) {
                voltage_consistency_fault_ticks = 0;
                hardware_fault_flags |= 0x10;
            } else {
                measurement_input_fault_flags &= 0xfe;
            }
        } else {
            voltage_consistency_fault_ticks = 0;
            measurement_input_fault_flags &= 0xfe;
        }
    }
}

/* Debounces current flow while the corresponding output status is disabled. */
void protection_check_current_output_consistency() {
    if (!(power_output_status_flags & 2) && measurement_current > 5000.0f &&
        !(hardware_fault_flags & 1)) {
        if (++positive_current_output_fault_ticks >= 10) {
            positive_current_output_fault_ticks = 0;
            hardware_fault_flags |= 1;
        }
    } else
        positive_current_output_fault_ticks = 0;
    if (((-1 < (int)((uint)power_output_status_flags << 0x1d)) &&
         (measurement_current < -2000.0f)) &&
        (-1 < (int)((uint)hardware_fault_flags << 0x1e))) {
        if (++negative_current_output_fault_ticks >= 10) {
            negative_current_output_fault_ticks = 0;
            hardware_fault_flags = hardware_fault_flags | 2;
        }
        return;
    }
    negative_current_output_fault_ticks = 0;
    return;
}

/* Latches a fault after sustained primary or auxiliary sensor faults. */
void protection_check_temperature_sensor_faults() {

    if (-1 < (int)((uint)hardware_fault_flags << 0x1d)) {
        if ((primary_temperature_sensor_faults | auxiliary_temperature_sensor_faults) == 0) {
            temperature_sensor_fault_ticks = 0;
            return;
        }
        if (++temperature_sensor_fault_ticks >= 10) {
            temperature_sensor_fault_ticks = 0;
            hardware_fault_flags = hardware_fault_flags | 4;
        }
    }
    return;
}

/* Returns the voltage/current protection byte. */
uchar protection_get_voltage_current_flags() {

    return voltage_current_protection_flags;
}

/* Tracks sustained positive current and logs event10.
   The activation and clearing delays differ; I do not assign time units to these counts. */
void protection_track_positive_current(void) {
    if (positive_current_activity_latch == 0) {
        if (measurement_current > 0.0f) {
            if (++positive_current_activity_ticks > 600) {
                positive_current_activity_ticks = 0;
                positive_current_activity_latch = 1;
                coordination_record_event(10);
            }
        } else {
            positive_current_activity_ticks = 0;
        }
    } else if (measurement_current <= 0.0f) {
        if (++positive_current_activity_ticks > 10) {
            positive_current_activity_ticks = 0;
            positive_current_activity_latch = 0;
        }
    } else {
        positive_current_activity_ticks = 0;
    }
}

int resistance_current_history[12] = {0xffff};
ushort resistance_cell_voltage_history[2][12] = {0xffff};
/* at: resistance_current_history 0x200004B8 */

/* Estimates voltage/current range ratios over twelve samples.
   The current and voltage extrema need not be paired samples; this is a range estimate.
   match: Each history initializer sets only its first element to 0xffff; the rest start at zero. */
void measurement_estimate_cell_resistance(void) {
    uchar sample_index;
    int maximum_current;
    int minimum_current;
    ushort maximum_high_cell;
    ushort minimum_high_cell;
    ushort maximum_low_cell;
    ushort minimum_low_cell;
    int current_range;
    ushort high_cell_range;
    ushort low_cell_range;
    int current_amperes;

    sample_index = 0;
    if (primary_soc < 30 || primary_soc > 80 || measurement_minimum_temperature < 2780 ||
        measurement_minimum_temperature > 3130) {
        if (resistance_cell_voltage_history[0][0] != 0xffff) {
            for (sample_index = 0; sample_index < 12; sample_index++) {
                resistance_cell_voltage_history[0][sample_index] = 0xffff;
                resistance_cell_voltage_history[1][sample_index] = 0xffff;
                resistance_current_history[sample_index] = 0xffff;
            }
        }
        return;
    }
    current_amperes = (int)measurement_current / 1000;
    resistance_current_history[resistance_sample_index] = current_amperes;
    resistance_cell_voltage_history[0][resistance_sample_index] = measurement_maximum_cell_voltage;
    resistance_cell_voltage_history[1][resistance_sample_index] = measurement_minimum_cell_voltage;
    if (resistance_sample_index < 11) {
        resistance_sample_index++;
    } else {
        resistance_sample_index = 0;
    }
    if (resistance_cell_voltage_history[0][11] < 0xffff &&
        resistance_cell_voltage_history[1][11] < 0xffff && resistance_current_history[0] < 0xffff) {
        maximum_current = resistance_current_history[0];
        minimum_current = resistance_current_history[0];
        maximum_high_cell = resistance_cell_voltage_history[0][0];
        minimum_high_cell = resistance_cell_voltage_history[0][0];
        maximum_low_cell = resistance_cell_voltage_history[1][0];
        minimum_low_cell = resistance_cell_voltage_history[1][0];
        for (sample_index = 0; sample_index < 12; sample_index++) {
            if (resistance_current_history[sample_index] < minimum_current) {
                minimum_current = resistance_current_history[sample_index];
            }
            if (resistance_current_history[sample_index] > maximum_current) {
                maximum_current = resistance_current_history[sample_index];
            }
            if (resistance_cell_voltage_history[0][sample_index] > maximum_high_cell) {
                maximum_high_cell = resistance_cell_voltage_history[0][sample_index];
            }
            if (resistance_cell_voltage_history[0][sample_index] < minimum_high_cell) {
                minimum_high_cell = resistance_cell_voltage_history[0][sample_index];
            }
            if (resistance_cell_voltage_history[1][sample_index] > maximum_low_cell) {
                maximum_low_cell = resistance_cell_voltage_history[1][sample_index];
            }
            if (resistance_cell_voltage_history[1][sample_index] < minimum_low_cell) {
                minimum_low_cell = resistance_cell_voltage_history[1][sample_index];
            }
        }
        current_range = maximum_current - minimum_current;
        high_cell_range = maximum_high_cell - minimum_high_cell;
        low_cell_range = maximum_low_cell - minimum_low_cell;
        if (abs(current_range) >= 5) {
            if (++resistance_current_range_ticks < 10) {
                return;
            }
        } else {
            resistance_current_range_ticks = 0;
            return;
        }
        resistance_current_range_ticks = 0;
        for (sample_index = 0; sample_index < 12; sample_index++) {
            resistance_cell_voltage_history[0][sample_index] = 0xffff;
            resistance_cell_voltage_history[1][sample_index] = 0xffff;
            resistance_current_history[sample_index] = 0xffff;
        }
        resistance_estimates[0] = high_cell_range * 1000 / abs(current_range);
        resistance_estimates[1] = low_cell_range * 1000 / abs(current_range);
        if ((resistance_estimates[0] > 2000 || resistance_estimates[1] > 2000) &&
            !(extended_limit_flags & 0x20)) {
            resistance_normal_ticks = 0;
            if (++resistance_high_ticks >= 2) {
                resistance_high_ticks = 0;
                extended_limit_flags |= 0x20;
            }
        } else if (resistance_estimates[0] < 1000 && resistance_estimates[1] < 1000) {
            resistance_high_ticks = 0;
            if (++resistance_normal_ticks >= 10) {
                resistance_normal_ticks = 0;
                extended_limit_flags &= 0xdf;
            }
        } else {
            resistance_high_ticks = 0;
            resistance_normal_ticks = 0;
        }
    }
}

extern uchar device_registers[];
void snapshot_publish(void);

/* Runs staggered sensing/protection checks then publishes snapshot.
   match: I retain the two unused halfword reads and the original call order. */
void protection_cycle(void) {
    ushort unused_27A;
    ushort unused_2C2;

    if (device_registers[57] == 0) {
        return;
    }
    runtime_timer_start(7, 4);
    unused_27A = protection_cycle_reserved_word;
    unused_2C2 = protection_cycle_aux_reserved_word;

    protection_cycle_phase++;
    if (protection_cycle_phase == 1) {
        measurement_update_voltage_and_temperature();
    } else if (protection_cycle_phase == 2) {
        measurement_update_auxiliary_temperatures();
        protection_check_temperature_sensor_faults();
        protection_update_temperature();
    } else if (protection_cycle_phase == 3) {
        protection_check_current_output_consistency();
    } else {
        protection_cycle_phase = 0;
    }
    capacity_update_cycle_count();
    capacity_update_soc();
    protection_check_voltage_consistency();
    protection_update_voltage();
    protection_update_current();
    if ((protection_cycle_phase & 1) == 0) {
        measurement_estimate_cell_resistance();
    }
    protection_track_positive_current();
    control_update_current_mode();
    snapshot_publish();
    if (battery_heating.enabled != 0) {
        control_update_battery_heating();
    } else {
        battery_heating.negative_current_stage = 0;
        battery_heating.positive_current_stage = 0;
    }
}
