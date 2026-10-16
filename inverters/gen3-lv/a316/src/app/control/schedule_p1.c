// clang-format off
/* at: crc32_init 0x08027474 */
/* at: bit_reflect 0x0802747C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* schedule_p1.c: the first two CRC-32 helpers.

   Entry points: crc32_init (bms_fw_update_process), bit_reflect (crc32_table_init, which has no
   caller). Data: none.
   Neighbours: measure.c before, schedule_p3.c after (crc32_table_init, crc32_update).

   name: descriptive (map/filenames.tsv). It cannot be joined with schedule_p3.c: with
   bit_reflect earlier in the same file armcc saves r4-r7 in crc32_table_init, the image saves
   r4-r8. So this object may be a crc32.c of its own. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/schedule.h"

/* Starts a CRC-32: *crc = 0xFFFFFFFF. */
void crc32_init(undefined4 *crc) {

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
