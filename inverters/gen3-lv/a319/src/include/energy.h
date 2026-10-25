/* energy.h: the energy counters (day and total, 0.1 kWh): EEPROM block 4 and the derived values
   the input registers serve. energy_accumulate (control/scheduler.c) integrates them;
   power_flow_update (control/monitor.c) takes the meter-based ones.
   name: descriptive (readability step 1). */
#ifndef ENERGY_H
#define ENERGY_H

#include "firmware_types.h"
#include "application_types.h"

/* EEPROM block 4 (0xBD bytes at 0x20001BD1): energy counters. energy_accumulate adds power/2 every 0.5 s into each acc and moves whole units (3600000 = 0.1 kWh, inference) into day and total; the day counters are cleared when last_day differs from the RTC day. Which counter is which comes from the input registers modbus_ir_update serves from them */
typedef __packed struct {
    int work_time;            /* +0x00 half seconds; IR47/48 = work_time / 7200 */
    uint pv1_acc;             /* +0x04 */
    ushort pv1_day;           /* +0x08 IR17 e_pv1_day */
    int pv1_total;            /* +0x0A */
    uint pv2_acc;             /* +0x0E */
    ushort pv2_day;           /* +0x12 IR19 e_pv2_day */
    int pv2_total;            /* +0x14 */
    int pv_total;             /* +0x18 IR11/12 e_pv_total, with the AC PV meters */
    uint p2d17_acc;           /* +0x1C from dsp_p2_d17 (meaning not known) */
    ushort p2d17_day;         /* +0x20 */
    int p2d17_total;          /* +0x22 */
    int inv_out_acc;          /* +0x26 */
    short inv_out_day;        /* +0x2A e_inverter_out */
    int inv_out_total;        /* +0x2C */
    int inv_in_acc;           /* +0x30 */
    short inv_in_day;         /* +0x34 e_inverter_in */
    int inv_in_total;         /* +0x36 */
    int batt_discharge_acc;   /* +0x3A */
    short batt_discharge_day; /* +0x3E IR37 e_battery_discharge_day */
    int batt_discharge_total; /* +0x40 */
    int batt_charge_acc;      /* +0x44 */
    short batt_charge_day;    /* +0x48 IR36 e_battery_charge_day */
    int batt_charge_total;    /* +0x4A */
    int grid_out_acc;         /* +0x4E */
    short grid_out_day;       /* +0x52 e_grid_out */
    int grid_out_total;       /* +0x54 */
    int grid_in_acc;          /* +0x58 */
    short grid_in_day;        /* +0x5C e_grid_in */
    int grid_in_total;        /* +0x5E */
    int calc6_acc;            /* +0x62 */
    short calc6_day;          /* +0x66 from load_power_estimate (meaning not known) */
    int calc6_total;          /* +0x68 */
    uchar last_day;           /* +0x6C the RTC day the day counters belong to */
    uchar f6D[16];
    int meter_in_day_start
        [8]; /* +0x7D each meter's import counter at the start of the day (also the symbol meter_in_day_start) */
    int meter_out_day_start[8]; /* +0x9D each meter's export counter at the start of the day */
} energy_t;

/* data */
extern int e_grid_in_day;
extern int e_grid_in_total;
extern int e_grid_out_day;
extern int e_grid_out_total;
extern int e_inverter_in_day;
extern int e_inverter_in_total;
extern int e_inverter_out_day;
extern int e_inverter_out_total;
extern int e_pv_meter2_day;
extern int e_pv_meter2_total;
extern int e_pv_meter3_day;
extern int e_pv_meter3_total;
extern energy_t energy_totals;

/* functions */
void energy_accumulate();

#endif
