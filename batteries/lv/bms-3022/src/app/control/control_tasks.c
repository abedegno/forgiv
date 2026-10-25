#include "../../include/bms_heating.h"
/* at: battery_heating 0x20000196 */
/* at: protection_recover_voltage_limits 0x08022B2C, protection_recover_voltage_current 0x08022D08, protection_recover_temperature 0x0802302C, control_clear_current_direction_latches 0x080231AA, protection_recover_fault_flags 0x08023206, control_clear_negative_current_latches 0x08023350, control_update_power_outputs 0x080233F4, control_enter_positive_current_mode 0x08023708, control_enter_negative_current_mode 0x08023728, control_enter_idle_mode 0x0802375E, control_update_current_mode 0x0802378A, control_update_cold_temperature_flag 0x0802389E, control_apply_current_mode 0x08023B26, control_check_shutdown 0x08023B3E, control_task 0x08023C08 */
/* at: pc8_output_state 0x2000005A, pb3_output_state 0x2000005B, pd2_output_state 0x2000005C, cold_temperature_latch 0x2000005D, charge_phase_ticks 0x2000005E, charge_phase_toggle 0x20000098, shutdown_request_flags 0x2000009D, control_current_mode 0x2000009F, shutdown_in_progress 0x200000B5, control_activity_count 0x200000C2, control_activity_ticks 0x200000C4, applied_current_mode 0x200000C6, afe_balance_mask 0x200000CE, afe_calibration_ready 0x200000D4, settings_initialized 0x200000D5, output_enable_delay 0x200000D6, control_indication_mode 0x200000E7, measurement_ready 0x200000F3, cell_voltage_step_detected_unsigned 0x200000F8, measurement_maximum_cell_voltage 0x20000110, measurement_minimum_cell_voltage 0x20000112, measurement_pack_voltage 0x20000114, maximum_reference_cell_voltage 0x2000011C, minimum_reference_cell_voltage 0x2000011E, independent_pack_voltage 0x20000120, measurement_maximum_temperature 0x2000012E, measurement_minimum_temperature 0x20000130, measurement_current 0x2000014C, measurement_cell_count 0x20000166, remaining_capacity 0x20000170, primary_soc 0x20000188, recovery_soc_reference 0x2000018A, event_log_index 0x2000018C, primary_temperature_sensor_faults 0x2000021D, auxiliary_temperature_sensor_faults 0x2000021E, charge_overcurrent_retry_count 0x2000021F, discharge_overcurrent_retry_count 0x20000220, capacity_full_voltage_seen_plain 0x20000221, voltage_current_protection_flags 0x20000279, temperature_protection_flags 0x2000027A, power_output_status_flags 0x2000027B, hardware_fault_flags 0x2000027C, capacity_latch_flags 0x2000027E, voltage_current_limit_flags 0x2000027F, temperature_limit_flags 0x20000280, extended_limit_flags 0x20000281, current_direction_latch_flags 0x20000519, coordination_packet_valid 0x20000533, g_2000054A 0x20000556, os_task_dly_idx 0x2000061E, device_registers 0x20000FB4, sorted_cell_voltages 0x200010EC, temperature_readings 0x200011B6, control_limit_settings 0x2000153A, control_recovery_thresholds 0x20001554, control_protection_thresholds 0x2000156A, control_scheduler_words 0x200074A4 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow the control tasks and their voltage checks here; they use the shared thresholds and drive hardware outputs.
   Entry examples: protection_recover_voltage_limits, protection_recover_voltage_current, protection_recover_temperature.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/bms_control.h"
#include "../../include/bms_measurements.h"
#include "../../include/bms_protection.h"
#include "../../include/bms_protocol.h"
#include "../../include/stdperiph_types.h"
#include <stdlib.h>
/* at: cell_overvoltage_limit_clear_ticks 0x20000556, cell_undervoltage_limit_clear_ticks 0x20000557, pack_overvoltage_limit_clear_ticks 0x20000558, pack_undervoltage_limit_clear_ticks 0x20000559, pack_undervoltage_protection_clear_ticks 0x2000055A, charge_overcurrent_clear_ticks 0x2000055B, discharge_overcurrent_clear_ticks 0x2000055C, current_latch_clear_ticks 0x2000055D, cell_overvoltage_protection_clear_ticks 0x2000055E, cell_undervoltage_protection_clear_ticks 0x2000055F, pack_overvoltage_protection_clear_ticks 0x20000560, cell_undervoltage_shutdown_pending 0x20000561, discharge_undertemperature_clear_ticks 0x20000562, mosfet_overtemperature_clear_ticks 0x20000563, ambient_overtemperature_clear_ticks 0x20000564, ambient_undertemperature_clear_ticks 0x20000565, charge_overtemperature_clear_ticks 0x20000566, discharge_overtemperature_clear_ticks 0x20000567, charge_undertemperature_clear_ticks 0x20000568, negative_current_latch_clear_ticks 0x20000569, positive_current_latch_clear_ticks 0x2000056A, temperature_sensor_fault_clear_ticks 0x2000056B, voltage_consistency_fault_clear_ticks 0x2000056C, transient_fault_clear_ticks 0x2000056D, positive_current_output_fault_clear_ticks 0x2000056E, negative_current_output_fault_clear_ticks 0x2000056F, capacity_latch_clear_ticks 0x20000570, indication_blink_ticks 0x20000571, charge_cycle_count 0x20000572, positive_current_mode_ticks 0x20000573, negative_current_mode_ticks 0x20000574, idle_current_mode_ticks 0x20000575, control_task_wake_flags 0x20000577, control_activity_pending 0x20000578, charge_cycle_period 0x2000057A, control_startup_ticks 0x20000588, control_slow_task_divider 0x2000058A, coordination_startup_ticks 0x2000058C, coordination_ready 0x2000058E, protection_run_ticks 0x20000590, protection_run_interval 0x20000592 */
uchar cell_overvoltage_limit_clear_ticks = 0;
uchar cell_undervoltage_limit_clear_ticks = 0;
uchar pack_overvoltage_limit_clear_ticks = 0;
uchar pack_undervoltage_limit_clear_ticks = 0;
uchar pack_undervoltage_protection_clear_ticks = 0;
uchar charge_overcurrent_clear_ticks = 0;
uchar discharge_overcurrent_clear_ticks = 0;
uchar current_latch_clear_ticks = 0;
uchar cell_overvoltage_protection_clear_ticks = 0;
uchar cell_undervoltage_protection_clear_ticks = 0;
uchar pack_overvoltage_protection_clear_ticks = 0;
uchar cell_undervoltage_shutdown_pending = 0;
uchar discharge_undertemperature_clear_ticks = 0;
uchar mosfet_overtemperature_clear_ticks = 0;
uchar ambient_overtemperature_clear_ticks = 0;
uchar ambient_undertemperature_clear_ticks = 0;
uchar charge_overtemperature_clear_ticks = 0;
uchar discharge_overtemperature_clear_ticks = 0;
uchar charge_undertemperature_clear_ticks = 0;
uchar negative_current_latch_clear_ticks = 0;
uchar positive_current_latch_clear_ticks = 0;
uchar temperature_sensor_fault_clear_ticks = 0;
uchar voltage_consistency_fault_clear_ticks = 0;
uchar transient_fault_clear_ticks = 0;
uchar positive_current_output_fault_clear_ticks = 0;
uchar negative_current_output_fault_clear_ticks = 0;
uchar capacity_latch_clear_ticks = 0;
uchar indication_blink_ticks = 0;
uchar charge_cycle_count = 0;
uchar positive_current_mode_ticks = 0;
uchar negative_current_mode_ticks = 0;
uchar idle_current_mode_ticks = 0;
/* Added SOC sample occupies the former alignment byte; event flags follow it. */
/* at: heating_soc_reference 0x20000576 */
uchar heating_soc_reference = 100;
uchar control_task_wake_flags = 0;
ushort control_activity_pending = 0;
ushort charge_cycle_period = 0;
/* Native 3022 control counters, between the charge period and startup counter. */
/* at: heating_cold_temperature_ticks 0x2000057C, heating_negative_current_run_ticks 0x2000057E */
/* at: heating_negative_current_wait_ticks 0x20000580, heating_negative_current_retries 0x20000582 */
/* at: heating_positive_current_retries 0x20000584, heating_positive_current_wait_ticks 0x20000586 */
ushort heating_cold_temperature_ticks = 0;
ushort heating_negative_current_run_ticks = 0;
ushort heating_negative_current_wait_ticks = 0;
ushort heating_negative_current_retries = 0;
ushort heating_positive_current_retries = 0;
/* match: Volatile retains the native timeout reset after a positive-current transition. */
volatile ushort heating_positive_current_wait_ticks = 0;
ushort control_startup_ticks = 0;
ushort control_slow_task_divider = 0;
ushort coordination_startup_ticks = 0;
ushort coordination_ready = 0;
ushort protection_run_ticks = 0;
ushort protection_run_interval = 0;
extern volatile uchar extended_limit_flags;
extern volatile uchar temperature_limit_flags;
extern volatile uchar voltage_current_limit_flags;
extern volatile uchar capacity_latch_flags;
extern volatile uchar hardware_fault_flags;
extern volatile uchar power_output_status_flags;
extern volatile uchar temperature_protection_flags;
extern volatile uchar voltage_current_protection_flags;
extern volatile uchar current_direction_latch_flags;
extern uchar device_registers[];
extern uchar pc8_output_state;
extern uchar pb3_output_state;
extern uchar pd2_output_state;
extern uchar cold_temperature_latch;
extern ushort charge_phase_ticks;
extern uchar charge_phase_toggle;
extern uchar shutdown_request_flags;
extern uchar control_current_mode;
extern uchar shutdown_in_progress;
extern ushort control_activity_count;
extern ushort control_activity_ticks;
extern uchar applied_current_mode;
extern ushort afe_balance_mask;
extern uchar afe_calibration_ready;
extern uchar settings_initialized;
extern ushort output_enable_delay;
extern uchar control_indication_mode;
extern uchar measurement_ready;
extern uchar cell_voltage_step_detected_unsigned;
extern ushort measurement_maximum_cell_voltage;
extern ushort measurement_minimum_cell_voltage;
extern ushort measurement_pack_voltage;
extern ushort maximum_reference_cell_voltage;
extern ushort minimum_reference_cell_voltage;
extern int independent_pack_voltage;
extern ushort measurement_maximum_temperature;
extern ushort measurement_minimum_temperature;
extern float measurement_current;
extern uchar measurement_cell_count;
extern float remaining_capacity;
extern uchar primary_soc;
extern uchar recovery_soc_reference;
extern ushort event_log_index;
extern uchar primary_temperature_sensor_faults;
extern uchar auxiliary_temperature_sensor_faults;
extern uchar charge_overcurrent_retry_count;
extern uchar discharge_overcurrent_retry_count;
extern uchar capacity_full_voltage_seen_plain;
extern uchar coordination_packet_valid;
extern ushort os_task_dly_idx;
extern ushort sorted_cell_voltages[];
extern short temperature_readings[];
#include "../../include/bms_thresholds.h"
extern limits_t control_limit_settings;
extern ushort control_recovery_thresholds[];
extern ushort control_protection_thresholds[];
extern ushort control_scheduler_words[];
void runtime_timer_start(int, uint);
void runtime_timer_stop(int);
void GPIO_ResetBits(void *, ushort);
void GPIO_SetBits(void *, ushort);
void board_set_control_output(int, int);
void os_task_delay(int, ushort, ushort);
void snapshot_publish(void);
uint eeprom_write_word(short, uint);
void eeprom_log_write_record(ushort *, int, int);
void board_set_pb2(int);
void board_set_pa8(int);
void board_set_pb9(int);
void board_enter_standby(void);
void board_initialize_pa0_interrupt(void);
uchar afe_write_register(short, ushort);
uint os_flags_pend(ushort);
void settings_startup(void);
void sub_0802126C(void);
void coordination_poll(void);

/* Clears four voltage limit bits after hysteresis and fifty qualifying checks. */
void protection_recover_voltage_limits(void) {
    uint i;

    if (voltage_current_limit_flags != 0) {
        i = 0;
        do {
            switch (1 << i & (uint)voltage_current_limit_flags) {
            case 1:
                if ((control_limit_settings.cell_overvoltage >= measurement_maximum_cell_voltage ||
                     sorted_cell_voltages[1] < control_limit_settings.cell_overvoltage) &&
                    (measurement_maximum_cell_voltage <= 3700 ||
                     cell_voltage_step_detected_unsigned != 0) &&
                    maximum_reference_cell_voltage <= 3750) {
                    if (control_limit_settings.cell_overvoltage - 100 >
                            measurement_maximum_cell_voltage &&
                        sorted_cell_voltages[1] < control_limit_settings.cell_overvoltage - 100 &&
                        control_limit_settings.cell_overvoltage - 100 >
                            maximum_reference_cell_voltage) {
                        if (++cell_overvoltage_limit_clear_ticks >= 50) {
                            cell_overvoltage_limit_clear_ticks = 0;
                            voltage_current_limit_flags &= 0xfe;
                        }
                    } else {
                        cell_overvoltage_limit_clear_ticks = 0;
                    }
                }
                break;
            case 2:
                if ((control_limit_settings.cell_undervoltage <= measurement_minimum_cell_voltage ||
                     sorted_cell_voltages[measurement_cell_count - 2] >
                         control_limit_settings.cell_undervoltage) &&
                    (measurement_minimum_cell_voltage > 0x95f ||
                     cell_voltage_step_detected_unsigned != 0) &&
                    minimum_reference_cell_voltage >= 0x92e) {
                    if (control_limit_settings.cell_undervoltage + 100U <
                            measurement_minimum_cell_voltage &&
                        sorted_cell_voltages[measurement_cell_count - 2] >
                            control_limit_settings.cell_undervoltage + 100U &&
                        control_limit_settings.cell_undervoltage + 100U <
                            minimum_reference_cell_voltage) {
                        if (++cell_undervoltage_limit_clear_ticks >= 50) {
                            cell_undervoltage_limit_clear_ticks = 0;
                            voltage_current_limit_flags &= 0xfd;
                        }
                    } else {
                        cell_undervoltage_limit_clear_ticks = 0;
                    }
                }
                break;
            case 4:
                if (control_limit_settings.pack_overvoltage - 1600 > measurement_pack_voltage) {
                    if (++pack_overvoltage_limit_clear_ticks >= 50) {
                        pack_overvoltage_limit_clear_ticks = 0;
                        voltage_current_limit_flags &= 0xfb;
                    }
                } else {
                    pack_overvoltage_limit_clear_ticks = 0;
                }
                break;
            case 8:
                if (control_limit_settings.pack_undervoltage + 1600U < measurement_pack_voltage) {
                    if (++pack_undervoltage_limit_clear_ticks >= 50) {
                        pack_undervoltage_limit_clear_ticks = 0;
                        voltage_current_limit_flags &= 0xf7;
                    }
                } else {
                    pack_undervoltage_limit_clear_ticks = 0;
                }
                break;
            }
            i = i + 1 & 0xff;
        } while (i < 8);
    }
}

/* Recovers voltage/current protection bits with per-condition counters and timers. */
void protection_recover_voltage_current(void) {
    uchar i;

    if (voltage_current_protection_flags != 0) {
        for (i = 0; i < 8; i++) {
            switch ((1 << i) & voltage_current_protection_flags) {
            case 1:
                if (((measurement_maximum_cell_voltage <
                          BMS_RECOVERY_FIELD(control_recovery_thresholds, cell_overvoltage) ||
                      measurement_maximum_cell_voltage <
                          BMS_PROTECTION_FIELD(control_protection_thresholds, cell_overvoltage) -
                              100) &&
                     BMS_PROTECTION_FIELD(control_protection_thresholds, cell_overvoltage) - 100 >
                         maximum_reference_cell_voltage) ||
                    (primary_soc < recovery_soc_reference &&
                     measurement_maximum_cell_voltage <
                         BMS_PROTECTION_FIELD(control_protection_thresholds, cell_overvoltage) &&
                     capacity_full_voltage_seen_plain != 0)) {
                    if (++cell_overvoltage_protection_clear_ticks >= 4) {
                        cell_overvoltage_protection_clear_ticks = 0;
                        voltage_current_protection_flags &= 0xfe;
                    }
                } else {
                    cell_overvoltage_protection_clear_ticks = 0;
                }
                break;
            case 2:
                if (device_registers[105] != 0) {
                    cell_undervoltage_shutdown_pending = 1;
                    runtime_timer_stop(13);
                    runtime_timer_stop(26);
                    applied_current_mode = BMS_CURRENT_MODE_IDLE;
                }
                if (measurement_minimum_cell_voltage <= 2200 &&
                    cell_undervoltage_shutdown_pending == 1) {
                    cell_undervoltage_shutdown_pending = 0;
                    shutdown_request_flags |= 2;
                }
                if ((BMS_RECOVERY_FIELD(control_recovery_thresholds, cell_undervoltage) <
                         measurement_minimum_cell_voltage &&
                     BMS_RECOVERY_FIELD(control_recovery_thresholds, cell_undervoltage) <
                         minimum_reference_cell_voltage) ||
                    measurement_current > 3000.0f) {
                    cell_undervoltage_shutdown_pending = 0;
                    if (++cell_undervoltage_protection_clear_ticks >= 4) {
                        cell_undervoltage_protection_clear_ticks = 0;
                        voltage_current_protection_flags &= 0xfd;
                        output_enable_delay = 0;
                        if ((voltage_current_protection_flags & 8) == 0) {
                            shutdown_request_flags &= 0xfd;
                        }
                    }
                } else {
                    cell_undervoltage_protection_clear_ticks = 0;
                }
                break;
            case 4:
                if (measurement_pack_voltage <
                        BMS_RECOVERY_FIELD(control_recovery_thresholds, pack_overvoltage) ||
                    measurement_pack_voltage <
                        BMS_PROTECTION_FIELD(control_protection_thresholds, pack_overvoltage) -
                            1000 ||
                    (primary_soc < recovery_soc_reference &&
                     measurement_pack_voltage <
                         BMS_PROTECTION_FIELD(control_protection_thresholds, pack_overvoltage) &&
                     capacity_full_voltage_seen_plain != 0)) {
                    if (++pack_overvoltage_protection_clear_ticks >= 4) {
                        pack_overvoltage_protection_clear_ticks = 0;
                        voltage_current_protection_flags &= 0xfb;
                    }
                } else {
                    pack_overvoltage_protection_clear_ticks = 0;
                }
                break;
            case 8:
                if (device_registers[105] != 0) {
                    shutdown_request_flags |= 2;
                    runtime_timer_stop(13);
                }
                if (BMS_RECOVERY_FIELD(control_recovery_thresholds, pack_undervoltage) <
                        measurement_pack_voltage ||
                    measurement_current > 3000.0f) {
                    if (++pack_undervoltage_protection_clear_ticks >= 4) {
                        pack_undervoltage_protection_clear_ticks = 0;
                        output_enable_delay = 0;
                        voltage_current_protection_flags &= 0xf7;
                        if ((voltage_current_protection_flags & 2) == 0) {
                            shutdown_request_flags &= 0xfd;
                        }
                    }
                } else {
                    pack_undervoltage_protection_clear_ticks = 0;
                }
                break;
            case 16:
                if (!((device_registers[81] == 0 || charge_overcurrent_retry_count >= 10) &&
                      measurement_current >= -3000.0f &&
                      (current_direction_latch_flags & 0x80) == 0)) {
                    if (++charge_overcurrent_clear_ticks >= 4) {
                        charge_overcurrent_clear_ticks = 0;
                        voltage_current_protection_flags &= 0xef;
                        runtime_timer_start(10, 1200);
                    }
                } else {
                    charge_overcurrent_clear_ticks = 0;
                }
                break;
            case 32:
                if (!((device_registers[89] == 0 || discharge_overcurrent_retry_count >= 10) &&
                      measurement_current <= 3000.0f &&
                      (current_direction_latch_flags & 0x80) == 0)) {
                    if (++discharge_overcurrent_clear_ticks >= 4) {
                        discharge_overcurrent_clear_ticks = 0;
                        voltage_current_protection_flags &= 0xdf;
                        runtime_timer_start(11, 1200);
                    }
                } else {
                    discharge_overcurrent_clear_ticks = 0;
                }
                break;
            case 64:
                break;
            case 128:
                if (!(device_registers[113] == 0 && measurement_current >= -3000.0f)) {
                    if (++current_latch_clear_ticks >= 4) {
                        current_latch_clear_ticks = 0;
                        voltage_current_protection_flags &= 0x7f;
                        runtime_timer_stop(14);
                    }
                } else {
                    current_latch_clear_ticks = 0;
                }
                break;
            }
        }
    }
}

/* Recovers seven temperature protection bits with four qualifying checks. */
void protection_recover_temperature(void) {
    uint bit_index;
    uint selected_flag;

    if (temperature_protection_flags != 0) {
        bit_index = 0;
        {
            do {
                selected_flag = 1 << bit_index & (uint)temperature_protection_flags;
                switch (selected_flag) {
                case 1: {
                    if ((measurement_maximum_temperature <
                         BMS_RECOVERY_FIELD(control_recovery_thresholds, charge_overtemperature)) ||
                        (measurement_current < -3000.0f)) {
                        charge_overtemperature_clear_ticks = charge_overtemperature_clear_ticks + 1;
                        if (charge_overtemperature_clear_ticks >= 4) {
                            charge_overtemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xfe;
                        }
                    } else {
                        charge_overtemperature_clear_ticks = 0;
                    }
                } break;
                case 2: {
                    if ((measurement_maximum_temperature <
                         BMS_RECOVERY_FIELD(control_recovery_thresholds,
                                            discharge_overtemperature)) ||
                        (measurement_current > 3000.0f)) {
                        discharge_overtemperature_clear_ticks =
                            discharge_overtemperature_clear_ticks + 1;
                        if (discharge_overtemperature_clear_ticks >= 4) {
                            discharge_overtemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xfd;
                        }
                    } else {
                        discharge_overtemperature_clear_ticks = 0;
                    }
                } break;
                case 4: {
                    if ((BMS_RECOVERY_FIELD(control_recovery_thresholds, charge_undertemperature) <
                         measurement_minimum_temperature) ||
                        (measurement_current < -3000.0f)) {
                        charge_undertemperature_clear_ticks =
                            charge_undertemperature_clear_ticks + 1;
                        if (charge_undertemperature_clear_ticks >= 4) {
                            charge_undertemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xfb;
                        }
                    } else {
                        charge_undertemperature_clear_ticks = 0;
                    }
                } break;
                case 8: {
                    if ((BMS_RECOVERY_FIELD(control_recovery_thresholds,
                                            discharge_undertemperature) <
                         measurement_minimum_temperature) ||
                        (measurement_current > 3000.0f)) {
                        discharge_undertemperature_clear_ticks =
                            discharge_undertemperature_clear_ticks + 1;
                        if (discharge_undertemperature_clear_ticks >= 4) {
                            discharge_undertemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xf7;
                        }
                    } else {
                        discharge_undertemperature_clear_ticks = 0;
                    }
                } break;
                case 16: {
                    if ((int)temperature_readings[3] <
                        (int)(uint)BMS_RECOVERY_FIELD(control_recovery_thresholds,
                                                      mosfet_overtemperature)) {
                        mosfet_overtemperature_clear_ticks = mosfet_overtemperature_clear_ticks + 1;
                        if (mosfet_overtemperature_clear_ticks >= 4) {
                            mosfet_overtemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xef;
                        }
                    } else {
                        mosfet_overtemperature_clear_ticks = 0;
                    }
                } break;
                case 32: {
                    if ((int)temperature_readings[4] <
                        (int)(uint)BMS_RECOVERY_FIELD(control_recovery_thresholds,
                                                      ambient_overtemperature)) {
                        ambient_overtemperature_clear_ticks =
                            ambient_overtemperature_clear_ticks + 1;
                        if (ambient_overtemperature_clear_ticks >= 4) {
                            ambient_overtemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xdf;
                        }
                    } else {
                        ambient_overtemperature_clear_ticks = 0;
                    }
                } break;
                case 64: {
                    if ((int)temperature_readings[4] >
                        (int)(uint)BMS_RECOVERY_FIELD(control_recovery_thresholds,
                                                      ambient_undertemperature)) {
                        ambient_undertemperature_clear_ticks =
                            ambient_undertemperature_clear_ticks + 1;
                        if (ambient_undertemperature_clear_ticks >= 4) {
                            ambient_undertemperature_clear_ticks = 0;
                            temperature_protection_flags = temperature_protection_flags & 0xbf;
                        }
                    } else {
                        ambient_undertemperature_clear_ticks = 0;
                    }
                } break;
                default:
                    break;
                }
                bit_index = bit_index + 1 & 0xff;
            } while (bit_index < 8);
        }
    }
    return;
}

/* Clears direction latch bits after opposite-sign current. */
void control_clear_current_direction_latches(void) {
    if (current_direction_latch_flags & 2) {
        if (measurement_current < -3000.0f) {
            if (++negative_current_latch_clear_ticks >= 4) {
                negative_current_latch_clear_ticks = 0;
                current_direction_latch_flags &= 0xfd;
            }
        } else
            negative_current_latch_clear_ticks = 0;
    }
    if (current_direction_latch_flags & 4) {
        if (measurement_current > 3000.0f) {
            if (++positive_current_latch_clear_ticks >= 4) {
                positive_current_latch_clear_ticks = 0;
                current_direction_latch_flags &= 0xfb;
            }
        } else
            positive_current_latch_clear_ticks = 0;
    }
}

/* Recovers current/output, sensor and voltage-consistency fault bits. */
void protection_recover_fault_flags(void) {
    uchar i;
    if (hardware_fault_flags != 0) {
        for (i = 0; i < 8; i++) {
            switch ((1 << i) & hardware_fault_flags) {
            case 1:
                if ((power_output_status_flags & 2) != 0 || measurement_current != 0) {
                    positive_current_output_fault_clear_ticks = 0;
                } else if (++positive_current_output_fault_clear_ticks >= 4) {
                    positive_current_output_fault_clear_ticks = 0;
                    hardware_fault_flags &= 0xfe;
                }
                break;
            case 2:
                if ((power_output_status_flags & 4) != 0 || measurement_current != 0) {
                    negative_current_output_fault_clear_ticks = 0;
                } else if (++negative_current_output_fault_clear_ticks >= 4) {
                    negative_current_output_fault_clear_ticks = 0;
                    hardware_fault_flags &= 0xfd;
                }
                break;
            case 4:
                if ((primary_temperature_sensor_faults | auxiliary_temperature_sensor_faults) !=
                    0) {
                    temperature_sensor_fault_clear_ticks = 0;
                } else if (++temperature_sensor_fault_clear_ticks >= 4) {
                    temperature_sensor_fault_clear_ticks = 0;
                    hardware_fault_flags &= 0xfb;
                }
                break;
            case 16:
                if (measurement_maximum_cell_voltage < measurement_minimum_cell_voltage + 500 &&
                    measurement_minimum_cell_voltage > 1000 && measurement_pack_voltage > 15000 &&
                    abs(independent_pack_voltage) + 5000 > measurement_pack_voltage &&
                    independent_pack_voltage < measurement_pack_voltage + 5000) {
                    if (++voltage_consistency_fault_clear_ticks >= 4) {
                        voltage_consistency_fault_clear_ticks = 0;
                        hardware_fault_flags &= 0xef;
                    }
                } else {
                    voltage_consistency_fault_clear_ticks = 0;
                }
                break;
            case 32:
                if (++transient_fault_clear_ticks >= 4) {
                    transient_fault_clear_ticks = 0;
                    hardware_fault_flags &= 0xdf;
                }
                break;
            case 64:
                break;
            }
        }
    }
}

/* Clears full-state latch bits after sustained negative current. */
void control_clear_negative_current_latches(void) {
    if ((capacity_latch_flags & 1) || (capacity_latch_flags & 2)) {
        if (measurement_current < -250.0f) {
            capacity_latch_clear_ticks++;
            if (capacity_latch_clear_ticks >= 4) {
                capacity_latch_clear_ticks = 0;
                capacity_latch_flags &= 0xfe;
                capacity_latch_flags &= 0xfd;
                capacity_latch_flags &= 0xef;
            }
        } else
            capacity_latch_clear_ticks = 0;
    }
}

#define gpio_b_byte_view (*(volatile uchar *)0x40010C00)
#define gpio_c_byte_view (*(volatile uchar *)0x40011000)
#define gpio_d_byte_view (*(volatile uchar *)0x40011400)

static __forceinline void advance_charge_cycle(void) {
    pd2_output_state = 1;
    power_output_status_flags = power_output_status_flags | 4;
    if (charge_phase_toggle == 0) {
        GPIO_SetBits(&gpio_c_byte_view, 0x100);
        pc8_output_state = 1;
    } else {
        GPIO_ResetBits(&gpio_c_byte_view, 0x100);
        pc8_output_state = 0;
    }
    if (charge_cycle_count >= 100) {
        charge_phase_toggle = '\x01';
        charge_cycle_period = 0x100;
    }
    charge_phase_ticks = charge_cycle_period;
}

static __forceinline void enable_charge_phase(void) {
    GPIO_ResetBits(&gpio_d_byte_view, 4);
    pd2_output_state = 0;
    power_output_status_flags = power_output_status_flags & 0xfb;
    GPIO_SetBits(&gpio_c_byte_view, 0x100);
    pc8_output_state = true;
    charge_phase_ticks = charge_phase_ticks + 1;
    charge_phase_toggle = '\0';
}

static __forceinline void disable_charge_phase(void) {
    if (output_enable_delay > 10) {
        output_enable_delay = 100;
    }
    power_output_status_flags = power_output_status_flags & 0xfb;
    GPIO_ResetBits(&gpio_d_byte_view, 4);
    pd2_output_state = 0;
    GPIO_ResetBits(&gpio_c_byte_view, 0x100);
    pc8_output_state = false;
    charge_phase_ticks = 11;
    capacity_latch_flags = capacity_latch_flags | 8;
}

static __forceinline void update_discharge_control(ushort first_value) {
    if ((((first_value & 0xd0) == 0) && ((temperature_protection_flags & 0x75) == 0)) &&
        ((((hardware_fault_flags & 0x7c) == 0 &&
           (((-1 < (int)((uint)current_direction_latch_flags << 0x1e) &&
              (-1 < (int)((uint)capacity_latch_flags << 0x1b))) &&
             (-1 < (int)((uint)capacity_latch_flags << 0x1e))))) &&
          ((voltage_current_protection_flags & 5) == 0)))) {
        power_output_status_flags = power_output_status_flags | 2;
        GPIO_SetBits(&gpio_b_byte_view, 8);
        pb3_output_state = 1;
    } else {
        power_output_status_flags = power_output_status_flags & 0xfc;
        capacity_latch_flags = capacity_latch_flags | 0x10;
        GPIO_ResetBits(&gpio_b_byte_view, 8);
        pb3_output_state = 0;
    }
}

/* Runs recovery, drives charge/discharge outputs and updates indication pins.
   match: I retain the initialized address comparison and the inline output helper boundaries. */
void control_update_power_outputs(void) {

    if (device_registers[65] != '\0') {
        runtime_timer_start(8, 0x14);
        protection_recover_voltage_current();

        protection_recover_temperature();
        control_clear_current_direction_latches();
        protection_recover_fault_flags();
        control_clear_negative_current_latches();
        protection_recover_voltage_limits();
    }
    if (((((voltage_current_protection_flags & 0x6f) == 0) &&
          ((temperature_protection_flags & 0x7a) == 0)) &&
         ((hardware_fault_flags & 0x7c) == 0)) &&
        (((-1 < (int)((uint)current_direction_latch_flags << 0x1d) &&
           (-1 < (int)((uint)capacity_latch_flags << 0x1c))) &&
          ((-1 < (int)((uint)extended_limit_flags << 0x1b) && (10 < output_enable_delay)))))) {
        if (charge_cycle_count == 0) {
            charge_cycle_period = 0x100;
        } else if (charge_cycle_count < 100) {
            charge_cycle_period = 4;
        } else {
            charge_cycle_count = 0;
            charge_cycle_period = 0x100;
        }
        if ((int)(uint)charge_phase_ticks < (int)(charge_cycle_period - 1)) {
            enable_charge_phase();
        } else {
            {
                uint cycle_address_diagnostic =
                    ((const void *)&charge_phase_ticks < (const void *)&charge_cycle_count);
            }

            charge_cycle_count = charge_cycle_count + 1;
            GPIO_SetBits(&gpio_d_byte_view, 4);
            advance_charge_cycle();
        }
    } else {
        disable_charge_phase();
    }
    update_discharge_control(voltage_current_protection_flags);
    if (control_indication_mode == 5) {
        indication_blink_ticks++;
        if (indication_blink_ticks < 50) {
            board_set_control_output(1, 0);
            board_set_control_output(2, 0);
        } else if (indication_blink_ticks < 100) {
            board_set_control_output(1, 1);
            board_set_control_output(2, 1);
        } else
            indication_blink_ticks = 0;
    } else if (control_indication_mode == 1 || control_indication_mode == 2 ||
               control_indication_mode == 3 || control_indication_mode == 4) {
        indication_blink_ticks++;
        if (indication_blink_ticks < 50) {
            board_set_control_output(1, 0);
            board_set_control_output(2, 1);
        } else if (indication_blink_ticks < 100) {
            board_set_control_output(1, 1);
            board_set_control_output(2, 0);
        } else
            indication_blink_ticks = 0;
    } else {
        if (battery_heating.positive_current_stage != 0 || battery_heating.negative_current_stage != 0) {
            indication_blink_ticks++;
            if (indication_blink_ticks < 50) {
                board_set_control_output(1, 1);
                board_set_control_output(2, 1);
            } else if (indication_blink_ticks < 100) {
                board_set_control_output(1, 0);
                board_set_control_output(2, 0);
            } else {
                indication_blink_ticks = 0;
            }
        } else if (voltage_current_protection_flags != 0 || temperature_protection_flags != 0 ||
            hardware_fault_flags != 0) {
            board_set_control_output(1, 0);
            board_set_control_output(2, 1);
        } else {
            if (voltage_current_limit_flags == 0 && temperature_limit_flags == 0 &&
                extended_limit_flags == 0) {
                board_set_control_output(1, 1);
                board_set_control_output(2, 0);
            } else {
                board_set_control_output(1, 1);
                board_set_control_output(2, 1);
            }
        }
    }
}
#undef gpio_b_byte_view
#undef gpio_c_byte_view
#undef gpio_d_byte_view

/* Enters current mode1 and stops its transition timer. */
void control_enter_positive_current_mode(void) {
    if (applied_current_mode != control_current_mode) {
        applied_current_mode = BMS_CURRENT_MODE_POSITIVE;
        power_output_status_flags &= 0xf7;
    }
    runtime_timer_stop(0x1a);
}

/* Enters current mode2 and resets balancing/temperature flags. */
void control_enter_negative_current_mode(void) {
    if (applied_current_mode != control_current_mode) {
        applied_current_mode = BMS_CURRENT_MODE_NEGATIVE;
        cold_temperature_latch = 0;
        power_output_status_flags |= 8;
        power_output_status_flags &= 0xfe;
        power_output_status_flags &= 0x7f;
        afe_balance_mask = 0;
    }
}

/* Enters current mode0 and clears balancing/temperature flags. */
void control_enter_idle_mode(void) {
    if (applied_current_mode != control_current_mode) {
        applied_current_mode = BMS_CURRENT_MODE_IDLE;
        cold_temperature_latch = 0;
        power_output_status_flags &= 0xf7;
        power_output_status_flags &= 0x7f;
        afe_balance_mask = 0;
    }
}

static __inline float read_measured_current(void) {
    return measurement_current;
}
/* Debounces current above positive250, below negative250 or near zero. */
void control_update_current_mode(void) {
    if (voltage_current_protection_flags == 0 && temperature_protection_flags == 0) {
        if (measurement_current > 250.0f) {
            idle_current_mode_ticks = 0;
            negative_current_mode_ticks = 0;
            if (!(power_output_status_flags & 2)) {
                control_current_mode = BMS_CURRENT_MODE_IDLE;
                return;
            }
            {
                if (control_current_mode != BMS_CURRENT_MODE_POSITIVE &&
                    ++positive_current_mode_ticks >= 5) {
                    positive_current_mode_ticks = 0;
                    control_current_mode = BMS_CURRENT_MODE_POSITIVE;
                }
            }
        } else if (measurement_current < -250.0f) {
            idle_current_mode_ticks = 0;
            positive_current_mode_ticks = 0;
            if (!(power_output_status_flags & 4)) {
                control_current_mode = BMS_CURRENT_MODE_IDLE;
                return;
            }
            {
                if (control_current_mode != BMS_CURRENT_MODE_NEGATIVE &&
                    ++negative_current_mode_ticks >= 5) {
                    negative_current_mode_ticks = 0;
                    control_current_mode = BMS_CURRENT_MODE_NEGATIVE;
                }
            }
        } else {
            positive_current_mode_ticks = 0;
            negative_current_mode_ticks = 0;
            if (control_current_mode != BMS_CURRENT_MODE_IDLE) {
                if (measurement_current < 200.0f && measurement_current > -200.0f) {
                    if (++idle_current_mode_ticks >= 5) {
                        idle_current_mode_ticks = 0;
                        control_current_mode = BMS_CURRENT_MODE_IDLE;
                    }
                } else
                    idle_current_mode_ticks = 0;
            }
        }
    }
}

/* Sets a cold-temperature flag with independent rising/falling thresholds. */
void control_update_cold_temperature_flag(void) {
    if (voltage_current_protection_flags == 0 && (temperature_protection_flags & 0x7f) == 0 &&
        (hardware_fault_flags & 0xbf) == 0) {
        if (measurement_minimum_temperature < 0xaaa && cold_temperature_latch == 0) {
            cold_temperature_latch = 1;
            power_output_status_flags |= 0x80;
        } else if (measurement_minimum_temperature > 0xb40 && cold_temperature_latch != 0) {
            cold_temperature_latch = 0;
            power_output_status_flags &= 0x7f;
        }
    } else {
        cold_temperature_latch = 0;
        power_output_status_flags &= 0x7f;
    }
}

/* Added in 3021: HR1 bit 2 enables staged battery heating.
   Temperature is represented on the firmware's offset scale, with freezing at 2730.
   Stages follow negative/positive measured current, with temperature and SOC limits. */
/* at: battery_heating 0x20000196, heating_inhibit 0x200000FE */
/* at: measurement_rated_capacity 0x20000186 */
extern BatteryHeatingState battery_heating;
extern uchar heating_inhibit;
extern ushort measurement_rated_capacity;

/* match: The positive-return magnitude helper preserves the native allocation. */
static __forceinline int heating_current_magnitude(int value) {
    if (value >= 0)
        return value;
    return -value;
}

/* Advances the cold-soak delay, current-polarity stages and timeout recovery. */
void control_update_battery_heating(void) {
    ushort minimum_temperature;
    uint rated_capacity_units;
    int measured_current;
    int current_magnitude;

    if (battery_heating.stop_pending == 1) {
        battery_heating.positive_current_stage = 0;
        battery_heating.negative_current_stage = 0;
        battery_heating.stop_pending = 0;
        battery_heating.enabled = 0;
    } else if (heating_inhibit == 0) {
        minimum_temperature = measurement_minimum_temperature;
        measured_current = (int)measurement_current;
        rated_capacity_units = measurement_rated_capacity / 10;
        if (minimum_temperature < 2750) {
            if (heating_cold_temperature_ticks < 100) {
                heating_cold_temperature_ticks = heating_cold_temperature_ticks + 1;
            } else {
                if (minimum_temperature <= 2630) {
                    battery_heating.positive_current_stage = 0;
                    battery_heating.negative_current_stage = 0;
                    battery_heating.stop_pending = 1;
                }
                if (battery_heating.negative_current_stage == 0) {
                    if (minimum_temperature > 2710 && primary_soc >= 33) {
                        battery_heating.negative_current_stage = 1;
                        battery_heating.positive_current_stage = 0;
                        heating_soc_reference = primary_soc;
                    } else if (minimum_temperature > 2680 && primary_soc >= 40) {
                        battery_heating.negative_current_stage = 2;
                        battery_heating.positive_current_stage = 0;
                        heating_soc_reference = primary_soc;
                    } else if (minimum_temperature > 2630 && primary_soc >= 55) {
                        battery_heating.negative_current_stage = 4;
                        battery_heating.positive_current_stage = 0;
                        heating_soc_reference = primary_soc;
                    } else {
                        battery_heating.negative_current_stage = 0;
                        battery_heating.positive_current_stage = 0;
                        battery_heating.stop_pending = 1;
                    }
                }
            }
        } else {
            heating_cold_temperature_ticks = 0;
            if (battery_heating.negative_current_stage != 0) {
                if ((heating_negative_current_run_ticks >= 1200) || (minimum_temperature >= 2780)) {
                    heating_negative_current_run_ticks = 0;
                    battery_heating.positive_current_stage = 1;
                    battery_heating.negative_current_stage = 0;
                }
            } else {
                if ((battery_heating.positive_current_stage == 0) && (primary_soc < 33)) {
                    battery_heating.positive_current_stage = 2;
                    heating_soc_reference = 33;
                }
            }
        }
        if (battery_heating.negative_current_stage != 0) {
            current_magnitude = heating_current_magnitude(measured_current);
            if (current_magnitude >= (int)(rated_capacity_units * 20) && measured_current < 0 &&
                primary_soc > 4) {
                heating_negative_current_wait_ticks = 0;
                heating_negative_current_run_ticks = heating_negative_current_run_ticks + 1;
                heating_negative_current_retries = 0;
            } else {
                heating_negative_current_run_ticks = 0;
                if ((heating_negative_current_wait_ticks++ > 900) || primary_soc <= 4) {
                    heating_negative_current_wait_ticks = 0;
                    battery_heating.positive_current_stage = 0;
                    battery_heating.negative_current_stage = 0;
                    if ((primary_soc <= 4) && (minimum_temperature < 2750)) {
                        shutdown_request_flags = shutdown_request_flags | 2;
                        battery_heating.stop_pending = 1;
                    } else if (minimum_temperature >= 2750) {
                        battery_heating.positive_current_stage = 1;
                    }
                    if (heating_negative_current_retries++ >= 3) {
                        heating_negative_current_retries = 0;
                        battery_heating.stop_pending = 1;
                    }
                }
            }
        } else {
            if (battery_heating.positive_current_stage != 0) {
                if ((primary_soc >= heating_soc_reference) && (minimum_temperature >= 2780)) {
                    battery_heating.positive_current_stage = 0;
                    battery_heating.negative_current_stage = 0;
                    battery_heating.enabled = 0;
                }
                current_magnitude = heating_current_magnitude(measured_current);
                if (current_magnitude >= (int)(rated_capacity_units * 20) && measured_current > 0) {
                    heating_positive_current_retries = 0;
                    heating_positive_current_wait_ticks = 0;
                    if (primary_soc >= heating_soc_reference) {
                        if (minimum_temperature >= 2780) {
                            battery_heating.positive_current_stage = 0;
                            battery_heating.negative_current_stage = 0;
                            battery_heating.enabled = 0;
                        } else {
                            battery_heating.negative_current_stage = 1;
                            if (battery_heating.positive_current_stage == 2) {
                                battery_heating.positive_current_stage = 0;
                                battery_heating.negative_current_stage = 0;
                                battery_heating.enabled = 0;
                            }
                            battery_heating.positive_current_stage = 0;
                        }
                    } else {
                        if (minimum_temperature >= 2750) {
                            if (primary_soc >= 100) {
                                battery_heating.positive_current_stage = 0;
                                battery_heating.negative_current_stage = 0;
                                heating_positive_current_wait_ticks = 0;
                                battery_heating.enabled = 0;
                            }
                        } else {
                            if (battery_heating.positive_current_stage != 2) {
                                battery_heating.positive_current_stage = 0;
                                battery_heating.negative_current_stage = 1;
                            } else {
                                battery_heating.positive_current_stage = 0;
                                battery_heating.negative_current_stage = 0;
                                battery_heating.stop_pending = 1;
                            }
                        }
                    }
                } else {
                    if (heating_positive_current_wait_ticks++ > 900) {
                        battery_heating.positive_current_stage = 0;
                        battery_heating.negative_current_stage = 0;
                        heating_positive_current_wait_ticks = 0;
                        if (heating_positive_current_retries++ >= 3) {
                            heating_positive_current_retries = 0;
                            battery_heating.stop_pending = 1;
                        }
                    }
                }
            }
        }
    } else {
        battery_heating.negative_current_stage = 0;
        battery_heating.positive_current_stage = 0;
    }
}

/* Dispatches the three current-mode transition handlers. */
void control_apply_current_mode(void) {

    if (control_current_mode == BMS_CURRENT_MODE_IDLE) {
        control_enter_idle_mode();
        return;
    }
    if (control_current_mode != BMS_CURRENT_MODE_POSITIVE) {
        if (control_current_mode == BMS_CURRENT_MODE_NEGATIVE) {
            control_enter_negative_current_mode();
            return;
        }
        return;
    }
    control_enter_positive_current_mode();
    return;
}

#define gpio_b_byte_view (*(volatile uchar *)0x40010C00)
#define gpio_c_byte_view (*(volatile uchar *)0x40011000)
#define gpio_d_byte_view (*(volatile uchar *)0x40011400)
#define coordination_reset_option (*(volatile uchar *)(device_registers + 209))

/* Persists state, disables outputs and enters board standby when requested. */
void control_check_shutdown(void) {

    if (coordination_reset_option != '\0') {
        coordination_packet_valid = 0;
        runtime_timer_stop(0x1a);
    }
    if (shutdown_request_flags != '\0') {
        board_set_pb9(1);
        runtime_timer_stop(0x1a);
        shutdown_in_progress = 1;
        power_output_status_flags = power_output_status_flags | 0x40;
        snapshot_publish();
        eeprom_log_write_record(&event_log_index, 0, 0);
        os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
        GPIO_ResetBits(&gpio_d_byte_view, 4);
        pd2_output_state = 0;
        GPIO_ResetBits(&gpio_b_byte_view, 8);
        pb3_output_state = 0;
        GPIO_ResetBits(&gpio_c_byte_view, 0x100);
        pc8_output_state = 0;
        cold_temperature_latch = 0;
        board_set_pb2(0);
        board_set_pa8(0);
        board_initialize_pa0_interrupt();
        afe_write_register(6, 0x10);
        eeprom_write_word(6, (((uint)remaining_capacity)) & 0xffff);
        afe_write_register(6, 0x10);
        os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
        board_set_pb9(0);
        board_enter_standby();
        return;
    }
    return;
}
#undef gpio_b_byte_view
#undef gpio_c_byte_view
#undef gpio_d_byte_view
#undef coordination_reset_option

/* Runs startup, measurement, protection, coordination and shutdown scheduling.
   I keep the startup delays, deferred coordination and protection scheduling in their original order. */
void control_task(void) {

    ushort activity_ticks;
    while (1) {
        control_task_wake_flags = (uchar)os_flags_pend(0);
        activity_ticks = control_activity_ticks;
        if (activity_ticks < 2000 && control_activity_count != 0) {
            control_activity_ticks = activity_ticks + 1;
        } else {
            control_activity_ticks = 0;
            control_activity_count = 0;
        }
        if (control_activity_pending == 1) {
            control_activity_pending = 0;
            control_activity_count = control_activity_count + 1;
        }
        if (settings_initialized == '\0') {
            settings_startup();
            settings_initialized = '\x01';
        }
        control_apply_current_mode();
        measurement_update_current();
        if (control_startup_ticks < 1000) {
            control_startup_ticks = control_startup_ticks + 1;
        } else if (afe_calibration_ready == '\x01') {
            if (control_activity_count != 0) {
                protection_run_interval = 40;
                protection_run_ticks = 0;
            }
            if (protection_run_ticks++ >= protection_run_interval) {
                protection_run_ticks = 0;
                protection_run_interval = 0;
                measurement_ready = 1;
                protection_cycle();
            }
            control_update_power_outputs();
        }
        pack_balance_and_control();
        if (control_slow_task_divider++ > 4) {
            control_slow_task_divider = 0;
            sub_0802126C();
        }
        if (coordination_startup_ticks < 2000) {
            coordination_startup_ticks = coordination_startup_ticks + 1;
        } else {
            coordination_ready = 1;
        }
        if ((settings_initialized == '\x01') && (coordination_ready == 1)) {
            coordination_poll();
        }
        control_check_shutdown();
        control_scheduler_words[6] = 0;
        control_scheduler_words[5] = 5;
    }
}
