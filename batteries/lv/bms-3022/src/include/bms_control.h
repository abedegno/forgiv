/* I share control task and state-transition entry points.
   match: bodies remain in the complete control compilation unit. */
#ifndef BMS_CONTROL_H
#define BMS_CONTROL_H
/* I name the observed current-sign modes without assuming an external sign convention. */
enum BmsCurrentMode {
    BMS_CURRENT_MODE_IDLE = 0,
    BMS_CURRENT_MODE_POSITIVE = 1,
    BMS_CURRENT_MODE_NEGATIVE = 2
};
void protection_recover_voltage_limits(void);
void protection_recover_voltage_current(void);
void protection_recover_temperature(void);
void control_clear_current_direction_latches(void);
void protection_recover_fault_flags(void);
void control_clear_negative_current_latches(void);
void control_update_power_outputs(void);
void control_enter_positive_current_mode(void);
void control_enter_negative_current_mode(void);
void control_enter_idle_mode(void);
void control_update_current_mode(void);
void control_update_cold_temperature_flag(void);
void control_apply_current_mode(void);
void control_check_shutdown(void);
void control_task(void);
#endif
