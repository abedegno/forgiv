/* The residual current monitor (trips at 0.9 x the VDE 0126-1-1 levels): the offset of the sensor
   taken after start-up (rcmu.a, unused but for its done flag), the self test that closes the test
   circuit on GPIO52 (rcmu.b, rcmu_selftest.c) and the watch on the readings (rcmu.c,
   rcmu_watch.c and rcmu_fast_check.c). rcmu is at 0xDB40. Inputs rcmu_i[0..2] (0xD488..0xD48C) in
   A: the RMS, the mean and a fast 10-sample mean of the residual current.
   docs/subsystems/rcmu.md has the levels and timings. */
#ifndef RCMU_H
#define RCMU_H

#include "F2806x_Device.h"
#include "util.h"

/* the zero offset: the mean of src[1] over 8 runs after start-up; out goes to rcmu_offset, which
   nothing reads */
struct rcmu_zero_t { /* 0xDB40 */
    Uint16 en;
    Uint16 arm;
    float *src;
    Uint16 cnt;
    Uint16 pad5;
    float sum;
    Uint16 done;
    Uint16 pad9;
    float out;
};

/* the self test, stepped by its caller: with the relay (GPIO52) open, average the reading;
 * close it and compare */
typedef struct rcmu_selftest_t {
    Uint16 en; /* 0x00 */
    Uint16 arm;
    float *src; /* 0x02: the measurements (rcmu_i) */
    volatile Uint16 step;
    Uint16 done; /* 0x05 */
    Uint16 passed;
    Uint16 failed;
    Uint16 pad8;
    Uint16 n;   /* 0x09 */
    Uint16 err; /* 0x0A */
    int16 retries;
    float open_avg;
    float avg1;
    float avg0;
    float sum1; /* 0x12: sum of meas[1] */
    float sum0; /* 0x14: sum of meas[0] */
    float fail_value;
} rcmu_selftest_t;

/* the watch: limit is the continuous level (0.3 A, set every run), base0/base1 the self test's
   open-circuit means (the clear test), filt0/filt1 slow filters of src[0]/src[1] (the
   sudden-change checks), trip and fast_trip the two trip flags (CF18 bit 6 is their OR) */
typedef struct rcmu_watch_t {
    Uint16 en; /* 0x00 */
    volatile Uint16 arm;
    Uint16 jump_check_on;
    Uint16 pad3;
    float base1; /* 0x04 */
    float base0;
    float limit;   /* 0x08: limit */
    float *src;    /* 0x0A: the measurements (rcmu_i) */
    float filt0;   /* 0x0C: meas[0] filtered */
    float filt1;   /* 0x0E: meas[1] filtered */
    Uint16 settle; /* 0x10: start-up count */
    Uint16 pad11;
    float last1; /* 0x12: meas[1], last run */
    float prev1; /* 0x14: meas[1], the run before */
    float last0; /* 0x16: meas[0], last run */
    float prev0; /* 0x18: meas[0], the run before */
    volatile Uint16 jump30_count;
    volatile Uint16 jump60_count;
    Uint16 over_count;
    Uint16 clear_count;
    Uint16 fast_trip;
    Uint16 trip;      /* 0x1F: tripped */
    float trip_value; /* 0x20: the value it tripped on */
} rcmu_watch_t;

struct
    rcmu_t { /* name: the residual current monitor (trips at 0.9 x the VDE 0126-1-1 levels), not an offset calibration */
    struct rcmu_zero_t a;
    rcmu_selftest_t b;
    rcmu_watch_t c;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* rcmu_watch.c */
void rcmu_watch(rcmu_watch_t *p);

/* rcmu.c */
extern struct rcmu_t rcmu;
void rcmu_init(void);
void rcmu_selftest_step(void);
void rcmu_step(void);
void rcmu_watch_step(void);
void rcmu_zero_cal(struct rcmu_zero_t *p);
void rcmu_zero_step(void);

/* rcmu_selftest.c */
void rcmu_selftest(rcmu_selftest_t *p);

/* defined by no source: the linker places it at its map address */
extern float rcmu_i[];
extern float rcmu_offset;
/* ---- end of declarations ---- */

#endif
