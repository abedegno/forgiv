#include "util.h"
// clang-format off
// cflags: -Ooff
/* Small helpers compiled without optimisation (-Ooff): every argument is stored in the frame and
 * reloaded. A checksum, min/max of floats, a median, clamps, a square root, the debounce counters
 * and a hex digit.
 * Entry points: the helpers below. In use: max3f, min3f, min7f (task 4's limits), median3ul
 * (task 1's frequency), limitf, limitu, limiti, udiv32_16. Never called in the image (dead code):
 * checksum_xor, clampu, cal_gain_or_unity, labs32, limit_sym, isqrt_div, debounce_out,
 * debounce_in, debounce_gt, debounce_lt and hexdigit.
 * Data: none.
 * name: descriptive (was util_ooff.c; map/filenames.tsv). Header util.h.
 * match:
 * This file was built by a later compiler than the rest of the application (16.9.x or 17.x, not
 * 15.12): 15.12 at -Ooff gives the arguments their frame slots in order and pads a long local,
 * where these frames hold the 32-bit values first (checksum_xor, isqrt_div), and 15.12 reuses the
 * pointer it compared through for the store where these reload it (limitu, limiti). The other
 * functions here come out the same with either compiler. docs/notes.md.
 * at: 0x3F0F69 .. 0x3F1159 */
// clang-format on

/* Sum of buf[i] XOR i over from..to; 0 or more than 16 bits gives 0xFFFF. Unused. */
unsigned int checksum_xor(unsigned int *buf, unsigned int from, unsigned int to) {
    unsigned long sum;
    unsigned int i;
    sum = 0;
    for (i = from; i <= to; i++)
        sum += buf[i] ^ i;
    if (sum == 0 || sum > 0xFFFF)
        sum = 0xFFFF;
    return sum;
}

/* The largest of three floats. */
float max3f(float a, float b, float c) {
    float m;
    m = a;
    if (b > m)
        m = b;
    if (c > m)
        m = c;
    return m;
}

/* The smallest of three floats. */
float min3f(float a, float b, float c) {
    float m;
    m = a;
    if (b < m)
        m = b;
    if (c < m)
        m = c;
    return m;
}

/* The smallest of seven floats (task 4's power limits). */
float min7f(float a, float b, float c, float d, float e, float f, float g) {
    float m;
    m = a;
    if (m > b)
        m = b;
    if (m > c)
        m = c;
    if (m > d)
        m = d;
    if (m > e)
        m = e;
    if (m > f)
        m = f;
    if (m > g)
        m = g;
    return m;
}

/* The median of three unsigned longs (task 1: the last three grid periods). */
unsigned long median3ul(unsigned long a, unsigned long b, unsigned long c) {
    if ((a >= b && a <= c) || (a <= b && a >= c))
        return a;
    if ((b >= a && b <= c) || (b <= a && b >= c))
        return b;
    return c;
}

/* Clamps *x to min..max in place (max is tested first). */
void limitf(float *x, float max, float min) {
    if (*x > max)
        *x = max;
    if (*x < min)
        *x = min;
}

/* Clamps an unsigned *x to min..max in place. */
void limitu(unsigned int *x, unsigned int max, unsigned int min) {
    if (*x > max)
        *x = max;
    if (*x < min)
        *x = min;
}

/* Clamps a signed *x to min..max in place. */
void limiti(int *x, int max, int min) {
    if (*x > max)
        *x = max;
    if (*x < min)
        *x = min;
}

/* b clamped to a..c. Unused. */
unsigned int clampu(unsigned int a, unsigned int b, unsigned int c) {
    if (b >= a && b <= c)
        return b;
    if (b < a)
        return a;
    return c;
}

/* A calibration gain, or 1024 (1.0) when it is outside 719..1126. Unused. */
unsigned int cal_gain_or_unity(unsigned int x) {
    if (x < 719 || x > 1126)
        return 0x400;
    return x;
}

/* The absolute value of a long. Unused. */
long labs32(long x) {
    if (x >= 0)
        return x;
    return -x;
}

/* x clamped to -lim..lim. Unused. */
int limit_sym(int x, int lim) {
    if (x > lim)
        x = lim;
    else if (x < -lim)
        x = -lim;
    return x;
}

/* Integer square root of x / d: a first guess by range, then five Newton steps. Unused. */
unsigned long isqrt_div(unsigned long x, unsigned int d) {
    unsigned long r;
    unsigned int i;
    if (x == 0 || d == 0)
        return 0;
    x = x / d;
    if (x <= 0x400000)
        r = ((x + 0x200) >> 10) + 63;
    else if (x <= 0x8000000)
        r = ((x + 0x800) >> 12) + 255;
    else
        r = ((x + 0x8000) >> 16) + 1023;
    for (i = 0; i < 5; i++)
        r = ((x + (r >> 1)) / r + r) >> 1;
    return r;
}

/* x / d, or 0 when d is 0 (task 4's 10-minute voltage mean). */
unsigned int udiv32_16(unsigned long x, unsigned int d) {
    if (d == 0)
        return 0;
    return x / d;
}

/* Counts calls with a outside c..b; returns 1 (and restarts) on the nth. Unused. */
unsigned int debounce_out(unsigned int *cnt, unsigned int a, unsigned int b, unsigned int c,
                          unsigned int n) {
    if (a > b || a < c)
        (*cnt)++;
    else
        *cnt = 0;
    if (*cnt >= n) {
        *cnt = 0;
        return 1;
    }
    return 0;
}

/* Counts calls with a inside c..b; returns 1 (and restarts) on the nth. Unused. */
unsigned int debounce_in(unsigned int *cnt, unsigned int a, unsigned int b, unsigned int c,
                         unsigned int n) {
    if (a > b || a < c)
        *cnt = 0;
    else
        (*cnt)++;
    if (*cnt >= n) {
        *cnt = 0;
        return 1;
    }
    return 0;
}

/* Counts calls with a > b; returns 1 (and restarts) on the nth. Unused. */
unsigned int debounce_gt(unsigned int *cnt, unsigned int a, unsigned int b, unsigned int n) {
    if (a > b)
        (*cnt)++;
    else
        *cnt = 0;
    if (*cnt >= n) {
        *cnt = 0;
        return 1;
    }
    return 0;
}

/* Counts calls with a < b; returns 1 (and restarts) on the nth. Unused. */
unsigned int debounce_lt(unsigned int *cnt, unsigned int a, unsigned int b, unsigned int n) {
    if (a < b)
        (*cnt)++;
    else
        *cnt = 0;
    if (*cnt >= n) {
        *cnt = 0;
        return 1;
    }
    return 0;
}

/* The ASCII hex digit of 0..15 (upper case). Unused. */
unsigned int hexdigit(unsigned int d) {
    if (d < 10)
        return d + '0';
    return d + 55;
}
