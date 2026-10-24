/* I share this matched caller view only where both unsigned halfword arguments agree.
   match: wider, signed and old-style caller declarations stay local. */
#ifndef BMS_EEPROM_H
#define BMS_EEPROM_H
#include "ghidra_types.h"
uint eeprom_read_word(ushort);
void eeprom_write_word(ushort, ushort);
#endif
