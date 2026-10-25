// clang-format off
/* at: calc_checksum_xor_index 0x08025028 */
/* at: min3 0x08025054, max3 0x08025072 */
/* at: median3 0x08025090, clamp 0x080250BE */
/* at: div_u16_safe 0x080250D8 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* util.c: small arithmetic helpers. None of them has a caller or a pointer anywhere in the
   image; armlink kept them because the project was linked without --split_sections. So this
   whole file is dead code.

   Entry points: calc_checksum_xor_index, min3, max3, median3, clamp, div_u16_safe (all unused).
   Data: none.
   Neighbours: board.c panel-output functions before it, monitor.c after it (isqrt_div at 0x080250E8 is the first
   function of monitor.c, not of this file: fault_set at 0x08025136 is 2 mod 4, so no file starts
   there).

   name: likely. A generic helper file; the original name is not known (map/filenames.tsv). Four
   parts joined, the joined file compiles to the same bytes. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/measure.h"
#include "../include/sys.h"

/* Dead. Sums buf[i] ^ i for i from first to last (i counted as a byte) and returns the sum, or
   0xFFFF when the sum is 0 or does not fit 16 bits. */
ushort calc_checksum_xor_index(uchar *buffer_addr, int first_index, int last_index) {
    int index;
    int sum;

    sum = 0;
    for (index = first_index; index <= last_index; index = (uchar)(index + 1)) {
        sum = sum + (buffer_addr[index] ^ index);
    }
    if (sum == 0 || (uint)sum >= 0x10000) {
        sum = 0xffff;
    }
    return (ushort)sum;
}

/* Dead. The smallest of three ints. */
int min3(int first, int second, int third) {

    if (first <= second && first <= third) {
        return first;
    }
    if (second <= first && second <= third) {
        return second;
    }
    return third;
}

/* Dead. The largest of three ints. */
int max3(int first, int second, int third) {

    if (first >= second && first >= third) {
        return first;
    }
    if (second >= first && second >= third) {
        return second;
    }
    return third;
}

/* Dead. The middle one of three ints. */
int median3(int first, int second, int third) {

    if (first >= second && first <= third || first <= second && first >= third) {
        return first;
    }
    if (second >= first && second <= third || second <= first && second >= third) {
        return second;
    }
    return third;
}

/* Dead. Returns v (the second argument) limited to lo..hi (the first and third). */
int clamp(int lower, int value, int upper) {

    if (value >= lower && value <= upper) {
        return value;
    }
    if (value < lower) {
        return lower;
    }
    return upper;
}

/* Dead. a / b truncated to 16 bits, or 0 when b is 0. */
uint div_u16_safe(uint dividend, uint divisor) {
    uint unused_quotient;

    if (divisor == 0) {
        return 0;
    } else {
        return dividend / divisor & 0xffff;
    }
}
