/* The BMS: what the battery reports over the BMS Modbus link (task6_bms_link, bms_parse_hr) and
   the limits the control task derives from it, the page at 0xD500.
   The protocol words (slave, function codes, the FC6 registers, the two polls) and the BMS link's
   variables (bms_link.c) are declared here too. docs/battery.md describes the whole path.
   Units: 0.1 V, 0.01 A for the BMS currents and caps, 0.1 W for the power limits, SoC in %. */
#ifndef BMS_H
#define BMS_H

#include "F2806x_Device.h"
#include "util.h"

/* 0xD500: the BMS page. The first words are what bms_parse_hr stores from the holding registers;
 * the floats are the control task's power and current limits (power_limits.c). "Never written"
 * below means no code in the image stores to it; the readers then always see 0 (or the cinit
 * value). */
struct bms_data_t {
    union bits16_u
        alarm_flags; /* 0xD500: bit 1 = HR20 bit 2 (full), bit 2 = HR20 bit 3 (empty); bits 0, 3, 5, 8, 12 are read but never written */
    union bits16_u
        hr19_status; /* 0xD501: HR19; bits 0-1 not both 0 = the BMS reports, bit 2 clear forces 300 W charge, bit 4 a battery fault, bit 5 forces 120 W discharge */
    volatile union bits16_u
        status2; /* 0xD502: never written; bit 0 full, bits 1/3/6/7 taper bypass, bit 11 battery fault */
    Uint16 charge_voltage; /* 0xD503: never written; copied to charge_voltage_copy */
    Uint16
        bms_charge_limit; /* 0xD504: HR26 charge limit, 0.01 A (HR25 with BMS firmware 3010 or lower) */
    Uint16
        bms_discharge_limit; /* 0xD505: HR27 discharge limit, 0.01 A (HR25 with firmware 3010 or lower) */
    Uint16 soc; /* 0xD506: SoC %, HR21 low byte (held to floor + 1 .. 99 in a calibration) */
    Uint16 rsvd0[1];
    Uint16 pack_voltage;       /* 0xD508: HR22 x 0.1, so 0.1 V */
    volatile int pack_current; /* 0xD509: HR23, 0.01 A, signed */
    Uint16 pack_temp_x10;      /* 0xD50A: HR24 x 10, sent to the ARM only */
    Uint16 cfg_d50b;           /* 0xD50B: set to 0, never read */
    Uint16 limit_scale;        /* 0xD50C: set to 1 (scales the bypass limits) */
    Uint16 rsvd1[1];
    float chg_plim_temp; /* 0xD50E: charger temperature derate, 0.1 W */
    float dis_plim_temp;
    float chg_plim_current; /* 0xD512: charge limit x V, at most HR308 */
    float dis_plim_current; /* 0xD514: discharge limit x V */
    float chg_plim_bus;     /* 0xD516: bus over-voltage derate */
    float dis_plim_bus;
    float chg_plim_soc; /* 0xD51A: soc_power_limits */
    float dis_plim_soc;
    Uint16 rsvd2[2];
    Uint16 charge_voltage_copy; /* 0xD520: charge_voltage copy (580 without BMS data), never read */
    Uint16 rsvd3[1];
    float chg_current_cap;   /* 0xD522: capacity x HR111 % + 1.5 A, at most HR309 */
    float dis_current_cap;   /* 0xD524: capacity x HR112 % + 1.5 A, at most HR309 + 15 A */
    float chg_current_limit; /* 0xD526: tapered HR26 limit, 0.01 A, 1..80 A */
    float dis_current_limit; /* 0xD528: tapered HR27 limit, 0.01 A, 2..80 A */
    Uint16 rsvd4[1];
    Uint16
        bms_charge_limit_copy; /* 0xD52B: HR26 copy (100 without BMS data), read by the full check in soc_floor */
    Uint16 max_charge_current_copy; /* 0xD52C: HR309 copy, never read */
    Uint16 start_permit;            /* 0xD52D: soft_start may run (batt_voltage_monitor) */
    Uint16 soft_start_enable;       /* 0xD52E: the run states call soft_start (soft_start_permit) */
};

/* the page as named words, and its first words as an array (soc_floor reaches them both ways) */
union bms_page_u {
    volatile union bits16_u w[3];
    struct bms_data_t s;
};

/* The BMS Modbus RTU link (task6_bms_link on SCI-B): the battery is slave 1 */
#define BMS_SLAVE 1
#define MB_READ_HOLDING 3 /* function codes */
#define MB_READ_INPUT 4
#define MB_WRITE_REGISTER 6
#define BMS_HR_WAKE                                                                                \
    1 /* written 1 to wake the battery, 0 to let it sleep (around HR175 batt_cal_request) */
#define BMS_HR_UPDATE 2           /* written bms_update_pending */
#define BMS_HR_200 3              /* written hr200_request */
#define BMS_HR_201 4              /* written hr201_request (1) */
#define BMS_POLL_SHORT_START 0x11 /* the short poll reads HR17 .. HR25 */
#define BMS_POLL_SHORT_COUNT 9
#define BMS_POLL_FULL_COUNT 0x1C /* the full poll reads HR0 .. HR27 */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* bms_link.c */
extern unsigned int batt_cal_request;
extern unsigned int batt_cal_request_prev;
extern unsigned int batt_cal_stage;
extern unsigned int batt_warning;
void batt_warning_update(void);
extern unsigned int bms_alarm;
extern unsigned int bms_alarm_masked;
extern unsigned int bms_crc;
extern unsigned int bms_fc4_count;
void bms_fc4_reply(void);
extern volatile unsigned int bms_hr15;
#ifndef OWN_bms_hr1_request
extern volatile union bits16_u bms_hr1_request;
#endif
#ifndef OWN_bms_hr15_edge
extern union bits16_u bms_hr15_edge;
#endif
extern union bits16_u bms_request_flags;
extern unsigned int bms_hr17;
extern unsigned int bms_hr18;
extern unsigned int bms_hr2_count;
extern unsigned int bms_hr3_count;
void bms_lost(void);
extern unsigned int bms_lost_count;
void bms_parse_hr(void);
extern unsigned int bms_poll_count;
extern unsigned int bms_poll_div;
extern unsigned int bms_poll_start;
extern unsigned int bms_present;
extern unsigned int bms_relay_active;
extern unsigned int bms_relay_idle;
extern unsigned int bms_reply_count;
extern unsigned int bms_rx_expect;
extern unsigned int bms_rx_len;
extern volatile unsigned int bms_tx_buf
    [100]; /* the request being sent (volatile: the image stores each 0 as an immediate) */
extern unsigned int bms_tx_len;
extern unsigned int bms_tx_pos;
extern unsigned int bms_tx_state;
extern unsigned int bms_update_pending;
extern unsigned int bms_wake_count;
extern unsigned int bms_wake_off;
extern unsigned int bms_wake_off_count;
extern unsigned int bms_wake_on;
extern unsigned int hr200_request;
extern unsigned int hr201_request;
extern unsigned int low_batt_wake;

/* defined by no source: the linker places it at its map address */
#ifndef OWN_bms
extern volatile struct bms_data_t bms;
#endif
extern unsigned int bms_fc4_ready;
/* ---- end of declarations ---- */

#endif
