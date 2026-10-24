#ifndef BMS_SETTINGS_TYPES_H
#define BMS_SETTINGS_TYPES_H
typedef unsigned int uint;
typedef unsigned short ushort, undefined2;
typedef unsigned char uchar, byte, undefined1;
typedef struct {
    uchar active, cell_count;
    ushort cells[16];
    uchar temp_count, pad23;
    ushort temperatures[10];
    ushort voltage, current, soc;
    uchar config, pad3F;
    ushort capacity, cycles, rated;
    uchar flags[9], pad4F;
} BmsSnapshot;
extern uchar settings_200002F8[];
extern ushort settings_20001522[];
extern ushort settings_20001552[];
extern ushort settings_2000153C[];
extern uchar settings_20000077[];
extern ushort settings_200000FA;
extern ushort settings_200000F8;
extern ushort settings_20000182;
extern const uchar c_080304FA[];
extern const uchar c_08030514[];
extern const uchar c_08030530[];
extern uchar settings_20000162;
extern BmsSnapshot snapshots[];
extern ushort settings_20001094[];
extern ushort settings_2000119E[];
extern volatile uchar settings_20000277;
extern volatile uchar settings_20000278;
extern float settings_20000148;
extern ushort settings_20000110;
extern float settings_2000016C;
extern float settings_20000164;
extern ushort settings_200000A2;
extern uchar settings_20000271;
extern uchar settings_20000272;
extern uchar settings_20000273;
extern uchar settings_2000050D;
extern uchar settings_20000274;
extern ushort settings_200000CE;
extern ushort settings_2000156E[];
extern ushort settings_20001578[];
#include "../../include/bms_eeprom.h"
void __aeabi_memcpy(void *, const void *, uint);
void coordination_initialize_registers(void);
void coordination_update_registers(void);
int __aeabi_f2iz(float);
uint __aeabi_f2uiz(float);
#endif
