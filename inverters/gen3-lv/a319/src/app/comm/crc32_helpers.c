// clang-format off
/* at: crc32_init 0x08027964 */
/* at: bit_reflect 0x0802796C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* crc32_helpers.c: CRC-32 seed and bit reflection helpers.
   crc32_init is used by the battery firmware relay. bit_reflect is used by the unused
   table initializer in control/scheduler.c; this unit owns no data.
   match: keep this compiler boundary. Moving bit_reflect into scheduler.c changes
   crc32_table_init's saved registers (r4-r7 instead of the image's r4-r8).
   name: descriptive; the original vendor filename is not known. */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/schedule.h"

/* Starts a CRC-32: *crc = 0xFFFFFFFF. */
void crc32_init(uint *crc) {

    *crc = 0xffffffff;
    return;
}

/* Reverses the low n bits of v. */
uint bit_reflect(uint value, int bit_count) {
    uint reversed;
    int bit_index;

    reversed = 0;
    for (bit_index = 0; bit_index < bit_count; bit_index = bit_index + 1) {
        if ((value & 1) != 0) {
            reversed = reversed | 1 << ((bit_count + -1) - bit_index & 0xffU);
        }
        value = value >> 1;
    }
    return reversed;
}
