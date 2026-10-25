/* schedule.h: charge and discharge scheduling, the 500 ms tick and CRC-32
   (comm/crc32_helpers.c, control/scheduler.c). The named tick counters belong to scheduler_periodic_update,
   energy_accumulate and task_schedule.
   name: descriptive (readability step 1). */
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "firmware_types.h"

/* data */
extern uchar charge_active;           /* sent to the DSP: frame byte 4 and page 0 */
extern uchar charge_status;           /* DSP frame byte 2 (IR14) */
extern uchar charge_target_soc_now[]; /* page 7 */
extern uint crc32_table[256];         /* never built (crc32_table_init is dead): all zero */
extern uchar discharge_active;        /* sent to the DSP: frame byte 4 and page 0 */
extern uchar discharge_min_soc_now;   /* page 4 */
extern uchar energy_day_changed[];
extern ushort energy_save_ticks;
extern uchar fault_recovery_ticks;
extern uchar dsp_command_hold_ticks;
extern ushort periodic_save_ticks;
extern uchar rtc_write_wait_ticks;
extern uchar usb_reset_wait_ticks;
extern uchar rtc_poll_phase;
extern uchar status_output_start_ticks;
extern ushort schedule_time_hhmm;
extern uchar rtc_init_retries;
extern uchar bms_update_start_ticks;
extern ushort hr200_request_ticks;
extern ushort forced_state_low_soc_ticks;
extern uchar rtc_init_finished;
extern uchar night_charge_active;
extern ushort night_charge_qual;
extern ushort night_charge_timer; /* passes of the recovery charge, up to HR108 x 120 */
extern ushort sched_table_b[48];  /* HR360-407: 48 half-hour values (table mode) */

/* functions */
uint bit_reflect(uint value, int bit_count);
void crc32_init(uint *crc);
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
void scheduler_periodic_update();
void task_schedule();

#endif
