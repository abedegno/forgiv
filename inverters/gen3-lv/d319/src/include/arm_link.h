/* arm_link.h: the ARM link (app/comm/arm_link.c, arm_page_write.c): the frame markers, the
   task and page handler declarations, and the settings words that pages 6 and 7 write.
   The frames are 45 bytes each way with a CRC16 in bytes 43-44; the byte layout is in
   arm_link.c's header and docs/subsystems/arm_link.md.
   name: descriptive, after arm_link.c. */
#ifndef ARM_LINK_H
#define ARM_LINK_H

#include "F2806x_Device.h"
#include "util.h"

/* The ARM link frames on SCI-A: the ARM sends 'C' frames, the DSP answers with 'D' frames */
#define ARM_FRAME_FROM_ARM 0x43 /* 'C' */
#define ARM_FRAME_TO_ARM 0x44   /* 'D' */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* arm_link.c */
int arm_charge_status(void);
extern unsigned int arm_crc_err_cnt;
#ifndef OWN_arm_fault_word
extern unsigned long arm_fault_word;
#endif
void arm_fault_word_build(void);
void arm_frame_parse(void);
void arm_link_frame(unsigned int *p, unsigned int len, int page);
void arm_page0(unsigned int i);
void arm_page1(unsigned int i);
void arm_page2(unsigned int i);
void arm_page3(unsigned int i);
void arm_page4(unsigned int i);
void arm_page5(unsigned int i);
extern void (*const arm_page_handlers[8])(unsigned int i);
void arm_run_command(int k);
int arm_run_mode(void);
extern unsigned int arm_rx_fail_cnt;
extern unsigned int arm_rx_idle_cnt;
extern unsigned int arm_rx_ok_cnt;
extern unsigned int arm_tx_halted;
extern unsigned int arm_tx_stop;
extern unsigned int arm_tx_tick;
extern unsigned int autotest_done_cnt;
extern unsigned int bms_fc4_idx;
extern unsigned int bms_fc4_subpage;
extern unsigned int bms_serial[10]; /* the BMS serial, sent on page 3 (bytes 32-41) */
extern unsigned int cal_highv_cnt;
extern unsigned int cal_lowv_cnt;
extern unsigned int peek_addr; /* the RAM word sent in 0x44 bytes 10-11 */
extern unsigned int run_cmd4_cnt;

/* arm_page_write.c */
void arm_page6(unsigned int reg);
void arm_page7(unsigned int reg);

/* defined by no source: the linker places it at its map address */
extern unsigned int arm_bms_request[6]; /* page 6 bytes 21-26: the ARM's own BMS request (0xD60A) */
extern unsigned int
    arm_bms_request_pending; /* page 6 byte 20; bms_link.c sends the request and clears it */
extern float batt_max_charge_current;
extern unsigned int charge_target_soc;
extern unsigned int charge_target_soc_now;
extern unsigned int enable_charge_target;
extern unsigned int fast_trip_v_high_set;
extern unsigned int fast_trip_v_low_set;
extern float force_charge_pct;
extern float force_discharge_pct;
extern unsigned int grid_import_amps;
extern float grid_v_nominal;
extern unsigned int power_ramp_rate;
extern unsigned int power_ramp_rate_reconnect;
extern unsigned int q_pt1_tau;
extern unsigned int sched_window;
/* ---- end of declarations ---- */

#endif
