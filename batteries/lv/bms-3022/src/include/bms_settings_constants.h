/* I name these values from the model branches and their EEPROM save/read pairs.
   I retain the original literal spellings so their C types stay unchanged. */
#ifndef BMS_SETTINGS_CONSTANTS_H
#define BMS_SETTINGS_CONSTANTS_H
#define BMS_MODEL_FAMILY_B 'B'
#define BMS_MODEL_FAMILY_D 'D'
#define BMS_MODEL_VARIANT_G 'G'
#define BMS_MODEL_VARIANT_Y 'Y'
#define BMS_MODEL_VARIANT_W 'W'
#define BMS_MODEL_VARIANT_X 'X'
#define BMS_MODEL_VARIANT_Z 'Z'
#define BMS_MODEL_VARIANT_F 'F'
#define BMS_MODEL_CAPACITY_STANDARD 16000
#define BMS_MODEL_CAPACITY_EXTENDED 0x48a8
#define BMS_LOW_CAPACITY_CUTOFF 6000
#define BMS_SETTINGS_MARKER 0x9876
#define BMS_LIMIT_BANK_MARKER 0x346a
#define BMS_EEPROM_RESET_MARKER 0x7ff5
#define BMS_EEPROM_CHARGE_LIMIT 0xa2
#define BMS_EEPROM_CHARGE_PROTECTION 0xa4
#define BMS_EEPROM_DISCHARGE_LIMIT 0xa8
#define BMS_EEPROM_DISCHARGE_PROTECTION 0xaa
#endif
