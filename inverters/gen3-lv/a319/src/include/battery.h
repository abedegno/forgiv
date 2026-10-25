/* battery.h: the battery as the ARM sees it. The ARM never talks to the BMS itself: the DSP
   polls it and relays values (frame pages 0, 2, 3, 4, 5) and the ARM's FC4 pack poll (bms_poll)
   goes out through the DSP too. Here are the pack records, the battery globals the DSP fills,
   and the battery firmware relay state.

   Defined in: control/measure.c (bms_poll, bms_modules), comm/dsp_link.c (the DSP values),
   control/monitor.c (SOC), control/scheduler.c (night recovery, batt_auto_judge_type),
   usb/usbh_usr.c (the firmware relay). docs/battery.md has the whole picture.
   name: descriptive (readability step 1). */
#ifndef BATTERY_H
#define BATTERY_H

#include "firmware_types.h"

/* one battery module's BMS record (bms_modules[6], 0x83 bytes each; packs 1-5 are polled): the
   bytes the DSP relays on its page 5, unpacked. Names follow the battery input registers
   modbus_fill_battery_regs serves from each field (the public GivEnergy battery register list,
   IR60-114 of slaves 0x32-0x36) */
typedef __packed struct {
    ushort pack_status_word; /* block 1 data 32-33: status word, bit 0 counts the pack in bms_packs_online */
    uchar serial[10]; /* +0x02 IR110-114 */
    uchar f0C[0x20];  /* bms_poll copies 21 serial bytes from +0x02, so the serial runs into this */
    uchar num_cells;  /* +0x2C IR97 */
    ushort num_cycles;   /* +0x2D IR96 */
    int v_out;           /* +0x2F IR82/83 */
    ushort v_cells_sum;  /* +0x33 IR80 */
    int pack_current_raw; /* +0x35: signed BMS current; IR95 divides it by 10. */
    uint cap_calibrated; /* +0x39 IR84/85 */
    uint cap_design;     /* +0x3D IR86/87, IR101/102 */
    uint cap_remaining;  /* +0x41 IR88/89 */
    uchar soc;           /* +0x45 IR100 */
    uchar status[9];     /* +0x46 IR90-94: status 1-7, warning 1-2 */
    ushort maximum_cell_voltage;          /* highest cell voltage; only values 0..699 are accepted */
    ushort minimum_cell_voltage;          /* lowest cell voltage; the same 0..699 check */
    ushort v_cell[16];   /* +0x53 IR60-75 */
    ushort temp_max;     /* +0x73 IR103 */
    ushort temp_min;     /* +0x75 IR104 */
    ushort fw_version;   /* +0x77 IR98 */
    ushort temp[5];      /* +0x79 IR76-79 cell temperatures, IR81 the MOS temperature */
} bms_t;

/* data */
extern uchar batt_cal_request;   /* HR175: while 1 the DSP gets "charge" and PV start 155.0 V */
extern short batt_current;       /* DSP page 0, 0.01 A (IR51) */
extern uchar batt_full_flag;     /* DSP page 2: forces SOC 100 in lead-acid mode */
extern int batt_power_calc;      /* grid power from the CT or meters; the default DSP setpoint is
                                -this / 10 */
extern ushort batt_power_remote; /* HR495: the setpoint while HR494 = 1 */
extern uchar batt_soc;           /* IR59 */
extern uchar batt_soc_dsp;       /* DSP page 0 (the BMS SOC as the DSP reports it) */
extern uchar batt_state_dsp;     /* DSP page 3: 3 or more forces SOC 0 in lead-acid mode */
extern short batt_temp;          /* IR56 */
extern ushort batt_temp_dsp;     /* DSP page 0 */
extern ushort batt_voltage;      /* DSP page 0, 0.01 V (IR50) */
extern ushort bms_chip_version;  /* DSP page 2 (HR46); a battery image is relayed only when non-
                                    zero */
extern ushort bms_fw_version;    /* DSP page 2 (HR18) */
extern uchar dsp_fault_words[];  /* DSP pages 4 and 6, 16 bytes (IR217-224) */
extern uchar bms_charge_limit_a;
extern uchar bms_discharge_limit_a;
extern bms_t bms_modules[6];
extern uchar bms_packs_online[]; /* packs whose status bit 0 is set (slave 0x30 IR99) */
extern uchar bms_poll_addr;
extern ushort bms_poll_count;
extern ushort bms_poll_reg;
extern ushort bms_poll_step;
extern ushort bms_poll_timer;
extern ushort bms_present; /* DSP page 4; read only by the dead batt_auto_judge_type */
extern uchar bms_rx_pack;
extern int bms_update_base;
extern int bms_update_block;
extern int bms_update_chunk;
extern uint bms_update_crc32;
extern int bms_update_cur;
extern int bms_update_end;
extern ushort bms_update_end_count;
extern uchar bms_update_failed; /* 1 blocks the battery relay until the next start */
extern uchar bms_update_last;
extern uchar bms_update_pending; /* sent on page 4: asks the DSP to start the relay */
extern ushort bms_update_state;
extern ushort bms_update_timeout;

/* functions */
void batt_auto_judge_type();
void batt_night_recovery_charge();
void batt_soc_from_voltage();
void bms_fw_update_process();
void bms_poll();
void bms_update_fail_flag_load();

#endif
