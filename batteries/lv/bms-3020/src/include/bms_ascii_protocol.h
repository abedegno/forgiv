/* I name the command and return codes of the UART4 ASCII protocol and its entry points.
   The command codes are the CID2 values that ascii_dispatch_command answers; each name says
   what the firmware does with the command, read from its case, not a published command name.
   The return codes are the values it writes into the reply's RTN field.
   Constants keep their original literal spellings; enum constants compile as int literals.
   name: descriptive. */
#ifndef BMS_ASCII_PROTOCOL_H
#define BMS_ASCII_PROTOCOL_H
#include "ghidra_types.h"
enum BmsAsciiCommand {
    /* Not received: uart4_protocol_task passes these when a frame fails a check, and the
       dispatcher answers with the same value as the return code. */
    ASCII_CMD_FRAME_CHECKSUM_ERROR = 0x02,
    ASCII_CMD_LENGTH_CHECKSUM_ERROR = 0x03,
    /* Readings and alarms of this pack, or of every pack when the address byte is 0xff. */
    ASCII_CMD_GET_ANALOG_VALUES = 0x42,
    ASCII_CMD_GET_ALARM_STATES = 0x44,
    ASCII_CMD_GET_PACK_COUNT = 0x90,
    /* Switch flags and output control. */
    ASCII_CMD_SET_SWITCH_FLAGS = 0x99,
    ASCII_CMD_CHARGE_OUTPUT = 0x9a,
    ASCII_CMD_DISCHARGE_OUTPUT = 0x9b,
    ASCII_CMD_NO_ACTION_9C = 0x9c,
    /* Fixed replies, capacities, references and calibration. */
    ASCII_CMD_GET_FIXED_A0 = 0xa0,
    ASCII_CMD_READ_EVENT_LOG = 0xa1,
    ASCII_CMD_SET_ADC_REFERENCE = 0xa2,
    ASCII_CMD_RESTORE_DEFAULT_LIMITS = 0xa4,
    ASCII_CMD_SET_RECOVERY_SOC = 0xa5,
    ASCII_CMD_GET_CAPACITIES = 0xa6,
    ASCII_CMD_SET_CAPACITIES = 0xa7,
    ASCII_CMD_NO_ACTION_A8 = 0xa8,
    ASCII_CMD_SET_CELL_ADC_REFERENCE = 0xa9,
    ASCII_CMD_CALIBRATE_NEGATIVE_CURRENT = 0xab,
    ASCII_CMD_CALIBRATE_POSITIVE_CURRENT = 0xac,
    ASCII_CMD_CALIBRATE_PACK_VOLTAGE = 0xad,
    ASCII_CMD_NO_ACTION_AE = 0xae,
    ASCII_CMD_GET_FIXED_AF = 0xaf,
    /* Clock, event log and cell count. */
    ASCII_CMD_GET_DATE = 0xb1,
    ASCII_CMD_SET_DATE = 0xb2,
    ASCII_CMD_CLEAR_EVENT_LOG = 0xb3,
    ASCII_CMD_SET_CELL_COUNT = 0xb4,
    ASCII_CMD_NO_ACTION_B5 = 0xb5,
    ASCII_CMD_GET_FIXED_B6 = 0xb6,
    ASCII_CMD_SET_LOG_INTERVAL = 0xb7,
    ASCII_CMD_GET_LOG_INTERVAL = 0xb8,
    ASCII_CMD_SET_LOG_START_DATE = 0xb9,
    ASCII_CMD_GET_LOG_START_DATE = 0xba,
    ASCII_CMD_SET_LOG_END_DATE = 0xbb,
    ASCII_CMD_GET_LOG_END_DATE = 0xbc,
    ASCII_CMD_SET_CYCLE_COUNT = 0xbd,
    /* Model code and firmware text. */
    ASCII_CMD_GET_SOFTWARE_VERSION = 0xc1,
    ASCII_CMD_GET_MODEL_CODE = 0xc2,
    ASCII_CMD_SET_MODEL_CODE = 0xc3,
    /* Threshold pairs: each set command is followed by the matching get command. */
    ASCII_CMD_SET_CELL_OVERVOLTAGE = 0xd0,
    ASCII_CMD_GET_CELL_OVERVOLTAGE = 0xd1,
    ASCII_CMD_SET_CELL_UNDERVOLTAGE = 0xd2,
    ASCII_CMD_GET_CELL_UNDERVOLTAGE = 0xd3,
    ASCII_CMD_SET_PACK_OVERVOLTAGE = 0xd4,
    ASCII_CMD_GET_PACK_OVERVOLTAGE = 0xd5,
    ASCII_CMD_SET_PACK_UNDERVOLTAGE = 0xd6,
    ASCII_CMD_GET_PACK_UNDERVOLTAGE = 0xd7,
    ASCII_CMD_SET_CHARGE_OVERCURRENT = 0xd8,
    ASCII_CMD_GET_CHARGE_OVERCURRENT = 0xd9,
    ASCII_CMD_SET_DISCHARGE_OVERCURRENT = 0xda,
    ASCII_CMD_GET_DISCHARGE_OVERCURRENT = 0xdb,
    ASCII_CMD_SET_OVERTEMPERATURE = 0xdc,
    ASCII_CMD_GET_OVERTEMPERATURE = 0xdd,
    ASCII_CMD_SET_UNDERTEMPERATURE = 0xde,
    ASCII_CMD_GET_UNDERTEMPERATURE = 0xdf,
    ASCII_CMD_SET_MOSFET_OVERTEMPERATURE = 0xe0,
    ASCII_CMD_GET_MOSFET_OVERTEMPERATURE = 0xe1,
    ASCII_CMD_SET_SHORT_CIRCUIT = 0xe2,
    ASCII_CMD_GET_SHORT_CIRCUIT = 0xe3,
    ASCII_CMD_SET_DELAY_STEPS = 0xe4,
    ASCII_CMD_GET_DELAY_STEPS = 0xe5,
    ASCII_CMD_SET_AMBIENT_TEMPERATURE = 0xe6,
    ASCII_CMD_GET_AMBIENT_TEMPERATURE = 0xe7,
    /* Service actions selected by the address byte. */
    ASCII_CMD_SERVICE = 0xff
};
enum BmsAsciiReturnCode {
    ASCII_RTN_NORMAL = 0,
    ASCII_RTN_FRAME_CHECKSUM_ERROR = 2,
    ASCII_RTN_LENGTH_CHECKSUM_ERROR = 3,
    ASCII_RTN_UNKNOWN_COMMAND = 4,
    ASCII_RTN_REJECTED = 9
};
#endif
