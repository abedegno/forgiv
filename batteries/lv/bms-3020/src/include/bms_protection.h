/* I share the protection checks and their paired status-byte roles.
   match: implementations stay in the complete measurement source unit. */
#ifndef BMS_PROTECTION_H
#define BMS_PROTECTION_H
#include "ghidra_types.h"
/* I keep limit and protection masks distinct: the temperature layouts differ.
   match: positive byte clear masks and shifted sign tests retain their original
   spellings and operators; I do not replace them with complements or new tests. */
enum BmsProtectionConstants {
    BMS_LIMIT_AMBIENT_OVERTEMPERATURE = 0x10,
    BMS_LIMIT_AMBIENT_OVERTEMPERATURE_CLEAR_MASK = 0xef,
    BMS_LIMIT_AMBIENT_OVERTEMPERATURE_SIGN_SHIFT = 0x1b,
    BMS_LIMIT_AMBIENT_UNDERTEMPERATURE = 0x20,
    BMS_LIMIT_AMBIENT_UNDERTEMPERATURE_CLEAR_MASK = 0xdf,
    BMS_LIMIT_AMBIENT_UNDERTEMPERATURE_SIGN_SHIFT = 0x1a,
    BMS_LIMIT_CELL_OVERVOLTAGE = 1,
    BMS_LIMIT_CELL_OVERVOLTAGE_CLEAR_MASK = 0xfe,
    BMS_LIMIT_CELL_UNDERVOLTAGE = 2,
    BMS_LIMIT_CELL_UNDERVOLTAGE_CLEAR_MASK = 0xfd,
    BMS_LIMIT_CHARGE_OVERCURRENT = 0x10,
    BMS_LIMIT_CHARGE_OVERCURRENT_CLEAR_MASK = 0xef,
    BMS_LIMIT_CHARGE_OVERCURRENT_SIGN_SHIFT = 0x1b,
    BMS_LIMIT_CHARGE_OVERTEMPERATURE = 1,
    BMS_LIMIT_CHARGE_OVERTEMPERATURE_CLEAR_MASK = 0xfe,
    BMS_LIMIT_CHARGE_UNDERTEMPERATURE = 4,
    BMS_LIMIT_CHARGE_UNDERTEMPERATURE_CLEAR_MASK = 0xfb,
    BMS_LIMIT_CHARGE_UNDERTEMPERATURE_SIGN_SHIFT = 0x1d,
    BMS_LIMIT_DISCHARGE_OVERCURRENT = 0x20,
    BMS_LIMIT_DISCHARGE_OVERCURRENT_CLEAR_MASK = 0xdf,
    BMS_LIMIT_DISCHARGE_OVERCURRENT_SIGN_SHIFT = 0x1a,
    BMS_LIMIT_DISCHARGE_OVERTEMPERATURE = 2,
    BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_CLEAR_MASK = 0xfd,
    BMS_LIMIT_DISCHARGE_OVERTEMPERATURE_SIGN_SHIFT = 0x1e,
    BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE = 8,
    BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_CLEAR_MASK = 0xf7,
    BMS_LIMIT_DISCHARGE_UNDERTEMPERATURE_SIGN_SHIFT = 0x1c,
    BMS_LIMIT_MOSFET_OVERTEMPERATURE = 0x40,
    BMS_LIMIT_MOSFET_OVERTEMPERATURE_CLEAR_MASK = 0xbf,
    BMS_LIMIT_MOSFET_OVERTEMPERATURE_SIGN_SHIFT = 0x19,
    BMS_LIMIT_PACK_OVERVOLTAGE = 4,
    BMS_LIMIT_PACK_OVERVOLTAGE_CLEAR_MASK = 0xfb,
    BMS_LIMIT_PACK_UNDERVOLTAGE = 8,
    BMS_LIMIT_PACK_UNDERVOLTAGE_CLEAR_MASK = 0xf7,
    BMS_LIMIT_PACK_UNDERVOLTAGE_SIGN_SHIFT = 0x1c,
    BMS_PROTECTION_AMBIENT_OVERTEMPERATURE = 0x20,
    BMS_PROTECTION_AMBIENT_OVERTEMPERATURE_SIGN_SHIFT = 0x1a,
    BMS_PROTECTION_AMBIENT_UNDERTEMPERATURE = 0x40,
    BMS_PROTECTION_AMBIENT_UNDERTEMPERATURE_SIGN_SHIFT = 0x19,
    BMS_PROTECTION_CELL_OVERVOLTAGE = 1,
    BMS_PROTECTION_CELL_UNDERVOLTAGE = 2,
    BMS_PROTECTION_CHARGE_OVERCURRENT = 0x10,
    BMS_PROTECTION_CHARGE_OVERCURRENT_SIGN_SHIFT = 0x1b,
    BMS_PROTECTION_CHARGE_OVERTEMPERATURE = 1,
    BMS_PROTECTION_CHARGE_UNDERTEMPERATURE = 4,
    BMS_PROTECTION_CHARGE_UNDERTEMPERATURE_SIGN_SHIFT = 0x1d,
    BMS_PROTECTION_DISCHARGE_OVERCURRENT = 0x20,
    BMS_PROTECTION_DISCHARGE_OVERCURRENT_SIGN_SHIFT = 0x1a,
    BMS_PROTECTION_DISCHARGE_OVERTEMPERATURE = 2,
    BMS_PROTECTION_DISCHARGE_OVERTEMPERATURE_SIGN_SHIFT = 0x1e,
    BMS_PROTECTION_DISCHARGE_UNDERTEMPERATURE = 8,
    BMS_PROTECTION_DISCHARGE_UNDERTEMPERATURE_SIGN_SHIFT = 0x1c,
    BMS_PROTECTION_MOSFET_OVERTEMPERATURE = 0x10,
    BMS_PROTECTION_MOSFET_OVERTEMPERATURE_SIGN_SHIFT = 0x1b,
    BMS_PROTECTION_PACK_OVERVOLTAGE = 4,
    BMS_PROTECTION_PACK_UNDERVOLTAGE = 8,
    BMS_PROTECTION_PACK_UNDERVOLTAGE_SIGN_SHIFT = 0x1c
};
extern volatile uchar voltage_current_protection_flags;
extern volatile uchar temperature_protection_flags;
extern volatile uchar voltage_current_limit_flags;
extern volatile uchar temperature_limit_flags;
void protection_update_pack_undervoltage(void);
void protection_update_cell_undervoltage(void);
void protection_update_pack_overvoltage(void);
void protection_update_cell_overvoltage(void);
void protection_update_discharge_overcurrent(void);
void protection_update_charge_overcurrent(void);
void protection_update_charge_overtemperature(void);
void protection_update_discharge_overtemperature(void);
void protection_update_charge_undertemperature(void);
void protection_update_discharge_undertemperature(void);
void protection_update_mosfet_overtemperature(void);
void protection_update_ambient_overtemperature(void);
void protection_update_ambient_undertemperature(void);
void protection_cycle(void);
void protection_update_voltage(void);
void protection_update_current(void);
void protection_update_temperature(void);
void protection_update_charge_discharge_temperature(void);
void protection_update_ambient_temperature(void);
uchar protection_get_voltage_current_flags(void);
#endif
