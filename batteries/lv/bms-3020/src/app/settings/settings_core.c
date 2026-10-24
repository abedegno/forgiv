#include "../../include/bms_settings_constants.h"
#include "../../include/bms_thresholds.h"
/* Settings load from EEPROM, defaults and model temperature limits. */
/* at: settings_initialize_defaults 0x0801739C, settings_load 0x080174B0, settings_save_limit_bank 0x080179EA, settings_startup 0x08017C5A */
/* at: g_2000005A 0x2000005A, g_2000005B 0x2000005B, g_2000005C 0x2000005C, g_20000077 0x20000077, g_20000090 0x20000090, g_20000094 0x20000094, g_20000096 0x20000096, g_200000A2 0x200000A2, g_200000A8 0x200000A8, g_200000A9 0x200000A9, g_200000EE 0x200000EE, g_200000EF 0x200000EF, g_200000F8 0x200000F8, g_200000FA 0x200000FA, g_2000014C 0x2000014C, g_20000150 0x20000150, g_20000154 0x20000154, g_20000162 0x20000162, g_20000164 0x20000164, g_20000168 0x20000168, g_2000016C 0x2000016C, g_20000170 0x20000170, g_20000174 0x20000174, g_20000178 0x20000178, g_20000182 0x20000182, g_20000184 0x20000184, g_20000185 0x20000185, g_20000186 0x20000186, g_20000187 0x20000187, g_20000188 0x20000188, g_2000018A 0x2000018A, g_20000271 0x20000271, g_20000272 0x20000272, g_20000273 0x20000273, g_20000274 0x20000274, g_20000275 0x20000275, g_20000276 0x20000276, g_20000277 0x20000277, g_20000278 0x20000278, g_200002F8 0x200002F8, g_2000050D 0x2000050D, g_20001522 0x20001522, g_2000153C 0x2000153C, g_20001552 0x20001552, g_2000156E 0x2000156E, g_20001578 0x20001578 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow settings defaults, model presets, persistence and startup here. Calibration and the control tasks consume the shared fields.
   Entry examples: settings_initialize_defaults, settings_load, settings_save_limit_bank.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
extern uchar g_2000005A;
extern uchar g_2000005B;
extern uchar g_2000005C;
extern uchar g_20000077[];
extern ushort g_20000090;
extern ushort g_20000094;
extern ushort g_20000096;
extern ushort g_200000A2;
extern uchar g_200000A8;
extern uchar g_200000A9;
extern uchar g_200000EE;
extern uchar g_200000EF;
extern float g_2000014C;
extern float g_20000150;
extern float g_20000154;
extern uchar g_20000162;
extern float g_20000164;
extern float g_20000168;
extern float g_2000016C;
extern float g_20000170;
extern float g_20000174;
extern float g_20000178;
extern uchar g_20000184;
extern uchar g_20000185;
extern uchar g_20000186;
extern uchar g_20000187;
extern ushort g_20000188;
extern ushort g_2000018A;
extern volatile uchar g_20000271;
extern uchar g_20000272;
extern volatile uchar g_20000273;
extern uchar g_20000274;
extern uchar g_20000275;
extern volatile uchar g_20000276;
extern volatile uchar g_20000277;
extern volatile uchar g_20000278;
extern uchar g_200002F8[];
extern uchar g_2000050D;
extern ushort g_2000153C[];
extern ushort g_2000156E[];
extern ushort g_20001578[];
void settings_clear_counters(void);
uint eeprom_read_word(ushort);
uint eeprom_write_word(ushort, ushort);
uint eeprom_write_value32(uint, float);
float eeprom_read_value32(uint);
void GPIO_ResetBits(void *, ushort);
void runtime_timer_start(int, undefined4);
void runtime_timer_stop(int);
void settings_load_limit_bank(int);
void settings_restore_factory_limits(void);
void snapshot_initialize(int);
void snapshot_publish(void);
void coordination_record_event(int);
void board_set_pc9(int);
void settings_initialize_defaults();
void settings_startup();
void settings_save_limit_bank(int factory_bank);
void settings_load();

#define model_family (g_20000077[0])
#define model_variant (g_20000077[1])

void settings_initialize_defaults() {
    uchar model_word_index;

    model_word_index = 0;
    eeprom_write_word(BMS_EEPROM_RESET_MARKER, 0);
    if ((eeprom_read_word(0)) != BMS_SETTINGS_MARKER) {
        eeprom_write_word(2, 0);
        eeprom_write_word(4, 1);
        eeprom_write_word(6, 0x11c6);
        eeprom_write_word(8, 16000);
        eeprom_write_word(10, 0);
        eeprom_write_word(0xc, 16000);
        eeprom_write_word(0xe, 0);
        eeprom_write_word(0x10, 0);
        eeprom_write_word(0x14, 0);
        eeprom_write_word(0x16, 0);
        eeprom_write_word(0x18, 0x60);
        eeprom_write_word(0x1a, 0);
        eeprom_write_word(0x1c, 0xce4);
        eeprom_write_word(0x1e, 0xce4);
        eeprom_write_value32(0x24, 1.0f);
        eeprom_write_value32(0x28, 1.0f);
        eeprom_write_value32(0x2c, 1.0f);
        eeprom_write_word(0x30, 0);
        eeprom_write_word(0x32, 0x10);
        eeprom_write_word(0x34, 0);
        eeprom_write_word(0x38, 0x10);
        eeprom_write_word(0x3a, 0);
        do {
            eeprom_write_word((ushort)(model_word_index << 1) + 0x3c,
                              ((uint)(&model_family)[model_word_index * 2] << 8) +
                                  (&model_variant)[model_word_index * 2]);
            model_word_index = model_word_index + 1 & 0xff;
        } while (model_word_index < 10);
        settings_clear_counters();
        eeprom_write_word(0, BMS_SETTINGS_MARKER);
        return;
    }
    return;
}
#undef model_family
#undef model_variant

#define model_family (g_20000077[0])
#define model_variant (g_20000077[1])
#define DAT_20000090 g_20000090
#define DAT_20000094 g_20000094
#define DAT_20000096 g_20000096
#define DAT_200000a2 g_200000A2
#define DAT_200000a8 g_200000A8
#define DAT_200000a9 g_200000A9
extern ushort g_200000F8;
#define model_code g_200000F8
extern ushort g_200000FA;
#define model_profile g_200000FA
#define DAT_2000014c g_2000014C
#define DAT_20000150 g_20000150
#define DAT_20000154 g_20000154
#define DAT_20000162 g_20000162
#define DAT_20000164 g_20000164
#define DAT_20000168 g_20000168
#define DAT_2000016c g_2000016C
#define DAT_20000170 g_20000170
#define DAT_20000174 g_20000174
#define DAT_20000178 g_20000178
extern ushort g_20000182;
#define rated_capacity g_20000182
#define DAT_20000186 g_20000186
#define DAT_20000187 g_20000187
#define DAT_20000188 g_20000188
#define DAT_2000018a g_2000018A
#define DAT_20000275 g_20000275
#define DAT_20000276 g_20000276
#define DAT_2000050d g_2000050D
extern uchar g_20001522[];
#define limit_charge_overcurrent BMS_LIMIT_FIELD(g_20001522, charge_overcurrent)
#define limit_discharge_overcurrent BMS_LIMIT_FIELD(g_20001522, discharge_overcurrent)
extern uchar g_20001552[];
#define protection_charge_overcurrent BMS_PROTECTION_FIELD(g_20001552, charge_overcurrent)
#define protection_discharge_overcurrent BMS_PROTECTION_FIELD(g_20001552, discharge_overcurrent)
#define DAT_20001570 (g_2000156E[1])
#define DAT_20001572 (g_2000156E[2])
#define DAT_20001574 (g_2000156E[3])
#define DAT_20001576 (g_2000156E[4])
#define g_2000156E_v (g_2000156E[0])
#define DAT_2000157a (g_20001578[1])
#define DAT_2000157c (g_20001578[2])
#define DAT_2000157e (g_20001578[3])
#define DAT_20001580 (g_20001578[4])
#define DAT_20001582 (g_20001578[5])
#define DAT_20001584 (g_20001578[6])
#define DAT_20001586 (g_20001578[7])
#define DAT_20001588 (g_20001578[8])
#define DAT_2000158a (g_20001578[9])
#define DAT_2000158c (g_20001578[10])
#define DAT_2000158e (g_20001578[11])
#define DAT_20001590 (g_20001578[12])
#define g_20001578_v (g_20001578[0])

void settings_load() {
    uint stored_word;
    uchar model_word_index;
    uint limit_diagnostic;

    model_word_index = 0;
    DAT_20000187 = (undefined1)(eeprom_read_word(2));
    DAT_200000a8 = (undefined1)(eeprom_read_word(4));
    DAT_2000016c = ((float)((eeprom_read_word(6))));
    DAT_20000178 = DAT_2000016c;
    DAT_20000164 = ((float)((eeprom_read_word(8))));
    DAT_200000a2 = (undefined2)(eeprom_read_word(10));
    stored_word = eeprom_read_word(0xc);
    rated_capacity = (ushort)stored_word;
    if (stored_word == 0) {
        rated_capacity = 0x157c;
    }
    DAT_20000168 = ((float)((eeprom_read_word(0xe))));
    DAT_200000a9 = (undefined1)(eeprom_read_word(0x10));
    DAT_20000188 = (undefined2)(eeprom_read_word(0x14));
    DAT_2000018a = (undefined2)(eeprom_read_word(0x16));
    DAT_20000186 = (byte)(eeprom_read_word(0x18));
    stored_word = eeprom_read_word(0x1a);
    DAT_20000090 = (undefined2)stored_word;
    if (stored_word > 10) {
        DAT_20000090 = 0;
    }
    if ((uint)(DAT_20000186 - 0x4b) >= 0x16) {
        DAT_20000186 = 0x60;
    }
    stored_word = eeprom_read_word(0x1c);
    DAT_20000094 = (undefined2)stored_word;
    if (stored_word - 0xcb2 >= 101) {
        DAT_20000094 = 0xce4;
    }
    stored_word = eeprom_read_word(0x1e);
    DAT_20000096 = (undefined2)stored_word;
    if (stored_word - 0xcb2 >= 101) {
        DAT_20000096 = 0xce4;
    }
    DAT_2000014c = eeprom_read_value32(0x24);
    if (DAT_2000014c < 0.8f || DAT_2000014c > 1.5f) {
        DAT_2000014c = 1.0f;
    }
    DAT_20000150 = eeprom_read_value32(0x28);
    if (DAT_20000150 < 0.8f || DAT_20000150 > 1.5f) {
        DAT_20000150 = 1.0f;
    }
    DAT_20000154 = eeprom_read_value32(0x2c);
    if (DAT_20000154 < 0.8f || DAT_20000154 > 1.5f) {
        DAT_20000154 = 1.0f;
    }
    DAT_20000275 = (undefined1)(eeprom_read_word(0x30));
    DAT_2000050d = (byte)(eeprom_read_word(0x38)) & 0x79;
    DAT_20000276 = (undefined1)(eeprom_read_word(0x3a));
    stored_word = eeprom_read_word(0x32);
    DAT_20000162 = (undefined1)stored_word;
    if ((stored_word & 0xff) - 8 >= 9) {
        DAT_20000162 = 0x10;
    }
    do {
        stored_word = eeprom_read_word((ushort)(model_word_index << 1) + 0x3c);
        (&model_family)[model_word_index * 2] = (char)(stored_word >> 8);
        (&model_variant)[model_word_index * 2] = (char)stored_word;
        model_word_index = model_word_index + 1 & 0xff;
    } while (model_word_index < 10);
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
    } else {
        model_profile = 0;
        model_code = 2;
        {
            /* match: I retain this unread equality of the two maximum-limit field addresses. It holds
         a register through allocation and emits nothing; without it armcc keeps
         -70 instead of 4 in a register across the model dispatch. */
            short *charge_max = &limit_charge_overcurrent;
            limit_diagnostic = (uint)(charge_max != &protection_charge_overcurrent);
        }
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
    }
    DAT_20001576 = (undefined2)(eeprom_read_word(100));
    DAT_20001574 = (undefined2)(eeprom_read_word(0x66));
    DAT_20001572 = (undefined2)(eeprom_read_word(0x68));
    DAT_20001570 = (undefined2)(eeprom_read_word(0x6a));
    g_2000156E_v = (undefined2)(eeprom_read_word(0x6c));
    g_20001578_v = (undefined2)(eeprom_read_word(0x500));
    DAT_2000157a = (undefined2)(eeprom_read_word(0x502));
    DAT_2000157c = (undefined2)(eeprom_read_word(0x504));
    DAT_2000157e = (undefined2)(eeprom_read_word(0x506));
    DAT_20001580 = (undefined2)(eeprom_read_word(0x508));
    DAT_20001582 = (undefined2)(eeprom_read_word(0x50a));
    DAT_20001584 = (undefined2)(eeprom_read_word(0x50c));
    DAT_20001586 = (undefined2)(eeprom_read_word(0x50e));
    DAT_20001588 = (undefined2)(eeprom_read_word(0x510));
    DAT_2000158a = (undefined2)(eeprom_read_word(0x512));
    DAT_2000158c = (undefined2)(eeprom_read_word(0x514));
    DAT_2000158e = (undefined2)(eeprom_read_word(0x516));
    DAT_20001590 = (undefined2)(eeprom_read_word(0x518));
    if ((((float)((uint)rated_capacity))) < DAT_20000164) {
        if (((model_family == BMS_MODEL_FAMILY_B) && (model_variant == BMS_MODEL_VARIANT_G)) ||
            ((model_family == BMS_MODEL_FAMILY_D && ((model_variant == BMS_MODEL_VARIANT_G ||
                                                      (model_variant == BMS_MODEL_VARIANT_Y)))))) {
            if (DAT_20000164 > 17300.0f) {
                DAT_20000164 = 17300.0f;
            }
        } else if ((model_family == BMS_MODEL_FAMILY_D) &&
                   ((((model_variant == BMS_MODEL_VARIANT_X ||
                       (model_variant == BMS_MODEL_VARIANT_Z)) ||
                      (model_variant == BMS_MODEL_VARIANT_W)) ||
                     (model_variant == BMS_MODEL_VARIANT_F)))) {
            if (DAT_20000164 > 20000.0f) {
                DAT_20000164 = 20000.0f;
            }
        } else {
            DAT_20000164 = ((float)((uint)rated_capacity));
        }
    }
    DAT_20000170 = DAT_20000164;
    DAT_20000174 = DAT_2000016c;
    return;
}
#undef model_family
#undef model_variant
#undef DAT_20000090
#undef DAT_20000094
#undef DAT_20000096
#undef DAT_200000a2
#undef DAT_200000a8
#undef DAT_200000a9
#undef model_code
#undef model_profile
#undef DAT_2000014c
#undef DAT_20000150
#undef DAT_20000154
#undef DAT_20000162
#undef DAT_20000164
#undef DAT_20000168
#undef DAT_2000016c
#undef DAT_20000170
#undef DAT_20000174
#undef DAT_20000178
#undef rated_capacity
#undef DAT_20000186
#undef DAT_20000187
#undef DAT_20000188
#undef DAT_2000018a
#undef DAT_20000275
#undef DAT_20000276
#undef DAT_2000050d
#undef limit_charge_overcurrent
#undef limit_discharge_overcurrent
#undef protection_charge_overcurrent
#undef protection_discharge_overcurrent
#undef DAT_20001570
#undef DAT_20001572
#undef DAT_20001574
#undef DAT_20001576
#undef g_2000156E_v
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
#undef g_20001578_v

#define DAT_200002f8 (g_200002F8[0])
#define DAT_200002f9 (g_200002F8[1])
#define DAT_200002fa (g_200002F8[2])
#define DAT_200002fb (g_200002F8[3])
#define DAT_200002fc (g_200002F8[4])
#define DAT_200002fd (g_200002F8[5])
#define DAT_200002fe (g_200002F8[6])
#define DAT_200002ff (g_200002F8[7])
extern uchar g_20001522[];
#define limit_cell_undervoltage BMS_LIMIT_FIELD(g_20001522, cell_undervoltage)
#define limit_pack_overvoltage BMS_LIMIT_FIELD(g_20001522, pack_overvoltage)
#define limit_pack_undervoltage BMS_LIMIT_FIELD(g_20001522, pack_undervoltage)
#define limit_charge_overcurrent BMS_LIMIT_FIELD(g_20001522, charge_overcurrent)
#define limit_discharge_overcurrent BMS_LIMIT_FIELD(g_20001522, discharge_overcurrent)
#define limit_charge_overtemperature BMS_LIMIT_FIELD(g_20001522, charge_overtemperature)
#define limit_discharge_overtemperature BMS_LIMIT_FIELD(g_20001522, discharge_overtemperature)
#define limit_charge_undertemperature BMS_LIMIT_FIELD(g_20001522, charge_undertemperature)
#define limit_discharge_undertemperature BMS_LIMIT_FIELD(g_20001522, discharge_undertemperature)
#define limit_mosfet_overtemperature BMS_LIMIT_FIELD(g_20001522, mosfet_overtemperature)
#define limit_ambient_overtemperature BMS_LIMIT_FIELD(g_20001522, ambient_overtemperature)
#define limit_ambient_undertemperature BMS_LIMIT_FIELD(g_20001522, ambient_undertemperature)
#define limit_cell_overvoltage BMS_LIMIT_FIELD(g_20001522, cell_overvoltage)
#define recovery_cell_undervoltage BMS_RECOVERY_FIELD(g_2000153C, cell_undervoltage)
#define recovery_pack_overvoltage BMS_RECOVERY_FIELD(g_2000153C, pack_overvoltage)
#define recovery_pack_undervoltage BMS_RECOVERY_FIELD(g_2000153C, pack_undervoltage)
#define recovery_charge_overtemperature BMS_RECOVERY_FIELD(g_2000153C, charge_overtemperature)
#define recovery_discharge_overtemperature BMS_RECOVERY_FIELD(g_2000153C, discharge_overtemperature)
#define recovery_charge_undertemperature BMS_RECOVERY_FIELD(g_2000153C, charge_undertemperature)
#define recovery_discharge_undertemperature                                                        \
    BMS_RECOVERY_FIELD(g_2000153C, discharge_undertemperature)
#define recovery_mosfet_overtemperature BMS_RECOVERY_FIELD(g_2000153C, mosfet_overtemperature)
#define recovery_ambient_overtemperature BMS_RECOVERY_FIELD(g_2000153C, ambient_overtemperature)
#define recovery_ambient_undertemperature BMS_RECOVERY_FIELD(g_2000153C, ambient_undertemperature)
#define recovery_cell_overvoltage BMS_RECOVERY_FIELD(g_2000153C, cell_overvoltage)
extern uchar g_20001552[];
#define protection_cell_undervoltage BMS_PROTECTION_FIELD(g_20001552, cell_undervoltage)
#define protection_pack_overvoltage BMS_PROTECTION_FIELD(g_20001552, pack_overvoltage)
#define protection_pack_undervoltage BMS_PROTECTION_FIELD(g_20001552, pack_undervoltage)
#define protection_charge_overcurrent BMS_PROTECTION_FIELD(g_20001552, charge_overcurrent)
#define protection_discharge_overcurrent BMS_PROTECTION_FIELD(g_20001552, discharge_overcurrent)
#define protection_short_circuit_current BMS_PROTECTION_FIELD(g_20001552, short_circuit_current)
#define protection_charge_overtemperature BMS_PROTECTION_FIELD(g_20001552, charge_overtemperature)
#define protection_discharge_overtemperature                                                       \
    BMS_PROTECTION_FIELD(g_20001552, discharge_overtemperature)
#define protection_charge_undertemperature BMS_PROTECTION_FIELD(g_20001552, charge_undertemperature)
#define protection_discharge_undertemperature                                                      \
    BMS_PROTECTION_FIELD(g_20001552, discharge_undertemperature)
#define protection_mosfet_overtemperature BMS_PROTECTION_FIELD(g_20001552, mosfet_overtemperature)
#define protection_ambient_overtemperature BMS_PROTECTION_FIELD(g_20001552, ambient_overtemperature)
#define protection_ambient_undertemperature                                                        \
    BMS_PROTECTION_FIELD(g_20001552, ambient_undertemperature)
#define protection_cell_overvoltage BMS_PROTECTION_FIELD(g_20001552, cell_overvoltage)

void settings_save_limit_bank(int factory_bank) {
    ushort bank_offset;

    if (factory_bank == 0) {
        bank_offset = 0x80;
    } else {
        bank_offset = 0x200;
    }
    if ((eeprom_read_word(bank_offset)) != BMS_LIMIT_BANK_MARKER) {
        eeprom_write_word(bank_offset + 2, (uint)limit_cell_overvoltage);
        eeprom_write_word(bank_offset + 4, (uint)protection_cell_overvoltage);
        eeprom_write_word(bank_offset + 6, (uint)recovery_cell_overvoltage);
        eeprom_write_word(bank_offset + 8, (uint)DAT_200002f8);
        eeprom_write_word(bank_offset + 10, (uint)limit_cell_undervoltage);
        eeprom_write_word(bank_offset + 0xc, (uint)protection_cell_undervoltage);
        eeprom_write_word(bank_offset + 0xe, (uint)recovery_cell_undervoltage);
        eeprom_write_word(bank_offset + 0x10, (uint)DAT_200002f9);
        eeprom_write_word(bank_offset + 0x12, (uint)limit_pack_overvoltage);
        eeprom_write_word(bank_offset + 0x14, (uint)protection_pack_overvoltage);
        eeprom_write_word(bank_offset + 0x16, (uint)recovery_pack_overvoltage);
        eeprom_write_word(bank_offset + 0x18, (uint)DAT_200002fa);
        eeprom_write_word(bank_offset + 0x1a, (uint)limit_pack_undervoltage);
        eeprom_write_word(bank_offset + 0x1c, (uint)protection_pack_undervoltage);
        eeprom_write_word(bank_offset + 0x1e, (uint)recovery_pack_undervoltage);
        eeprom_write_word(bank_offset + 0x20, (uint)DAT_200002fb);
        eeprom_write_word(bank_offset + 0x22, (uint)limit_charge_overcurrent);
        eeprom_write_word(bank_offset + 0x24, (uint)protection_charge_overcurrent);
        eeprom_write_word(bank_offset + 0x26, (uint)DAT_200002fc);
        eeprom_write_word(bank_offset + 0x28, (uint)limit_discharge_overcurrent);
        eeprom_write_word(bank_offset + 0x2a, (uint)protection_discharge_overcurrent);
        eeprom_write_word(bank_offset + 0x2c, (uint)DAT_200002fd);
        eeprom_write_word(bank_offset + 0x2e, (uint)limit_charge_overtemperature);
        eeprom_write_word(bank_offset + 0x30, (uint)protection_charge_overtemperature);
        eeprom_write_word(bank_offset + 0x32, (uint)recovery_charge_overtemperature);
        eeprom_write_word(bank_offset + 0x34, (uint)limit_discharge_overtemperature);
        eeprom_write_word(bank_offset + 0x36, (uint)protection_discharge_overtemperature);
        eeprom_write_word(bank_offset + 0x38, (uint)recovery_discharge_overtemperature);
        eeprom_write_word(bank_offset + 0x3a, (uint)limit_charge_undertemperature);
        eeprom_write_word(bank_offset + 0x3c, (uint)protection_charge_undertemperature);
        eeprom_write_word(bank_offset + 0x3e, (uint)recovery_charge_undertemperature);
        eeprom_write_word(bank_offset + 0x40, (uint)limit_discharge_undertemperature);
        eeprom_write_word(bank_offset + 0x42, (uint)protection_discharge_undertemperature);
        eeprom_write_word(bank_offset + 0x44, (uint)recovery_discharge_undertemperature);
        eeprom_write_word(bank_offset + 0x46, (uint)limit_mosfet_overtemperature);
        eeprom_write_word(bank_offset + 0x48, (uint)protection_mosfet_overtemperature);
        eeprom_write_word(bank_offset + 0x4a, (uint)recovery_mosfet_overtemperature);
        eeprom_write_word(bank_offset + 0x4c, (uint)limit_ambient_overtemperature);
        eeprom_write_word(bank_offset + 0x4e, (uint)limit_ambient_undertemperature);
        eeprom_write_word(bank_offset + 0x50, (uint)protection_short_circuit_current);
        eeprom_write_word(bank_offset + 0x52, (uint)DAT_200002fe);
        eeprom_write_word(bank_offset + 0x54, (uint)DAT_200002ff);
        eeprom_write_word(bank_offset + 0x70, (uint)protection_ambient_overtemperature);
        eeprom_write_word(bank_offset + 0x72, (uint)recovery_ambient_overtemperature);
        eeprom_write_word(bank_offset + 0x74, (uint)protection_ambient_undertemperature);
        eeprom_write_word(bank_offset + 0x76, (uint)recovery_ambient_undertemperature);
        eeprom_write_word(bank_offset, BMS_LIMIT_BANK_MARKER);
        return;
    }
    return;
}
#undef DAT_200002f8
#undef DAT_200002f9
#undef DAT_200002fa
#undef DAT_200002fb
#undef DAT_200002fc
#undef DAT_200002fd
#undef DAT_200002fe
#undef DAT_200002ff
#undef limit_cell_undervoltage
#undef limit_pack_overvoltage
#undef limit_pack_undervoltage
#undef limit_charge_overcurrent
#undef limit_discharge_overcurrent
#undef limit_charge_overtemperature
#undef limit_discharge_overtemperature
#undef limit_charge_undertemperature
#undef limit_discharge_undertemperature
#undef limit_mosfet_overtemperature
#undef limit_ambient_overtemperature
#undef limit_ambient_undertemperature
#undef limit_cell_overvoltage
#undef recovery_cell_undervoltage
#undef recovery_pack_overvoltage
#undef recovery_pack_undervoltage
#undef recovery_charge_overtemperature
#undef recovery_discharge_overtemperature
#undef recovery_charge_undertemperature
#undef recovery_discharge_undertemperature
#undef recovery_mosfet_overtemperature
#undef recovery_ambient_overtemperature
#undef recovery_ambient_undertemperature
#undef recovery_cell_overvoltage
#undef protection_cell_undervoltage
#undef protection_pack_overvoltage
#undef protection_pack_undervoltage
#undef protection_charge_overcurrent
#undef protection_discharge_overcurrent
#undef protection_short_circuit_current
#undef protection_charge_overtemperature
#undef protection_discharge_overtemperature
#undef protection_charge_undertemperature
#undef protection_discharge_undertemperature
#undef protection_mosfet_overtemperature
#undef protection_ambient_overtemperature
#undef protection_ambient_undertemperature
#undef protection_cell_overvoltage

#define DAT_40010c00 (*(volatile uchar *)0x40010C00)
#define DAT_40011000 (*(volatile uchar *)0x40011000)
#define DAT_40011400 (*(volatile uchar *)0x40011400)
#define DAT_2000005a g_2000005A
#define DAT_2000005b g_2000005B
#define DAT_2000005c g_2000005C
#define DAT_200000ee g_200000EE
#define DAT_200000ef g_200000EF
#define DAT_20000164 g_20000164
#define DAT_2000016c g_2000016C
#define DAT_20000178 g_20000178
#define DAT_20000184 g_20000184
#define DAT_20000185 g_20000185
#define DAT_20000271 g_20000271
#define DAT_20000272 g_20000272
#define DAT_20000273 g_20000273
#define DAT_20000274 g_20000274
#define DAT_20000275 g_20000275
#define DAT_20000276 g_20000276
#define DAT_20000277 g_20000277
#define DAT_20000278 g_20000278
#define DAT_2000050d g_2000050D
extern uchar g_20001522[];
#define limit_pack_overvoltage BMS_LIMIT_FIELD(g_20001522, pack_overvoltage)

static __inline float read_capacity(void) {
    return DAT_20000164;
}

static __forceinline float pct_ratio(float numerator, float denominator) {
    return numerator / denominator;
}
void settings_startup() {
    float scale, numerator, quotient;
    uint startup_value;

    board_set_pc9(1);
    DAT_20000271 = 0;
    DAT_20000272 = 0;
    DAT_20000273 = 0;
    DAT_2000050d = 0;
    DAT_20000274 = 0;
    DAT_20000275 = 0;
    DAT_20000276 = 0;
    DAT_20000277 = 0;
    DAT_20000278 = 0;
    settings_initialize_defaults();
    settings_load();
    settings_restore_factory_limits();
    settings_save_limit_bank(0);
    settings_load_limit_bank(0);
    startup_value = eeprom_read_word(0x92);
    limit_pack_overvoltage = (undefined2)startup_value;
    if (startup_value < 0xe09c) {
        limit_pack_overvoltage = 0xe09c;
        eeprom_write_word(0x92, 0xe09c);
    }
    startup_value = 0;
    do {
        snapshot_initialize(startup_value);
        startup_value = (uchar)(startup_value + 1);
    } while (startup_value == 0);
    startup_value = 0;
    do {
        runtime_timer_stop(startup_value);
        startup_value = startup_value + 1 & 0xff;
    } while (startup_value < 0x1f);
    runtime_timer_start(7, 1);
    runtime_timer_start(8, 0x14);
    runtime_timer_start(0x10, 72000);
    runtime_timer_start(0x15, 400);
    DAT_20000273 &= 0xfe;
    DAT_20000273 &= 0xfb;
    GPIO_ResetBits(&DAT_40011000, 0x100);
    DAT_2000005a = 0;
    GPIO_ResetBits(&DAT_40011400, 4);
    DAT_2000005c = 0;
    GPIO_ResetBits(&DAT_40010c00, 8);
    DAT_2000005b = 0;
    DAT_20000276 |= 8;
    DAT_20000276 |= 0x10;
    DAT_20000184 = (uchar)(DAT_2000016c / DAT_20000164 * 100 + 0.5);
    DAT_20000185 = (uchar)(DAT_20000178 / DAT_20000164 * 100 + 0.5);
    snapshot_publish();
    coordination_record_event(0xc);
    DAT_200000ee = 0;
    DAT_200000ef = 0;
    return;
}
#undef DAT_40010c00
#undef DAT_40011000
#undef DAT_40011400
#undef DAT_2000005a
#undef DAT_2000005b
#undef DAT_2000005c
#undef DAT_200000ee
#undef DAT_200000ef
#undef DAT_20000164
#undef DAT_2000016c
#undef DAT_20000178
#undef DAT_20000184
#undef DAT_20000185
#undef DAT_20000271
#undef DAT_20000272
#undef DAT_20000273
#undef DAT_20000274
#undef DAT_20000275
#undef DAT_20000276
#undef DAT_20000277
#undef DAT_20000278
#undef DAT_2000050d
#undef limit_pack_overvoltage
