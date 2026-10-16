/* The converter control: the control interrupt (19.2 kHz), its controllers and their state blocks.
   The control RAM is 0xC000 .. 0xCCBF: the converter module's globals (0xC000), the islanding
   detector (0xC04E), the grid voltage history (0xC080), the scaled ADC values (0xC0C0), the PLL
   (0xC100), the eight-sample averages (0xC140), the reactive loop (0xC180), the raw ADC channels
   (0xC1C0), the battery converter's references (0xC200) and current controller (0xC240), the
   grid voltage window (0xC300), the grid current controller with its repetitive ring (0xC400 ..
   0xC7FF), the slot counter (0xC83A), and the EPS voltage controller with its ring (0xC840 ..
   0xCC8F). Several blocks have two declarations here: a typed one the controller functions take
   by pointer, and a "view" with the names the callers use (the image needs both; docs/notes.md).
   Units: voltages 0.1 V, inverter current 0.1 A, PV currents 0.01 A, angles in rad.
   docs/subsystems/control.md describes the loops. */
#ifndef CONTROL_H
#define CONTROL_H

#include "F2806x_Device.h"
#include "util.h"

/* A PI controller's state block (18 words), run by pi_step and its kin */
#ifndef OWN_PI_BLK
typedef struct {
    float e;    /* 0x0: error input */
    float kp;   /* 0x2 */
    float ki;   /* 0x4 */
    float kd;   /* 0x6 */
    float umax; /* 0x8 */
    float umin; /* 0xA */
    float e1;   /* 0xC: previous error */
    float i;    /* 0xE: integrator */
    float u;    /* 0x10: output */
} PI_BLK;
#endif

/* 0xC034: the relay commands the state machine sets and the control interrupt drives */
struct relay_cmd_t {
    Uint16 relay_a, relay_b, relay_c;
};

/* the repetitive-control delay line of the grid and EPS controllers (ac_ctrl.c) */
typedef struct rep_filter_t {
    volatile Uint16 w; /* 0x00: write index */
    volatile Uint16 r; /* 0x01: read index */
    float in;          /* 0x02 */
    float max;         /* 0x04 */
    float min;         /* 0x06 */
    float a;           /* 0x08 */
    float b;           /* 0x0A */
    float gain;        /* 0x0C */
    float y;           /* 0x0E */
    float out;         /* 0x10 */
    float buf[500];    /* 0x12 */
} rep_filter_t;

/* the grid-tied current controller (bus voltage PI outer loop), at 0xC400 (ac_ctrl.c's grid_ictl).
 * Currents 0.1 A (amp_* are peak values), voltages 0.1 V; v_cmd is the bridge voltage that
 * control_isr multiplies by mod_gain to get the duty. n and len are the PLL's sample index and
 * cycle length; mode is pv_bus_mode (always 0 in this image). */
typedef struct grid_ictl_t {
    float v_ff, i_meas, i_ref_p, i_ref_q, q_cmd, bus_v, bus_v_ref, mod_gain;
    float amp_ramp, amp_max, amp_min, sin_ref, cos_ref, kp_i;
    Uint16 mode; /* 0x1C */
    Uint16 rep_lead;
    Uint16 n;   /* 0x1E */
    Uint16 len; /* 0x1F */
    float i_ref, i_err, v_cmd, island_inj, q_corr, v_ff2, amp_cmd;
    PI_BLK pi_bus;    /* 0x2E */
    rep_filter_t res; /* 0x40 */
    Uint16 slot;      /* 0x43A */
} grid_ictl_t; /* name: was VCTL; it is the grid-tied current controller (bus voltage PI outer loop) */

/* the off-grid (EPS) voltage controller at 0xC840 (ac_ctrl.c's eps_vctl): a voltage loop with a
 * repetitive filter makes the current reference, a current loop makes the duty. Voltages 0.1 V,
 * currents 0.1 A; v_amp_ref is the peak of the RMS reference. */
typedef struct eps_vctl_t {
    float v_amp_ref, v_meas, i_meas, v_ff, i_filt, sin_ref, i_limit;
    Uint16 mode; /* 0x0E */
    Uint16 rep_lead;
    float v_peak, v_ref;
    Uint16 ramp_done; /* 0x14 */
    Uint16 pad;
    rep_filter_t res; /* 0x16 */
    PI_BLK pi_v;      /* 0x410 */
    PI_BLK pi_i;      /* 0x422 */
    float dc_trim, amp_trim, i_ref, mod_gain, duty, theta;
    Uint16 len;        /* 0x440 */
    volatile Uint16 n; /* 0x441 */
} eps_vctl_t;          /* name: was ICTL; it is the off-grid (EPS) voltage controller */

/* the island detector's state at 0xC04E (island_detect.c's island_detect; converter_ctrl.c's
 * island_det): the input is 1000 x the PLL's angle step, hist1..3 the last three cycles' values */
typedef struct island_det_t {
    float in;     /* 0x00 */
    float max;    /* 0x02 */
    float min;    /* 0x04 */
    float thr;    /* 0x06 */
    float gain;   /* 0x08 */
    Uint16 limit; /* 0x0A */
    Uint16 pad;
    float hist1;         /* 0x0C */
    float hist2;         /* 0x0E */
    float hist3;         /* 0x10 */
    float out;           /* 0x12 */
    volatile Uint16 cnt; /* 0x14 */
    Uint16 flag;         /* 0x15 */
} island_det_t;

/* the cycle-averaged reactive current loop at 0xC180 (island_detect.c's q_loop_step): the sums run
   every sample, the PI steps once per grid cycle; the fNN words are unused padding */
typedef struct q_loop_t {
    float in; /* 0x00 */
    float f02, f04;
    float i_in; /* 0x06 */
    float f08, f0A;
    float cos_in; /* 0x0C */
    float f0E, f10;
    float kp;  /* 0x12 */
    float ki;  /* 0x14 */
    float max; /* 0x16 */
    float min; /* 0x18 */
    Uint16 n;  /* 0x1A */
    Uint16 pad;
    float ref_sum; /* 0x1C */
    float f1E, f20;
    float meas_sum; /* 0x22 */
    float f24, f26;
    float e;  /* 0x28 */
    float e1; /* 0x2A */
    float f2C, f2E, f30, f32;
    float u; /* 0x34 */
} q_loop_t;

/* the grid PLL at 0xC100 (pll.c's pll_step): phase detector filters, angle theta and its step w
   (rad per sample), the sin/cos outputs with a lead of shift degrees, the cycle's sample count n
   and length period, and the lock counters */
typedef struct pll_t {
    float in;       /* 0x00: the measured voltage */
    float k;        /* 0x02: filter gain */
    float shift;    /* 0x04: phase shift in degrees */
    float pd_filt1; /* 0x06 */
    float pd_filt2; /* 0x08 */
    float sum;      /* 0x0A */
    float last;     /* 0x0C: the sum over the last cycle */
    float theta;    /* 0x0E */
    float w;        /* 0x10 */
    float s;        /* 0x12 */
    float c;        /* 0x14 */
    Uint16 good;    /* 0x16 */
    Uint16 bad;     /* 0x17 */
    Uint16 n;       /* 0x18 */
    Uint16 period;  /* 0x19 */
    Uint16 pad;
    Uint16 wrap;   /* 0x1B */
    Uint16 locked; /* 0x1C */
    Uint16 pad2;
    PI_BLK pi; /* 0x1E */
} PLL;

/* 0xC040: the PV bus limiter's PI (converter_ctrl.c's pv_bus_limit): proportional and integral
   parts kept apart; u (0.01 A) caps both PV current references */
struct pv_bus_pi_t {
    float e;
    float kp;
    float p;
    float ki;
    float i;
    float u;
    float spare;
};

/* 0xC000 .. 0xC03F seen as one struct (grid_freq_setup.c, ctrl_glue.c): the same words as
   converter_ctrl.c's own globals. xC002 is conv_mode, xC004 inv_on, xC00C inv_duty,
   grid_cycle_samples and the relay sync points as named, pll_sin/pll_cos the PLL outputs */
struct ctrl_globals_view_t {
    Uint16 xC000;
    Uint16 xC001;
    Uint16 xC002; /* converter mode */
    Uint16 xC003;
    Uint16 xC004;
    Uint16 xC005;
    Uint16 xC006;
    Uint16 xC007;
    Uint16 xC008;
    int16 xC009;
    Uint16 xC00A;
    int16 xC00B;
    volatile int16 xC00C;
    Uint16 xC00D;
    Uint16 xC00E;
    Uint16 xC00F;
    Uint16 xC010;
    int16 xC011;
    Uint16 xC012;
    Uint16 xC013;
    Uint16 grid_cycle_samples;
    volatile Uint16 xC015;
    Uint16 xC016;
    Uint16 xC017;
    Uint16 xC018;
    Uint16 xC019;
    Uint16 xC01A;
    Uint16 xC01B;
    Uint16 xC01C;
    Uint16 xC01D;
    Uint16 xC01E;
    Uint16 relay_sync_b;
    Uint16 xC020;
    Uint16 relay_sync_a;
    Uint32 xC022;
    Uint32 xC024;
    Uint32 xC026;
    float pll_sin;
    float pll_cos;
    Uint16 xC02C;
    Uint16 xC02D;
    float xC02E;
    Uint16 xC030;
    Uint16 xC031;
    Uint16 xC032;
    Uint16 xC033;
    Uint16 xC034;
    Uint16 xC035;
    Uint16 xC036;
    Uint16 xC037;
    Uint16 xC038;
    Uint16 xC039;
    Uint16 xC03A;
    Uint16 xC03B;
    Uint16 xC03C;
    Uint16 xC03D;
    Uint16 xC03E;
    Uint16 xC03F;
};

/* 0xC040 .. 0xC07F as plain words: the PV bus limiter (0xC040) and the islanding detector
   (0xC04E); a layout view only */
struct pv_bus_island_view_t {
    float xC040;
    float xC042;
    float xC044;
    float xC046;
    float xC048;
    float xC04A;
    float xC04C;
    float xC04E;
    float xC050;
    float xC052;
    float xC054;
    float xC056;
    Uint16 xC058;
    Uint16 xC059;
    float xC05A;
    float xC05C;
    float xC05E;
    float xC060;
    Uint16 xC062;
    Uint16 xC063;
    Uint16 xC064;
    Uint16 xC065;
    Uint16 xC066;
    Uint16 xC067;
    Uint16 xC068;
    Uint16 xC069;
    Uint16 xC06A;
    Uint16 xC06B;
    Uint16 xC06C;
    Uint16 xC06D;
    Uint16 xC06E;
    Uint16 xC06F;
    Uint16 xC070;
    Uint16 xC071;
    Uint16 xC072;
    Uint16 xC073;
    Uint16 xC074;
    Uint16 xC075;
    Uint16 xC076;
    Uint16 xC077;
    Uint16 xC078;
    Uint16 xC079;
    Uint16 xC07A;
    Uint16 xC07B;
    Uint16 xC07C;
    Uint16 xC07D;
    Uint16 xC07E;
    Uint16 xC07F;
};

/* 0xC080: the last three grid voltage samples and the lead term adc_scale builds from them (a
   filtered second difference, +-10.0 V) for the grid current controller's feed-forward */
struct grid_v_hist_t {
    float v0;
    Uint16 xC082;
    Uint16 xC083;
    Uint16 xC084;
    Uint16 xC085;
    float v1;
    Uint16 xC088;
    Uint16 xC089;
    Uint16 xC08A;
    Uint16 xC08B;
    float v2;
    Uint16 xC08E;
    Uint16 xC08F;
    Uint16 xC090;
    Uint16 xC091;
    float lead_prev;
    Uint16 xC094;
    Uint16 xC095;
    Uint16 xC096;
    Uint16 xC097;
    float lead;
    Uint16 xC09A;
    Uint16 xC09B;
    Uint16 xC09C;
    Uint16 xC09D;
    Uint16 xC09E;
    Uint16 xC09F;
    Uint16 xC0A0;
    Uint16 xC0A1;
    Uint16 xC0A2;
    Uint16 xC0A3;
    Uint16 xC0A4;
    Uint16 xC0A5;
    Uint16 xC0A6;
    Uint16 xC0A7;
    Uint16 xC0A8;
    Uint16 xC0A9;
    Uint16 xC0AA;
    Uint16 xC0AB;
    Uint16 xC0AC;
    Uint16 xC0AD;
    Uint16 xC0AE;
    Uint16 xC0AF;
    Uint16 xC0B0;
    Uint16 xC0B1;
    Uint16 xC0B2;
    Uint16 xC0B3;
    Uint16 xC0B4;
    Uint16 xC0B5;
    Uint16 xC0B6;
    Uint16 xC0B7;
    Uint16 xC0B8;
    Uint16 xC0B9;
    Uint16 xC0BA;
    Uint16 xC0BB;
    Uint16 xC0BC;
    Uint16 xC0BD;
    Uint16 xC0BE;
    Uint16 xC0BF;
};

/* 0xC0C0: this sample's scaled ADC values (adc_scale.c). Voltages 0.1 V (AC ones signed about
   mid-scale); inv_i 0.1 A; pv1_i, pv2_i 0.01 A; hv_i the DC/DC high-side current (0.01 A, inferred
   from its limits); leak_i and leak_i_slow the residual current in mA; dci_i the DC injection
   channel; aux17/aux18 copy ADC channels nothing fills (always 0) */
struct meas_fast_t {
    float pv1_v;
    float pv2_v;
    float iso_v;
    float bus_v;
    float hv_v;
    float batt_v;
    float pv1_i;
    float pv2_i;
    float hv_i;
    float leak_i_slow;
    float dci_i;
    float xC0D6;
    float leak_i;
    float aux17;
    float aux18;
    float xC0DE;
    float inv_v;
    float xC0E2;
    float xC0E4;
    float eps_v;
    float grid_v;
    float inv_i;
    Uint16 xC0EC;
    Uint16 xC0ED;
    Uint16 xC0EE;
    Uint16 xC0EF;
    Uint16 xC0F0;
    Uint16 xC0F1;
    Uint16 xC0F2;
    Uint16 xC0F3;
    Uint16 xC0F4;
    Uint16 xC0F5;
    Uint16 xC0F6;
    Uint16 xC0F7;
    Uint16 xC0F8;
    Uint16 xC0F9;
    Uint16 xC0FA;
    Uint16 xC0FB;
    Uint16 xC0FC;
    Uint16 xC0FD;
    Uint16 xC0FE;
    Uint16 xC0FF;
};

/* 0xC100: the PLL block (PLL above) with the names converter_ctrl.c and ctrl_glue.c use */
struct grid_pll_view_t {
    float in;
    float k;
    float shift;
    Uint16 xC106;
    Uint16 xC107;
    Uint16 xC108;
    Uint16 xC109;
    Uint16 xC10A;
    Uint16 xC10B;
    Uint16 xC10C;
    Uint16 xC10D;
    float theta;
    float w;
    float s;
    float c;
    Uint16 xC116;
    Uint16 xC117;
    Uint16 n;
    Uint16 period;
    Uint16 xC11A;
    Uint16 wrap;
    Uint16 xC11C;
    Uint16 xC11D;
    float pi_e;
    float pi_kp;
    float pi_ki;
    float pi_kd;
    float pi_umax;
    float pi_umin;
    float pi_e1;
    float pi_i;
    float pi_u;
    Uint16 xC130;
    Uint16 xC131;
    Uint16 xC132;
    Uint16 xC133;
    Uint16 xC134;
    Uint16 xC135;
    Uint16 xC136;
    Uint16 xC137;
    Uint16 xC138;
    Uint16 xC139;
    Uint16 xC13A;
    Uint16 xC13B;
    Uint16 xC13C;
    Uint16 xC13D;
    Uint16 xC13E;
    Uint16 xC13F;
};

/* 0xC140: the eight-sample averages and sums (meas_avg8.c), the bus voltage notch filter state
   (bus_v_filt, without the 100/120 Hz ripple), the MPPT voltage references the PV loops use and
   pv_ramp_tick, the PV soft ceiling's cycle counter */
struct meas_avg_t {
    float pv1_v_ref;
    float pv2_v_ref;
    float pv1_v_acc;
    float pv2_v_acc;
    float bus_v_acc;
    float hv_v_acc;
    float batt_v_acc;
    float pv1_v;
    float pv2_v;
    float bus_v;
    float bus_v_z1;
    float bus_v_z2;
    float hv_v;
    float batt_v;
    float bus_v_filt;
    float bus_v_filt_z1;
    float bus_v_filt_z2;
    float pv1_i_acc;
    float pv2_i_acc;
    float pv1_i;
    float pv2_i;
    float pv1_v_k;
    float pv2_v_k;
    float xC16E;
    float xC170;
    float bus_v_k;
    Uint16 pv_ramp_tick;
    Uint16 xC175;
    Uint16 xC176;
    Uint16 xC177;
    Uint16 xC178;
    Uint16 xC179;
    Uint16 xC17A;
    Uint16 xC17B;
    Uint16 xC17C;
    Uint16 xC17D;
    Uint16 xC17E;
    Uint16 xC17F;
};

/* 0xC180: the reactive loop block (q_loop_t above), the names ctrl_glue.c and grid_ictl_clear use */
struct q_loop_view_t {
    float in;
    Uint16 xC182;
    Uint16 xC183;
    Uint16 xC184;
    Uint16 xC185;
    float i_in;
    Uint16 xC188;
    Uint16 xC189;
    Uint16 xC18A;
    Uint16 xC18B;
    float cos_in;
    Uint16 xC18E;
    Uint16 xC18F;
    Uint16 xC190;
    Uint16 xC191;
    float kp;
    float ki;
    float max;
    float min;
    Uint16 n;
    Uint16 xC19B;
    float ref_sum;
    Uint16 xC19E;
    Uint16 xC19F;
    Uint16 xC1A0;
    Uint16 xC1A1;
    float meas_sum;
    Uint16 xC1A4;
    Uint16 xC1A5;
    Uint16 xC1A6;
    Uint16 xC1A7;
    float e;
    Uint16 xC1AA;
    Uint16 xC1AB;
    Uint16 xC1AC;
    Uint16 xC1AD;
    Uint16 xC1AE;
    Uint16 xC1AF;
    Uint16 xC1B0;
    Uint16 xC1B1;
    Uint16 xC1B2;
    Uint16 xC1B3;
    float u;
    Uint16 xC1B6;
    Uint16 xC1B7;
    Uint16 xC1B8;
    Uint16 xC1B9;
    Uint16 xC1BA;
    Uint16 xC1BB;
    Uint16 xC1BC;
    Uint16 xC1BD;
    Uint16 xC1BE;
    Uint16 xC1BF;
};

/* 0xC1C0: twenty ADC channels of three words (the averaged reading less its offset first; adc_scale
 * writes only v), cleared with memset by the start-up code (sub_3ED38A's range) */
typedef struct {
    int16 v;
    int16 w1;
    int16 w2;
} adc_ch_t;

struct adc_channels_t {
    adc_ch_t pv1_v_raw;    /* 0xC1C0 */
    adc_ch_t pv2_v_raw;    /* 0xC1C3 */
    adc_ch_t pv1_i_raw;    /* 0xC1C6 */
    adc_ch_t pv2_i_raw;    /* 0xC1C9 */
    adc_ch_t iso_raw;      /* 0xC1CC */
    adc_ch_t bus_v_raw;    /* 0xC1CF */
    adc_ch_t hv_v_raw;     /* 0xC1D2 */
    adc_ch_t spare_ac_raw; /* 0xC1D5 */
    adc_ch_t batt_v_raw;   /* 0xC1D8 */
    adc_ch_t hv_i_raw;     /* 0xC1DB */
    adc_ch_t inv_v_raw;    /* 0xC1DE */
    adc_ch_t c11;          /* 0xC1E1 */
    adc_ch_t leak_i_raw;   /* 0xC1E4 */
    adc_ch_t dci_i_raw;    /* 0xC1E7 */
    adc_ch_t grid_v_raw;   /* 0xC1EA */
    adc_ch_t inv_i_raw;    /* 0xC1ED */
    adc_ch_t eps_v_raw;    /* 0xC1F0 */
    adc_ch_t aux17_raw;    /* 0xC1F3 */
    adc_ch_t aux18_raw;    /* 0xC1F6 */
    adc_ch_t c19;          /* 0xC1F9 */
};

/* 0xC200: the battery converter's references and limits that task 2 (power_limits.c) writes and
   converter_mode copies into the current controller each sample, the two low-battery wake PIs,
   and soft_duty, the isolated stage's duty (EPWM7/EPWM8, up to 1054). The padding words put
   i_limit at 0xC204, i_offset (full_trim_isr) at 0xC206, batt_v_max/batt_v_min at 0xC208/0xC20A
   (power_limits.h's isr_limits view: 56.0/46.0 V, 50.0 or 54.0 V in the aging test), i_cmd_max
   (hv_current_max, 15.0 A) at 0xC20E, the charge/discharge bounds at 0xC212/0xC214 and the
   current command at 0xC216. Currents 0.1 A on the high side. */
struct dcdc_ref_t {
    Uint16 xC200;
    Uint16 xC201;
    Uint16 xC202;
    Uint16 xC203;
    float i_limit;
    float i_offset;
    float batt_v_max;
    float batt_v_min;
    Uint16 xC20C;
    Uint16 xC20D;
    float i_cmd_max;
    float duty_ff;
    float chg_bound; /* charge bound */
    float dis_bound; /* discharge bound */
    float i_cmd;
    PI_BLK wake_batt_pi; /* 0xC218 */
    PI_BLK wake_bus_pi;  /* 0xC22A */
    Uint16 soft_duty;
    Uint16 xC23D;
    Uint16 xC23E;
    Uint16 xC23F;
};

/* The current controller's block at 0xC240 (current_controller's argument): inputs copied in by
 * converter_mode, mode words, and seven PI blocks. */
typedef struct {
    float batt_v; /* 0xC240 */
    float bus_v;
    float hv_i;
    float bus_v_ref;
    float i_cmd;
    float i_limit;
    float i_offset;
    float batt_v_max;
    float batt_v_min;
    float i_cmd_max;
    float duty_ff;
    float chg_bound; /* 0xC256 */
    float dis_bound;
    float spare;
    float bus_band;
    Uint16 mode; /* 0xC25E */
    Uint16 slot;
    Uint16 bms_mode;
    Uint16 batt_full;
    float i_cmd_prev;
    float i_ref;
    float duty; /* 0xC266 */
    Uint16 at_chg_bound;
    Uint16 start_ok;    /* 0xC269 */
    PI_BLK pi_bus_sync; /* 0xC26A */
    PI_BLK pi_bus;
    PI_BLK pi_bus_hi;
    PI_BLK pi_bus_lo;
    PI_BLK pi_v_max; /* 0xC2B2 */
    PI_BLK pi_v_min;
    PI_BLK pi_i; /* 0xC2D6 */
} dcdc_cc_t;

/* 0xC300: a running average of |in| over a quarter of a grid period (80 samples at 60 Hz, 96 at
 * 50 Hz), with a window check (avg_win_step; grid_v_check's alternative check) */
typedef struct {
    float in;    /* 0x00 */
    float max;   /* 0x02 */
    float min;   /* 0x04 */
    Uint16 mode; /* 0x06: 1 for 80 samples, else 96 */
    Uint16 pad;
    float abs_in;   /* 0x08 */
    float sum;      /* 0x0A */
    float avg;      /* 0x0C */
    float buf[100]; /* 0x0E */
    Uint16 idx;     /* 0xD6 */
    Uint16 ok;      /* 0xD7 */
} AVG_WIN;

/* 0xC400: the grid current controller block (grid_ictl_t above) with the callers' names; xC42A is
   v_ff2, the DC injection correction (dci_offset) task 4 writes */
struct grid_ictl_view_t {
    float v_ff;
    float i_meas;
    float i_ref_p;
    float i_ref_q;
    float q_cmd;
    float bus_v;
    float bus_v_ref;
    float mod_gain;
    float amp_ramp;
    float amp_max;
    float amp_min;
    float sin_ref;
    float cos_ref;
    float kp_i;
    Uint16 ramp_mode;
    Uint16 rep_lead;
    Uint16 rep_idx;
    Uint16 rep_len;
    float i_ref;
    float i_err;
    float v_cmd;
    float island_inj;
    float q_corr;
    Uint32 xC42A;
    float amp_cmd;
    float pi_e;
    float pi_kp;
    float pi_ki;
    float pi_kd;
    float pi_umax;
    float pi_umin;
    Uint16 xC43A;
    Uint16 xC43B;
    float pi_i;
    float pi_u;
};

/* 0xC440: the grid controller's repetitive filter, rep_filter_t's fields: the two index words,
 * then the parameters and state; the ring follows at 0xC452 */
struct grid_rep_view_t {
    Uint16 xC440;
    Uint16 xC441;
    float rep_in;
    float rep_max;
    float rep_min;
    float rep_a;
    float rep_b;
    float rep_gain;
    float rep_y;
    float rep_out;
    Uint16 xC452;
    Uint16 xC453;
    Uint16 xC454;
    Uint16 xC455;
    Uint16 xC456;
    Uint16 xC457;
    Uint16 xC458;
    Uint16 xC459;
    Uint16 xC45A;
    Uint16 xC45B;
    Uint16 xC45C;
    Uint16 xC45D;
    Uint16 xC45E;
    Uint16 xC45F;
    Uint16 xC460;
    Uint16 xC461;
    Uint16 xC462;
    Uint16 xC463;
    Uint16 xC464;
    Uint16 xC465;
    Uint16 xC466;
    Uint16 xC467;
    Uint16 xC468;
    Uint16 xC469;
    Uint16 xC46A;
    Uint16 xC46B;
    Uint16 xC46C;
    Uint16 xC46D;
    Uint16 xC46E;
    Uint16 xC46F;
    Uint16 xC470;
    Uint16 xC471;
    Uint16 xC472;
    Uint16 xC473;
    Uint16 xC474;
    Uint16 xC475;
    Uint16 xC476;
    Uint16 xC477;
    Uint16 xC478;
    Uint16 xC479;
    Uint16 xC47A;
    Uint16 xC47B;
    Uint16 xC47C;
    Uint16 xC47D;
    Uint16 xC47E;
    Uint16 xC47F;
};

/* 0xC83A: ctrl_slot, the eight-slot counter meas_avg8 steps every sample; the slow loops run on
   fixed slots (0 grid bus PI and PV1, 1 PV2, 2 PV precharge, 4 battery outer loops, 6 PV bus
   limiter). grid_ictl_t's last word, slot, is the same word */
struct grid_rep_tail_t {
    Uint16 xC800;
    Uint16 xC801;
    Uint16 xC802;
    Uint16 xC803;
    Uint16 xC804;
    Uint16 xC805;
    Uint16 xC806;
    Uint16 xC807;
    Uint16 xC808;
    Uint16 xC809;
    Uint16 xC80A;
    Uint16 xC80B;
    Uint16 xC80C;
    Uint16 xC80D;
    Uint16 xC80E;
    Uint16 xC80F;
    Uint16 xC810;
    Uint16 xC811;
    Uint16 xC812;
    Uint16 xC813;
    Uint16 xC814;
    Uint16 xC815;
    Uint16 xC816;
    Uint16 xC817;
    Uint16 xC818;
    Uint16 xC819;
    Uint16 xC81A;
    Uint16 xC81B;
    Uint16 xC81C;
    Uint16 xC81D;
    Uint16 xC81E;
    Uint16 xC81F;
    Uint16 xC820;
    Uint16 xC821;
    Uint16 xC822;
    Uint16 xC823;
    Uint16 xC824;
    Uint16 xC825;
    Uint16 xC826;
    Uint16 xC827;
    Uint16 xC828;
    Uint16 xC829;
    Uint16 xC82A;
    Uint16 xC82B;
    Uint16 xC82C;
    Uint16 xC82D;
    Uint16 xC82E;
    Uint16 xC82F;
    Uint16 xC830;
    Uint16 xC831;
    Uint16 xC832;
    Uint16 xC833;
    Uint16 xC834;
    Uint16 xC835;
    Uint16 xC836;
    Uint16 xC837;
    Uint16 xC838;
    Uint16 xC839;
    Uint16 ctrl_slot;
    Uint16 xC83B;
    Uint16 xC83C;
    Uint16 xC83D;
    Uint16 xC83E;
    Uint16 xC83F;
};

/* 0xC840: the EPS voltage controller block (eps_vctl_t above) with the callers' names, up to the
   repetitive filter's parameters */
struct eps_vctl_view_t {
    float v_amp_ref;
    float v_meas;
    float i_meas;
    float v_ff;
    float i_filt;
    float sin_ref;
    float i_limit;
    Uint16 mode;
    Uint16 rep_lead;
    float v_peak;
    Uint16 xC852;
    Uint16 xC853;
    volatile Uint16 ramp_done;
    Uint16 xC855;
    Uint16 xC856;
    Uint16 xC857;
    Uint16 xC858;
    Uint16 xC859;
    float rep_max;
    float rep_min;
    float rep_a;
    float rep_b;
    float rep_gain;
    Uint16 xC864;
    Uint16 xC865;
    Uint16 xC866;
    Uint16 xC867;
    Uint16 xC868;
    Uint16 xC869;
    Uint16 xC86A;
    Uint16 xC86B;
    Uint16 xC86C;
    Uint16 xC86D;
    Uint16 xC86E;
    Uint16 xC86F;
    Uint16 xC870;
    Uint16 xC871;
    Uint16 xC872;
    Uint16 xC873;
    Uint16 xC874;
    Uint16 xC875;
    Uint16 xC876;
    Uint16 xC877;
    Uint16 xC878;
    Uint16 xC879;
    Uint16 xC87A;
    Uint16 xC87B;
    Uint16 xC87C;
    Uint16 xC87D;
    Uint16 xC87E;
    Uint16 xC87F;
};

/* 0xCC40: the EPS controller's two PI blocks and outputs as flat fields (eps_vctl_t from 0x400),
   then the trims (xCC74 dc_trim, always 0; xCC76 amp_trim from task 1), i_ref, mod_gain, duty and
   the EPS angle theta (0xCC7E) */
struct eps_vctl_tail_t {
    Uint16 xCC40;
    Uint16 xCC41;
    Uint16 xCC42;
    Uint16 xCC43;
    Uint16 xCC44;
    Uint16 xCC45;
    Uint16 xCC46;
    Uint16 xCC47;
    Uint16 xCC48;
    Uint16 xCC49;
    Uint16 xCC4A;
    Uint16 xCC4B;
    Uint16 xCC4C;
    Uint16 xCC4D;
    Uint16 xCC4E;
    Uint16 xCC4F;
    float pi_v_e;
    float pi_v_kp;
    float pi_v_ki;
    float pi_v_kd;
    float pi_v_umax;
    float pi_v_umin;
    float pi_v_e1;
    float pi_v_i;
    float pi_v_u;
    Uint16 xCC62;
    Uint16 xCC63;
    float pi_i_kp;
    float pi_i_ki;
    float pi_i_kd;
    float pi_i_umax;
    float pi_i_umin;
    float pi_i_e1;
    float pi_i_i;
    float pi_i_u;
    float xCC74;
    Uint32 xCC76;
    float i_ref;
    float mod_gain;
    float duty;
    float theta;
#ifdef C_CTRL_EPS_TAIL_WITH_CYCLE
    /* converter_ctrl.c sees the grid angle's step count, index and wrap flag as part of this object:
     * stores to them keep their order relative to the angle's (grid_angle_update) */
    Uint16 cycle_len;
    Uint16 cycle_idx;
    Uint16 cycle_wrap;
#endif
};

/* 0xCC80: the EPS angle's cycle length, index and wrap flag (grid_angle_update), then the current
   sensor offsets measured at start-up (dci, hv_i, pv1_i, pv2_i, inv_i) that adc_scale takes off */
struct eps_cycle_page_t {
    Uint16 cycle_len;
    Uint16 cycle_idx;
    Uint16 cycle_wrap;
    Uint16 xCC83;
    Uint16 xCC84;
    Uint16 xCC85;
    Uint16 xCC86;
    Uint16 xCC87;
    Uint16 xCC88;
    Uint16 xCC89;
    Uint16 xCC8A;
    Uint16 xCC8B;
    Uint32 xCC8C;
    Uint32 xCC8E;
    Uint16 xCC90;
    Uint16 xCC91;
    Uint16 xCC92;
    Uint16 xCC93;
    Uint16 xCC94;
    Uint16 xCC95;
    Uint16 xCC96;
    Uint16 xCC97;
    Uint16 xCC98;
    Uint16 xCC99;
    Uint16 xCC9A;
    Uint16 xCC9B;
    Uint16 dci_ofs;
    Uint16 xCC9D;
    Uint32 xCC9E;
    Uint16 hv_i_ofs;
    Uint16 xCCA1;
    Uint32 xCCA2;
    Uint16 pv1_i_ofs;
    Uint16 xCCA5;
    Uint32 xCCA6;
    Uint16 pv2_i_ofs;
    Uint16 xCCA9;
    Uint32 xCCAA;
    Uint16 inv_i_ofs;
    Uint16 xCCAD;
    Uint32 xCCAE;
    Uint16 xCCB0;
    Uint16 xCCB1;
    Uint16 xCCB2;
    Uint16 xCCB3;
    Uint32 xCCB4;
    Uint32 xCCB6;
    Uint32 xCCB8;
    Uint16 xCCBA;
    Uint16 xCCBB;
    Uint16 xCCBC;
    Uint16 xCCBD;
    float xCCBE;
};

/* 0xC02C: the leakage current's fast sum (control_isr) */
struct leak_acc_t {
    Uint16 count;
    float sum;
};

/* 0xC030: the relay drive state (relay_drive): a count, and the three relay commands at 0xC034 */
struct relay_ctl_t {
    Uint16 relay_a_count;
    Uint16 x31;
    Uint16 x32;
    Uint16 x33;
    Uint16 relay_a;
    Uint16 relay_b;
    Uint16 relay_c;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_Device.h"
#include "util.h"

/* converter_ctrl.c */
#ifndef OWN_batt_current_cmd_i
extern Uint16 batt_current_cmd_i;
#endif
extern Uint16 bms_hr15_copy;
#ifndef OWN_chg_current_bound_i
extern Uint16 chg_current_bound_i;
#endif
interrupt void control_isr(void);
extern Uint16 conv_mode;
void converter_mode(void);
void dcdc_ctrl_init(void);
void dcdc_ctrl_reset(void);
#ifndef OWN_dis_current_bound_i
extern Uint16 dis_current_bound_i;
#endif
extern Uint32 ecap_period;
extern Uint16 ecap_valid_count;
void eps_vctl_run(void);
void fast_fault_check(void);
extern Uint16 force_chg_soc_isr;
void grid_angle_update(void);
void grid_ctrl_init(void);
extern Uint16 grid_cycle_samples;
void grid_ictl_clear(void);
interrupt void grid_period_isr(void);
void grid_v_check(void);
extern Uint16 hr4000_value;
extern Uint16 inv_on;
void inv_pwm_update(void);
void island_detect_run(void);
void iso_dcdc_pwm(void);
extern Uint16 pv1_boost_on;
void pv1_ctrl(void);
extern Uint16 pv1_ref_isr;
void pv1_stop(void);
extern Uint16 pv2_boost_on;
void pv2_ctrl(void);
extern Uint16 pv2_ref_isr;
void pv2_stop(void);
void pv_bus_limit(void);
extern struct pv_bus_pi_t pv_bus_pi;
void pv_ctrl_init(void);
void pv_ctrl_reset(void);
void pv_stop_all(void);
extern int16 pwm_period;
extern struct relay_ctl_t relay_ctl;
void relay_drive(void);
extern Uint16 soft_start_phase;
extern Uint16 state_trace;
interrupt void tz_trip_isr(void);

/* ctrl_glue.c */
void grid_pll_run(void);
void q_loop_run(void);

/* grid_freq_setup.c */
void grid_freq_setup(Uint16 mode);

/* pi_ctrl.c */
float pi_inc_step(PI_BLK *p);
float pi_step(PI_BLK *p);

/* current_ctrl.c */
void current_controller(dcdc_cc_t *s);

/* island_detect.c */
void island_detect(island_det_t *p);
void q_loop_step(q_loop_t *p);

/* pll.c */
void pll_step(PLL *p);

/* eps_vctl_clear.c */
void eps_vctl_clear(void);

/* eps_vctl_init.c */
void eps_vctl_init(void);

/* pwm_force.c */
void pwm34_force_off(void);
void pwm34_release(void);
void pwm5_force_off(void);
void pwm5_release(void);
void pwm78_force_low(void);
void pwm78_release(void);

/* ac_ctrl.c */
void eps_vctl(eps_vctl_t *p);
void grid_ictl(grid_ictl_t *p);
void rep_filter_step(rep_filter_t *p);

/* defined by no source: the linker places it at its map address */
extern struct adc_channels_t adc_ch;
extern float bus_v_ref;
extern struct ctrl_globals_view_t
    ctrl_globals; /* name: converter_ctrl.c's own globals from state_trace (0xC000) seen as one struct (an alias row in map/data.tsv) */
extern Uint16 ctrl_slot;
extern dcdc_cc_t dcdc_cc;
extern struct dcdc_ref_t dcdc_ref;
#ifndef OWN_eps_cycle
extern struct eps_cycle_page_t eps_cycle;
#endif
extern float eps_rep_buf[500];
extern float eps_v_ref;
extern struct eps_vctl_view_t eps_vctl_blk;
extern struct eps_vctl_tail_t eps_vctl_tail;
extern float grid_current_max;
#ifndef OWN_grid_ictl_blk
extern struct grid_ictl_view_t grid_ictl_blk;
#endif
extern float grid_omega_step; /* the grid angle step */
extern struct grid_pll_view_t grid_pll;
extern float grid_rep_buf[430];
extern struct grid_rep_tail_t grid_rep_tail;
extern struct grid_v_hist_t grid_v_hist;
extern AVG_WIN grid_v_win;
extern struct meas_avg_t meas_avg;
#ifndef OWN_meas_fast
extern struct meas_fast_t meas_fast;
#endif
#ifndef OWN_mppt_flags
extern union bits16_u mppt_flags;
#endif
extern struct q_loop_view_t q_loop;
extern float rcmu_i_fast;
/* ---- end of declarations ---- */

#endif
