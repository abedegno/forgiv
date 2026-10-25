#include "../../include/bms_settings_constants.h"
#include "../../include/bms_thresholds.h"
/* Battery settings, factory calibration limits and snapshot publication. */
/* at: settings_load_limit_bank 0x08016A8C, settings_restore_factory_limits 0x08016F1E, snapshot_initialize 0x08017138, snapshot_publish 0x080171AC, settings_clear_counters 0x080172B2, settings_200002F8 0x20000304, settings_20001522 0x2000153A, settings_20001552 0x2000156A, settings_2000153C 0x20001554, settings_20000077 0x20000077, settings_200000FA 0x200000FC, settings_200000F8 0x200000FA, settings_20000182 0x20000186, c_080304FA 0x0803093E, c_08030514 0x08030958, c_08030530 0x08030974, settings_20000162 0x20000166, settings_200014D2 0x200014EA, snapshots 0x200014EA, settings_20001094 0x200010AC, settings_2000119E 0x200011B6, settings_20000277 0x2000027F, settings_20000278 0x20000280, settings_20000148 0x2000014C, settings_20000110 0x20000114, settings_2000016C 0x20000170, settings_20000164 0x20000168, settings_200000A2 0x200000A2, settings_20000271 0x20000279, settings_20000272 0x2000027A, settings_20000273 0x2000027B, settings_2000050D 0x20000519, settings_20000274 0x2000027C, settings_200000CE 0x200000CE, settings_2000156E 0x20001586, settings_20001578 0x20001590, eeprom_read_word 0x080225B2, eeprom_write_word 0x080225D6, __aeabi_memcpy 0x0802F58C, coordination_initialize_registers 0x08021530, coordination_update_registers 0x08021580, __aeabi_f2iz 0x0803042C, __aeabi_f2uiz 0x08030464 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow limit-bank loading, factory presets and snapshot publication here; EEPROM and measurement provide the inputs.
   Entry examples: settings_load_limit_bank, settings_restore_factory_limits, snapshot_initialize.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "bms_settings_types.h"
#define DAT_200002f8 (*(uchar *)(settings_200002F8 + 0))
#define limit_cell_undervoltage BMS_LIMIT_FIELD(settings_20001522, cell_undervoltage)
#define protection_cell_undervoltage BMS_PROTECTION_FIELD(settings_20001552, cell_undervoltage)
#define recovery_cell_undervoltage BMS_RECOVERY_FIELD(settings_2000153C, cell_undervoltage)
#define DAT_200002f9 (*(uchar *)(settings_200002F8 + 1))
#define limit_pack_overvoltage BMS_LIMIT_FIELD(settings_20001522, pack_overvoltage)
#define protection_pack_overvoltage BMS_PROTECTION_FIELD(settings_20001552, pack_overvoltage)
#define recovery_pack_overvoltage BMS_RECOVERY_FIELD(settings_2000153C, pack_overvoltage)
#define DAT_200002fa (*(uchar *)(settings_200002F8 + 2))
#define limit_pack_undervoltage BMS_LIMIT_FIELD(settings_20001522, pack_undervoltage)
#define protection_pack_undervoltage BMS_PROTECTION_FIELD(settings_20001552, pack_undervoltage)
#define recovery_pack_undervoltage BMS_RECOVERY_FIELD(settings_2000153C, pack_undervoltage)
#define DAT_200002fb (*(uchar *)(settings_200002F8 + 3))
#define limit_charge_overcurrent BMS_LIMIT_FIELD(settings_20001522, charge_overcurrent)
#define protection_charge_overcurrent BMS_PROTECTION_FIELD(settings_20001552, charge_overcurrent)
#define DAT_200002fc (*(uchar *)(settings_200002F8 + 4))
#define limit_discharge_overcurrent BMS_LIMIT_FIELD(settings_20001522, discharge_overcurrent)
#define limit_cell_overvoltage BMS_LIMIT_FIELD(settings_20001522, cell_overvoltage)
#define protection_discharge_overcurrent                                                           \
    BMS_PROTECTION_FIELD(settings_20001552, discharge_overcurrent)
#define recovery_cell_overvoltage BMS_RECOVERY_FIELD(settings_2000153C, cell_overvoltage)
#define DAT_200002fd (*(uchar *)(settings_200002F8 + 5))
#define protection_cell_overvoltage BMS_PROTECTION_FIELD(settings_20001552, cell_overvoltage)
#define limit_charge_overtemperature BMS_LIMIT_FIELD(settings_20001522, charge_overtemperature)
#define protection_charge_overtemperature                                                          \
    BMS_PROTECTION_FIELD(settings_20001552, charge_overtemperature)
#define recovery_charge_overtemperature                                                            \
    BMS_RECOVERY_FIELD(settings_2000153C, charge_overtemperature)
#define limit_discharge_overtemperature                                                            \
    BMS_LIMIT_FIELD(settings_20001522, discharge_overtemperature)
#define protection_discharge_overtemperature                                                       \
    BMS_PROTECTION_FIELD(settings_20001552, discharge_overtemperature)
#define recovery_discharge_overtemperature                                                         \
    BMS_RECOVERY_FIELD(settings_2000153C, discharge_overtemperature)
#define limit_charge_undertemperature BMS_LIMIT_FIELD(settings_20001522, charge_undertemperature)
#define protection_charge_undertemperature                                                         \
    BMS_PROTECTION_FIELD(settings_20001552, charge_undertemperature)
#define recovery_charge_undertemperature                                                           \
    BMS_RECOVERY_FIELD(settings_2000153C, charge_undertemperature)
#define limit_discharge_undertemperature                                                           \
    BMS_LIMIT_FIELD(settings_20001522, discharge_undertemperature)
#define protection_discharge_undertemperature                                                      \
    BMS_PROTECTION_FIELD(settings_20001552, discharge_undertemperature)
#define recovery_discharge_undertemperature                                                        \
    BMS_RECOVERY_FIELD(settings_2000153C, discharge_undertemperature)
#define limit_mosfet_overtemperature BMS_LIMIT_FIELD(settings_20001522, mosfet_overtemperature)
#define protection_mosfet_overtemperature                                                          \
    BMS_PROTECTION_FIELD(settings_20001552, mosfet_overtemperature)
#define recovery_mosfet_overtemperature                                                            \
    BMS_RECOVERY_FIELD(settings_2000153C, mosfet_overtemperature)
#define limit_ambient_overtemperature BMS_LIMIT_FIELD(settings_20001522, ambient_overtemperature)
#define limit_ambient_undertemperature BMS_LIMIT_FIELD(settings_20001522, ambient_undertemperature)
#define protection_short_circuit_current                                                           \
    BMS_PROTECTION_FIELD(settings_20001552, short_circuit_current)
#define DAT_200002fe (*(uchar *)(settings_200002F8 + 6))
#define DAT_200002ff (*(uchar *)(settings_200002F8 + 7))
#define protection_ambient_overtemperature                                                         \
    BMS_PROTECTION_FIELD(settings_20001552, ambient_overtemperature)
#define recovery_ambient_overtemperature                                                           \
    BMS_RECOVERY_FIELD(settings_2000153C, ambient_overtemperature)
#define protection_ambient_undertemperature                                                        \
    BMS_PROTECTION_FIELD(settings_20001552, ambient_undertemperature)
#define recovery_ambient_undertemperature                                                          \
    BMS_RECOVERY_FIELD(settings_2000153C, ambient_undertemperature)
#define model_family (*(uchar *)(settings_20000077 + 0))
#define model_profile settings_200000FA
#define model_code settings_200000F8
#define rated_capacity settings_20000182
#define model_variant (*(uchar *)(settings_20000077 + 1))

void settings_load_limit_bank(int factory_bank)

{
    uint stored_word;
    short bank_offset;

    if (factory_bank == 0) {
        bank_offset = 0x80;
    } else {
        bank_offset = 0x200;
    }
    stored_word = eeprom_read_word(bank_offset + 2);
    limit_cell_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 4);
    protection_cell_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 6);
    recovery_cell_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 8);
    DAT_200002f8 = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 10);
    limit_cell_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0xc);
    protection_cell_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0xe);
    recovery_cell_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x10);
    DAT_200002f9 = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x12);
    limit_pack_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x14);
    protection_pack_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x16);
    recovery_pack_overvoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x18);
    DAT_200002fa = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x1a);
    limit_pack_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x1c);
    protection_pack_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x1e);
    recovery_pack_undervoltage = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x20);
    DAT_200002fb = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x22);
    limit_charge_overcurrent = (ushort)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x24);
    protection_charge_overcurrent = (ushort)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x26);
    DAT_200002fc = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x28);
    limit_discharge_overcurrent = (ushort)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x2a);
    protection_discharge_overcurrent = (ushort)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x2c);
    DAT_200002fd = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x2e);
    limit_charge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x30);
    protection_charge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x32);
    recovery_charge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x34);
    limit_discharge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x36);
    protection_discharge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x38);
    recovery_discharge_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x3a);
    limit_charge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x3c);
    protection_charge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x3e);
    recovery_charge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x40);
    limit_discharge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x42);
    protection_discharge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x44);
    recovery_discharge_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x46);
    limit_mosfet_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x48);
    protection_mosfet_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x4a);
    recovery_mosfet_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x4c);
    limit_ambient_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x4e);
    limit_ambient_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x50);
    protection_short_circuit_current = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x52);
    DAT_200002fe = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x54);
    DAT_200002ff = (undefined1)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x70);
    protection_ambient_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x72);
    recovery_ambient_overtemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x74);
    protection_ambient_undertemperature = (undefined2)stored_word;
    stored_word = eeprom_read_word(bank_offset + 0x76);
    recovery_ambient_undertemperature = (undefined2)stored_word;
    if (model_family == BMS_MODEL_FAMILY_B) {
        model_profile = 0;
        if ((model_variant == BMS_MODEL_VARIANT_G) &&
            (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            model_code = 1;
            limit_discharge_overcurrent = -70;
            limit_charge_overcurrent = 0x46;
            protection_discharge_overcurrent = -75;
            protection_charge_overcurrent = 0x4b;
        } else if ((model_variant == BMS_MODEL_VARIANT_Y) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            model_code = 4;
            limit_discharge_overcurrent = -90;
            limit_charge_overcurrent = 0x5a;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5f;
        } else {
            model_code = 2;
            if (rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
                limit_discharge_overcurrent = -60;
                limit_charge_overcurrent = 0x3c;
                protection_discharge_overcurrent = -65;
                protection_charge_overcurrent = 0x41;
            } else {
                limit_discharge_overcurrent = -70;
                limit_charge_overcurrent = 0x46;
                protection_discharge_overcurrent = -75;
                protection_charge_overcurrent = 0x4b;
            }
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    } else if (model_family == BMS_MODEL_FAMILY_D) {
        model_profile = 1;
        if ((model_variant == BMS_MODEL_VARIANT_G) &&
            (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            model_code = 1;
            limit_discharge_overcurrent = -90;
            limit_charge_overcurrent = 0x55;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5a;
        } else if ((model_variant == BMS_MODEL_VARIANT_Y) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            model_code = 4;
            limit_discharge_overcurrent = -90;
            limit_charge_overcurrent = 0x5a;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5f;
        } else if (((((model_variant == BMS_MODEL_VARIANT_W) ||
                      (model_variant == BMS_MODEL_VARIANT_X)) ||
                     (model_variant == BMS_MODEL_VARIANT_Z)) ||
                    (model_variant == BMS_MODEL_VARIANT_F)) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_EXTENDED)) {
            model_code = 4;
            limit_discharge_overcurrent = -90;
            limit_charge_overcurrent = 0x5a;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5f;
        } else {
            model_code = 2;
            if (rated_capacity >= BMS_MODEL_CAPACITY_STANDARD) {
                limit_discharge_overcurrent = -90;
                limit_charge_overcurrent = 0x55;
                protection_discharge_overcurrent = -95;
                protection_charge_overcurrent = 0x5a;
            } else {
                limit_discharge_overcurrent = -(rated_capacity / 200 + 10);
                limit_charge_overcurrent = rated_capacity / 200 + 10;
                protection_discharge_overcurrent = -(rated_capacity / 200 + 0xf);
                protection_charge_overcurrent = rated_capacity / 200 + 0xf;
            }
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    } else {
        model_profile = 0;
        model_code = 2;
        if (rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
            limit_discharge_overcurrent = -60;
            limit_charge_overcurrent = 0x3c;
            protection_discharge_overcurrent = -65;
            protection_charge_overcurrent = 0x41;
        } else {
            limit_discharge_overcurrent = -70;
            limit_charge_overcurrent = 0x46;
            protection_discharge_overcurrent = -75;
            protection_charge_overcurrent = 0x4b;
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    }
}

#undef DAT_200002f8
#undef limit_cell_undervoltage
#undef protection_cell_undervoltage
#undef recovery_cell_undervoltage
#undef DAT_200002f9
#undef limit_pack_overvoltage
#undef protection_pack_overvoltage
#undef recovery_pack_overvoltage
#undef DAT_200002fa
#undef limit_pack_undervoltage
#undef protection_pack_undervoltage
#undef recovery_pack_undervoltage
#undef DAT_200002fb
#undef limit_charge_overcurrent
#undef protection_charge_overcurrent
#undef DAT_200002fc
#undef limit_discharge_overcurrent
#undef limit_cell_overvoltage
#undef protection_discharge_overcurrent
#undef recovery_cell_overvoltage
#undef DAT_200002fd
#undef protection_cell_overvoltage
#undef limit_charge_overtemperature
#undef protection_charge_overtemperature
#undef recovery_charge_overtemperature
#undef limit_discharge_overtemperature
#undef protection_discharge_overtemperature
#undef recovery_discharge_overtemperature
#undef limit_charge_undertemperature
#undef protection_charge_undertemperature
#undef recovery_charge_undertemperature
#undef limit_discharge_undertemperature
#undef protection_discharge_undertemperature
#undef recovery_discharge_undertemperature
#undef limit_mosfet_overtemperature
#undef protection_mosfet_overtemperature
#undef recovery_mosfet_overtemperature
#undef limit_ambient_overtemperature
#undef limit_ambient_undertemperature
#undef protection_short_circuit_current
#undef DAT_200002fe
#undef DAT_200002ff
#undef protection_ambient_overtemperature
#undef recovery_ambient_overtemperature
#undef protection_ambient_undertemperature
#undef recovery_ambient_undertemperature
#undef model_family
#undef model_profile
#undef model_code
#undef rated_capacity
#undef model_variant
#define DAT_200002f8 (*(uchar *)(settings_200002F8 + 0))
#define DAT_200002f9 (*(uchar *)(settings_200002F8 + 1))
#define DAT_200002fa (*(uchar *)(settings_200002F8 + 2))
#define DAT_200002fb (*(uchar *)(settings_200002F8 + 3))
#define DAT_200002fc (*(uchar *)(settings_200002F8 + 4))
#define DAT_200002fd (*(uchar *)(settings_200002F8 + 5))
#define DAT_200002fe (*(uchar *)(settings_200002F8 + 6))
#define DAT_200002ff (*(uchar *)(settings_200002F8 + 7))
#define model_family (*(uchar *)(settings_20000077 + 0))
#define rated_capacity settings_20000182
#define limit_discharge_overcurrent BMS_LIMIT_FIELD(settings_20001522, discharge_overcurrent)
#define limit_charge_overcurrent BMS_LIMIT_FIELD(settings_20001522, charge_overcurrent)
#define protection_discharge_overcurrent                                                           \
    BMS_PROTECTION_FIELD(settings_20001552, discharge_overcurrent)
#define protection_charge_overcurrent BMS_PROTECTION_FIELD(settings_20001552, charge_overcurrent)
#define model_variant (*(uchar *)(settings_20000077 + 1))
#define limit_cell_overvoltage BMS_LIMIT_FIELD(settings_20001522, cell_overvoltage)
#define recovery_cell_overvoltage BMS_RECOVERY_FIELD(settings_2000153C, cell_overvoltage)
#define protection_cell_overvoltage BMS_PROTECTION_FIELD(settings_20001552, cell_overvoltage)

void settings_restore_factory_limits(void)

{
    uint marker_word;

    marker_word = eeprom_read_word(0x200);
    if (marker_word == BMS_LIMIT_BANK_MARKER) {
        settings_load_limit_bank(1);
        return;
    }
    DAT_200002f8 = 10;
    DAT_200002f9 = 10;
    DAT_200002fa = 10;
    DAT_200002fb = 10;
    DAT_200002fc = 10;
    DAT_200002fd = 10;
    DAT_200002fe = 4;
    DAT_200002ff = 6;
    __aeabi_memcpy((uint *)&limit_cell_overvoltage, (uint *)c_080304FA, 0x1a);
    __aeabi_memcpy((uint *)&protection_cell_overvoltage, (uint *)c_08030514, 0x1c);
    __aeabi_memcpy((uint *)&recovery_cell_overvoltage, (uint *)c_08030530, 0x16);
    if (model_family == BMS_MODEL_FAMILY_B) {
        if ((model_variant == BMS_MODEL_VARIANT_G) &&
            (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            limit_discharge_overcurrent = -70;
            limit_charge_overcurrent = 0x46;
            protection_discharge_overcurrent = -75;
            protection_charge_overcurrent = 0x4b;
        } else if ((model_variant == BMS_MODEL_VARIANT_Y) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            limit_discharge_overcurrent = -95;
            limit_charge_overcurrent = 0x5d;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5d;
        } else if (rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
            limit_discharge_overcurrent = -60;
            limit_charge_overcurrent = 0x3c;
            protection_discharge_overcurrent = -65;
            protection_charge_overcurrent = 0x41;
        } else {
            limit_discharge_overcurrent = -70;
            limit_charge_overcurrent = 0x46;
            protection_discharge_overcurrent = -75;
            protection_charge_overcurrent = 0x4b;
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    } else if (model_family == BMS_MODEL_FAMILY_D) {
        if ((model_variant == BMS_MODEL_VARIANT_G) &&
            (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            limit_discharge_overcurrent = -90;
            limit_charge_overcurrent = 0x55;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5a;
        } else if ((model_variant == BMS_MODEL_VARIANT_Y) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_STANDARD)) {
            limit_discharge_overcurrent = -95;
            limit_charge_overcurrent = 0x5d;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5d;
        } else if (((((model_variant == BMS_MODEL_VARIANT_W) ||
                      (model_variant == BMS_MODEL_VARIANT_X)) ||
                     (model_variant == BMS_MODEL_VARIANT_Z)) ||
                    (model_variant == BMS_MODEL_VARIANT_F)) &&
                   (rated_capacity == BMS_MODEL_CAPACITY_EXTENDED)) {
            limit_discharge_overcurrent = -95;
            limit_charge_overcurrent = 0x5d;
            protection_discharge_overcurrent = -95;
            protection_charge_overcurrent = 0x5d;
        } else if (rated_capacity >= BMS_MODEL_CAPACITY_STANDARD) {
            {
                int charge_min = -90;
                int charge_max = 0x55;
                int discharge_min = -95;
                int discharge_max = 0x5a;
                limit_discharge_overcurrent = charge_min;
                limit_charge_overcurrent = charge_max;
                protection_discharge_overcurrent = discharge_min;
                protection_charge_overcurrent = discharge_max;
            }
        } else {
            limit_discharge_overcurrent = -(rated_capacity / 200 + 5);
            limit_charge_overcurrent = rated_capacity / 200 + 5;
            protection_discharge_overcurrent = -(rated_capacity / 200 + 10);
            protection_charge_overcurrent = rated_capacity / 200 + 10;
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    } else {
        if (rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
            limit_discharge_overcurrent = -60;
            limit_charge_overcurrent = 0x3c;
            protection_discharge_overcurrent = -65;
            protection_charge_overcurrent = 0x41;
        } else {
            limit_discharge_overcurrent = -70;
            limit_charge_overcurrent = 0x46;
            protection_discharge_overcurrent = -75;
            protection_charge_overcurrent = 0x4b;
        }
        eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, (uint)limit_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION, (uint)protection_discharge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, (uint)limit_charge_overcurrent);
        eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION, (uint)protection_charge_overcurrent);
        return;
    }
}

#undef DAT_200002f8
#undef DAT_200002f9
#undef DAT_200002fa
#undef DAT_200002fb
#undef DAT_200002fc
#undef DAT_200002fd
#undef DAT_200002fe
#undef DAT_200002ff
#undef model_family
#undef rated_capacity
#undef limit_discharge_overcurrent
#undef limit_charge_overcurrent
#undef protection_discharge_overcurrent
#undef protection_charge_overcurrent
#undef model_variant
#undef limit_cell_overvoltage
#undef recovery_cell_overvoltage
#undef protection_cell_overvoltage
#define g_20000162 (*(uchar *)(((uchar *)&settings_20000162) + 0))
#define DAT_200014d3 (*(uchar *)(((uchar *)snapshots) + 1))
#define DAT_200014f4 (*(uchar *)(((uchar *)snapshots) + 34))
#define DAT_2000150a (*(ushort *)(((uchar *)snapshots) + 56))
#define DAT_2000150c (*(ushort *)(((uchar *)snapshots) + 58))
#define DAT_2000150e (*(ushort *)(((uchar *)snapshots) + 60))
#define DAT_20001510 (*(uchar *)(((uchar *)snapshots) + 62))
#define DAT_20001512 (*(ushort *)(((uchar *)snapshots) + 64))
#define DAT_20001514 (*(ushort *)(((uchar *)snapshots) + 66))
#define DAT_20001516 (*(ushort *)(((uchar *)snapshots) + 68))
#define DAT_20001518 (*(uchar *)(((uchar *)snapshots) + 70))
#define DAT_20001519 (*(uchar *)(((uchar *)snapshots) + 71))
#define DAT_2000151a (*(uchar *)(((uchar *)snapshots) + 72))
#define DAT_2000151b (*(uchar *)(((uchar *)snapshots) + 73))
#define DAT_2000151c (*(uchar *)(((uchar *)snapshots) + 74))
#define DAT_2000151d (*(uchar *)(((uchar *)snapshots) + 75))
#define DAT_2000151e (*(uchar *)(((uchar *)snapshots) + 76))
#define DAT_2000151f (*(uchar *)(((uchar *)snapshots) + 77))
#define DAT_20001520 (*(uchar *)(((uchar *)snapshots) + 78))
#define g_200014D2 (*(uchar *)(((uchar *)snapshots) + 0))
void snapshot_initialize(int index) {
    uchar i;
    BmsSnapshot *p;
    (p = &snapshots[index])->active = 0;
    p->cell_count = g_20000162;
    for (i = 0; i < p->cell_count; i++)
        p->cells[i] = 0;
    i = 5;
    p->temp_count = i;
    for (i = 0; i < p->temp_count; i++)
        p->temperatures[i] = 0xaaa;
    snapshots[index].voltage = 0;
    snapshots[index].current = 0;
    snapshots[index].soc = 0;
    snapshots[index].config = 3;
    snapshots[index].capacity = 0;
    snapshots[index].cycles = 0;
    snapshots[index].rated = 0;
    snapshots[index].flags[0] = 0;
    snapshots[index].flags[1] = 0;
    snapshots[index].flags[2] = 0;
    snapshots[index].flags[3] = 0;
    snapshots[index].flags[4] = 0;
    snapshots[index].flags[5] = 0;
    snapshots[index].flags[6] = 0;
    snapshots[index].flags[7] = 0;
    snapshots[index].flags[8] = 0;
    coordination_initialize_registers();
}
#undef g_20000162
#undef DAT_200014d3
#undef DAT_200014f4
#undef DAT_2000150a
#undef DAT_2000150c
#undef DAT_2000150e
#undef DAT_20001510
#undef DAT_20001512
#undef DAT_20001514
#undef DAT_20001516
#undef DAT_20001518
#undef DAT_20001519
#undef DAT_2000151a
#undef DAT_2000151b
#undef DAT_2000151c
#undef DAT_2000151d
#undef DAT_2000151e
#undef DAT_2000151f
#undef DAT_20001520
#undef g_200014D2
#define g_20001094 (*(ushort *)(((uchar *)settings_20001094) + 0))
#define g_2000119E (*(ushort *)(((uchar *)settings_2000119E) + 0))
#define DAT_20000277 settings_20000277
#define DAT_20000278 settings_20000278
#define DAT_200014d3 (*(uchar *)(((uchar *)snapshots) + 1))
#define DAT_200014f4 (*(uchar *)(((uchar *)snapshots) + 34))
#define DAT_20000148 settings_20000148
#define g_200014D2 (*(uchar *)(((uchar *)snapshots) + 0))
#define DAT_2000150a (*(ushort *)(((uchar *)snapshots) + 56))
#define DAT_20000110 settings_20000110
#define DAT_2000150c (*(ushort *)(((uchar *)snapshots) + 58))
#define DAT_2000016c settings_2000016C
#define DAT_2000150e (*(ushort *)(((uchar *)snapshots) + 60))
#define DAT_20001510 (*(uchar *)(((uchar *)snapshots) + 62))
#define DAT_20000164 settings_20000164
#define DAT_20001512 (*(ushort *)(((uchar *)snapshots) + 64))
#define DAT_200000a2 settings_200000A2
#define DAT_20001514 (*(ushort *)(((uchar *)snapshots) + 66))
#define rated_capacity settings_20000182
#define DAT_20001516 (*(ushort *)(((uchar *)snapshots) + 68))
#define DAT_20000271 settings_20000271
#define DAT_20001518 (*(uchar *)(((uchar *)snapshots) + 70))
#define DAT_20000272 settings_20000272
#define DAT_20001519 (*(uchar *)(((uchar *)snapshots) + 71))
#define DAT_20000273 settings_20000273
#define DAT_2000151a (*(uchar *)(((uchar *)snapshots) + 72))
#define DAT_2000050d settings_2000050D
#define DAT_2000151b (*(uchar *)(((uchar *)snapshots) + 73))
#define DAT_20000274 settings_20000274
#define DAT_2000151c (*(uchar *)(((uchar *)snapshots) + 74))
#define DAT_200000ce settings_200000CE
#define DAT_2000151d (*(uchar *)(((uchar *)snapshots) + 75))
#define DAT_2000151e (*(uchar *)(((uchar *)snapshots) + 76))
#define DAT_2000151f (*(uchar *)(((uchar *)snapshots) + 77))
#define DAT_20001520 (*(uchar *)(((uchar *)snapshots) + 78))
#define DAT_200014d4 (*(ushort *)(((uchar *)snapshots) + 2))
#define DAT_200014f6 (*(ushort *)(((uchar *)snapshots) + 36))

void snapshot_publish(void)

{
    uint snapshot_value;
    int current_word;

    if ((DAT_20000277 == '\0') && (DAT_20000278 == '\0')) {
        g_200014D2 = 0;
    } else {
        g_200014D2 = 1;
    }
    for (snapshot_value = 0; snapshot_value < DAT_200014d3;
         snapshot_value = snapshot_value + 1 & 0xff) {
        (&DAT_200014d4)[snapshot_value] = (&g_20001094)[snapshot_value];
    }
    for (snapshot_value = 0; snapshot_value < DAT_200014f4;
         snapshot_value = snapshot_value + 1 & 0xff) {
        (&DAT_200014f6)[snapshot_value] = (&g_2000119E)[snapshot_value];
    }
    if (DAT_20000148 > 0.0f && DAT_20000148 < 5000.0f) {
        current_word = __aeabi_f2iz((DAT_20000148 + 50.0f) / 10.0f);
        DAT_2000150a = (undefined2)current_word;
    } else if (DAT_20000148 < 0.0f && DAT_20000148 > -5000.0f) {
        current_word = __aeabi_f2iz((DAT_20000148 - 50.0f) / 10.0f);
        DAT_2000150a = (undefined2)current_word;
    } else {
        current_word = __aeabi_f2iz(DAT_20000148 / 10.0f);
        DAT_2000150a = (undefined2)current_word;
    }
    DAT_2000150c = DAT_20000110;
    snapshot_value = __aeabi_f2uiz(DAT_2000016c);
    DAT_2000150e = (undefined2)snapshot_value;
    DAT_20001510 = 3;
    snapshot_value = __aeabi_f2uiz(DAT_20000164);
    DAT_20001512 = (undefined2)snapshot_value;
    DAT_20001514 = DAT_200000a2;
    DAT_20001516 = rated_capacity;
    DAT_20001518 = DAT_20000271;
    DAT_20001519 = DAT_20000272;
    DAT_2000151a = DAT_20000273;
    DAT_2000151b = DAT_2000050d;
    DAT_2000151c = DAT_20000274;
    DAT_2000151d = (undefined1)DAT_200000ce;
    DAT_2000151e = (undefined1)((ushort)DAT_200000ce >> 8);
    DAT_2000151f = DAT_20000277;
    DAT_20001520 = DAT_20000278;
    coordination_update_registers();
    return;
}

#undef g_20001094
#undef g_2000119E
#undef DAT_20000277
#undef DAT_20000278
#undef DAT_200014d3
#undef DAT_200014f4
#undef DAT_20000148
#undef g_200014D2
#undef DAT_2000150a
#undef DAT_20000110
#undef DAT_2000150c
#undef DAT_2000016c
#undef DAT_2000150e
#undef DAT_20001510
#undef DAT_20000164
#undef DAT_20001512
#undef DAT_200000a2
#undef DAT_20001514
#undef rated_capacity
#undef DAT_20001516
#undef DAT_20000271
#undef DAT_20001518
#undef DAT_20000272
#undef DAT_20001519
#undef DAT_20000273
#undef DAT_2000151a
#undef DAT_2000050d
#undef DAT_2000151b
#undef DAT_20000274
#undef DAT_2000151c
#undef DAT_200000ce
#undef DAT_2000151d
#undef DAT_2000151e
#undef DAT_2000151f
#undef DAT_20001520
#undef DAT_200014d4
#undef DAT_200014f6
#define DAT_20001576 (*(ushort *)(((uchar *)settings_2000156E) + 8))
#define DAT_20001574 (*(ushort *)(((uchar *)settings_2000156E) + 6))
#define DAT_20001572 (*(ushort *)(((uchar *)settings_2000156E) + 4))
#define DAT_20001570 (*(ushort *)(((uchar *)settings_2000156E) + 2))
#define DAT_2000157a (*(ushort *)(((uchar *)settings_20001578) + 2))
#define DAT_2000157c (*(ushort *)(((uchar *)settings_20001578) + 4))
#define DAT_2000157e (*(ushort *)(((uchar *)settings_20001578) + 6))
#define DAT_20001580 (*(ushort *)(((uchar *)settings_20001578) + 8))
#define DAT_20001582 (*(ushort *)(((uchar *)settings_20001578) + 10))
#define DAT_20001584 (*(ushort *)(((uchar *)settings_20001578) + 12))
#define DAT_20001586 (*(ushort *)(((uchar *)settings_20001578) + 14))
#define DAT_20001588 (*(ushort *)(((uchar *)settings_20001578) + 16))
#define DAT_2000158a (*(ushort *)(((uchar *)settings_20001578) + 18))
#define DAT_2000158c (*(ushort *)(((uchar *)settings_20001578) + 20))
#define DAT_2000158e (*(ushort *)(((uchar *)settings_20001578) + 22))
#define DAT_20001590 (*(ushort *)(((uchar *)settings_20001578) + 24))
#define g_2000156E (*(ushort *)(((uchar *)settings_2000156E) + 0))
#define g_20001578 (*(ushort *)(((uchar *)settings_20001578) + 0))

void settings_clear_counters(void)

{
    uchar event_index;

    DAT_20001576 = 0;
    DAT_20001574 = 0;
    DAT_20001572 = 0;
    DAT_20001570 = 0;
    g_2000156E = 0;
    eeprom_write_word(100, 0);
    eeprom_write_word(0x66, 0);
    eeprom_write_word(0x68, 0);
    eeprom_write_word(0x6a, 0);
    eeprom_write_word(0x6c, 0);
    g_20001578 = 0;
    DAT_2000157a = 0;
    DAT_2000157c = 0;
    DAT_2000157e = 0;
    DAT_20001580 = 0;
    DAT_20001582 = 0;
    DAT_20001584 = 0;
    DAT_20001586 = 0;
    DAT_20001588 = 0;
    DAT_2000158a = 0;
    DAT_2000158c = 0;
    DAT_2000158e = 0;
    DAT_20001590 = 0;
    event_index = 0;
    do {
        eeprom_write_word(event_index * 2 + 0x500, 0);
        event_index = event_index + 1 & 0xff;
    } while (event_index < 0xd);
    return;
}

#undef DAT_20001576
#undef DAT_20001574
#undef DAT_20001572
#undef DAT_20001570
#undef DAT_2000157a
#undef DAT_2000157c
#undef DAT_2000157e
#undef DAT_20001580
#undef DAT_20001582
#undef DAT_20001584
#undef DAT_20001586
#undef DAT_20001588
#undef DAT_2000158a
#undef DAT_2000158c
#undef DAT_2000158e
#undef DAT_20001590
#undef g_2000156E
#undef g_20001578
