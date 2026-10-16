/* os.h: the RTOS (src/rtos/): the task control block, the task ids and the events.
   Eight TCBs exist (OS_TASKS), seven tasks are created (main, os_main.c). The priority is the
   task id, 0 highest; scheduling is cooperative (a task runs until it calls os_wait); the tick
   is 1 ms (timer2_isr). The periods: tasks 0, 1, 2 and 4 every 20 ms, task 3 every 10 ms,
   task 5 every 60 ms, task 6 every 40 ms. Details in docs/subsystems/rtos.md.
   name: descriptive. */
#ifndef OS_H
#define OS_H

#include "F2806x_Device.h"
#include "util.h"

/* A task's control block, 8 words (os_tcb at 0xDA80). os_tcb[5].events, at 0xDAAC, is also
 * reached as arm_rx_block (sci.h). */
typedef struct {
    unsigned int *sp;              /* saved stack pointer */
    unsigned int period;           /* ticks between periodic wake-ups, 0 = none */
    unsigned int count;            /* ticks since the last wake-up */
    volatile unsigned long events; /* posted event bits */
    unsigned long mask;            /* bits os_wait does not wait for */
} OS_TCB;

#define OS_TASKS 8

/* The tasks: os_task_create's ids (os_main.c), one per RTOS task */
#define TASK_STATE 0    /* task0_state_machine */
#define TASK_AC_MEAS 1  /* task1_ac_measure */
#define TASK_CONTROL 2  /* task2_control */
#define TASK_MEASURE 3  /* task3_measure */
#define TASK_GRID 4     /* task4 (grid_support.c) */
#define TASK_ARM_LINK 5 /* task5_arm_link */
#define TASK_BMS_LINK 6 /* task6_bms_link */

/* Events: os_post(task, EV) sets bit EV of the task's events, os_wait returns them as a mask
 * (EV_..._M). Event 0 is every task's periodic tick (os_tick). The names say who posts them. */
#define EV_TICK 0
#define EV_TICK_M 1
/* task 0, the state machine */
#define EV0_STOP_FAULT 1 /* task3_measure: has_stop_fault (CF17, CF18 low byte) */
#define EV0_STOP_FAULT_M 2
#define EV0_GRID_FAULT 2 /* task3_measure: has_grid_fault; grid_loss_watchdog */
#define EV0_GRID_FAULT_M 4
#define EV0_PV_FAULT 3 /* task3_measure: has_pv_fault */
#define EV0_PV_FAULT_M 8
#define EV0_STOP                                                                                   \
    4 /* stop: fast faults in the control interrupt, ARM run command 10, the end of a battery calibration */
#define EV0_STOP_M 0x10
#define EV0_EPS_FAULT 5 /* task3_measure: has_eps_fault */
#define EV0_EPS_FAULT_M 0x20
#define EV0_UPDATE 6 /* ARM run command 4 (5 times): go to the update state */
#define EV0_UPDATE_M 0x40
#define EV0_BOOT 7 /* state4_update after 100 ticks: run the boot loader's update */
#define EV0_BOOT_M 0x80
/* task 1, the AC measurement */
#define EV1_GRID_PERIOD 1 /* grid_period_isr: a grid period's sums are in ac_sums_isr */
#define EV1_GRID_PERIOD_M 2
#define EV1_RMS_SUMS 2 /* eps_rms_accum.c: the EPS period's RMS sums are handed over */
#define EV1_RMS_SUMS_M 4
/* task 3, the measurement */
#define EV3_SLOW_AVG 1 /* dc_acc_step: the slow averages are copied to dc_acc */
#define EV3_SLOW_AVG_M 2
/* task 5, the ARM link */
#define EV5_FRAME 8 /* sci_rx: 45 bytes of an ARM frame have arrived */
#define EV5_FRAME_M 0x100

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* arm_link.c */
void task5_arm_link(void);

/* grid_support.c */
void task4(void);

/* bms_link.c */
void task6_bms_link(void);

/* power_limits.c */
void task2_control(void);

/* os_event.c */
void os_post(unsigned int id, unsigned int ev);
void os_post_flag(unsigned int id, unsigned int ev);
void os_post_isr(unsigned int id, unsigned int ev);
unsigned long os_take_events(void);
unsigned long os_wait(unsigned long mask);

/* os_main.c */
unsigned int os_highest(void);
void os_init_stack(void);
void os_init_tcbs(void);
void os_init_vars(void);
void os_sched(void);
extern unsigned int os_unmap_tbl[16];
void start(void);

/* os_task.c */
extern volatile unsigned int os_cur;
extern OS_TCB *volatile os_cur_tcb;
void os_init(void);
extern unsigned int os_next;
extern OS_TCB *volatile os_next_tcb;
extern volatile unsigned int os_ready;
void os_sched_isr(void);
extern unsigned int *volatile os_stack_free;
void os_start(void);
void os_task_create(unsigned int id, unsigned int stk, void (*task)(void));
extern OS_TCB os_tcb[OS_TASKS];

/* os_tick.c */
void os_sleep(unsigned int id, unsigned int ticks, unsigned int period);
unsigned int *os_stack_init(void (*task)(void), unsigned int *sp);
void os_tick(void);

/* ac_measure.c */
void task1_ac_measure(void);

/* os_tick_isr.c */
interrupt void timer2_isr(void);

/* state_task.c */
void task0_state_machine(void);

/* meas_task.c */
void task3_measure(void);

/* defined by no source: the linker places it at its map address */
void c_int00(void);
interrupt void os_ctx_sw_isr(void);
void os_int_ctx_sw(void);
extern unsigned int os_stack[];
void os_start_high(void);
/* ---- end of declarations ---- */

#endif
