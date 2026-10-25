/* The power limits: the control task's tapers and SoC floor, and the grid code's responses.
   power_limits.c (task 2) declares its steps here; the ISR limit block isr_limits (0xC208) is what
   the battery converter's current controller clamps to. Units: 0.1 V, 0.1 W, 0.1 A on the HV
   side. */
#ifndef POWER_LIMITS_H
#define POWER_LIMITS_H

#include "F2806x_Device.h"
#include "util.h"

/* 0xCE00: the grid code's frequency and voltage power responses (grid_support.c): the under- and
 * over-frequency settings and states, the over- and under-voltage response flags, and the
 * power each asks for; power_limits.c applies grid_resp_power and stores resp_restore_power */
struct freq_resp_t {
    float uf_start_freq; /* 0xCE00 */
    Uint16 uf_delay;     /* 0xCE02 */
    Uint16 pad_ce03;
    Uint16 uf_recover_delay; /* 0xCE04 */
    Uint16 pad_ce05;
    Uint16 uf_recover_freq; /* 0xCE06 */
    Uint16 pad_ce07;
    float uf_full_freq; /* 0xCE08 */
    Uint16 uf_active;   /* 0xCE0A */
    Uint16 pad_ce0b;
    float uf_ramp_step;  /* 0xCE0C */
    float of_start_freq; /* 0xCE0E */
    Uint16 of_delay;     /* 0xCE10 */
    Uint16 pad_ce11;
    Uint16 of_recover_delay; /* 0xCE12 */
    Uint16 pad_ce13;
    Uint16 of_recover_freq; /* 0xCE14 */
    Uint16 pad_ce15;
    float of_full_freq; /* 0xCE16 */
    Uint16 of_active;   /* 0xCE18 */
    Uint16 pad_ce19;
    float of_ramp_step;       /* 0xCE1A */
    Uint16 ov_resp_active;    /* 0xCE1C */
    Uint16 uv_resp_active;    /* 0xCE1D */
    float grid_resp_power;    /* 0xCE1E */
    float of_resp_power;      /* 0xCE20 */
    float uf_resp_power;      /* 0xCE22 */
    float ov_resp_power;      /* 0xCE24 */
    float uv_resp_power;      /* 0xCE26 */
    float resp_restore_power; /* 0xCE28 */
};

/* 0xD892: the battery's rated power (0.1 W, HR308, 1.0 .. 6.0 kW, default 3.6 kW) and the same
 * plus 300 W, set from ARM page 7 (arm_page_write.c); the same words as rating_params
 * batt_chg_power_max/batt_dis_power_max */
struct batt_power_rating_t {
    float chg_max;
    float dis_max;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "util.h"

/* power_limits.c */
void aging_test_update(void);
float batt_power_blocks(float p);
void batt_power_setpoint(void);
void batt_voltage_monitor(void);
void ctl_voltage_limits_set(void);
void current_limit_tapers(float v);
void full_current_trim(void);
void hv_current_max_set(void);
void soc_floor(void);
extern union bits16_u soc_floor_latch;
void soc_power_limits(void);
void soft_start_permit(void);
void status_led_and_reset(void);

/* defined by no source: the linker places it at its map address */
extern unsigned int aging_test_enable; /* 0xD8EF: HR332 */
extern unsigned int aging_test_mode;   /* 0xD8F0: 0, 1 (charge) or 2 (discharge) */
extern float batt_current_cmd_isr;     /* 0xC216: the current command to the ISR */
extern struct batt_power_rating_t batt_power_rating;
extern float batt_voltage;             /* 0xD212: dc_meas.batt_v by name, 0.1 V */
extern unsigned int battery_type;      /* 0xD8E1: HR54, 0 = no BMS */
extern union bits16_u bms_hr19;        /* 0xD501: bms.hr19_status by name */
extern unsigned int cfg_changed_flags; /* 0xD8CC: settings change pending (reset after 15 s) */
extern float dis_current_limit;        /* 0xD528: bms.dis_current_limit by name */
extern float force_chg_soc_f;
extern float full_trim_isr; /* 0xC206: full_current_trim output */
extern struct isr_limits_t {
    float batt_v_max; /* 0xC208: 56.0 V (aging 50.0/54.0), used only without a BMS */
    float batt_v_min; /* 0xC20A: 46.0 V, used only without a BMS */
    float c20c;
    float hv_current_max; /* 0xC20E: 15.0 A, the current command limit */
    float c210;
    float chg_current_bound; /* 0xC212: charge bound, 0.1 A HV side */
    float dis_current_bound; /* 0xC214: discharge bound (negative) */
} isr_limits;
extern float lim_soft_start;           /* 0xCF1C: the soft-start power ramp (grid_support.c) */
extern unsigned int soft_start_enable; /* 0xD52E: bms.soft_start_enable by name */
extern unsigned int upgrade_tag[];
/* ---- end of declarations ---- */

#endif
