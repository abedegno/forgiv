/* settings.h: the settings blocks (holding registers saved to the EEPROM), the grid code globals
   and the calibration gains. The blocks are defined in board.c's data; the Modbus write handlers
   (comm/modbus_slave.c) change them, board.c loads them and sets their defaults, and
   drivers/eeprom.c stores them.
   name: descriptive (readability step 2). */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "ghidra_types.h"

/* EEPROM block 1 (0xD3 bytes at 0x20001AAE): the holding registers the inverter keeps. Each field's comment gives the HR that modbus_fc06_write_reg stores there; the names of HR0-116 follow the public GivEnergy register list, the rest are named by register number */
typedef __packed struct {
    ushort inverter_module_h; /* +0x00 HR1 */
    ushort inverter_module_l; /* +0x02 HR2 */
    uchar f04[2]; /* +0x04: holds the CRC of bytes 0-5 in the EEPROM image (eeprom_pack_block) */
    uchar f06;    /* +0x06 1 after defaults; another value reloads the
                                             grid limits at boot */
    ushort inverter_start_time; /* +0x07 HR61 */
    uchar f09;                  /* +0x09 HR53 high byte; cfg_load_all clears it at boot */
    uchar hr129;                /* +0x0A HR129 */
    uchar active_power_rate;    /* +0x0B HR50 (also HR489) */
    uchar reactive_power_rate;  /* +0x0C HR51 */
    ushort power_factor;        /* +0x0D HR52 */
    uchar inverter_state;       /* +0x0F HR53 (also HR490) */
    uchar hr128;                /* +0x10 HR128 */
    uchar hr123;                /* +0x11 HR123 */
    uchar hr31;                 /* +0x12 HR31 */
    uchar inverter_serial
        [5][2];   /* +0x13 HR13-17: the 10-character serial number, two characters a register */
    ushort hr138; /* +0x1D HR138 */
    ushort hr139; /* +0x1F HR139 */
    ushort hr140; /* +0x21 HR140 */
    ushort hr141; /* +0x23 HR141 */
    ushort hr130; /* +0x25 HR130 */
    ushort hr131; /* +0x27 HR131 */
    ushort hr132; /* +0x29 HR132 */
    ushort hr133; /* +0x2B HR133 */
    ushort hr134; /* +0x2D HR134 */
    ushort hr135; /* +0x2F HR135 */
    ushort hr136; /* +0x31 HR136 */
    ushort hr137; /* +0x33 HR137 */
    uchar hr122;  /* +0x35 HR122 */
    uchar hr142;  /* +0x36 HR142 */
    ushort hr144; /* +0x37 HR144 */
    ushort hr145; /* +0x39 HR145 */
    ushort v_pv_input_start;                   /* +0x3B HR60 */
    ushort v_ac_high_out;                      /* +0x3D HR64 */
    ushort v_ac_low_out;                       /* +0x3F HR63 */
    ushort f_ac_high_out;                      /* +0x41 HR66 */
    ushort f_ac_low_out;                       /* +0x43 HR65 */
    ushort v_ac_high_in;                       /* +0x45 HR72 */
    ushort v_ac_low_in;                        /* +0x47 HR71 */
    ushort f_ac_high_in;                       /* +0x49 HR74 */
    ushort f_ac_low_in;                        /* +0x4B HR73 */
    ushort v_ac_high_c;                        /* +0x4D HR80 */
    ushort v_ac_low_c;                         /* +0x4F HR79 */
    ushort v_ac_high_out_time;                 /* +0x51 HR68 */
    ushort v_ac_low_out_time;                  /* +0x53 HR67 */
    ushort f_ac_high_out_time;                 /* +0x55 HR70 */
    ushort f_ac_low_out_time;                  /* +0x57 HR69 */
    ushort v_ac_high_in_time;                  /* +0x59 HR76 */
    ushort v_ac_low_in_time;                   /* +0x5B HR75 */
    ushort f_ac_high_in_time;                  /* +0x5D HR78 */
    ushort f_ac_low_in_time;                   /* +0x5F HR77 */
    ushort f_ac_high_c;                        /* +0x61 HR82 */
    ushort f_ac_low_c;                         /* +0x63 HR81 */
    ushort v_10_min_protection;                /* +0x65 HR83 */
    uchar modbus_address;                      /* +0x67 HR30 */
    uchar hr101;                               /* +0x68 HR101 */
    ushort f69;                                /* +0x69 from the grid table, DSP page 4 */
    ushort f6B;                                /* +0x6B from the grid table, DSP page 4 */
    uchar hr121;                               /* +0x6D HR121 */
    uchar hr143;                               /* +0x6E HR143 */
    uchar hr125;                               /* +0x6F HR125 */
    ushort inverter_restart_delay;             /* +0x70 HR62 */
    uchar hr166;                               /* +0x72 HR166: 1 keeps many writes in RAM only */
    uchar f73;                                 /* +0x73 HR167 (FC16 only, 0..4) */
    uchar f74;                                 /* +0x74 DSP page 0 */
    ushort hr321;                              /* +0x75 HR321 */
    uchar hr113;                               /* +0x77 HR113 */
    uchar battery_type;                        /* +0x78 HR54 */
    ushort battery_capacity;                   /* +0x79 HR55 (also HR512) */
    ushort grid_port_max_power;                /* +0x7B HR26 */
    uchar battery_power_mode;                  /* +0x7D HR27 (also HR481) */
    ushort battery_low_voltage_limit;          /* +0x7E HR97 */
    ushort battery_high_voltage_limit;         /* +0x80 HR98 */
    uchar enable_discharge;                    /* +0x82 HR59 (also HR485) */
    ushort discharge_slot_1_start;             /* +0x83 HR56 (also HR483) */
    ushort discharge_slot_1_end;               /* +0x85 HR57 (also HR484) */
    ushort discharge_slot_2_start;             /* +0x87 HR44 */
    ushort discharge_slot_2_end;               /* +0x89 HR45 */
    uchar enable_charge;                       /* +0x8B HR96 (also HR488) */
    ushort charge_slot_1_start;                /* +0x8C HR94 (also HR486) */
    ushort charge_slot_1_end;                  /* +0x8E HR95 (also HR487) */
    uchar enable_60hz_freq_mode;               /* +0x90 HR28 */
    uchar soc_force_adjust;                    /* +0x91 HR29 (also HR482) */
    uchar enable_drm_rj45;                     /* +0x92 HR41 */
    uchar enable_ammeter;                      /* +0x93 HR7 */
    uchar enable_reverse_ct_clamp;             /* +0x94 HR42 */
    uchar user_code;                           /* +0x95 HR33 */
    uchar battery_soc_reserve;                 /* +0x96 HR110 */
    uchar hr108;                               /* +0x97 HR108 */
    uchar hr109;                               /* +0x98 HR109 */
    uchar battery_charge_limit;                /* +0x99 HR111 */
    uchar battery_discharge_limit;             /* +0x9A HR112 */
    uchar hr176;                               /* +0x9B HR176 */
    uchar battery_discharge_min_power_reserve; /* +0x9C HR114 */
    uchar hr115;                               /* +0x9D HR115 */
    uchar hr177;                               /* +0x9E HR177 */
    uchar meter_type;                          /* +0x9F HR47 */
    uchar reverse_115_meter_dir;               /* +0xA0 HR48 */
    uchar reverse_418_meter_dir;               /* +0xA1 HR49 */
    uchar fA2;                                 /* +0xA2 served as HR43; 3 after an HR48/49 change */
    uchar enable_auto_judge_battery_type;      /* +0xA3 HR58 */
    uchar enable_charge_target;                /* +0xA4 HR20 */
    uchar charge_target_soc;                   /* +0xA5 HR116 */
    uchar fA6;                                 /* +0xA6 layout version: cfg_load_all resets HR97/98
                                             while it is <= 0x14 */
    ushort hr300;                              /* +0xA7 HR300 */
    ushort hr301;                              /* +0xA9 HR301 */
    ushort hr302;                              /* +0xAB HR302 */
    ushort hr303;                              /* +0xAD HR303 */
    uchar hr178;                               /* +0xAF HR178 */
    uchar hr179;                               /* +0xB0 HR179 */
    uchar hr305;                               /* +0xB1 HR305 (also HR332) */
    ushort hr306;                              /* +0xB2 HR306 */
    ushort hr307;                              /* +0xB4 HR307 */
    ushort hr308;                              /* +0xB6 HR308 */
    ushort hr309;                              /* +0xB8 HR309 */
    uchar hr310;                               /* +0xBA HR310 */
    uchar hr311;                               /* +0xBB HR311 */
    ushort hr312;                              /* +0xBC HR312 */
    uchar hr313;                               /* +0xBE HR313 */
    uchar hr314;                               /* +0xBF HR314 (also HR515) */
    ushort hr315;                              /* +0xC0 HR315 */
    ushort hr316;                              /* +0xC2 HR316 */
    ushort hr102;                              /* +0xC4 HR102 (also HR317) */
    uchar hr318;                               /* +0xC6 HR318 */
    ushort hr319;                              /* +0xC7 HR319 */
    ushort hr320;                              /* +0xC9 HR320 */
    uchar hr327;                               /* +0xCB HR327 */
    ushort hr326;                              /* +0xCC HR326 */
    ushort hr328;                              /* +0xCE HR328 */
    uchar hr330;                               /* +0xD0 HR330 */
    uchar hr331;                               /* +0xD1 HR331 */
    uchar fD2;                                 /* +0xD2 HR332 (1 lead acid), DSP page 2 */
} settings_t;

/* EEPROM block 6 (0xFA bytes at 0x20001C82): more holding registers, mostly time slots. Each slot is an HHMM start, an HHMM end and a SOC byte */
typedef __packed struct {
    uchar hr322;          /* +0x00 HR322: 1 = half-hour table mode */
    ushort hr323;         /* +0x01 HR323 */
    ushort hr324;         /* +0x03 HR324 */
    ushort hr325;         /* +0x05 HR325 */
    uchar hr272;          /* +0x07 HR272 */
    uchar hr275;          /* +0x08 HR275 */
    ushort slot_a1_start; /* +0x09 HR276 */
    ushort slot_a1_end;   /* +0x0B HR277 */
    uchar slot_a1_soc;    /* +0x0D HR278 */
    ushort slot_a2_start; /* +0x0E HR279 */
    ushort slot_a2_end;   /* +0x10 HR280 */
    uchar slot_a2_soc;    /* +0x12 HR281 */
    ushort slot_a3_start; /* +0x13 HR282 */
    ushort slot_a3_end;   /* +0x15 HR283 */
    uchar slot_a3_soc;    /* +0x17 HR284 */
    ushort slot_a4_start; /* +0x18 HR285 */
    ushort slot_a4_end;   /* +0x1A HR286 */
    uchar slot_a4_soc;    /* +0x1C HR287 */
    ushort slot_a5_start; /* +0x1D HR288 */
    ushort slot_a5_end;   /* +0x1F HR289 */
    uchar slot_a5_soc;    /* +0x21 HR290 */
    ushort slot_a6_start; /* +0x22 HR291 */
    ushort slot_a6_end;   /* +0x24 HR292 */
    uchar slot_a6_soc;    /* +0x26 HR293 */
    ushort slot_a7_start; /* +0x27 HR294 */
    ushort slot_a7_end;   /* +0x29 HR295 */
    uchar slot_a7_soc;    /* +0x2B HR296 */
    ushort slot_a8_start; /* +0x2C HR297 */
    ushort slot_a8_end;   /* +0x2E HR298 */
    uchar slot_a8_soc;    /* +0x30 HR299 */
    uchar hr242;          /* +0x31 HR242 */
    ushort slot_b1_start; /* +0x32 HR243 */
    ushort slot_b1_end;   /* +0x34 HR244 */
    uchar slot_b1_soc;    /* +0x36 HR245 */
    ushort slot_b2_start; /* +0x37 HR246 */
    ushort slot_b2_end;   /* +0x39 HR247 */
    uchar slot_b2_soc;    /* +0x3B HR248 */
    ushort slot_b3_start; /* +0x3C HR249 */
    ushort slot_b3_end;   /* +0x3E HR250 */
    uchar slot_b3_soc;    /* +0x40 HR251 */
    ushort slot_b4_start; /* +0x41 HR252 */
    ushort slot_b4_end;   /* +0x43 HR253 */
    uchar slot_b4_soc;    /* +0x45 HR254 */
    ushort slot_b5_start; /* +0x46 HR255 */
    ushort slot_b5_end;   /* +0x48 HR256 */
    uchar slot_b5_soc;    /* +0x4A HR257 */
    ushort slot_b6_start; /* +0x4B HR258 */
    ushort slot_b6_end;   /* +0x4D HR259 */
    uchar slot_b6_soc;    /* +0x4F HR260 */
    ushort slot_b7_start; /* +0x50 HR261 */
    ushort slot_b7_end;   /* +0x52 HR262 */
    uchar slot_b7_soc;    /* +0x54 HR263 */
    ushort slot_b8_start; /* +0x55 HR264 */
    ushort slot_b8_end;   /* +0x57 HR265 */
    uchar slot_b8_soc;    /* +0x59 HR266 */
    ushort slot_b9_start; /* +0x5A HR267 */
    ushort slot_b9_end;   /* +0x5C HR268 */
    uchar slot_b9_soc;    /* +0x5E HR269 */
    uchar hr230;          /* +0x5F HR230 */
    uchar hr124;          /* +0x60 HR124 */
    uchar hr494;          /* +0x61 HR494: 1 = remote battery power (HR495) */
    uchar hr351;          /* +0x62 HR351 */
    uchar hr352;          /* +0x63 HR352 */
    ushort hr353;         /* +0x64 HR353 */
    uchar f66[148];       /* +0x66: cleared by cfg_load_defaults(6) */
} settings6_t;

/* EEPROM block 2 (0x44 bytes at 0x20001B81): calibration, a gain (around 0x400) and an offset per channel, written by HR4097-4385; byte +0x34 is the stored "BMS update failed" flag */
typedef __packed struct {
    ushort gain0;  /* +0x00 HR4097 */
    short offset0; /* +0x02 HR4354 */
    ushort gain1;  /* +0x04 HR4104 */
    short offset1; /* +0x06 HR4361 */
    ushort gain2;  /* +0x08 HR4098 */
    short offset2; /* +0x0A HR4355 */
    ushort gain3;  /* +0x0C HR4105 */
    short offset3; /* +0x0E HR4362 */
    ushort gain4;  /* +0x10 HR4099 */
    short offset4; /* +0x12 HR4356 */
    ushort gain5;  /* +0x14 HR4101 */
    short offset5; /* +0x16 HR4358 */
    ushort gain6;  /* +0x18 HR4106 */
    short offset6; /* +0x1A HR4363 */
    ushort gain7;  /* +0x1C HR4120 */
    short offset7; /* +0x1E HR4366 */
    ushort gain8;  /* +0x20 HR4146 */
    short offset8; /* +0x22 HR4378 */
    /* +0x24-0x33: four more gain and offset pairs with no holding register;
       cal_gains_validate_all checks the gains, calib_table_defaults clears the offsets */
    ushort gain_24;
    short offset_26;
    ushort gain_28;
    short offset_2A;
    ushort gain_2C;
    short offset_2E;
    ushort gain_30;
    short offset_32;
    ushort bms_update_failed; /* +0x34 loaded by bms_update_fail_flag_load */
    short offset_36;          /* +0x36 the offset slot of the pair bms_update_failed takes */
    ushort gain9;             /* +0x38 HR4382 */
    short offset9;            /* +0x3A HR4383 */
    ushort gain10;            /* +0x3C HR4384 */
    short offset10;           /* +0x3E HR4385 */
    ushort gain11;            /* +0x40 HR4380 */
    short offset11;           /* +0x42 HR4381 */
} cal_gains_t;

/* data */
extern cal_gains_t cal_gains;
extern settings6_t cfg_block6;
extern uchar cfg_block7[];
extern uchar cfg_block8[];
extern uchar cfg_changed_flags;
extern ushort cfg_hr360_table[2]
                             [48]; /* table mode: two tables of 48 half-hour values, [0] HR420-467
                                        and [1] HR360-407 (the symbol sched_table_b) */
extern uchar cfg_reset_mode;
extern uchar cfg_serial_and_settings[];
extern settings_t cfg_settings;
extern uchar grid_country; /* HR2 high byte, 0-28 */
extern uchar grid_country_byte_a;
extern uchar grid_country_byte_b;
extern ushort grid_current;
extern uchar grid_table_set; /* row set of grid_limits_tbl, always 0 */

/* functions */
uint cal_gain_validate(ushort *gain);
void cal_gains_validate_all();
uint calib_apply_q10(int sample, int gain, int offset);
void calib_table_defaults();
void cfg_defaults_grid();
uint cfg_limit_check(uint value, int lower_limit, int upper_limit);
void cfg_load_all();
void cfg_load_defaults(int block);
int grid_country_from_setting();
void grid_load_country_bytes();
void grid_load_country_limits();

#endif
