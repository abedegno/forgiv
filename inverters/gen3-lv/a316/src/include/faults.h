/* faults.h: the fault flags and code, the fault and event logs (never written: their push
   functions are dead), and the state of task_monitor (control/monitor.c), which owns most of
   what is declared here.
   name: descriptive (readability step 1). */
#ifndef FAULTS_H
#define FAULTS_H

#include "ghidra_types.h"

/* fault_flags bits whose meaning the code shows: set when a link stops answering, cleared by the
   next good frame (task_monitor for USART1, task_dsp_rx and dsp_parse_page for UART4) */
#define FAULT_AUX_LINK 0x40
#define FAULT_DSP_LINK 0x80

/* data */
extern uchar event_log[];   /* 50 records of 5 bytes, EEPROM block 3 */
extern uchar fault_code;    /* lowest set bit of fault_flags, 1-32 (0 none) */
extern int fault_code_word; /* IR39/40: the DSP's fault word with the ARM's bits 6-11 */
#ifndef OWN_fault_flags
extern ushort fault_flags;
#endif
extern uchar fault_led_on;
extern uchar fault_led_tick;
extern uchar fault_log[];    /* 5 records of 9 bytes, saved with block 2 */
extern int p_grid_meter_net; /* meter 1 minus meters 5-8 */
extern uchar p_eps_avg_count;
extern int p_eps_avg_sum;
extern uchar eeprom_save_result;
extern uchar eeprom_save_retries;
extern ushort eeprom_reload_count;
extern uchar fault_code_prev;
extern ushort fault_code_settle;
extern uchar aux_tx_last_byte;
extern ushort g_200000D0;
extern ushort aux_tx_idle_count;
extern ushort aux_link_timeout;
extern uchar ntc_battery_valid;
extern uchar fault_led_armed;
extern uchar ct_reversed; /* HR42 1 or 3: CT power negated */
extern ushort ct_reverse_debounce;
extern uchar g_20000266;
extern uchar eeprom_busy;
extern uchar eeprom_busy_2;
extern short i_rms_ch7;
extern int p_eps_avg; /* EPS power, average of 50 windows */
extern int g_200002B8;
extern int p_load_calc; /* load power (IR42 via p_load_demand) */
extern int g_200003C4;
extern short pf_inverter_output;
extern ushort v_ac1; /* IR5 grid voltage */

/* functions */
void event_log_push();
int fault_check_mode();
void fault_clear(uint fault_mask);
void fault_code_update();
void fault_led_update();
void fault_log_push();
int fault_lowest_bit(int flags);
uchar fault_none();
void fault_set(uint fault_mask);
uchar fault_test(uint fault_mask);

#endif
