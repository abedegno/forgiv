/* Shared 3021/3022 battery-heating state, enabled by remote HR1 bit 2.
   name: Descriptive names follow the measured-current sign tests and native
   byte offsets. Physical charge/discharge interpretation and vendor names remain unconfirmed. */
#ifndef BMS_HEATING_H
#define BMS_HEATING_H
#include "ghidra_types.h"

typedef struct {
    uchar enabled;
    /* match: Volatile preserves the target's repeated zero writes at offset 1. */
    volatile uchar negative_current_stage;
    uchar positive_current_stage;
    uchar stop_pending; /* Consumed on the next update to clear and disable heating. */
} BatteryHeatingState;

extern BatteryHeatingState battery_heating;
void control_update_battery_heating(void);
#endif
