/* I name the correction-state values from their state transitions.
   match: character spellings and the upper special-state bound remain unchanged. */
#ifndef BMS_SOC_STATE_H
#define BMS_SOC_STATE_H
enum BmsSocCorrectionState {
    BMS_SOC_CORRECTION_IDLE = 0,
    BMS_SOC_CORRECTION_NEGATIVE_CURRENT = 0x10,
    BMS_SOC_CORRECTION_POSITIVE_CURRENT = ' ',
    BMS_SOC_CORRECTION_ACTIVE = '!',
    BMS_SOC_CORRECTION_SETTLED = '#',
    BMS_SOC_CORRECTION_BLOCKED = '$',
    BMS_SOC_CORRECTION_SPECIAL_LAST = 47
};
#endif
