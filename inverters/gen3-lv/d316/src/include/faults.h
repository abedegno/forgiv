/* The fault and status words. op_states.c defines fault_words (0xCF12 .. 0xCF19) and the
   queries has_*_fault; each task sets its own bits. A bit in CF15 is a battery fault: it stops
   the battery converter and its soft start (has_batt_fault). CF19 holds the battery blocks. */
#ifndef FAULTS_H
#define FAULTS_H

#include "F2806x_Device.h"
#include "util.h"

/* 0xD7C0 .. 0xD7C2 */
struct eps_cfg_bits_t {
    union bits16_u eps_cfg;
    union bits16_u run_status;
    union bits16_u grid_func;
};

/* 0xCF12: status and fault words */
struct fault_words_t {
    Uint16 fault_grid1; /* 0xCF12 */
    Uint16 w13;
    Uint16 w14;
    Uint16
        fault_batt; /* 0xCF15: bit 1 V mismatch, 2 UV, 3 calibration end, 4 DC-DC ratio, 5 soft-start timeout, 6 OV, 8 BMS alarm (never set), 9 BMS comms, 15 HR19 bit 4 */
    Uint16 w16;
    Uint16 fault_stop1;         /* 0xCF17 */
    union bits16_u fault_stop2; /* 0xCF18: bit 2 stops the BMS task, bit 6 calibration fault */
    Uint16
        batt_blocks; /* 0xCF19: bit 2 no power in mode 0 (ARM), 7 EPS overload, 8 full, 9 force charge, 10 0xD502 bit 11 (never set), 11 no discharge */
    Uint16 precharge_step; /* 0xCF1A */
};

/* Bits of the fault words (0xCF12 .. 0xCF19) where the code uses masks; the meanings are what
 * the code that sets them does (docs/names/A-notes.md, B-notes.md). */
#define FG1_TZ_TRIP 0x10     /* CF12 bit 4: the trip-zone interrupt (tz_trip_isr) */
#define FG1_GRID_VOLT 0x1000 /* CF12 bit 12: grid voltage trip */
#define FGE_EPS_OVERLOAD                                                                           \
    0x1000 /* CF14 bit 12: EPS overload (over 1.1 x rated for 150 runs, 1.5 x for 5) */
#define FB_UNDER_V 0x0004 /* CF15 bit 2: battery under-voltage */
#define FB_CAL_END 0x0008 /* CF15 bit 3: set when a battery calibration ends (BMS link) */
#define FB_SOFT_START_TIMEOUT                                                                      \
    0x20 /* CF15 bit 5: the soft start did not finish in 8000 passes (160 s); never cleared after start-up */
#define FB_OVER_V 0x0040         /* CF15 bit 6: battery over-voltage */
#define FB_BMS_COMMS 0x0200      /* CF15 bit 9: no BMS reply (BMS mode) */
#define FS1_CHARGER_OT 0x0040    /* CF17 bit 6: charger over-temperature */
#define FS1_BAD_PRESET 0x0400    /* CF17 bit 10: unknown model, preset_index 0 (rating_apply) */
#define FS2_ARM_LINK_LOST 0x0004 /* CF18 bit 2: no ARM frames (also set at start-up) */
#define BB_EPS_OVERLOAD 0x0080   /* CF19 bit 7: EPS output over 1.1 x rated (each run) */
#define BB_FULL 0x100            /* CF19 bit 8: battery full, no charging */
/* the other bits the notes name; the code reads and writes these through the .bit views */
#define FG1_GRID_FREQ 0x8000       /* CF12 bit 15: grid frequency trip (grid_freq_protect) */
#define FGE_EXPORT_OVERLOAD 0x0020 /* CF14 bit 5: export over the HR26 limit for too long */
#define FGE_RECONNECT_WAIT 0x0080  /* CF14 bit 7: grid V or f outside the reconnect window */
#define FGE_EPS_BUS_UV 0x0800      /* CF14 bit 11: EPS bus under-voltage */
#define FB_V_MISMATCH 0x0002 /* CF15 bit 1: measured and BMS (HR22) voltages differ by over 5.0 V */
#define FB_DCDC_RATIO 0x0010 /* CF15 bit 4: the HV side too far from the DC-DC ratio */
#define FB_BMS_ALARM                                                                               \
    0x0100 /* CF15 bit 8: an alarm bit of 0xD500 (bits 0, 3, 5, 8, 12, which nothing writes) */
#define FB_HR19_B4 0x8000      /* CF15 bit 15: schedule window 3, or BMS HR19 bit 4 (soc_floor) */
#define FPV_ABSENT 0x0020      /* CF16 bit 5: no PV (also set at start-up) */
#define FS1_DCI 0x0001         /* CF17 bit 0: DC injection trip (dci_control) */
#define FS1_ISO 0x0002         /* CF17 bit 1: insulation test */
#define FS1_NTC 0x0004         /* CF17 bit 2: NTC (never set: ntc_temp_check compares a constant) */
#define FS1_BUS_OV 0x0008      /* CF17 bit 3: bus over-voltage */
#define FS1_PV_OV 0x0010       /* CF17 bit 4: PV over-voltage */
#define FS1_HEATSINK_OT 0x0080 /* CF17 bit 7: heatsink over-temperature */
#define FS1_PV_SHORT 0xC000    /* CF17 bits 14 and 15: a PV input under 30 V with more than 3 A */
#define FS2_CAL_FAULT 0x0040   /* CF18 bit 6: calibration cross-check (rcmu_fast_check.c) */
#define BB_FORCE_CHARGE                                                                            \
    0x0200 /* CF19 bit 9: SoC at or below the floor - 3 (or 1): at least 300 W charge */
#define BB_NO_DISCHARGE 0x0800 /* CF19 bit 11: SoC at or below the floor: no discharge */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "util.h"

/* op_states.c */
#ifndef OWN_fault_words
extern union bits16_u fault_words[8];
#endif
unsigned int has_batt_fault(void);
unsigned int has_grid_fault(void);
unsigned int has_pv_fault(void);
#ifndef OWN_state_flags
extern union bits16_u state_flags;
#endif

/* defined by no source: the linker places it at its map address */
extern union bits16_u batt_blocks;
#ifndef OWN_eps_cfg_bits
extern struct eps_cfg_bits_t eps_cfg_bits;
#endif
extern union bits16_u fault_batt;
extern union bits16_u fault_grid_eps;
extern union bits16_u fault_pv;
extern union bits16_u fault_stop1;
extern union bits16_u fault_stop2;
extern union bits16_u grid_func_flags;
extern union bits16_u run_status_bits;
/* ---- end of declarations ---- */

#endif
