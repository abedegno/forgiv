/* dsp_link.h: the UART4 link to the DSP: 0x43 frames out, 0x44 frames in, the settings and
   telemetry pages, and the DSP firmware update state. Globals named dsp_pN_xNN are page N's
   field at offset NN whose meaning I could not tell (b byte, w word, d 32 bits). Frame layouts:
   the header of comm/dsp_link.c.

   Defined in: comm/dsp_link.c, usb/usbh_usr.c (update state), drivers/uart.c.
   name: descriptive (readability step 1). */
#ifndef DSP_LINK_H
#define DSP_LINK_H

#include "firmware_types.h"

/* the first byte of a frame on the DSP link (UART4): 0x43 ("C") from the ARM, 0x44 ("D") from the DSP */
#define DSP_FRAME_TX 0x43
#define DSP_FRAME_RX 0x44

/* data */
extern uchar bms_request_flags; /* 319 page 6 byte 24, including the HR15 transition flag. */
extern ushort bms_hr1_request_pending;
extern uchar arm_update_state; /* 0 none, 1 ARM image found, 2 version word cleared */
extern short charger_temp;
extern uchar dsp_bms_page_valid;
extern uchar dsp_bms_raw[]; /* the BMS mailbox: reply +0x02, request +0x84, flags +0x98/+0x99 */
extern uchar dsp_bms_subpage;
extern uchar dsp_flash_mode; /* 1 while the battery firmware relay runs */
extern uchar dsp_flash_ok;
extern uchar dsp_flash_ok_state;
extern uchar dsp_flash_reply;
extern uchar dsp_flash_step; /* DSP update step (dsp_fw_update_process); 0 none */
extern ushort dsp_fw_version;
extern uchar dsp_rx_byte;
extern ushort dsp_rx_crc;
extern uchar dsp_rx_crc_errs;
extern uchar dsp_rx_flags;
extern uchar dsp_rx_frame[];
extern uchar dsp_rx_idx;
extern uchar dsp_rx_ok_count;
extern uchar dsp_rx_pages_seen;
extern uchar dsp_rx_timeout;
extern ushort dsp_tx_crc;
extern uchar dsp_tx_flags;
extern uchar dsp_tx_frame[];
extern uchar dsp_tx_page;
extern uint dsp_update_base;
extern uint dsp_update_block;
extern uint dsp_update_chunk;
extern uint dsp_update_cur;
extern uint dsp_update_end;
extern ushort dsp_update_timeout;
extern uchar bms_serial_bytes[];
extern ushort dsp_warning_word; /* frame bytes 7-8 (IR57) */
extern ushort eps_frequency;
extern ushort eps_voltage;
extern uchar force_state_hr329; /* HR329: forces state 2 and a discharge stop SOC of 4 */
extern ushort dsp_link_settle_count;
extern ushort calibration_release_count;
extern ushort bms_hr4_retry_count;
extern uchar dsp_page0_fresh;
extern int dsp_p3_w14;
extern int dsp_hr4_hr5;
extern ushort dsp_reserved_page4_word;
extern uchar dsp_ignored_mode_byte;
extern ushort v_ac_dsp;
extern ushort dsp_p3_w11;
extern int p_apparent_dsp;
extern ushort f_ac_dsp;
extern uchar dsp_output_permitted; /* frame byte 9: 0 keeps the inverter output off */
extern short pf_dsp;
extern uchar dsp_pv_bus_mode_byte;
extern ushort dsp_p0_w17;
extern short hr4000_value;
extern uchar dsp_p2_b01;
extern uchar g_2000022C;
extern ushort dsp_fault_data1;
extern ushort dsp_fault_data2;
extern ushort dsp_p3_w07;
extern ushort dsp_p2_w14;
extern ushort dsp_p3_w03;
extern ushort dsp_p3_w05;
extern ushort dsp_p2_w12;
extern ushort dsp_version_hr19;
extern uchar auto_test_state;
extern uchar dsp_p4_b02[];
extern ushort dsp_p4_w03;
extern ushort dsp_p4_w07;
extern ushort dsp_p4_w14;
extern ushort dsp_p4_w12;
extern ushort dsp_p4_w05;
extern ushort dsp_p4_w10;
extern uchar dsp_p4_b09[];
extern ushort dsp_p4_w19;
extern uint dsp_p2_d17;
extern uchar dsp_p1_b21;
extern int p_eps_sample;
extern uchar ct_cycle_count;
extern ushort batt_capacity_bms;
extern short peek_value;
extern uchar sched_window_out[];
extern uchar dsp_p3_b19[];
extern uchar dsp_p4_b16; /* the DSP asks for the battery firmware relay */
extern uchar hr200_request;
extern ushort hr201_request;
extern short heatsink_temp;
extern uchar hold_active;      /* table mode "hold": the DSP gets HR27 = 1 */
extern uchar save_req_pending; /* ask task_monitor to save the dirty EEPROM blocks */

/* functions */
void dsp_build_page(uchar frame_idx, int unused_page);
void dsp_fw_update_process();
void dsp_link_settle_tick();
void dsp_parse_page(uchar frame_idx);
uchar dsp_warning_test(ushort warning_mask);
int fault_debounce_count(int unused_fault, uchar count, int threshold, int value, int limit);
void task_dsp_rx();
void task_dsp_tx();

#endif
