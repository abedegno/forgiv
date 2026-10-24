/* SOC from open-circuit voltage, its ten-sample average, and the current-rate level.
   Matched as one source unit: 080267B8..08027804 text, 080309B8 const tables,
   one initialised byte at 200005CC and twenty zero-init bytes at 20004596. */
/* at: soc_lookup_voltage 0x080267B8, soc_update_voltage_average 0x0802753C, soc_classify_current_rate 0x080275CA */
/* at: soc_voltage_curve_0_1 0x080309B8, soc_voltage_curve_2 0x08030A5A, soc_voltage_curve_3 0x08030AFC, soc_voltage_curve_4 0x08030B9E */
/* at: soc_avg_index 0x200005CC, soc_avg_buf 0x20004596, soc_rate_count 0x200045A0, g_20000182 0x20000182, g_2000009F 0x2000009F */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow voltage/temperature lookup, trimmed averaging and current-rate classification here.
   This unit owns four voltage-curve tables, ten SOC samples and five duration counts.
   name: Curve variants follow the selector values; I do not infer cell chemistry.
   match: I retain the short interpolation scopes and four unread address comparisons. */
#include <stdlib.h>
#include "../../include/ghidra_types.h"

const ushort soc_voltage_curve_0_1[9][9] = {
    {0, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500},
    {5, 3125, 3127, 3131, 3113, 3064, 2978, 2852, 2681},
    {10, 3173, 3171, 3174, 3155, 3128, 3082, 2999, 2813},
    {15, 3194, 3193, 3196, 3180, 3160, 3131, 3075, 2918},
    {20, 3220, 3219, 3223, 3207, 3189, 3166, 3126, 2999},
    {25, 3236, 3236, 3240, 3224, 3208, 3188, 3157, 3059},
    {30, 3256, 3253, 3255, 3238, 3221, 3203, 3177, 3101},
    {35, 3265, 3261, 3262, 3246, 3230, 3213, 3190, 3131},
    {40, 3268, 3264, 3266, 3250, 3235, 3220, 3200, 3153},
};
const ushort soc_voltage_curve_2[9][9] = {
    {0, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500},
    {5, 3123, 3123, 3111, 3092, 3012, 2867, 2721, 2625},
    {10, 3167, 3164, 3153, 3142, 3103, 3018, 2853, 2724},
    {15, 3189, 3187, 3180, 3169, 3143, 3091, 2954, 2812},
    {20, 3215, 3213, 3207, 3196, 3174, 3133, 3031, 2890},
    {25, 3231, 3230, 3225, 3215, 3194, 3159, 3082, 2955},
    {30, 3250, 3245, 3239, 3228, 3207, 3176, 3116, 3007},
    {35, 3258, 3253, 3247, 3235, 3216, 3187, 3139, 3047},
    {40, 3262, 3257, 3252, 3240, 3222, 3195, 3156, 3077},
};
const ushort soc_voltage_curve_3[9][9] = {
    {0, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500},
    {5, 3110, 3109, 3072, 3072, 2921, 2832, 2686, 2625},
    {10, 3153, 3152, 3130, 3126, 3048, 2982, 2806, 2722},
    {15, 3176, 3175, 3161, 3154, 3107, 3065, 2903, 2806},
    {20, 3201, 3200, 3188, 3179, 3142, 3112, 2980, 2880},
    {25, 3218, 3217, 3205, 3197, 3165, 3141, 3037, 2942},
    {30, 3235, 3231, 3217, 3210, 3179, 3160, 3076, 2991},
    {35, 3243, 3239, 3226, 3218, 3190, 3173, 3104, 3028},
    {40, 3248, 3245, 3232, 3224, 3198, 3183, 3124, 3057},
};
const ushort soc_voltage_curve_4[9][9] = {
    {0, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500},
    {5, 3088, 3089, 2999, 3050, 2867, 2788, 2672, 2682},
    {10, 3135, 3134, 3097, 3109, 3006, 2935, 2782, 2881},
    {15, 3156, 3157, 3138, 3134, 3074, 3025, 2872, 2958},
    {20, 3182, 3182, 3166, 3161, 3114, 3079, 2945, 3004},
    {25, 3199, 3199, 3183, 3179, 3139, 3111, 3001, 3035},
    {30, 3214, 3213, 3195, 3191, 3155, 3133, 3042, 3057},
    {35, 3223, 3221, 3205, 3200, 3166, 3148, 3071, 3073},
    {40, 3229, 3228, 3212, 3207, 3175, 3159, 3092, 3085},
};

/* The lookup is the same column block 40 times, with a few spellings varied. The four
   unread pointer comparisons into diag emit no code: armcc deletes them after register
   allocation, but until then each keeps a table address live and so decides which of
   r3, r7 and ip the block's upper value, row index and previous entry take (case 3
   column 7, case 4 columns 3, 5 and 7). See evidence/matching/soc-context-review.md. */
/* Interpolates a selected voltage curve across temperature columns.
   match: The short interpolation temporaries and four address diagnostics affect allocation.
   I preserve their scopes, casts and expression grouping. */
uchar soc_lookup_voltage(ushort cell_voltage, ushort temperature, uchar curve_variant) {
    uchar row;
    uchar found_columns = 0;
    short column_soc[9] = {0, 40, 40, 40, 40, 40, 40, 40, 40};
    int alternate_voltage_slope;
    short soc_delta;
    int voltage_slope;
    uint address_diagnostic;
    if (curve_variant == 0) {
        for (row = 1; row < 9; row++) {
            if (((found_columns & 0x01) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][1])) {
                found_columns |= 0x01;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][1] - soc_voltage_curve_0_1[row - 1][1]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][1]) * 1000 / voltage_slope;
                    column_soc[1] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x02) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][2])) {
                found_columns |= 0x02;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][2] - soc_voltage_curve_0_1[row - 1][2]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][2]) * 1000 / voltage_slope;
                    column_soc[2] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x04) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][3])) {
                found_columns |= 0x04;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][3] - soc_voltage_curve_0_1[row - 1][3]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][3]) * 1000 / voltage_slope;
                    column_soc[3] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x08) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][4])) {
                found_columns |= 0x08;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][4] - soc_voltage_curve_0_1[row - 1][4]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][4]) * 1000 / voltage_slope;
                    column_soc[4] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x10) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][5])) {
                found_columns |= 0x10;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][5] - soc_voltage_curve_0_1[row - 1][5]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][5]) * 1000 / voltage_slope;
                    column_soc[5] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x20) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][6])) {
                found_columns |= 0x20;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][6] - soc_voltage_curve_0_1[row - 1][6]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][6]) * 1000 / voltage_slope;
                    column_soc[6] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x40) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][7])) {
                found_columns |= 0x40;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][7] - soc_voltage_curve_0_1[row - 1][7]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][7]) * 1000 / voltage_slope;
                    column_soc[7] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x80) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][8])) {
                found_columns |= 0x80;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][8] - soc_voltage_curve_0_1[row - 1][8]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][8]) * 1000 / voltage_slope;
                    column_soc[8] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
        }
    } else if (curve_variant == 1) {
        for (row = 1; row < 9; row++) {
            if (((found_columns & 0x01) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][1])) {
                found_columns |= 0x01;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][1] - soc_voltage_curve_0_1[row - 1][1]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][1]) * 1000 / voltage_slope;
                    column_soc[1] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x02) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][2])) {
                found_columns |= 0x02;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][2] - soc_voltage_curve_0_1[row - 1][2]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][2]) * 1000 / voltage_slope;
                    column_soc[2] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x04) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][3])) {
                found_columns |= 0x04;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][3] - soc_voltage_curve_0_1[row - 1][3]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][3]) * 1000 / voltage_slope;
                    column_soc[3] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x08) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][4])) {
                found_columns |= 0x08;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][4] - soc_voltage_curve_0_1[row - 1][4]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][4]) * 1000 / voltage_slope;
                    column_soc[4] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x10) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][5])) {
                found_columns |= 0x10;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][5] - soc_voltage_curve_0_1[row - 1][5]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][5]) * 1000 / voltage_slope;
                    column_soc[5] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x20) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][6])) {
                found_columns |= 0x20;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][6] - soc_voltage_curve_0_1[row - 1][6]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][6]) * 1000 / voltage_slope;
                    column_soc[6] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x40) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][7])) {
                found_columns |= 0x40;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][7] - soc_voltage_curve_0_1[row - 1][7]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][7]) * 1000 / voltage_slope;
                    column_soc[7] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x80) == 0) && (cell_voltage <= soc_voltage_curve_0_1[row][8])) {
                found_columns |= 0x80;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_0_1[row][8] - soc_voltage_curve_0_1[row - 1][8]) * 1000 /
                        5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_0_1[row - 1][8]) * 1000 / voltage_slope;
                    column_soc[8] = soc_voltage_curve_0_1[row - 1][0] + soc_offset;
                }
            }
        }
    } else if (curve_variant == 2) {
        for (row = 1; row < 9; row++) {
            if (((found_columns & 0x01) == 0) && (cell_voltage <= soc_voltage_curve_2[row][1])) {
                found_columns |= 0x01;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][1] - soc_voltage_curve_2[row - 1][1]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][1]) * 1000 / voltage_slope;
                    column_soc[1] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x02) == 0) && (cell_voltage <= soc_voltage_curve_2[row][2])) {
                found_columns |= 0x02;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][2] - soc_voltage_curve_2[row - 1][2]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][2]) * 1000 / voltage_slope;
                    column_soc[2] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x04) == 0) && (cell_voltage <= soc_voltage_curve_2[row][3])) {
                found_columns |= 0x04;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][3] - soc_voltage_curve_2[row - 1][3]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][3]) * 1000 / voltage_slope;
                    column_soc[3] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x08) == 0) && (cell_voltage <= soc_voltage_curve_2[row][4])) {
                found_columns |= 0x08;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][4] - soc_voltage_curve_2[row - 1][4]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][4]) * 1000 / voltage_slope;
                    column_soc[4] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x10) == 0) && (cell_voltage <= soc_voltage_curve_2[row][5])) {
                found_columns |= 0x10;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][5] - soc_voltage_curve_2[row - 1][5]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][5]) * 1000 / voltage_slope;
                    column_soc[5] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x20) == 0) && (cell_voltage <= soc_voltage_curve_2[row][6])) {
                found_columns |= 0x20;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][6] - soc_voltage_curve_2[row - 1][6]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][6]) * 1000 / voltage_slope;
                    column_soc[6] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x40) == 0) && (cell_voltage <= soc_voltage_curve_2[row][7])) {
                found_columns |= 0x40;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_2[row][7] - soc_voltage_curve_2[row - 1][7]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_2[row - 1][7]) * 1000 / voltage_slope;
                    column_soc[7] = soc_voltage_curve_2[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x80) == 0) && (cell_voltage <= soc_voltage_curve_2[row][8])) {
                found_columns |= 0x80;
                alternate_voltage_slope =
                    (soc_voltage_curve_2[row][8] - soc_voltage_curve_2[row - 1][8]) * 1000 / 5;
                column_soc[8] = soc_voltage_curve_2[row - 1][0] +
                                (cell_voltage - soc_voltage_curve_2[row - 1][8]) * 1000 /
                                    alternate_voltage_slope;
            }
        }
    } else if (curve_variant == 3) {
        for (row = 1; row < 9; row++) {
            if (((found_columns & 0x01) == 0) && (cell_voltage <= soc_voltage_curve_3[row][1])) {
                found_columns |= 0x01;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][1] - soc_voltage_curve_3[row - 1][1]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][1]) * 1000 / voltage_slope;
                    column_soc[1] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x02) == 0) && (cell_voltage <= soc_voltage_curve_3[row][2])) {
                found_columns |= 0x02;
                alternate_voltage_slope =
                    (soc_voltage_curve_3[row][2] - soc_voltage_curve_3[row - 1][2]) * 1000 / 5;
                column_soc[2] = soc_voltage_curve_3[row - 1][0] +
                                (cell_voltage - soc_voltage_curve_3[row - 1][2]) * 1000 /
                                    alternate_voltage_slope;
            }
            if (((found_columns & 0x04) == 0) && (cell_voltage <= soc_voltage_curve_3[row][3])) {
                found_columns |= 0x04;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][3] - soc_voltage_curve_3[row - 1][3]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][3]) * 1000 / voltage_slope;
                    column_soc[3] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x08) == 0) && (cell_voltage <= soc_voltage_curve_3[row][4])) {
                found_columns |= 0x08;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][4] - soc_voltage_curve_3[row - 1][4]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][4]) * 1000 / voltage_slope;
                    column_soc[4] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x10) == 0) && (cell_voltage <= soc_voltage_curve_3[row][5])) {
                found_columns |= 0x10;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][5] - soc_voltage_curve_3[row - 1][5]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][5]) * 1000 / voltage_slope;
                    column_soc[5] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x20) == 0) && (cell_voltage <= soc_voltage_curve_3[row][6])) {
                found_columns |= 0x20;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][6] - soc_voltage_curve_3[row - 1][6]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][6]) * 1000 / voltage_slope;
                    column_soc[6] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x40) == 0) && (cell_voltage <= soc_voltage_curve_3[row][7])) {
                address_diagnostic = (uint)(&soc_voltage_curve_3[row][7] != 0);
                found_columns |= 0x40;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][7] - soc_voltage_curve_3[row - 1][7]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][7]) * 1000 / voltage_slope;
                    column_soc[7] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x80) == 0) && (cell_voltage <= soc_voltage_curve_3[row][8])) {
                found_columns |= 0x80;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_3[row][8] - soc_voltage_curve_3[row - 1][8]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_3[row - 1][8]) * 1000 / voltage_slope;
                    column_soc[8] = soc_voltage_curve_3[row - 1][0] + soc_offset;
                }
            }
        }
    } else if (curve_variant == 4) {
        for (row = 1; row < 9; row++) {
            if (((found_columns & 0x01) == 0) && (cell_voltage <= soc_voltage_curve_4[row][1])) {
                found_columns |= 0x01;
                voltage_slope =
                    (soc_voltage_curve_4[row][1] - soc_voltage_curve_4[row - 1][1]) * 1000 / 5;
                soc_delta = (cell_voltage - soc_voltage_curve_4[row - 1][1]) * 1000 / voltage_slope;
                column_soc[1] = soc_voltage_curve_4[row - 1][0] + soc_delta;
            }
            if (((found_columns & 0x02) == 0) && (cell_voltage <= soc_voltage_curve_4[row][2])) {
                found_columns |= 0x02;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][2] - soc_voltage_curve_4[row - 1][2]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][2]) * 1000 / voltage_slope;
                    column_soc[2] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x04) == 0) && (cell_voltage <= soc_voltage_curve_4[row][3])) {
                found_columns |= 0x04;
                address_diagnostic = (uint)(soc_voltage_curve_4[row - 1] != 0);
                voltage_slope =
                    (soc_voltage_curve_4[row][3] - soc_voltage_curve_4[row - 1][3]) * 1000 / 5;
                column_soc[3] =
                    soc_voltage_curve_4[row - 1][0] +
                    (cell_voltage - soc_voltage_curve_4[row - 1][3]) * 1000 / voltage_slope;
            }
            if (((found_columns & 0x08) == 0) && (cell_voltage <= soc_voltage_curve_4[row][4])) {
                found_columns |= 0x08;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][4] - soc_voltage_curve_4[row - 1][4]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][4]) * 1000 / voltage_slope;
                    column_soc[4] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x10) == 0) && (cell_voltage <= soc_voltage_curve_4[row][5])) {
                address_diagnostic = (uint)(&soc_voltage_curve_4[row][5] != 0);
                found_columns |= 0x10;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][5] - soc_voltage_curve_4[row - 1][5]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][5]) * 1000 / voltage_slope;
                    column_soc[5] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x20) == 0) && (cell_voltage <= soc_voltage_curve_4[row][6])) {
                found_columns |= 0x20;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][6] - soc_voltage_curve_4[row - 1][6]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][6]) * 1000 / voltage_slope;
                    column_soc[6] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x40) == 0) && (cell_voltage <= soc_voltage_curve_4[row][7])) {
                address_diagnostic = (uint)(&soc_voltage_curve_4[row][7] != 0);
                found_columns |= 0x40;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][7] - soc_voltage_curve_4[row - 1][7]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][7]) * 1000 / voltage_slope;
                    column_soc[7] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
            if (((found_columns & 0x80) == 0) && (cell_voltage <= soc_voltage_curve_4[row][8])) {
                found_columns |= 0x80;
                {
                    short soc_offset;
                    voltage_slope =
                        (soc_voltage_curve_4[row][8] - soc_voltage_curve_4[row - 1][8]) * 1000 / 5;
                    soc_offset =
                        (cell_voltage - soc_voltage_curve_4[row - 1][8]) * 1000 / voltage_slope;
                    column_soc[8] = soc_voltage_curve_4[row - 1][0] + soc_offset;
                }
            }
        }
    }
    if (temperature >= 3180)
        column_soc[0] = column_soc[1];
    else if (temperature >= 3080)
        column_soc[0] =
            column_soc[1] + (3180 - temperature) * (short)(column_soc[2] - column_soc[1]) / 100;
    else if (temperature >= 2980)
        column_soc[0] =
            column_soc[2] + (3080 - temperature) * (short)(column_soc[3] - column_soc[2]) / 100;
    else if (temperature >= 2930)
        column_soc[0] =
            column_soc[3] + (2980 - temperature) * (short)(column_soc[4] - column_soc[3]) / 50;
    else if (temperature >= 2830)
        column_soc[0] =
            column_soc[4] + (2930 - temperature) * (short)(column_soc[5] - column_soc[4]) / 100;
    else if (temperature >= 2780)
        column_soc[0] =
            column_soc[5] + (2830 - temperature) * (short)(column_soc[6] - column_soc[5]) / 50;
    else if (temperature >= 2730)
        column_soc[0] =
            column_soc[6] + (2780 - temperature) * (short)(column_soc[7] - column_soc[6]) / 50;
    else if (temperature >= 2630)
        column_soc[0] =
            column_soc[7] + (2730 - temperature) * (short)(column_soc[8] - column_soc[7]) / 100;
    else
        column_soc[0] = column_soc[8];
    if (column_soc[0] < 1)
        column_soc[0] = 1;
    else if (column_soc[0] > 40)
        column_soc[0] = 40;
    return (uchar)column_soc[0];
}

uchar soc_avg_index = 0;
uchar soc_avg_buf[10];

/* Averages ten lookup samples after discarding one minimum and maximum.
   Zero samples postpone the result. Selector 0xff conditionally clears the buffer,
   returns zero and retains the ring index; I keep that behavior. */
uchar soc_update_voltage_average(ushort cell_voltage, ushort temperature, uchar curve_variant) {
    ushort sample_sum = 0;
    uchar sample_index = 0;
    uchar maximum_sample, minimum_sample;
    if (curve_variant == 0xff) {
        if (soc_avg_buf[0] != 0) {
            for (sample_index = 0; sample_index < 10; sample_index++)
                soc_avg_buf[sample_index] = 0;
        }
        return 0;
    }
    if (soc_avg_index < 10) {
        soc_avg_buf[soc_avg_index] = soc_lookup_voltage(cell_voltage, temperature, curve_variant);
        soc_avg_index++;
    } else {
        soc_avg_index = 0;
        soc_avg_buf[soc_avg_index] = soc_lookup_voltage(cell_voltage, temperature, curve_variant);
        soc_avg_index++;
    }
    maximum_sample = soc_avg_buf[0];
    minimum_sample = soc_avg_buf[0];
    for (sample_index = 0; sample_index < 10; sample_index++) {
        if (soc_avg_buf[sample_index] == 0)
            return 0;
        if (soc_avg_buf[sample_index] > maximum_sample)
            maximum_sample = soc_avg_buf[sample_index];
        if (soc_avg_buf[sample_index] < minimum_sample)
            minimum_sample = soc_avg_buf[sample_index];
        sample_sum += soc_avg_buf[sample_index];
    }
    sample_sum = sample_sum - maximum_sample - minimum_sample;
    sample_sum = sample_sum >> 3;
    if (sample_sum > 100)
        sample_sum = 100;
    return sample_sum;
}

extern ushort g_20000182;
extern uchar g_2000009F;
ushort soc_rate_count[5];
/* Classifies sustained absolute current relative to a capacity reference.
   Class 0xff also covers an unsustained rate; this is not a percentage result. */
uchar soc_classify_current_rate(ushort unused, int current) {
    int current_sample = current;
    uchar rate_class = 0xff;
    uint capacity_reference = g_20000182;
    uchar rate_index;
    if (g_2000009F == 2) {

        if (abs(current_sample) < capacity_reference * 5 / 10) {
            if (soc_rate_count[0] <= 300)
                soc_rate_count[0]++;
            for (rate_index = 1; rate_index < 5; rate_index++) {
                if (soc_rate_count[rate_index] > 5)
                    soc_rate_count[rate_index] -= 5;
                else
                    soc_rate_count[rate_index] = 0;
            }
        } else if (abs(current_sample) < capacity_reference * 15 / 10) {
            if (soc_rate_count[0] <= 300)
                soc_rate_count[0]++;
            if (soc_rate_count[1] <= 300)
                soc_rate_count[1]++;
            for (rate_index = 2; rate_index < 5; rate_index++) {
                if (soc_rate_count[rate_index] > 5)
                    soc_rate_count[rate_index] -= 5;
                else
                    soc_rate_count[rate_index] = 0;
            }
        } else if (abs(current_sample) < capacity_reference * 28 / 10) {
            if (soc_rate_count[0] <= 300)
                soc_rate_count[0]++;
            if (soc_rate_count[1] <= 300)
                soc_rate_count[1]++;
            if (soc_rate_count[2] <= 300)
                soc_rate_count[2]++;
            for (rate_index = 3; rate_index < 5; rate_index++) {
                if (soc_rate_count[rate_index] > 5)
                    soc_rate_count[rate_index] -= 5;
                else
                    soc_rate_count[rate_index] = 0;
            }
        } else if (abs(current_sample) < capacity_reference * 43 / 10) {
            if (soc_rate_count[0] <= 300)
                soc_rate_count[0]++;
            if (soc_rate_count[1] <= 300)
                soc_rate_count[1]++;
            if (soc_rate_count[2] <= 300)
                soc_rate_count[2]++;
            if (soc_rate_count[3] <= 300)
                soc_rate_count[3]++;
            for (rate_index = 4; rate_index < 5; rate_index++) {
                if (soc_rate_count[rate_index] > 5)
                    soc_rate_count[rate_index] -= 5;
                else
                    soc_rate_count[rate_index] = 0;
            }
        } else if (abs(current_sample) < capacity_reference * 55 / 10) {
            if (soc_rate_count[0] <= 300)
                soc_rate_count[0]++;
            if (soc_rate_count[1] <= 300)
                soc_rate_count[1]++;
            if (soc_rate_count[2] <= 300)
                soc_rate_count[2]++;
            if (soc_rate_count[3] <= 300)
                soc_rate_count[3]++;
            if (soc_rate_count[4] <= 300)
                soc_rate_count[4]++;

        } else {
            soc_rate_count[0] = 0;
            soc_rate_count[1] = 0;
            soc_rate_count[2] = 0;
            soc_rate_count[3] = 0;
            soc_rate_count[4] = 0;
        }
        if (soc_rate_count[4] >= 300)
            rate_class = 4;
        else if (soc_rate_count[3] >= 300)
            rate_class = 3;
        else if (soc_rate_count[2] >= 300)
            rate_class = 2;
        else if (soc_rate_count[1] >= 300)
            rate_class = 1;
        else if (soc_rate_count[0] >= 300)
            rate_class = 0xff;
        else
            rate_class = 0xff;
    } else {
        soc_rate_count[0] = 0;
        soc_rate_count[1] = 0;
        soc_rate_count[2] = 0;
        soc_rate_count[3] = 0;
        soc_rate_count[4] = 0;
    }
    return rate_class;
}
