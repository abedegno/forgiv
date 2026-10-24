/* I name the halfword layouts from their setting, protection and recovery uses.
   match: current fields retain the signed views used by the model presets. */
#ifndef BMS_THRESHOLDS_H
#define BMS_THRESHOLDS_H
#include "ghidra_types.h"
typedef struct {
    ushort cell_overvoltage;
    ushort cell_undervoltage;
    ushort pack_overvoltage;
    ushort pack_undervoltage;
    short charge_overcurrent;
    short discharge_overcurrent;
    ushort charge_overtemperature;
    ushort discharge_overtemperature;
    ushort charge_undertemperature;
    ushort discharge_undertemperature;
    ushort mosfet_overtemperature;
    ushort ambient_overtemperature;
    ushort ambient_undertemperature;
} BmsLimitThresholds;
typedef struct {
    ushort cell_overvoltage;
    ushort cell_undervoltage;
    ushort pack_overvoltage;
    ushort pack_undervoltage;
    short charge_overcurrent;
    short discharge_overcurrent;
    ushort short_circuit_current;
    ushort charge_overtemperature;
    ushort discharge_overtemperature;
    ushort charge_undertemperature;
    ushort discharge_undertemperature;
    ushort mosfet_overtemperature;
    ushort ambient_overtemperature;
    ushort ambient_undertemperature;
} BmsProtectionThresholds;
typedef struct {
    ushort cell_overvoltage;
    ushort cell_undervoltage;
    ushort pack_overvoltage;
    ushort pack_undervoltage;
    ushort charge_overtemperature;
    ushort discharge_overtemperature;
    ushort charge_undertemperature;
    ushort discharge_undertemperature;
    ushort mosfet_overtemperature;
    ushort ambient_overtemperature;
    ushort ambient_undertemperature;
} BmsRecoveryThresholds;
typedef BmsLimitThresholds limits_t;
#define BMS_LIMIT_FIELD(base, field) (((BmsLimitThresholds *)(base))->field)
#define BMS_PROTECTION_FIELD(base, field) (((BmsProtectionThresholds *)(base))->field)
#define BMS_RECOVERY_FIELD(base, field) (((BmsRecoveryThresholds *)(base))->field)
#endif
