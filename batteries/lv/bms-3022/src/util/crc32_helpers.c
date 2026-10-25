/* at: bit_reflect 0x08015160 */
/* at: crc32_init 0x08015158 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* Shared vendor unit, recompiled and linked at the battery addresses. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/schedule.h"

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
