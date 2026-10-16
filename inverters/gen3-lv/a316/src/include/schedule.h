/* schedule.h: charge and discharge scheduling, the 500 ms tick and CRC-32
   (control/schedule_p1.c, control/schedule_p3.c). The g_ counters belong to sys_tick_1s,
   energy_accumulate and task_schedule.
   name: descriptive (readability step 1). */
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "ghidra_types.h"

/* data */
extern uchar charge_active;           /* sent to the DSP: frame byte 4 and page 0 */
extern uchar charge_status;           /* DSP frame byte 2 (IR14) */
extern uchar charge_target_soc_now[]; /* page 7 */
extern uint crc32_table[256];         /* never built (crc32_table_init is dead): all zero */
extern uchar discharge_active;        /* sent to the DSP: frame byte 4 and page 0 */
extern uchar discharge_min_soc_now;   /* page 4 */
extern uchar g_200000EC[];
extern ushort g_200000F4;
extern uchar g_200000FC;
extern uchar g_200000FD;
extern ushort g_200000FE;
extern uchar g_20000100;
extern uchar g_20000101;
extern uchar g_20000102;
extern uchar g_20000103;
extern ushort g_20000104;
extern uchar g_20000106;
extern uchar g_20000107;
extern ushort g_20000108;
extern ushort g_2000010A;
extern uchar g_200001AA;
extern uchar night_charge_active;
extern ushort night_charge_qual;
extern ushort night_charge_timer; /* passes of the recovery charge, up to HR108 x 120 */
extern ushort sched_table_b[48];  /* HR360-407: 48 half-hour values (table mode) */

/* functions */
uint bit_reflect(uint value, int bit_count);
void crc32_init(undefined4 *crc);
void crc32_table_init(int polynomial);
void crc32_update(uint *crc, byte *data, int length);
int max3_u32(int first, int second, int third);
void sched_charge_discharge_eval();
void sched_charge_slots_eval();
void sched_discharge_slots_eval();
void sched_halfhour_table_eval();
void sched_slot_soc_targets();
void sched_window_value();
void status_outputs_update();
void sys_tick_1s();
void task_schedule();

#endif
