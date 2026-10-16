/* Range A's view of RAM (grid_support.c and arm_link.c only): the variables as one struct per
   64-word page, and a macro per word naming it. match: range A's code was matched against this
   view; an extern struct per page gives the image's one MOVW DP per page, and these two files keep
   it.
   Include it last: its macros (state_trace, fault_grid1, ...) would rename the other headers'
   declarations. The other files see the same words through the subsystem headers.
   First generated during the matching work from map/data.tsv; the
   readability pass moved its plain declarations and its structured types into the subsystem
   headers (docs/readability.md). The page comments were added by hand in the comment step;
   regenerating the file would drop them.
   Names: renamed by tools/rename.py (build/names/merged.tsv). The macro names come
   from map/data.tsv, except five that differ from the symbol's name in other files: eps_cycle_len,
   uf_start_freq, fault_grid1, batt_chg_power_max and inv_power_abs (scope a_ram.h in merged.tsv).
   Units, as elsewhere: voltages 0.1 V, frequency 0.01 Hz, power 0.1 W. */
#ifndef RAM_VIEW_H
#define RAM_VIEW_H

#include "util.h"
#include "power_limits.h"
#include "grid.h"

/* 0xC000: the converter module's globals (converter_ctrl.c) */
extern struct a_page_C000 {
    unsigned int v_C000;
    unsigned int v_C001;
    unsigned int v_C002;
    unsigned int v_C003;
    unsigned int v_C004;
    unsigned int v_C005;
    unsigned int v_C006;
    unsigned int v_C007;
    unsigned int v_C008;
    unsigned int v_C009;
    unsigned int v_C00A;
    unsigned int v_C00B;
    unsigned int v_C00C;
    unsigned int v_C00D;
    unsigned int v_C00E;
    unsigned int v_C00F;
    unsigned int v_C010;
    unsigned int v_C011;
    unsigned int v_C012;
    unsigned int v_C013;
    unsigned int v_C014;
    unsigned int v_C015;
    unsigned int v_C016;
    unsigned int v_C017;
    unsigned int v_C018;
    unsigned int v_C019;
    unsigned int v_C01A;
    unsigned int v_C01B;
    unsigned int v_C01C;
    unsigned int v_C01D;
    unsigned int v_C01E;
    unsigned int v_C01F;
    unsigned int v_C020;
    unsigned int v_C021;
    float v_C022;
    float v_C024;
    float v_C026;
    float v_C028;
    float v_C02A;
    unsigned int v_C02C;
    unsigned int p_C02D[1];
    float v_C02E;
    unsigned int v_C030;
    unsigned int p_C031[3];
    unsigned int v_C034;
    unsigned int v_C035;
    unsigned int v_C036;
} ram_C000;
#define state_trace (ram_C000.v_C000)
#define tz_trip_count (ram_C000.v_C001)
#define conv_mode (ram_C000.v_C002)
#define dcdc_soft_done (ram_C000.v_C003)
#define inv_on (ram_C000.v_C004)
#define dcdc_duty (ram_C000.v_C005)
#define fast_trip_code (ram_C000.v_C006)
#define soft_start_phase (ram_C000.v_C007)
#define pv2_ref_isr (ram_C000.v_C008)
#define inv_cmp_a (ram_C000.v_C009)
#define batt_current_cmd_i (ram_C000.v_C00A)
#define inv_cmp_b (ram_C000.v_C00B)
#define inv_duty (ram_C000.v_C00C)
#define pv1_ref_isr (ram_C000.v_C00D)
#define force_chg_soc_isr (ram_C000.v_C00E)
#define hr4000_value (ram_C000.v_C00F)
#define dis_current_bound_i (ram_C000.v_C010)
#define pwm_period (ram_C000.v_C011)
#define bms_hr15_copy (ram_C000.v_C012)
#define chg_current_bound_i (ram_C000.v_C013)
#define grid_cycle_samples (ram_C000.v_C014)
#define dcdc_soft_lim (ram_C000.v_C015)
#define grid_buf_clear_idx (ram_C000.v_C016)
#define dcdc_soft_div (ram_C000.v_C017)
#define pv2_boost_on (ram_C000.v_C018)
#define bus_high (ram_C000.v_C019)
#define grid_vpk_count (ram_C000.v_C01A)
#define batt_ov_count (ram_C000.v_C01B)
#define hv_oc_count (ram_C000.v_C01C)
#define hv_ov_count (ram_C000.v_C01D)
#define pv1_boost_on (ram_C000.v_C01E)
#define relay_sync_b (ram_C000.v_C01F)
#define ecap_valid_count (ram_C000.v_C020)
#define relay_sync_a (ram_C000.v_C021)
#define ecap_delta (ram_C000.v_C022)
#define ecap_last (ram_C000.v_C024)
#define ecap_period (ram_C000.v_C026)
#define pll_sin (ram_C000.v_C028)
#define pll_cos (ram_C000.v_C02A)
#define leak_fast_acc (ram_C000.v_C02C)
#define leak_fast_acc_sum (ram_C000.v_C02E)
#define relay_ctl (ram_C000.v_C030)
#define relay_cmd (ram_C000.v_C034)
#define relay_ctl_relay_b (ram_C000.v_C035)
#define eps_active (ram_C000.v_C036)

extern struct a_page_C040 {
    float v_C040;
    float v_C042;
    float v_C044;
    float v_C046;
    float v_C048;
    float v_C04A;
    float v_C04C;
    float v_C04E;
    float v_C050;
    float v_C052;
    float v_C054;
    float v_C056;
    unsigned int v_C058;
    unsigned int p_C059[1];
    float v_C05A;
    float v_C05C;
    float v_C05E;
    float v_C060;
    unsigned int v_C062;
    unsigned int v_C063;
} ram_C040;
#define pv_bus_pi (ram_C040.v_C040)
#define pv_bus_pi_kp (ram_C040.v_C042)
#define pv_bus_pi_p (ram_C040.v_C044)
#define pv_bus_pi_ki (ram_C040.v_C046)
#define pv_bus_pi_i (ram_C040.v_C048)
#define pv_bus_pi_u (ram_C040.v_C04A)
#define ctrl_word_c04c (ram_C040.v_C04C)
#define island_det (ram_C040.v_C04E)
#define island_det_max (ram_C040.v_C050)
#define island_det_min (ram_C040.v_C052)
#define island_det_thr (ram_C040.v_C054)
#define island_det_gain (ram_C040.v_C056)
#define island_det_limit (ram_C040.v_C058)
#define island_det_hist1 (ram_C040.v_C05A)
#define island_det_hist2 (ram_C040.v_C05C)
#define island_det_hist3 (ram_C040.v_C05E)
#define island_det_out (ram_C040.v_C060)
#define island_det_cnt (ram_C040.v_C062)
#define island_det_flag (ram_C040.v_C063)

extern struct a_page_C080 {
    float v_C080;
    unsigned int p_C082[4];
    float v_C086;
    unsigned int p_C088[4];
    float v_C08C;
    unsigned int p_C08E[4];
    float v_C092;
    unsigned int p_C094[4];
    float v_C098;
} ram_C080;
#define grid_v_hist (ram_C080.v_C080)
#define ram_word_c086 (ram_C080.v_C086)
#define ram_word_c08c (ram_C080.v_C08C)
#define grid_v_hist_lead_prev (ram_C080.v_C092)
#define grid_v_hist_lead (ram_C080.v_C098)

/* 0xC0C0: this sample's scaled ADC values (meas_fast) */
extern struct a_page_C0C0 {
    float v_C0C0;
    float v_C0C2;
    float v_C0C4;
    float v_C0C6;
    float v_C0C8;
    float v_C0CA;
    float v_C0CC;
    float v_C0CE;
    float v_C0D0;
    float v_C0D2;
    float v_C0D4;
    unsigned int p_C0D6[2];
    float v_C0D8;
    float v_C0DA;
    float v_C0DC;
    unsigned int p_C0DE[2];
    float v_C0E0;
    unsigned int p_C0E2[6];
    float v_C0E8;
    float v_C0EA;
} ram_C0C0;
#define meas_fast (ram_C0C0.v_C0C0)
#define meas_fast_pv2_v (ram_C0C0.v_C0C2)
#define meas_fast_iso_v (ram_C0C0.v_C0C4)
#define meas_fast_bus_v (ram_C0C0.v_C0C6)
#define meas_fast_hv_v (ram_C0C0.v_C0C8)
#define meas_fast_batt_v (ram_C0C0.v_C0CA)
#define meas_fast_pv1_i (ram_C0C0.v_C0CC)
#define meas_fast_pv2_i (ram_C0C0.v_C0CE)
#define meas_fast_hv_i (ram_C0C0.v_C0D0)
#define meas_fast_leak_i_slow (ram_C0C0.v_C0D2)
#define meas_fast_dci_i (ram_C0C0.v_C0D4)
#define meas_fast_leak_i (ram_C0C0.v_C0D8)
#define meas_fast_aux17 (ram_C0C0.v_C0DA)
#define meas_fast_aux18 (ram_C0C0.v_C0DC)
#define meas_fast_inv_v (ram_C0C0.v_C0E0)
#define meas_fast_grid_v (ram_C0C0.v_C0E8)
#define meas_fast_inv_i (ram_C0C0.v_C0EA)

/* 0xC100: the grid PLL */
extern struct a_page_C100 {
    float v_C100;
    unsigned int p_C102[12];
    float v_C10E;
    float v_C110;
    float v_C112;
    float v_C114;
    unsigned int p_C116[2];
    unsigned int v_C118;
    unsigned int v_C119;
    unsigned int p_C11A[1];
    unsigned int v_C11B;
    unsigned int v_C11C;
    unsigned int p_C11D[1];
    float v_C11E;
    float v_C120;
    float v_C122;
    float v_C124;
    float v_C126;
    float v_C128;
    float v_C12A;
    float v_C12C;
    float v_C12E;
} ram_C100;
#define grid_pll (ram_C100.v_C100)
#define grid_pll_theta (ram_C100.v_C10E)
#define ram_word_c110 (ram_C100.v_C110)
#define ram_word_c112 (ram_C100.v_C112)
#define grid_pll_c (ram_C100.v_C114)
#define grid_pll_n (ram_C100.v_C118)
#define grid_pll_period (ram_C100.v_C119)
#define grid_pll_wrap (ram_C100.v_C11B)
#define pll_locked (ram_C100.v_C11C)
#define grid_pll_pi_e (ram_C100.v_C11E)
#define grid_pll_pi_kp (ram_C100.v_C120)
#define grid_pll_pi_ki (ram_C100.v_C122)
#define grid_pll_pi_kd (ram_C100.v_C124)
#define grid_pll_pi_umax (ram_C100.v_C126)
#define grid_pll_pi_umin (ram_C100.v_C128)
#define grid_pll_pi_e1 (ram_C100.v_C12A)
#define grid_pll_pi_i (ram_C100.v_C12C)
#define grid_pll_pi_u (ram_C100.v_C12E)

extern struct a_page_C140 {
    float v_C140;
    float v_C142;
    float v_C144;
    float v_C146;
    float v_C148;
    float v_C14A;
    float v_C14C;
    float v_C14E;
    float v_C150;
    float v_C152;
    float v_C154;
    float v_C156;
    float v_C158;
    float v_C15A;
    float v_C15C;
    float v_C15E;
    float v_C160;
    float v_C162;
    float v_C164;
    float v_C166;
    float v_C168;
    float v_C16A;
    float v_C16C;
    float v_C16E;
    float v_C170;
    float v_C172;
    unsigned int v_C174;
} ram_C140;
#define meas_avg (ram_C140.v_C140)
#define meas_avg_pv2_v_ref (ram_C140.v_C142)
#define meas_avg_pv1_v_acc (ram_C140.v_C144)
#define meas_avg_pv2_v_acc (ram_C140.v_C146)
#define meas_avg_bus_v_acc (ram_C140.v_C148)
#define meas_avg_hv_v_acc (ram_C140.v_C14A)
#define meas_avg_batt_v_acc (ram_C140.v_C14C)
#define meas_avg_pv1_v (ram_C140.v_C14E)
#define meas_avg_pv2_v (ram_C140.v_C150)
#define meas_avg_bus_v (ram_C140.v_C152)
#define meas_avg_bus_v_z1 (ram_C140.v_C154)
#define meas_avg_bus_v_z2 (ram_C140.v_C156)
#define meas_avg_hv_v (ram_C140.v_C158)
#define meas_avg_batt_v (ram_C140.v_C15A)
#define meas_avg_bus_v_filt (ram_C140.v_C15C)
#define meas_avg_bus_v_filt_z1 (ram_C140.v_C15E)
#define meas_avg_bus_v_filt_z2 (ram_C140.v_C160)
#define meas_avg_pv1_i_acc (ram_C140.v_C162)
#define meas_avg_pv2_i_acc (ram_C140.v_C164)
#define meas_avg_pv1_i (ram_C140.v_C166)
#define meas_avg_pv2_i (ram_C140.v_C168)
#define pv_gain_isr (ram_C140.v_C16A)
#define meas_avg_pv2_v_k (ram_C140.v_C16C)
#define dc_gain_isr (ram_C140.v_C16E)
#define dc_gain_isr_hv_i (ram_C140.v_C170)
#define meas_avg_bus_v_k (ram_C140.v_C172)
#define meas_avg_pv_ramp_tick (ram_C140.v_C174)

extern struct a_page_C180 {
    float v_C180;
    unsigned int p_C182[4];
    float v_C186;
    unsigned int p_C188[4];
    float v_C18C;
    unsigned int p_C18E[4];
    float v_C192;
    float v_C194;
    float v_C196;
    float v_C198;
    unsigned int v_C19A;
    unsigned int p_C19B[1];
    float v_C19C;
    unsigned int p_C19E[4];
    float v_C1A2;
    unsigned int p_C1A4[4];
    float v_C1A8;
    unsigned int p_C1AA[10];
    float v_C1B4;
} ram_C180;
#define q_loop (ram_C180.v_C180)
#define q_loop_i_in (ram_C180.v_C186)
#define q_loop_cos_in (ram_C180.v_C18C)
#define q_loop_kp (ram_C180.v_C192)
#define q_loop_ki (ram_C180.v_C194)
#define q_loop_max (ram_C180.v_C196)
#define q_loop_min (ram_C180.v_C198)
#define q_loop_n (ram_C180.v_C19A)
#define q_loop_ref_sum (ram_C180.v_C19C)
#define q_loop_meas_sum (ram_C180.v_C1A2)
#define q_loop_e (ram_C180.v_C1A8)
#define q_loop_u (ram_C180.v_C1B4)

extern struct a_page_C1C0 {
    unsigned int v_C1C0;
    unsigned int p_C1C1[2];
    unsigned int v_C1C3;
    unsigned int p_C1C4[2];
    unsigned int v_C1C6;
    unsigned int p_C1C7[2];
    unsigned int v_C1C9;
    unsigned int p_C1CA[2];
    unsigned int v_C1CC;
    unsigned int p_C1CD[2];
    unsigned int v_C1CF;
    unsigned int p_C1D0[2];
    unsigned int v_C1D2;
    unsigned int p_C1D3[2];
    unsigned int v_C1D5;
    unsigned int p_C1D6[2];
    unsigned int v_C1D8;
    unsigned int p_C1D9[2];
    unsigned int v_C1DB;
    unsigned int p_C1DC[2];
    unsigned int v_C1DE;
    unsigned int p_C1DF[5];
    unsigned int v_C1E4;
    unsigned int p_C1E5[2];
    unsigned int v_C1E7;
    unsigned int p_C1E8[2];
    unsigned int v_C1EA;
    unsigned int p_C1EB[2];
    unsigned int v_C1ED;
    unsigned int p_C1EE[2];
    unsigned int v_C1F0;
    unsigned int p_C1F1[2];
    unsigned int v_C1F3;
    unsigned int p_C1F4[2];
    unsigned int v_C1F6;
} ram_C1C0;
#define adc_ch (ram_C1C0.v_C1C0)
#define pv2_v_raw_v (ram_C1C0.v_C1C3)
#define pv1_i_raw_v (ram_C1C0.v_C1C6)
#define pv2_i_raw_v (ram_C1C0.v_C1C9)
#define iso_raw_v (ram_C1C0.v_C1CC)
#define bus_v_raw_v (ram_C1C0.v_C1CF)
#define hv_v_raw_v (ram_C1C0.v_C1D2)
#define spare_ac_raw_v (ram_C1C0.v_C1D5)
#define batt_v_raw_v (ram_C1C0.v_C1D8)
#define hv_i_raw_v (ram_C1C0.v_C1DB)
#define inv_v_raw_v (ram_C1C0.v_C1DE)
#define leak_i_raw_v (ram_C1C0.v_C1E4)
#define dci_i_raw_v (ram_C1C0.v_C1E7)
#define grid_v_raw_v (ram_C1C0.v_C1EA)
#define inv_i_raw_v (ram_C1C0.v_C1ED)
#define eps_v_raw_v (ram_C1C0.v_C1F0)
#define aux17_raw_v (ram_C1C0.v_C1F3)
#define aux18_raw_v (ram_C1C0.v_C1F6)

/* 0xC200: the battery converter's references (dcdc_ref, isr_limits) */
extern struct a_page_C200 {
    unsigned int p_C200[4];
    float v_C204;
    float v_C206;
    float v_C208;
    float v_C20A;
    unsigned int p_C20C[2];
    float v_C20E;
    float v_C210;
    float v_C212;
    float v_C214;
    float v_C216;
    float v_C218;
    float v_C21A;
    float v_C21C;
    unsigned int p_C21E[2];
    float v_C220;
    float v_C222;
    float v_C224;
    float v_C226;
    float v_C228;
    float v_C22A;
    float v_C22C;
    float v_C22E;
    unsigned int p_C230[2];
    float v_C232;
    float v_C234;
    float v_C236;
    float v_C238;
    float v_C23A;
    unsigned int v_C23C;
} ram_C200;
#define dcdc_ref_i_limit (ram_C200.v_C204)
#define full_trim_isr (ram_C200.v_C206)
#define isr_limits (ram_C200.v_C208)
#define dcdc_ref_batt_v_min (ram_C200.v_C20A)
#define dcdc_ref_i_cmd_max (ram_C200.v_C20E)
#define dcdc_ref_duty_ff (ram_C200.v_C210)
#define dcdc_ref_chg_bound (ram_C200.v_C212)
#define dcdc_ref_dis_bound (ram_C200.v_C214)
#define batt_current_cmd_isr (ram_C200.v_C216)
#define wake_batt_pi_e (ram_C200.v_C218)
#define wake_batt_pi_kp (ram_C200.v_C21A)
#define wake_batt_pi_ki (ram_C200.v_C21C)
#define wake_batt_pi_umax (ram_C200.v_C220)
#define wake_batt_pi_umin (ram_C200.v_C222)
#define wake_batt_pi_e1 (ram_C200.v_C224)
#define wake_batt_pi_i (ram_C200.v_C226)
#define wake_batt_pi_u (ram_C200.v_C228)
#define wake_bus_pi_e (ram_C200.v_C22A)
#define wake_bus_pi_kp (ram_C200.v_C22C)
#define wake_bus_pi_ki (ram_C200.v_C22E)
#define wake_bus_pi_umax (ram_C200.v_C232)
#define wake_bus_pi_umin (ram_C200.v_C234)
#define wake_bus_pi_e1 (ram_C200.v_C236)
#define wake_bus_pi_i (ram_C200.v_C238)
#define wake_bus_pi_u (ram_C200.v_C23A)
#define dcdc_soft_duty (ram_C200.v_C23C)

extern struct a_page_C240 {
    float v_C240;
    float v_C242;
    float v_C244;
    float v_C246;
    float v_C248;
    float v_C24A;
    float v_C24C;
    float v_C24E;
    float v_C250;
    float v_C252;
    float v_C254;
    float v_C256;
    float v_C258;
    float v_C25A;
    float v_C25C;
    unsigned int v_C25E;
    unsigned int v_C25F;
    unsigned int v_C260;
    unsigned int v_C261;
    float v_C262;
    float v_C264;
    float v_C266;
    unsigned int p_C268[1];
    unsigned int v_C269;
    unsigned int p_C26A[2];
    float v_C26C;
    float v_C26E;
    unsigned int p_C270[2];
    float v_C272;
    float v_C274;
    float v_C276;
    float v_C278;
    float v_C27A;
    float v_C27C;
    float v_C27E;
} ram_C240;
#define dcdc_cc (ram_C240.v_C240)
#define dcdc_cc_bus_v (ram_C240.v_C242)
#define dcdc_cc_hv_i (ram_C240.v_C244)
#define dcdc_cc_bus_v_ref (ram_C240.v_C246)
#define dcdc_cc_i_cmd (ram_C240.v_C248)
#define dcdc_cc_i_limit (ram_C240.v_C24A)
#define dcdc_cc_i_offset (ram_C240.v_C24C)
#define dcdc_cc_batt_v_max (ram_C240.v_C24E)
#define dcdc_cc_batt_v_min (ram_C240.v_C250)
#define dcdc_cc_i_cmd_max (ram_C240.v_C252)
#define dcdc_cc_duty_ff (ram_C240.v_C254)
#define dcdc_cc_chg_bound (ram_C240.v_C256)
#define dcdc_cc_dis_bound (ram_C240.v_C258)
#define ram_word_c25a (ram_C240.v_C25A)
#define dcdc_cc_bus_band (ram_C240.v_C25C)
#define dcdc_cc_mode (ram_C240.v_C25E)
#define dcdc_cc_slot (ram_C240.v_C25F)
#define dcdc_cc_bms_mode (ram_C240.v_C260)
#define dcdc_cc_batt_full (ram_C240.v_C261)
#define dcdc_cc_i_cmd_prev (ram_C240.v_C262)
#define cc_batt_current_ref (ram_C240.v_C264)
#define dcdc_cc_duty (ram_C240.v_C266)
#define eps_start_ok (ram_C240.v_C269)
#define pi_bus_sync_kp (ram_C240.v_C26C)
#define pi_bus_sync_ki (ram_C240.v_C26E)
#define pi_bus_sync_umax (ram_C240.v_C272)
#define pi_bus_sync_umin (ram_C240.v_C274)
#define pi_bus_sync_e1 (ram_C240.v_C276)
#define pi_bus_sync_i (ram_C240.v_C278)
#define pi_bus_sync_u (ram_C240.v_C27A)
#define pi_bus_e (ram_C240.v_C27C)
#define pi_bus_kp (ram_C240.v_C27E)

extern struct a_page_C280 {
    float v_C280;
    unsigned int p_C282[2];
    float v_C284;
    float v_C286;
    float v_C288;
    float v_C28A;
    float v_C28C;
    float v_C28E;
    float v_C290;
    float v_C292;
    unsigned int p_C294[2];
    float v_C296;
    float v_C298;
    float v_C29A;
    float v_C29C;
    float v_C29E;
    float v_C2A0;
    float v_C2A2;
    float v_C2A4;
    unsigned int p_C2A6[2];
    float v_C2A8;
    float v_C2AA;
    float v_C2AC;
    float v_C2AE;
    float v_C2B0;
    float v_C2B2;
    float v_C2B4;
    float v_C2B6;
    unsigned int p_C2B8[2];
    float v_C2BA;
    float v_C2BC;
    float v_C2BE;
} ram_C280;
#define pi_bus_ki (ram_C280.v_C280)
#define pi_bus_umax (ram_C280.v_C284)
#define pi_bus_umin (ram_C280.v_C286)
#define pi_bus_e1 (ram_C280.v_C288)
#define pi_bus_i (ram_C280.v_C28A)
#define pi_bus_u (ram_C280.v_C28C)
#define pi_bus_hi_e (ram_C280.v_C28E)
#define pi_bus_hi_kp (ram_C280.v_C290)
#define pi_bus_hi_ki (ram_C280.v_C292)
#define pi_bus_hi_umax (ram_C280.v_C296)
#define pi_bus_hi_umin (ram_C280.v_C298)
#define pi_bus_hi_e1 (ram_C280.v_C29A)
#define pi_bus_hi_i (ram_C280.v_C29C)
#define pi_bus_hi_u (ram_C280.v_C29E)
#define pi_bus_lo_e (ram_C280.v_C2A0)
#define pi_bus_lo_kp (ram_C280.v_C2A2)
#define pi_bus_lo_ki (ram_C280.v_C2A4)
#define pi_bus_lo_umax (ram_C280.v_C2A8)
#define pi_bus_lo_umin (ram_C280.v_C2AA)
#define pi_bus_lo_e1 (ram_C280.v_C2AC)
#define pi_bus_lo_i (ram_C280.v_C2AE)
#define pi_bus_lo_u (ram_C280.v_C2B0)
#define pi_v_max_e (ram_C280.v_C2B2)
#define pi_v_max_kp (ram_C280.v_C2B4)
#define pi_v_max_ki (ram_C280.v_C2B6)
#define pi_v_max_umax (ram_C280.v_C2BA)
#define pi_v_max_umin (ram_C280.v_C2BC)
#define pi_v_max_e1 (ram_C280.v_C2BE)

extern struct a_page_C2C0 {
    float v_C2C0;
    float v_C2C2;
    float v_C2C4;
    float v_C2C6;
    float v_C2C8;
    unsigned int p_C2CA[2];
    float v_C2CC;
    float v_C2CE;
    float v_C2D0;
    float v_C2D2;
    float v_C2D4;
    float v_C2D6;
    float v_C2D8;
    float v_C2DA;
    unsigned int p_C2DC[2];
    float v_C2DE;
    float v_C2E0;
    float v_C2E2;
    float v_C2E4;
    float v_C2E6;
} ram_C2C0;
#define pi_v_max_i (ram_C2C0.v_C2C0)
#define pi_v_max_u (ram_C2C0.v_C2C2)
#define pi_v_min_e (ram_C2C0.v_C2C4)
#define pi_v_min_kp (ram_C2C0.v_C2C6)
#define pi_v_min_ki (ram_C2C0.v_C2C8)
#define pi_v_min_umax (ram_C2C0.v_C2CC)
#define pi_v_min_umin (ram_C2C0.v_C2CE)
#define pi_v_min_e1 (ram_C2C0.v_C2D0)
#define pi_v_min_i (ram_C2C0.v_C2D2)
#define pi_v_min_u (ram_C2C0.v_C2D4)
#define pi_i_e (ram_C2C0.v_C2D6)
#define dcdc_cc_pi_i_kp (ram_C2C0.v_C2D8)
#define dcdc_cc_pi_i_ki (ram_C2C0.v_C2DA)
#define dcdc_cc_pi_i_umax (ram_C2C0.v_C2DE)
#define dcdc_cc_pi_i_umin (ram_C2C0.v_C2E0)
#define dcdc_cc_pi_i_e1 (ram_C2C0.v_C2E2)
#define dcdc_cc_pi_i_i (ram_C2C0.v_C2E4)
#define dcdc_cc_pi_i_u (ram_C2C0.v_C2E6)

extern struct a_page_C300 {
    float v_C300;
    float v_C302;
    float v_C304;
    unsigned int v_C306;
} ram_C300;
#define grid_v_win (ram_C300.v_C300)
#define grid_v_win_max (ram_C300.v_C302)
#define grid_v_win_min (ram_C300.v_C304)
#define grid_v_win_mode (ram_C300.v_C306)

extern struct a_page_C340 {
    unsigned int p_C340[16];
    unsigned int v_C350;
} ram_C340;
#define ram_word_c350 (ram_C340.v_C350)

extern struct a_page_C3C0 {
    unsigned int p_C3C0[23];
    unsigned int v_C3D7;
} ram_C3C0;
#define grid_v_win_ok (ram_C3C0.v_C3D7)

/* 0xC400: the grid current controller: i_ref_max/i_ref_min (C412/C414), q_current_ref (C408) and
   dci_offset (C42A) are task 4's outputs */
extern struct a_page_C400 {
    float v_C400;
    float v_C402;
    float v_C404;
    float v_C406;
    float v_C408;
    float v_C40A;
    float v_C40C;
    float v_C40E;
    float v_C410;
    float v_C412;
    float v_C414;
    float v_C416;
    float v_C418;
    float v_C41A;
    unsigned int v_C41C;
    unsigned int v_C41D;
    unsigned int v_C41E;
    unsigned int v_C41F;
    float v_C420;
    float v_C422;
    float v_C424;
    float v_C426;
    float v_C428;
    float v_C42A;
    float v_C42C;
    float v_C42E;
    float v_C430;
    float v_C432;
    float v_C434;
    float v_C436;
    float v_C438;
    unsigned int p_C43A[2];
    float v_C43C;
    float v_C43E;
} ram_C400;
#define grid_ictl_blk (ram_C400.v_C400)
#define grid_ictl_blk_i_meas (ram_C400.v_C402)
#define grid_ictl_blk_i_ref_p (ram_C400.v_C404)
#define grid_ictl_blk_i_ref_q (ram_C400.v_C406)
#define q_current_ref (ram_C400.v_C408)
#define grid_ictl_blk_bus_v (ram_C400.v_C40A)
#define bus_ref_isr (ram_C400.v_C40C)
#define mod_gain_isr (ram_C400.v_C40E)
#define grid_ictl_blk_amp_ramp (ram_C400.v_C410)
#define i_ref_max (ram_C400.v_C412)
#define i_ref_min (ram_C400.v_C414)
#define grid_ictl_blk_sin_ref (ram_C400.v_C416)
#define grid_ictl_blk_cos_ref (ram_C400.v_C418)
#define grid_ictl_blk_kp_i (ram_C400.v_C41A)
#define grid_ictl_blk_ramp_mode (ram_C400.v_C41C)
#define grid_ictl_blk_rep_lead (ram_C400.v_C41D)
#define grid_ictl_blk_rep_idx (ram_C400.v_C41E)
#define grid_ictl_blk_rep_len (ram_C400.v_C41F)
#define grid_ictl_blk_i_ref (ram_C400.v_C420)
#define grid_ictl_blk_i_err (ram_C400.v_C422)
#define grid_ictl_blk_v_cmd (ram_C400.v_C424)
#define grid_ictl_blk_island_inj (ram_C400.v_C426)
#define grid_ictl_blk_q_corr (ram_C400.v_C428)
#define dci_offset (ram_C400.v_C42A)
#define grid_ictl_blk_amp_cmd (ram_C400.v_C42C)
#define grid_ictl_blk_pi_e (ram_C400.v_C42E)
#define grid_ictl_blk_pi_kp (ram_C400.v_C430)
#define grid_ictl_blk_pi_ki (ram_C400.v_C432)
#define grid_ictl_blk_pi_kd (ram_C400.v_C434)
#define grid_ictl_blk_pi_umax (ram_C400.v_C436)
#define grid_ictl_blk_pi_umin (ram_C400.v_C438)
#define grid_ictl_blk_pi_i (ram_C400.v_C43C)
#define grid_ictl_blk_pi_u (ram_C400.v_C43E)

extern struct a_page_C440 {
    unsigned int p_C440[2];
    float v_C442;
    float v_C444;
    float v_C446;
    float v_C448;
    float v_C44A;
    float v_C44C;
    float v_C44E;
    float v_C450;
    unsigned int v_C452;
} ram_C440;
#define grid_rep_rep_in (ram_C440.v_C442)
#define grid_rep_rep_max (ram_C440.v_C444)
#define grid_rep_rep_min (ram_C440.v_C446)
#define grid_rep_rep_a (ram_C440.v_C448)
#define grid_rep_rep_b (ram_C440.v_C44A)
#define grid_rep_rep_gain (ram_C440.v_C44C)
#define grid_rep_rep_y (ram_C440.v_C44E)
#define grid_rep_rep_out (ram_C440.v_C450)
#define grid_rep_buf (ram_C440.v_C452)

extern struct a_page_C800 {
    unsigned int p_C800[58];
    unsigned int v_C83A;
} ram_C800;
#define ctrl_slot (ram_C800.v_C83A)

/* 0xC840: the EPS voltage controller */
extern struct a_page_C840 {
    float v_C840;
    float v_C842;
    float v_C844;
    float v_C846;
    float v_C848;
    float v_C84A;
    float v_C84C;
    unsigned int v_C84E;
    unsigned int v_C84F;
    float v_C850;
    unsigned int p_C852[2];
    unsigned int v_C854;
    unsigned int p_C855[5];
    float v_C85A;
    float v_C85C;
    float v_C85E;
    float v_C860;
    float v_C862;
    unsigned int p_C864[4];
    unsigned int v_C868;
} ram_C840;
#define eps_vctl_blk (ram_C840.v_C840)
#define eps_vctl_blk_v_meas (ram_C840.v_C842)
#define eps_vctl_blk_i_meas (ram_C840.v_C844)
#define ram_word_c846 (ram_C840.v_C846)
#define eps_vctl_blk_i_filt (ram_C840.v_C848)
#define eps_vctl_blk_sin_ref (ram_C840.v_C84A)
#define eps_vctl_blk_i_limit (ram_C840.v_C84C)
#define eps_vctl_blk_mode (ram_C840.v_C84E)
#define eps_vctl_blk_rep_lead (ram_C840.v_C84F)
#define eps_v_peak (ram_C840.v_C850)
#define eps_vctl_enable (ram_C840.v_C854)
#define eps_vctl_blk_rep_max (ram_C840.v_C85A)
#define eps_vctl_blk_rep_min (ram_C840.v_C85C)
#define eps_vctl_blk_rep_a (ram_C840.v_C85E)
#define eps_vctl_blk_rep_b (ram_C840.v_C860)
#define eps_vctl_blk_rep_gain (ram_C840.v_C862)
#define eps_rep_buf (ram_C840.v_C868)

extern struct a_page_CC40 {
    unsigned int p_CC40[16];
    float v_CC50;
    float v_CC52;
    float v_CC54;
    float v_CC56;
    float v_CC58;
    float v_CC5A;
    float v_CC5C;
    float v_CC5E;
    float v_CC60;
    unsigned int p_CC62[2];
    float v_CC64;
    float v_CC66;
    float v_CC68;
    float v_CC6A;
    float v_CC6C;
    float v_CC6E;
    float v_CC70;
    float v_CC72;
    float v_CC74;
    float v_CC76;
    float v_CC78;
    float v_CC7A;
    float v_CC7C;
    float v_CC7E;
} ram_CC40;
#define eps_vctl_tail_pi_v_e (ram_CC40.v_CC50)
#define eps_vctl_tail_pi_v_kp (ram_CC40.v_CC52)
#define eps_vctl_tail_pi_v_ki (ram_CC40.v_CC54)
#define eps_vctl_tail_pi_v_kd (ram_CC40.v_CC56)
#define eps_vctl_tail_pi_v_umax (ram_CC40.v_CC58)
#define eps_vctl_tail_pi_v_umin (ram_CC40.v_CC5A)
#define eps_vctl_tail_pi_v_e1 (ram_CC40.v_CC5C)
#define eps_vctl_tail_pi_v_i (ram_CC40.v_CC5E)
#define eps_vctl_tail_pi_v_u (ram_CC40.v_CC60)
#define eps_vctl_tail_pi_i_kp (ram_CC40.v_CC64)
#define eps_vctl_tail_pi_i_ki (ram_CC40.v_CC66)
#define eps_vctl_tail_pi_i_kd (ram_CC40.v_CC68)
#define eps_vctl_tail_pi_i_umax (ram_CC40.v_CC6A)
#define eps_vctl_tail_pi_i_umin (ram_CC40.v_CC6C)
#define eps_vctl_tail_pi_i_e1 (ram_CC40.v_CC6E)
#define eps_vctl_tail_pi_i_i (ram_CC40.v_CC70)
#define eps_vctl_tail_pi_i_u (ram_CC40.v_CC72)
#define eps_dc_trim_isr (ram_CC40.v_CC74)
#define eps_vctl_out (ram_CC40.v_CC76)
#define eps_vctl_tail_i_ref (ram_CC40.v_CC78)
#define mod_gain (ram_CC40.v_CC7A)
#define eps_vctl_tail_duty (ram_CC40.v_CC7C)
#define eps_theta (ram_CC40.v_CC7E)

extern struct a_page_CC80 {
    unsigned int v_CC80;
    unsigned int v_CC81;
    unsigned int v_CC82;
    unsigned int p_CC83[9];
    float v_CC8C;
    float v_CC8E;
    unsigned int p_CC90[12];
    unsigned int v_CC9C;
    unsigned int p_CC9D[1];
    float v_CC9E;
    unsigned int v_CCA0;
    unsigned int p_CCA1[1];
    float v_CCA2;
    unsigned int v_CCA4;
    unsigned int p_CCA5[1];
    float v_CCA6;
    unsigned int v_CCA8;
    unsigned int p_CCA9[1];
    float v_CCAA;
    unsigned int v_CCAC;
    unsigned int p_CCAD[1];
    float v_CCAE;
    unsigned int p_CCB0[2];
    unsigned int v_CCB2;
    unsigned int v_CCB3;
    float v_CCB4;
    float v_CCB6;
    float v_CCB8;
    unsigned int v_CCBA;
    unsigned int p_CCBB[3];
    float v_CCBE;
} ram_CC80;
#define eps_cycle_len (ram_CC80.v_CC80)
#define eps_cycle_cycle_idx (ram_CC80.v_CC81)
#define eps_cycle_cycle_wrap (ram_CC80.v_CC82)
#define CpuTimer2 (ram_CC80.v_CC8C)
#define CpuTimer2_InterruptCount (ram_CC80.v_CC8E)
#define adc_offsets (ram_CC80.v_CC9C)
#define adc_offsets_sum (ram_CC80.v_CC9E)
#define eps_cycle_hv_i_ofs (ram_CC80.v_CCA0)
#define adc_offsets_sum_cca2 (ram_CC80.v_CCA2)
#define eps_cycle_pv1_i_ofs (ram_CC80.v_CCA4)
#define adc_offsets_sum_cca6 (ram_CC80.v_CCA6)
#define eps_cycle_pv2_i_ofs (ram_CC80.v_CCA8)
#define adc_offsets_sum_ccaa (ram_CC80.v_CCAA)
#define eps_cycle_inv_i_ofs (ram_CC80.v_CCAC)
#define adc_offsets_sum_ccae (ram_CC80.v_CCAE)
#define iq_flag (ram_CC80.v_CCB2)
#define iq_count (ram_CC80.v_CCB3)
#define iq_consts (ram_CC80.v_CCB4)
#define iq_consts_one_twelfth (ram_CC80.v_CCB6)
#define iq_consts_k1000h (ram_CC80.v_CCB8)
#define iq_consts_zero (ram_CC80.v_CCBA)
#define cc_i_work (ram_CC80.v_CCBE)

#define fast_trip_hold (rocof_blk.cnt)
#define rocof_freq_in (rocof_blk.osc.x)
#define rocof_window (rocof_blk.osc.n)
#define rocof_enable (rocof_blk.osc.en)
#define rocof_threshold (rocof_blk.osc.thr)
#define rocof_trip (rocof_blk.osc.flag)
#define osc_sum (rocof_blk.osc.sum)

#define auto_test_state (autotest.v_CDE0)
#define autotest_ramp_cnt (autotest.v_CDE1)
#define autotest_trip_ms (autotest.v_CDE2)
#define autotest_elapsed_ms (autotest.v_CDE3)
#define autotest_phase (autotest.v_CDE4)
#define autotest_trip_value (autotest.v_CDE5)
#define autotest_value (autotest.v_CDE6)
#define autotest_expect_time (autotest.v_CDE7)
#define autotest_limit (autotest.v_CDE8)
#define autotest_trip_value2 (autotest.v_CDE9)
#define autotest_result (autotest.v_CDEA)
#define autotest_hold (autotest.v_CDEB)

/* 0xCF12: the fault words CF12..CF19 (faults.h) */
extern struct a_blk_CF12 {
    union bits16_u v_CF12;
    union bits16_u v_CF13;
    union bits16_u v_CF14;
    union bits16_u v_CF15;
    union bits16_u v_CF16;
    union bits16_u v_CF17;
    union bits16_u v_CF18;
    union bits16_u v_CF19;
} ram_CF12;
#define fault_grid1 (ram_CF12.v_CF12)
#define fault_grid2 (ram_CF12.v_CF13)
#define fault_grid_eps (ram_CF12.v_CF14)
#define fault_batt (ram_CF12.v_CF15)
#define fault_pv (ram_CF12.v_CF16)
#define fault_stop1 (ram_CF12.v_CF17)
#define fault_stop2 (ram_CF12.v_CF18)
#define batt_blocks (ram_CF12.v_CF19)

/* 0xCF1C: the power limits block (out_power_limit at CF2C) */
extern struct a_blk_CF1C {
    float v_CF1C;
    float v_CF1E;
    float v_CF20;
    float v_CF22;
    float v_CF24;
    float v_CF26;
    float v_CF28;
    unsigned int p_CF2A[2];
    float v_CF2C;
    float v_CF2E;
    float v_CF30;
    float v_CF32;
} ram_CF1C;
#define lim_soft_start (ram_CF1C.v_CF1C)
#define lim_ofr (ram_CF1C.v_CF1E)
#define lim_volt_watt (ram_CF1C.v_CF20)
#define lim_rated (ram_CF1C.v_CF22)
#define lim_temp (ram_CF1C.v_CF24)
#define lim_dc_side (ram_CF1C.v_CF26)
#define lim_export (ram_CF1C.v_CF28)
#define out_power_limit (ram_CF1C.v_CF2C)
#define lim_by_q (ram_CF1C.v_CF2E)
#define lim_rate (ram_CF1C.v_CF30)
#define p_rate_limit_prev (ram_CF1C.v_CF32)

extern struct a_page_CF40 {
    unsigned int v_CF40;
    unsigned int v_CF41;
    unsigned int v_CF42;
    unsigned int v_CF43;
    unsigned int v_CF44;
    unsigned int p_CF45[1];
    unsigned int v_CF46;
    unsigned int p_CF47[1];
    unsigned int v_CF48;
    unsigned int p_CF49[9];
    unsigned int v_CF52;
} ram_CF40;
#define arm_rx_len (ram_CF40.v_CF40)
#define sci_tx_done (ram_CF40.v_CF41)
#define upgrade_tag (ram_CF40.v_CF42)
#define sci_word_cf43 (ram_CF40.v_CF43)
#define sci_word_cf44 (ram_CF40.v_CF44)
#define sci_tx_state_1_pos (ram_CF40.v_CF48)

extern struct a_page_CF80 {
    unsigned int v_CF80;
} ram_CF80;
#define sci_tx_buf0 (ram_CF80.v_CF80)

extern struct a_page_D000 {
    unsigned int v_D000;
} ram_D000;

extern struct a_page_D100 {
    unsigned int p_D100[44];
    unsigned int v_D12C;
    unsigned int v_D12D;
    unsigned int v_D12E;
    unsigned int v_D12F;
    unsigned int v_D130;
    unsigned int v_D131;
    unsigned int v_D132;
    unsigned int v_D133;
    unsigned int v_D134;
    unsigned int v_D135;
    unsigned int v_D136;
    unsigned int v_D137;
    unsigned int v_D138;
    unsigned int v_D139;
    unsigned int v_D13A;
    unsigned int v_D13B;
    unsigned int v_D13C;
    unsigned int v_D13D;
    float v_D13E;
} ram_D100;
#define pv2_hold_count (ram_D100.v_D12C)
#define pv1_hold_count (ram_D100.v_D12D)
#define pv1_low_power_count (ram_D100.v_D12E)
#define pv1_plim_step_count (ram_D100.v_D12F)
#define pv1_po_prev (ram_D100.v_D130)
#define pv1_po_state (ram_D100.v_D131)
#define pv2_po_prev (ram_D100.v_D132)
#define pv2_po_state (ram_D100.v_D133)
#define pv2_ref_low_count (ram_D100.v_D134)
#define pv2_ref_high_count (ram_D100.v_D135)
#define voc_wait_count (ram_D100.v_D136)
#define pv_both_low_count (ram_D100.v_D137)
#define pv1_ref_low_count (ram_D100.v_D138)
#define pv1_ref_high_count (ram_D100.v_D139)
#define pv2_low_power_count (ram_D100.v_D13A)
#define pv2_start_count (ram_D100.v_D13B)
#define pv2_plim_step_count (ram_D100.v_D13C)
#define pv1_start_count (ram_D100.v_D13D)

extern struct a_page_D140 {
    unsigned int v_D140;
    unsigned int v_D141;
    unsigned int v_D142;
    unsigned int v_D143;
    unsigned int v_D144;
    unsigned int v_D145;
    unsigned int v_D146;
    unsigned int v_D147;
    unsigned int v_D148;
    unsigned int p_D149[1];
    unsigned int v_D14A;
    unsigned int v_D14B;
    unsigned int v_D14C;
    unsigned int v_D14D;
    unsigned int v_D14E;
    unsigned int v_D14F;
    unsigned int v_D150;
    unsigned int v_D151;
    unsigned int v_D152;
    unsigned int v_D153;
    unsigned int v_D154;
    unsigned int v_D155;
    unsigned int v_D156;
    unsigned int v_D157;
    unsigned int v_D158;
    unsigned int v_D159;
    unsigned int v_D15A;
    unsigned int v_D15B;
    unsigned int v_D15C;
    unsigned int p_D15D[1];
    float v_D15E;
    float v_D160;
    float v_D162;
    float v_D164;
    float v_D166;
    float v_D168;
    float v_D16A;
    float v_D16C;
    float v_D16E;
    float v_D170;
    float v_D172;
    float v_D174;
    unsigned int v_D176;
    unsigned int p_D177[1];
    unsigned int v_D178;
} ram_D140;
#define bus_ov_count (ram_D140.v_D140)
#define cf12b3_clear_count (ram_D140.v_D141)
#define pv_low_wait_count (ram_D140.v_D142)
#define pv_restart_count (ram_D140.v_D143)
#define pv_low_count (ram_D140.v_D144)
#define pv_back_count (ram_D140.v_D145)
#define pv_ov_clear_count (ram_D140.v_D146)
#define rescan_settle_count (ram_D140.v_D147)
#define iso_step (ram_D140.v_D148)
#define pv_detect_retry (ram_D140.v_D14A)
#define pv_ov_count (ram_D140.v_D14B)
#define pv_detect_done (ram_D140.v_D14C)
#define eps_bus_uv_slow_count (ram_D140.v_D14D)
#define pv2_short_count (ram_D140.v_D14E)
#define pv1_short_count (ram_D140.v_D14F)
#define pv2_plim_count (ram_D140.v_D150)
#define eps_bus_vs_hv_count (ram_D140.v_D151)
#define eps_bus_uv_fast_count (ram_D140.v_D152)
#define pv1_plim_count (ram_D140.v_D153)
#define ntc_count (ram_D140.v_D154)
#define heatsink_temp_count (ram_D140.v_D155)
#define iso_fail_count (ram_D140.v_D156)
#define batt_temp_count (ram_D140.v_D157)
#define iso_fault_timer (ram_D140.v_D158)
#define bus_low_count (ram_D140.v_D159)
#define charger_temp_count (ram_D140.v_D15A)
#define pv2_present (ram_D140.v_D15B)
#define pv1_present (ram_D140.v_D15C)
#define iso_resistance_out (ram_D140.v_D15E)
#define iso_sample (ram_D140.v_D160)
#define bus_v_filt (ram_D140.v_D162)
#define pv2_ref_max (ram_D140.v_D164)
#define pv1_ref_max (ram_D140.v_D166)
#define iso_avg_a (ram_D140.v_D168)
#define iso_avg_b (ram_D140.v_D16A)
#define iso_acc (ram_D140.v_D16C)
#define iso_bus_v (ram_D140.v_D16E)
#define iso_const50 (ram_D140.v_D170)
#define iso_resistance (ram_D140.v_D172)
#define iso_conductance (ram_D140.v_D174)
#define mppt_flags_blk (ram_D140.v_D176)
#define mppt_flags (ram_D140.v_D178)

extern struct a_page_D180 {
    unsigned int v_D180;
    unsigned int p_D181[1];
    float v_D182;
    float v_D184;
    float v_D186;
    float v_D188;
    float v_D18A;
    float v_D18C;
    float v_D18E;
    float v_D190;
    float v_D192;
    float v_D194;
    float v_D196;
    unsigned int p_D198[4];
    unsigned int v_D19C;
    unsigned int p_D19D[3];
    float v_D1A0;
    float v_D1A2;
    float v_D1A4;
    float v_D1A6;
    float v_D1A8;
    float v_D1AA;
    float v_D1AC;
    float v_D1AE;
    float v_D1B0;
    float v_D1B2;
} ram_D180;
#define dc_acc (ram_D180.v_D180)
#define dc_acc_a_inv_n (ram_D180.v_D182)
#define dc_acc_a_batt_v (ram_D180.v_D184)
#define dc_acc_a_hv_i (ram_D180.v_D186)
#define dc_acc_a_hv_v (ram_D180.v_D188)
#define dc_acc_a_pv1_v (ram_D180.v_D18A)
#define dc_acc_a_pv2_v (ram_D180.v_D18C)
#define dc_acc_a_pv1_i (ram_D180.v_D18E)
#define dc_acc_a_pv2_i (ram_D180.v_D190)
#define dc_acc_a_bus_v (ram_D180.v_D192)
#define dc_acc_a_iso (ram_D180.v_D194)
#define dc_acc_a_aux (ram_D180.v_D196)
#define dc_acc_isr (ram_D180.v_D19C)
#define dc_acc_isr_batt_v (ram_D180.v_D1A0)
#define dc_acc_isr_hv_i (ram_D180.v_D1A2)
#define dc_acc_isr_hv_v (ram_D180.v_D1A4)
#define dc_acc_isr_pv1_v (ram_D180.v_D1A6)
#define dc_acc_isr_pv2_v (ram_D180.v_D1A8)
#define dc_acc_isr_pv1_i (ram_D180.v_D1AA)
#define dc_acc_isr_pv2_i (ram_D180.v_D1AC)
#define dc_acc_isr_bus_v (ram_D180.v_D1AE)
#define dc_acc_isr_iso (ram_D180.v_D1B0)
#define dc_acc_isr_aux (ram_D180.v_D1B2)

/* 0xD1C0: the MPPT block */
extern struct a_page_D1C0 {
    unsigned int v_D1C0;
    unsigned int p_D1C1[1];
    float v_D1C2;
    float v_D1C4;
    float v_D1C6;
    float v_D1C8;
    float v_D1CA;
    float v_D1CC;
    float v_D1CE;
    float v_D1D0;
    float v_D1D2;
    float v_D1D4;
    float v_D1D6;
    float v_D1D8;
    float v_D1DA;
    float v_D1DC;
    unsigned int v_D1DE;
    unsigned int v_D1DF;
    unsigned int v_D1E0;
    unsigned int v_D1E1;
} ram_D1C0;
#define mppt (ram_D1C0.v_D1C0)
#define mppt_pv1_ref (ram_D1C0.v_D1C2)
#define mppt_pv2_ref (ram_D1C0.v_D1C4)
#define mppt_pv1_voc (ram_D1C0.v_D1C6)
#define mppt_pv2_voc (ram_D1C0.v_D1C8)
#define mppt_pv1_sweep_min (ram_D1C0.v_D1CA)
#define mppt_pv2_sweep_min (ram_D1C0.v_D1CC)
#define mppt_pv1_ref_prev (ram_D1C0.v_D1CE)
#define mppt_pv2_ref_prev (ram_D1C0.v_D1D0)
#define mppt_pv1_p_sum (ram_D1C0.v_D1D2)
#define mppt_pv1_p_prev (ram_D1C0.v_D1D4)
#define mppt_pv2_p_sum (ram_D1C0.v_D1D6)
#define mppt_pv2_p_prev (ram_D1C0.v_D1D8)
#define mppt_p_hyst (ram_D1C0.v_D1DA)
#define mppt_v_step (ram_D1C0.v_D1DC)
#define mppt_pv1_sweep_steps (ram_D1C0.v_D1DE)
#define mppt_pv2_sweep_steps (ram_D1C0.v_D1DF)
#define mppt_pv1_p_count (ram_D1C0.v_D1E0)
#define mppt_pv2_p_count (ram_D1C0.v_D1E1)

/* 0xD200: the DC results of task 3 (dc_meas) */
extern struct a_page_D200 {
    float v_D200;
    float v_D202;
    float v_D204;
    float v_D206;
    float v_D208;
    float v_D20A;
    float v_D20C;
    float v_D20E;
    float v_D210;
    float v_D212;
    float v_D214;
    float v_D216;
    float v_D218;
    float v_D21A;
    float v_D21C;
    float v_D21E;
    float v_D220;
    float v_D222;
    float v_D224;
    float v_D226;
    float v_D228;
    float v_D22A;
    float v_D22C;
    float v_D22E;
    float v_D230;
} ram_D200;
#define dc_meas (ram_D200.v_D200)
#define dc_meas_pv2_v_raw (ram_D200.v_D202)
#define pv1_voltage (ram_D200.v_D204)
#define pv2_voltage (ram_D200.v_D206)
#define pv_voltage_max (ram_D200.v_D208)
#define dc_meas_pv_v_min (ram_D200.v_D20A)
#define bus_v_ref (ram_D200.v_D20C)
#define bus_v_ref_prev (ram_D200.v_D20E)
#define bus_voltage (ram_D200.v_D210)
#define batt_voltage (ram_D200.v_D212)
#define bus_hv_voltage (ram_D200.v_D214)
#define dc_meas_hv_i (ram_D200.v_D216)
#define batt_power (ram_D200.v_D218)
#define pv1_current_raw (ram_D200.v_D21A)
#define pv1_current (ram_D200.v_D21C)
#define pv2_current_raw (ram_D200.v_D21E)
#define pv2_current (ram_D200.v_D220)
#define dc_meas_pv1_p_filt (ram_D200.v_D222)
#define dc_meas_pv2_p_filt (ram_D200.v_D224)
#define pv_power (ram_D200.v_D226)
#define pv1_power (ram_D200.v_D228)
#define pv2_power (ram_D200.v_D22A)
#define heatsink_temp (ram_D200.v_D22C)
#define charger_temp (ram_D200.v_D22E)
#define batt_temp (ram_D200.v_D230)

extern struct a_page_D240 {
    float v_D240;
    float v_D242;
    float v_D244;
    float v_D246;
    float v_D248;
    float v_D24A;
    unsigned int v_D24C;
    unsigned int v_D24D;
    float v_D24E;
    unsigned int p_D250[14];
    float v_D25E;
    float v_D260;
    unsigned int p_D262[14];
    float v_D270;
    float v_D272;
    unsigned int p_D274[10];
    float v_D27E;
} ram_D240;
#define pv_phase (ram_D240.v_D240)
#define pv_phase_1_i_limit (ram_D240.v_D242)
#define pv_phase_1_i_max (ram_D240.v_D244)
#define pv_phase_1_v_loop_out (ram_D240.v_D246)
#define pv_phase_1_i_ref (ram_D240.v_D248)
#define pv_phase_1_duty (ram_D240.v_D24A)
#define pv_phase_1_cmp (ram_D240.v_D24C)
#define pv_phase_1_cmp_pending (ram_D240.v_D24D)
#define pv_phase_1_pi_bus_e (ram_D240.v_D24E)
#define pv_phase_1_pi_bus_u (ram_D240.v_D25E)
#define pv_phase_1_pi_precharge_e (ram_D240.v_D260)
#define pv_phase_1_pi_precharge_u (ram_D240.v_D270)
#define pv_phase_1_pi_v_e (ram_D240.v_D272)
#define pv_phase_1_pi_v_e1 (ram_D240.v_D27E)

extern struct a_page_D280 {
    float v_D280;
    float v_D282;
    float v_D284;
    unsigned int p_D286[2];
    float v_D288;
    unsigned int p_D28A[6];
    float v_D290;
    float v_D292;
    float v_D294;
    float v_D296;
    float v_D298;
    unsigned int p_D29A[2];
    float v_D29C;
    float v_D29E;
    float v_D2A0;
    unsigned int p_D2A2[1];
    unsigned int v_D2A3;
    float v_D2A4;
    unsigned int p_D2A6[16];
    float v_D2B6;
} ram_D280;
#define pv_phase_1_pi_v_i (ram_D280.v_D280)
#define pv_phase_1_pi_v_u (ram_D280.v_D282)
#define pv_phase_1_pi_i_e (ram_D280.v_D284)
#define pv_phase_1_pi_i_ki (ram_D280.v_D288)
#define pv_phase_1_pi_i_e1 (ram_D280.v_D290)
#define pv_phase_1_pi_i_i (ram_D280.v_D292)
#define pv_phase_1_pi_i_u (ram_D280.v_D294)
#define pv2_phase_acc (ram_D280.v_D296)
#define pv2_i_limit_alias (ram_D280.v_D298)
#define pv_phase_2_v_loop_out (ram_D280.v_D29C)
#define pv_phase_2_i_ref (ram_D280.v_D29E)
#define pv_phase_2_duty (ram_D280.v_D2A0)
#define pv_phase_2_cmp_pending (ram_D280.v_D2A3)
#define pv_phase_2_pi_bus_e (ram_D280.v_D2A4)
#define pv_phase_2_pi_precharge_e (ram_D280.v_D2B6)

extern struct a_page_D2C0 {
    unsigned int p_D2C0[30];
    float v_D2DE;
    unsigned int p_D2E0[12];
    unsigned int v_D2EC[16];
    float v_D2FC;
    unsigned int v_D2FE;
} ram_D2C0;
#define pv_phase_2_pi_i_ki (ram_D2C0.v_D2DE)

/* 0xD340: task 4's reactive power results and the grid code (D34A) */
extern struct a_page_D340 {
    float v_D340;
    float v_D342;
    float v_D344;
    float v_D346;
    float v_D348;
    unsigned int v_D34A;
    unsigned int v_D34B;
    unsigned int p_D34C[1];
    unsigned int v_D34D;
    unsigned int v_D34E;
    unsigned int p_D34F[1];
    float v_D350;
    float v_D352;
    float v_D354;
    float v_D356;
    float v_D358;
    float v_D35A;
    unsigned int p_D35C[4];
    float v_D360;
    float v_D362;
    unsigned int p_D364[2];
    float v_D366;
    unsigned int p_D368[2];
    float v_D36A;
    unsigned int p_D36C[2];
    float v_D36E;
    unsigned int v_D370;
    unsigned int v_D371;
} ram_D340;
#define q_setpoint (ram_D340.v_D340)
#define q_cap_comp (ram_D340.v_D342)
#define q_base_power (ram_D340.v_D344)
#define pfp_pf_code (ram_D340.v_D346)
#define q_max (ram_D340.v_D348)
#define grid_code (ram_D340.v_D34A)
#define ofr_it_delay_run (ram_D340.v_D34B)
#define ofr_it_delay_done (ram_D340.v_D34D)
#define fast_trip_low_v (ram_D340.v_D34E)
#define ac_sums (ram_D340.v_D350)
#define ac_sums_inv_n_eps (ram_D340.v_D352)
#define ac_sums_inv_v_sq (ram_D340.v_D354)
#define ac_sums_grid_i_sq (ram_D340.v_D356)
#define ac_sums_leak_i_sq (ram_D340.v_D358)
#define ac_sums_grid_v_sq (ram_D340.v_D35A)
#define ac_sums_eps_v_sq (ram_D340.v_D360)
#define ac_sums_eps_i_sq (ram_D340.v_D362)
#define ac_sums_eps_p (ram_D340.v_D366)
#define ac_sums_grid_p (ram_D340.v_D36A)
#define ac_sums_dc_sum (ram_D340.v_D36E)
#define ac_sums_n_eps (ram_D340.v_D370)
#define ac_sums_n_grid (ram_D340.v_D371)

extern struct a_page_D380 {
    unsigned int v_D380;
    unsigned int p_D381[3];
    float v_D384;
    float v_D386;
    float v_D388;
    float v_D38A;
    unsigned int p_D38C[4];
    float v_D390;
    float v_D392;
    unsigned int p_D394[2];
    float v_D396;
    unsigned int p_D398[2];
    float v_D39A;
    unsigned int p_D39C[2];
    float v_D39E;
    unsigned int v_D3A0;
    unsigned int v_D3A1;
} ram_D380;
#define ac_sums_isr (ram_D380.v_D380)
#define ac_sums_isr_inv_v_sq (ram_D380.v_D384)
#define ac_sums_isr_grid_i_sq (ram_D380.v_D386)
#define ac_sums_isr_leak_i_sq (ram_D380.v_D388)
#define ac_sums_isr_grid_v_sq (ram_D380.v_D38A)
#define ac_sums_isr_eps_v_sq (ram_D380.v_D390)
#define ac_sums_isr_eps_i_sq (ram_D380.v_D392)
#define ac_sums_isr_eps_p (ram_D380.v_D396)
#define ac_sums_isr_grid_p (ram_D380.v_D39A)
#define ac_sums_isr_dc_sum (ram_D380.v_D39E)
#define ac_sums_isr_n_eps (ram_D380.v_D3A0)
#define ac_sums_isr_n_grid (ram_D380.v_D3A1)

/* 0xD3C0: task 1's AC results (ac): grid voltage D3CA (one cycle) and D3CC (filtered), the DC
   component D3FA */
extern struct a_page_D3C0 {
    unsigned int v_D3C0;
    unsigned int p_D3C1[3];
    float v_D3C4;
    float v_D3C6;
    float v_D3C8;
    float v_D3CA;
    float v_D3CC;
    float v_D3CE;
    float v_D3D0;
    float v_D3D2;
    unsigned int p_D3D4[16];
    float v_D3E4;
    float v_D3E6;
    float v_D3E8;
    float v_D3EA;
    float v_D3EC;
    float v_D3EE;
    float v_D3F0;
    float v_D3F2;
    float v_D3F4;
    float v_D3F6;
    unsigned int p_D3F8[2];
    float v_D3FA;
    float v_D3FC;
    float v_D3FE;
} ram_D3C0;
#define ac (ram_D3C0.v_D3C0)
#define vw_v_in (ram_D3C0.v_D3C4)
#define vw_v_in2 (ram_D3C0.v_D3C6)
#define ac_grid_v_raw (ram_D3C0.v_D3C8)
#define grid_voltage_fast (ram_D3C0.v_D3CA)
#define grid_voltage (ram_D3C0.v_D3CC)
#define ac_inv_v (ram_D3C0.v_D3CE)
#define grid_current (ram_D3C0.v_D3D0)
#define rms_d3d2 (ram_D3C0.v_D3D2)
#define rating_word_d3e4 (ram_D3C0.v_D3E4)
#define rating_word_d3e6 (ram_D3C0.v_D3E6)
#define eps_voltage (ram_D3C0.v_D3E8)
#define ac_eps_v_filt (ram_D3C0.v_D3EA)
#define eps_current (ram_D3C0.v_D3EC)
#define eps_power (ram_D3C0.v_D3EE)
#define ac_eps_p (ram_D3C0.v_D3F0)
#define ac_eps_dc (ram_D3C0.v_D3F2)
#define eps_v_ref (ram_D3C0.v_D3F4)
#define p_limit_trim (ram_D3C0.v_D3F6)
#define dc_injection (ram_D3C0.v_D3FA)
#define ac_apparent_p (ram_D3C0.v_D3FC)
#define apparent_power (ram_D3C0.v_D3FE)

/* 0xD400: task 1's frequency and power results and the PI blocks of task 4 */
extern struct a_blk_D400 {
    float v_D400;
    float v_D402;
    unsigned int p_D404[2];
    float v_D406;
    float v_D408;
    float v_D40A;
    float v_D40C;
    float v_D40E;
    float v_D410;
    float v_D412;
    float v_D414;
    float v_D416;
    float v_D418;
    float v_D41A;
    float v_D41C;
    float v_D41E;
    float v_D420;
    unsigned int p_D422[2];
    float v_D424;
    float v_D426;
    float v_D428;
    float v_D42A;
    float v_D42C;
    float v_D42E;
    float v_D430;
    float v_D432;
    unsigned int p_D434[2];
    float v_D436;
    float v_D438;
    float v_D43A;
    float v_D43C;
    float v_D43E;
    float v_D440;
    float v_D442;
    float v_D444;
    unsigned int p_D446[2];
    float v_D448;
    float v_D44A;
    float v_D44C;
    float v_D44E;
    float v_D450;
    float v_D452;
    float v_D454;
    float v_D456;
    unsigned int p_D458[2];
    float v_D45A;
    float v_D45C;
    float v_D45E;
    float v_D460;
    float v_D462;
    float v_D464;
    float v_D466;
    float v_D468;
    unsigned int p_D46A[2];
    float v_D46C;
    float v_D46E;
    float v_D470;
    float v_D472;
    float v_D474;
    float v_D476;
    float v_D478;
    float v_D47A;
    unsigned int p_D47C[2];
    float v_D47E;
} ram_D400;
#define batt_max_charge_current (ram_D400.v_D400)
#define ac_batt_max_discharge_current (ram_D400.v_D402)
#define grid_freq_f (ram_D400.v_D406)
#define grid_freq_filt (ram_D400.v_D408)
#define ac_active_p_raw (ram_D400.v_D40A)
#define ac_active_p (ram_D400.v_D40C)
#define inv_power (ram_D400.v_D40E)
#define reactive_power (ram_D400.v_D410)
#define rcmu_offset (ram_D400.v_D412)
#define p_eps_sample (ram_D400.v_D414)
#define eps_pi_e (ram_D400.v_D416)
#define eps_pi_kp (ram_D400.v_D418)
#define eps_pi_ki (ram_D400.v_D41A)
#define eps_pi_kd (ram_D400.v_D41C)
#define eps_pi_umax (ram_D400.v_D41E)
#define eps_pi_umin (ram_D400.v_D420)
#define eps_pi_i (ram_D400.v_D424)
#define eps_pi_u (ram_D400.v_D426)
#define grid_pi_e (ram_D400.v_D428)
#define grid_pi_kp (ram_D400.v_D42A)
#define grid_pi_ki (ram_D400.v_D42C)
#define grid_pi_kd (ram_D400.v_D42E)
#define grid_pi_umax (ram_D400.v_D430)
#define grid_pi_umin (ram_D400.v_D432)
#define grid_pi_i (ram_D400.v_D436)
#define grid_pi_u (ram_D400.v_D438)
#define pv_power_pi_e (ram_D400.v_D43A)
#define pv_power_pi_kp (ram_D400.v_D43C)
#define pv_power_pi_ki (ram_D400.v_D43E)
#define pv_power_pi_kd (ram_D400.v_D440)
#define pv_power_pi_umax (ram_D400.v_D442)
#define pv_power_pi_umin (ram_D400.v_D444)
#define pv_power_pi_i (ram_D400.v_D448)
#define pv_power_pi_u (ram_D400.v_D44A)
#define pvp_pi_e (ram_D400.v_D44C)
#define pvp_pi_kp (ram_D400.v_D44E)
#define pvp_pi_ki (ram_D400.v_D450)
#define pvp_pi_kd (ram_D400.v_D452)
#define pvp_pi_umax (ram_D400.v_D454)
#define pvp_pi_umin (ram_D400.v_D456)
#define pvp_pi_i (ram_D400.v_D45A)
#define pvp_pi_u (ram_D400.v_D45C)
#define pv_current_pi_e (ram_D400.v_D45E)
#define pv_current_pi_kp (ram_D400.v_D460)
#define pv_current_pi_ki (ram_D400.v_D462)
#define pv_current_pi_kd (ram_D400.v_D464)
#define pv_current_pi_umax (ram_D400.v_D466)
#define pv_current_pi_umin (ram_D400.v_D468)
#define pv_current_pi_i (ram_D400.v_D46C)
#define pv_current_pi_u (ram_D400.v_D46E)
#define pvi_pi_e (ram_D400.v_D470)
#define pvi_pi_kp (ram_D400.v_D472)
#define pvi_pi_ki (ram_D400.v_D474)
#define pvi_pi_kd (ram_D400.v_D476)
#define pvi_pi_umax (ram_D400.v_D478)
#define pvi_pi_umin (ram_D400.v_D47A)
#define pvi_pi_i (ram_D400.v_D47E)

/* 0xD480: power factor, load %, the grid watchdog and the RCMU inputs rcmu_i (D488..D48C) */
extern struct a_page_D480 {
    float v_D480;
    unsigned int v_D482;
    unsigned int v_D483;
    unsigned int v_D484;
    unsigned int v_D485;
    unsigned int v_D486;
    unsigned int p_D487[1];
    float v_D488;
    float v_D48A;
    float v_D48C;
} ram_D480;
#define pvi_pi_u (ram_D480.v_D480)
#define power_factor (ram_D480.v_D482)
#define ac_pf_code (ram_D480.v_D483)
#define load_percent (ram_D480.v_D484)
#define state_word_d485 (ram_D480.v_D485)
#define grid_cycle_watchdog (ram_D480.v_D486)
#define rcmu_i (ram_D480.v_D488)
#define aux_f16_k (ram_D480.v_D48A)
#define rcmu_i_fast (ram_D480.v_D48C)

extern struct a_page_D4C0 {
    unsigned int v_D4C0;
    unsigned int v_D4C1;
    unsigned int v_D4C2;
    unsigned int v_D4C3;
    unsigned int v_D4C4;
    unsigned int v_D4C5;
    unsigned int v_D4C6;
    unsigned int v_D4C7;
    unsigned int v_D4C8;
    unsigned int v_D4C9;
    unsigned int v_D4CA;
    unsigned int v_D4CB;
    unsigned int v_D4CC;
    unsigned int v_D4CD;
    unsigned int v_D4CE;
    unsigned int v_D4CF;
    unsigned int v_D4D0;
    unsigned int v_D4D1;
    unsigned int v_D4D2;
    unsigned int v_D4D3;
    unsigned int v_D4D4;
    unsigned int v_D4D5;
    unsigned int v_D4D6;
    unsigned int v_D4D7;
    unsigned int v_D4D8;
    unsigned int v_D4D9;
    unsigned int v_D4DA;
    unsigned int v_D4DB;
    float v_D4DC;
    float v_D4DE;
    float v_D4E0;
    float v_D4E2;
    float v_D4E4;
    float v_D4E6;
    float v_D4E8;
    float v_D4EA;
    float v_D4EC;
    float v_D4EE;
    float v_D4F0;
    float v_D4F2;
    float v_D4F4;
    float v_D4F6;
    float v_D4F8;
    float v_D4FA;
} ram_D4C0;
#define vmis_set_count (ram_D4C0.v_D4C0)
#define vmis_clear_count (ram_D4C0.v_D4C1)
#define no_dis_count (ram_D4C0.v_D4C2)
#define force_chg_count (ram_D4C0.v_D4C3)
#define aging_delay (ram_D4C0.v_D4C4)
#define start_hold_count (ram_D4C0.v_D4C5)
#define soc_floor_latch (ram_D4C0.v_D4C6)
#define ov_slow_count (ram_D4C0.v_D4C7)
#define uv_clear_count (ram_D4C0.v_D4C8)
#define ov_clear_count (ram_D4C0.v_D4C9)
#define uv_clear_limit (ram_D4C0.v_D4CA)
#define full_trim_div (ram_D4C0.v_D4CB)
#define ov_fast_count (ram_D4C0.v_D4CC)
#define floor_latch_set_count (ram_D4C0.v_D4CD)
#define full_set_count (ram_D4C0.v_D4CE)
#define force_chg_soc (ram_D4C0.v_D4CF)
#define full_dis_count (ram_D4C0.v_D4D0)
#define uv_set_count (ram_D4C0.v_D4D1)
#define floor_latch_clear_count (ram_D4C0.v_D4D2)
#define cfg_reset_count (ram_D4C0.v_D4D3)
#define led_count (ram_D4C0.v_D4D4)
#define cf18b2_reset_count (ram_D4C0.v_D4D5)
#define upgrade_end_count (ram_D4C0.v_D4D6)
#define start_permit_count (ram_D4C0.v_D4D7)
#define cf15b3_clear_count (ram_D4C0.v_D4D8)
#define dcdc_ratio_count (ram_D4C0.v_D4D9)
#define full_clear_count (ram_D4C0.v_D4DA)
#define dcdc_ratio_clear_count (ram_D4C0.v_D4DB)
#define full_trim_integ (ram_D4C0.v_D4DC)
#define full_trim_out (ram_D4C0.v_D4DE)
#define chg_plim_pv_prev (ram_D4C0.v_D4E0)
#define chg_plim_pv (ram_D4C0.v_D4E2)
#define pv_current_max (ram_D4C0.v_D4E4)
#define pv_pi_max (ram_D4C0.v_D4E6)
#define chg_lowv_taper (ram_D4C0.v_D4E8)
#define hr19b5_chg_ramp (ram_D4C0.v_D4EA)
#define bus_ov_derate (ram_D4C0.v_D4EC)
#define force_chg_cap (ram_D4C0.v_D4EE)
#define batt_current_cmd (ram_D4C0.v_D4F0)
#define pack_current_filt (ram_D4C0.v_D4F2)
#define force_dis_cap (ram_D4C0.v_D4F4)
#define dis_highv_taper (ram_D4C0.v_D4F6)
#define arm_setpoint_prev (ram_D4C0.v_D4F8)
#define batt_power_cmd (ram_D4C0.v_D4FA)

extern struct a_page_D500 {
    unsigned int v_D500;
    unsigned int v_D501;
    unsigned int v_D502;
    unsigned int v_D503;
    unsigned int v_D504;
    unsigned int v_D505;
    unsigned int v_D506;
    unsigned int p_D507[1];
    unsigned int v_D508;
    unsigned int v_D509;
    unsigned int v_D50A;
    unsigned int v_D50B;
    unsigned int v_D50C;
    unsigned int p_D50D[1];
    float v_D50E;
    float v_D510;
    float v_D512;
    float v_D514;
    float v_D516;
    float v_D518;
    float v_D51A;
    float v_D51C;
    unsigned int p_D51E[2];
    unsigned int v_D520;
    unsigned int p_D521[1];
    float v_D522;
    float v_D524;
    float v_D526;
    float v_D528;
    unsigned int p_D52A[1];
    unsigned int v_D52B;
    unsigned int v_D52C;
    unsigned int v_D52D;
    unsigned int v_D52E;
} ram_D500;
#define bms (ram_D500.v_D500)
#define bms_hr19 (ram_D500.v_D501)
#define bms_status2 (ram_D500.v_D502)
#define bms_charge_voltage (ram_D500.v_D503)
#define bms_lim_hr26 (ram_D500.v_D504)
#define bms_lim_hr27 (ram_D500.v_D505)
#define bms_soc (ram_D500.v_D506)
#define bms_pack_voltage (ram_D500.v_D508)
#define bms_pack_current (ram_D500.v_D509)
#define bms_temp (ram_D500.v_D50A)
#define plim_word_d50b (ram_D500.v_D50B)
#define bms_limit_scale (ram_D500.v_D50C)
#define bms_chg_plim_temp (ram_D500.v_D50E)
#define bms_dis_plim_temp (ram_D500.v_D510)
#define bms_chg_plim_current (ram_D500.v_D512)
#define bms_dis_plim_current (ram_D500.v_D514)
#define bms_chg_plim_bus (ram_D500.v_D516)
#define bms_dis_plim_bus (ram_D500.v_D518)
#define bms_chg_plim_soc (ram_D500.v_D51A)
#define bms_dis_plim_soc (ram_D500.v_D51C)
#define bms_charge_voltage_copy (ram_D500.v_D520)
#define bms_chg_current_cap (ram_D500.v_D522)
#define bms_dis_current_cap (ram_D500.v_D524)
#define bms_chg_current_limit (ram_D500.v_D526)
#define dis_current_limit (ram_D500.v_D528)
#define bms_bms_charge_limit_copy (ram_D500.v_D52B)
#define bms_max_charge_current_copy (ram_D500.v_D52C)
#define bms_start_permit (ram_D500.v_D52D)
#define soft_start_enable (ram_D500.v_D52E)

extern struct a_page_D600 {
    unsigned int p_D600[10];
    unsigned int v_D60A;
    unsigned int p_D60B[19];
    unsigned int v_D61E;
    unsigned int v_D61F;
} ram_D600;
#define arm_bms_request (ram_D600.v_D60A)
#define arm_bms_request_pending (ram_D600.v_D61E)
#define bms_fc4_ready (ram_D600.v_D61F)

extern struct a_page_D640 {
    unsigned int v_D640;
    unsigned int p_D641[1];
    unsigned int v_D642;
    unsigned int p_D643[3];
    unsigned int v_D646;
    unsigned int v_D647;
} ram_D640;
#define boot_rx_active (ram_D640.v_D640)
#define FlashStatus (ram_D640.v_D642)
#define boot_reply (ram_D640.v_D646)
#define ram_word_d647 (ram_D640.v_D647)

extern struct a_page_D680 {
    unsigned int v_D680;
} ram_D680;
#define boot_prog_buf (ram_D680.v_D680)

extern struct a_page_D700 {
    unsigned int v_D700;
    unsigned int v_D701;
    unsigned int v_D702;
} ram_D700;
#define boot_rx_frame (ram_D700.v_D700)
#define ram_word_d701 (ram_D700.v_D701)
#define ram_word_d702 (ram_D700.v_D702)

/* 0xD7C0: the configuration bits eps_cfg (D7C0), run_status (D7C1), grid_func (D7C2) */
extern struct a_page_D7C0 {
    union bits16_u v_D7C0;
    union bits16_u v_D7C1;
    union bits16_u v_D7C2;
} ram_D7C0;
#define eps_cfg_bits (ram_D7C0.v_D7C0)
#define run_status_bits (ram_D7C0.v_D7C1)
#define grid_func_flags (ram_D7C0.v_D7C2)

/* 0xD800: the settings from the ARM (settings.c defaults; the HR numbers are in map/names.tsv) */
extern struct a_blk_D800 {
    unsigned int p_D800[1];
    unsigned int v_D801;
    unsigned int v_D802;
    unsigned int v_D803;
    unsigned int v_D804;
    unsigned int v_D805;
    unsigned int v_D806;
    unsigned int v_D807;
    unsigned int v_D808;
    unsigned int v_D809;
    unsigned int v_D80A;
    unsigned int v_D80B;
    unsigned int v_D80C;
    unsigned int v_D80D;
    unsigned int v_D80E;
    unsigned int v_D80F;
    unsigned int v_D810;
    unsigned int v_D811;
    unsigned int v_D812;
    unsigned int v_D813;
    unsigned int v_D814;
    unsigned int v_D815;
    unsigned int v_D816;
    unsigned int v_D817;
    unsigned int v_D818;
    unsigned int v_D819;
    unsigned int v_D81A;
    unsigned int v_D81B;
    unsigned int v_D81C;
    unsigned int v_D81D;
    unsigned int v_D81E;
    unsigned int v_D81F;
    unsigned int v_D820;
    unsigned int v_D821;
    unsigned int v_D822;
    unsigned int v_D823;
    unsigned int v_D824;
    unsigned int v_D825;
    unsigned int v_D826;
    unsigned int v_D827;
    unsigned int v_D828;
    unsigned int v_D829;
    unsigned int v_D82A;
    unsigned int v_D82B;
    unsigned int v_D82C;
    unsigned int v_D82D;
    unsigned int v_D82E;
    unsigned int v_D82F;
    unsigned int v_D830;
    unsigned int v_D831;
    unsigned int v_D832;
    unsigned int v_D833;
    unsigned int p_D834[1];
    unsigned int v_D835;
    unsigned int v_D836;
    unsigned int v_D837;
    unsigned int p_D838[1];
    unsigned int v_D839;
    unsigned int v_D83A;
    unsigned int v_D83B;
    unsigned int v_D83C;
    unsigned int v_D83D;
    unsigned int v_D83E;
    unsigned int v_D83F;
    unsigned int v_D840;
    unsigned int v_D841;
    unsigned int v_D842;
    unsigned int v_D843;
    unsigned int v_D844;
    unsigned int v_D845;
    int v_D846;
    int v_D847;
    int v_D848;
    int v_D849;
    int v_D84A;
    int v_D84B;
    int v_D84C;
    int v_D84D;
    unsigned int v_D84E;
    unsigned int v_D84F;
    unsigned int p_D850[1];
    unsigned int v_D851;
    unsigned int v_D852;
    unsigned int v_D853;
    unsigned int v_D854;
    unsigned int v_D855;
    unsigned int v_D856;
    unsigned int v_D857;
    unsigned int v_D858;
    unsigned int v_D859;
    unsigned int v_D85A;
    unsigned int v_D85B;
    unsigned int v_D85C;
    unsigned int v_D85D;
    unsigned int v_D85E;
    unsigned int p_D85F[33];
    float v_D880;
    float v_D882;
    float v_D884;
    float v_D886;
    float v_D888;
    float v_D88A;
    float v_D88C;
    float v_D88E;
    float v_D890;
    float v_D892;
    float v_D894;
    float v_D896;
    float v_D898;
    float v_D89A;
    float v_D89C;
    float v_D89E;
    float v_D8A0;
    float v_D8A2;
    float v_D8A4;
    unsigned int p_D8A6[2];
    float v_D8A8;
    float v_D8AA;
    float v_D8AC;
    float v_D8AE;
    float v_D8B0;
    float v_D8B2;
    float v_D8B4;
    float v_D8B6;
    float v_D8B8;
    float v_D8BA;
    float v_D8BC;
    float v_D8BE;
    float v_D8C0;
    float v_D8C2;
    unsigned int v_D8C4;
    unsigned int v_D8C5;
    unsigned int v_D8C6;
    unsigned int v_D8C7;
    unsigned int v_D8C8;
    unsigned int v_D8C9;
    unsigned int v_D8CA;
    unsigned int v_D8CB;
    unsigned int v_D8CC;
    unsigned int v_D8CD;
    unsigned int v_D8CE;
    unsigned int v_D8CF;
    unsigned int v_D8D0;
    unsigned int v_D8D1;
    unsigned int v_D8D2;
    unsigned int v_D8D3;
    unsigned int p_D8D4[1];
    unsigned int v_D8D5;
    unsigned int v_D8D6;
    unsigned int p_D8D7[1];
    unsigned int v_D8D8;
    unsigned int v_D8D9;
    unsigned int v_D8DA;
    unsigned int p_D8DB[3];
    unsigned int v_D8DE;
    unsigned int v_D8DF;
    unsigned int v_D8E0;
    unsigned int v_D8E1;
    unsigned int v_D8E2;
    unsigned int v_D8E3;
    unsigned int v_D8E4;
    unsigned int v_D8E5;
    unsigned int p_D8E6[1];
    unsigned int v_D8E7;
    unsigned int v_D8E8;
    unsigned int v_D8E9;
    unsigned int v_D8EA;
    unsigned int v_D8EB;
    unsigned int v_D8EC;
    unsigned int v_D8ED;
    unsigned int v_D8EE;
    unsigned int v_D8EF;
    unsigned int v_D8F0;
    unsigned int v_D8F1;
    unsigned int p_D8F2[14];
} ram_D800;
#define cfg_flag_a (ram_D800.v_D801)
#define cfg_flag_b (ram_D800.v_D802)
#define pv_start_voltage (ram_D800.v_D803)
#define pv_mode_cfg (ram_D800.v_D804)
#define pv_bus_mode (ram_D800.v_D805)
#define qu_q_max (ram_D800.v_D806)
#define q_mode (ram_D800.v_D807)
#define cal_batt_v (ram_D800.v_D808)
#define cal_hv_i (ram_D800.v_D809)
#define cal_pv1_v (ram_D800.v_D80A)
#define cal_pv1_i (ram_D800.v_D80B)
#define cal_pv2_v (ram_D800.v_D80C)
#define cal_pv2_i (ram_D800.v_D80D)
#define cal_pv1_p (ram_D800.v_D80E)
#define cal_pv2_p (ram_D800.v_D80F)
#define cal_grid_v (ram_D800.v_D810)
#define cal_spare_11 (ram_D800.v_D811)
#define cal_spare_12 (ram_D800.v_D812)
#define cal_power (ram_D800.v_D813)
#define cal_spare_14 (ram_D800.v_D814)
#define cal_spare_15 (ram_D800.v_D815)
#define cal_spare_16 (ram_D800.v_D816)
#define cal_spare_17 (ram_D800.v_D817)
#define cal_bus_v (ram_D800.v_D818)
#define cal_d819 (ram_D800.v_D819)
#define inverter_start_time (ram_D800.v_D81A)
#define inverter_restart_delay (ram_D800.v_D81B)
#define v_ac_high_out (ram_D800.v_D81C)
#define v_ac_low_out (ram_D800.v_D81D)
#define v_ac_high_in (ram_D800.v_D81E)
#define v_ac_low_in (ram_D800.v_D81F)
#define f_ac_high_out (ram_D800.v_D820)
#define f_ac_low_out (ram_D800.v_D821)
#define f_ac_high_in (ram_D800.v_D822)
#define f_ac_low_in (ram_D800.v_D823)
#define v_ac_high_c (ram_D800.v_D824)
#define v_ac_low_c (ram_D800.v_D825)
#define f_ac_high_c (ram_D800.v_D826)
#define f_ac_low_c (ram_D800.v_D827)
#define v_10_min_protection (ram_D800.v_D828)
#define dci_limit (ram_D800.v_D829)
#define dci_limit_b (ram_D800.v_D82A)
#define v_ac_high_out_time (ram_D800.v_D82B)
#define v_ac_low_out_time (ram_D800.v_D82C)
#define v_ac_high_in_time (ram_D800.v_D82D)
#define v_ac_low_in_time (ram_D800.v_D82E)
#define f_ac_high_out_time (ram_D800.v_D82F)
#define f_ac_low_out_time (ram_D800.v_D830)
#define f_ac_high_in_time (ram_D800.v_D831)
#define f_ac_low_in_time (ram_D800.v_D832)
#define cfg_spare_150 (ram_D800.v_D833)
#define dci_trip_time (ram_D800.v_D835)
#define dci_trip_time_it (ram_D800.v_D836)
#define ofr_start_freq (ram_D800.v_D837)
#define ofr_droop (ram_D800.v_D839)
#define ofr_recover_delay (ram_D800.v_D83A)
#define ofr_delay (ram_D800.v_D83B)
#define ofr_recover_freq (ram_D800.v_D83C)
#define ufr_start_freq (ram_D800.v_D83D)
#define ufr_droop (ram_D800.v_D83E)
#define ufr_delay (ram_D800.v_D83F)
#define ufr_latched (ram_D800.v_D840)
#define power_ramp_rate (ram_D800.v_D841)
#define freq_stage_sel (ram_D800.v_D842)
#define qu_lock_in_p (ram_D800.v_D843)
#define qu_lock_out_p (ram_D800.v_D844)
#define q_pt1_tau (ram_D800.v_D845)
#define pf_curve_p1 (ram_D800.v_D846)
#define pf_curve_p2 (ram_D800.v_D847)
#define pf_curve_p3 (ram_D800.v_D848)
#define pf_curve_p4 (ram_D800.v_D849)
#define pf_curve_pf1 (ram_D800.v_D84A)
#define pf_curve_pf2 (ram_D800.v_D84B)
#define pf_curve_pf3 (ram_D800.v_D84C)
#define pf_curve_pf4 (ram_D800.v_D84D)
#define pfp_lock_in_v (ram_D800.v_D84E)
#define pfp_lock_out_v (ram_D800.v_D84F)
#define reactive_power_rate (ram_D800.v_D851)
#define active_power_rate (ram_D800.v_D852)
#define power_factor_set (ram_D800.v_D853)
#define ofr_it_param (ram_D800.v_D854)
#define power_ramp_rate_reconnect (ram_D800.v_D855)
#define fast_trip_v_low_set (ram_D800.v_D856)
#define fast_trip_v_high_set (ram_D800.v_D857)
#define qu_v_low1 (ram_D800.v_D858)
#define qu_v_low2 (ram_D800.v_D859)
#define qu_v_high1 (ram_D800.v_D85A)
#define qu_v_high2 (ram_D800.v_D85B)
#define sched_window (ram_D800.v_D85C)
#define enable_charge_target (ram_D800.v_D85D)
#define charge_target_soc (ram_D800.v_D85E)
#define rating_params (ram_D800.v_D880)
#define grid_v_nominal (ram_D800.v_D882)
#define grid_freq_nominal (ram_D800.v_D884)
#define grid_omega_step (ram_D800.v_D886)
#define rating_params_rated_current (ram_D800.v_D888)
#define rated_ac_power (ram_D800.v_D88A)
#define grid_power_max_pv (ram_D800.v_D88C)
#define grid_voltage_nominal (ram_D800.v_D88E)
#define grid_current_max (ram_D800.v_D890)
#define batt_chg_power_max (ram_D800.v_D892)
#define rating_params_batt_dis_power_max (ram_D800.v_D894)
#define force_charge_pct (ram_D800.v_D896)
#define force_discharge_pct (ram_D800.v_D898)
#define force_chg_soc_f (ram_D800.v_D89A)
#define pv_power_rated (ram_D800.v_D89C)
#define pv_power_limit (ram_D800.v_D89E)
#define rating_params_spare_v340 (ram_D800.v_D8A0)
#define pv_derate_voltage (ram_D800.v_D8A2)
#define rating_params_grid_v_87pct (ram_D800.v_D8A4)
#define pv_ov_limit (ram_D800.v_D8A8)
#define batt_temp_limit (ram_D800.v_D8AA)
#define rating_params_batt_temp_clear (ram_D800.v_D8AC)
#define charger_temp_limit (ram_D800.v_D8AE)
#define charger_temp_derate (ram_D800.v_D8B0)
#define heatsink_temp_limit (ram_D800.v_D8B2)
#define heatsink_temp_derate (ram_D800.v_D8B4)
#define grid_v_low_derate (ram_D800.v_D8B6)
#define arm_batt_setpoint (ram_D800.v_D8B8)
#define grid_port_max_power (ram_D800.v_D8BA)
#define exp_target_raw (ram_D800.v_D8BC)
#define exp_target_filt (ram_D800.v_D8BE)
#define inv_power_abs (ram_D800.v_D8C0)
#define bus_ov_limit (ram_D800.v_D8C2)
#define rating_params_settings (ram_D800.v_D8C4)
#define battery_charge_limit (ram_D800.v_D8C5)
#define battery_discharge_limit (ram_D800.v_D8C6)
#define charge_target_soc_now (ram_D800.v_D8C7)
#define batt_min_reserve (ram_D800.v_D8C8)
#define discharge_min_soc (ram_D800.v_D8C9)
#define grid_import_amps (ram_D800.v_D8CA)
#define inv_countdown_ticks (ram_D800.v_D8CB)
#define cfg_changed_flags (ram_D800.v_D8CC)
#define inverter_state (ram_D800.v_D8CD)
#define batt_state_cmd (ram_D800.v_D8CE)
#define dsp_settings_start_fault_now (ram_D800.v_D8CF)
#define dsp_settings_start_fault_entry (ram_D800.v_D8D0)
#define charge_active (ram_D800.v_D8D1)
#define discharge_active (ram_D800.v_D8D2)
#define battery_power_mode (ram_D800.v_D8D3)
#define iso_check_enable (ram_D800.v_D8D5)
#define rcmu_enable (ram_D800.v_D8D6)
#define dci_comp_enable (ram_D800.v_D8D8)
#define hr1_check3 (ram_D800.v_D8D9)
#define rcmu_step_check_off (ram_D800.v_D8DA)
#define batt_v_high_limit (ram_D800.v_D8DE)
#define batt_v_low_limit (ram_D800.v_D8DF)
#define battery_capacity (ram_D800.v_D8E0)
#define battery_type (ram_D800.v_D8E1)
#define enable_bms_read (ram_D800.v_D8E2)
#define bms_hr0 (ram_D800.v_D8E3)
#define bms_fw_version (ram_D800.v_D8E4)
#define bms_relay_request (ram_D800.v_D8E5)
#define bms_capacity (ram_D800.v_D8E7)
#define bms_soc_raw (ram_D800.v_D8E8)
#define pv_input_mode (ram_D800.v_D8E9)
#define run_mode_req (ram_D800.v_D8EA)
#define run_command (ram_D800.v_D8EB)
#define arm_page_no (ram_D800.v_D8EC)
#define grid_is_60hz (ram_D800.v_D8ED)
#define dsp_settings_relay_test_pass (ram_D800.v_D8EE)
#define aging_test_enable (ram_D800.v_D8EF)
#define aging_test_mode (ram_D800.v_D8F0)
#define model_power_code (ram_D800.v_D8F1)

extern struct a_page_D940 {
    unsigned int v_D940;
    unsigned int v_D941;
} ram_D940;
#define bms_rx_buf (ram_D940.v_D940)
#define bms_word_d941 (ram_D940.v_D941)

extern struct a_page_D9C0 {
    unsigned int v_D9C0;
} ram_D9C0;
#define bms_tx_buf (ram_D9C0.v_D9C0)

extern struct a_page_DA40 {
    unsigned int v_DA40;
    unsigned int v_DA41;
    unsigned int v_DA42;
    unsigned int p_DA43[1];
    float v_DA44;
    float v_DA46;
    float v_DA48;
} ram_DA40;

extern struct a_page_DA80 {
    unsigned int v_DA80;
    unsigned int p_DA81[1];
    unsigned int v_DA82;
    unsigned int v_DA83;
    unsigned int v_DA84;
    unsigned int p_DA85[39];
    float v_DAAC;
    unsigned int p_DAAE[10];
    unsigned int v_DAB8;
} ram_DA80;
#define os_tcb0 (ram_DA80.v_DA80)
#define os_tcb_1_period (ram_DA80.v_DA82)
#define os_tcb_1_count (ram_DA80.v_DA83)
#define os_tcb0_flags (ram_DA80.v_DA84)
#define arm_rx_block (ram_DA80.v_DAAC)
#define os_tcb_8_sp (ram_DA80.v_DAB8)

/* 0xDB40: the residual current monitor (rcmu) */
extern struct a_page_DB40 {
    unsigned int v_DB40;
    unsigned int v_DB41;
    float v_DB42;
    unsigned int p_DB44[4];
    unsigned int v_DB48;
    unsigned int p_DB49[1];
    float v_DB4A;
    unsigned int v_DB4C;
    unsigned int v_DB4D;
    float v_DB4E;
    unsigned int p_DB50[2];
    unsigned int v_DB52;
    unsigned int v_DB53;
    unsigned int p_DB54[6];
    float v_DB5A;
    float v_DB5C;
    unsigned int p_DB5E[6];
    unsigned int v_DB64;
    unsigned int v_DB65;
    unsigned int v_DB66;
    unsigned int p_DB67[1];
    float v_DB68;
    float v_DB6A;
    float v_DB6C;
    float v_DB6E;
    unsigned int p_DB70[2];
    float v_DB72;
    unsigned int v_DB74;
} ram_DB40;
#define rcmu (ram_DB40.v_DB40)
#define rcmu_a_arm (ram_DB40.v_DB41)
#define rcmu_a_src (ram_DB40.v_DB42)
#define rcmu_a_done (ram_DB40.v_DB48)
#define rcmu_a_out (ram_DB40.v_DB4A)
#define rcmu_b_en (ram_DB40.v_DB4C)
#define rcmu_b_arm (ram_DB40.v_DB4D)
#define rcmu_b_src (ram_DB40.v_DB4E)
#define rcmu_b_passed (ram_DB40.v_DB52)
#define rcmu_b_failed (ram_DB40.v_DB53)
#define rcmu_b_avg1 (ram_DB40.v_DB5A)
#define rcmu_b_avg0 (ram_DB40.v_DB5C)
#define rcmu_c_en (ram_DB40.v_DB64)
#define rcmu_c_arm (ram_DB40.v_DB65)
#define rcmu_c_jump_check_on (ram_DB40.v_DB66)
#define rcmu_c_base1 (ram_DB40.v_DB68)
#define rcmu_c_base0 (ram_DB40.v_DB6A)
#define rcmu_c_limit (ram_DB40.v_DB6C)
#define rcmu_c_src (ram_DB40.v_DB6E)
#define rcmu_c_filt1 (ram_DB40.v_DB72)
#define rcmu_c_settle (ram_DB40.v_DB74)

extern struct a_page_DB80 {
    unsigned int p_DB80[2];
    unsigned int v_DB82;
    unsigned int v_DB83;
    unsigned int p_DB84[2];
    unsigned int v_DB86;
} ram_DB80;
#define rcmu_c_fast_trip (ram_DB80.v_DB82)
#define rcmu_c_trip (ram_DB80.v_DB83)
#define adc_dma_buf (ram_DB80.v_DB86)

extern struct a_page_DBC0 {
    unsigned int v_DBC0;
} ram_DBC0;

#endif
