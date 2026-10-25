/* Small shared types and helpers: the bit view of a 16-bit word used for every flag and fault
   word, the IQ constants block, and the helpers of util.c (clamps, min/max, median, division),
   rate_limit.c, crc16.c and iq_consts.c. The run-time library's float functions (expf, fabs,
   sinf, sqrtf) are declared here too. */
#ifndef UTIL_H
#define UTIL_H

#include "F2806x_Device.h"

/* a word read bit by bit, as TI's headers declare registers: x.all, x.bit.b3, x.byte.lo */
struct bits16 {
    Uint16 b0 : 1;
    Uint16 b1 : 1;
    Uint16 b2 : 1;
    Uint16 b3 : 1;
    Uint16 b4 : 1;
    Uint16 b5 : 1;
    Uint16 b6 : 1;
    Uint16 b7 : 1;
    Uint16 b8 : 1;
    Uint16 b9 : 1;
    Uint16 b10 : 1;
    Uint16 b11 : 1;
    Uint16 b12 : 1;
    Uint16 b13 : 1;
    Uint16 b14 : 1;
    Uint16 b15 : 1;
};
/* the two halves of a word */
struct word_bytes {
    Uint16 lo : 8;
    Uint16 hi : 8;
};
union bits16_u {
    Uint16 all;
    struct bits16 bit;
    struct word_bytes byte;
};

/* 0xCCB4 .. 0xCCBA: IQ30 constants set by iq_consts_init (iq_consts.c) */
struct iq_consts_t {
    long one_sixth;
    long one_twelfth;
    long k1000h;
    unsigned int zero;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* iq_consts.c */
void iq_consts_init(void);

/* crc16.c */
unsigned int CRC16(unsigned int *puchMsg, unsigned int usDataLen);

/* rate_limit.c */
void rate_limit(float *x, float ref, float up, float lo);

/* util.c */
void limitf(float *x, float max, float min);
void limiti(int *x, int max, int min);
void limitu(unsigned int *x, unsigned int max, unsigned int min);
float max3f(float a, float b, float c);
unsigned long median3ul(unsigned long a, unsigned long b, unsigned long c);
float min3f(float a, float b, float c);
float min7f(float a, float b, float c, float d, float e, float f, float g);
unsigned int udiv32_16(unsigned long x, unsigned int d);

/* defined by no source: the linker places it at its map address */
float expf(float x);
double fabs(double x);
float sinf(float x);
float sqrtf(float x);
/* ---- end of declarations ---- */

#endif
