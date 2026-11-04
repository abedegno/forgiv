/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* at: uart4_modbus_read_peer_registers 0x0801D57C, uart4_modbus_write_peer_setting 0x0801D75E, uart4_receive_byte 0x0801D83C, ascii_append_analog_values 0x0801DA18, ascii_append_alarm_states 0x0801DC92, ascii_dispatch_command 0x0801E0C8, uart4_transmit_next_byte 0x08020EAA, peer_record_publish_own 0x08020F98, pending_settings_apply_own 0x080210FC, uart4_protocol_task 0x080213C4 */
/* at: uart4_frame 0x20003602, modbus_tx_frame 0x200036AC, modbus_reply 0x20003743, modbus_register_image 0x200037DA */
/* at: uart4_frame_ready 0x2000051A, ascii_multi_pack_cid2 0x2000051B, modbus_peer_index 0x2000051C, uart4_rx_field_count 0x2000051D, uart4_rx_index 0x2000051E, uart4_rx_state 0x2000051F, ascii_tx_pack_index 0x20000520, modbus_reply_length 0x20000521, modbus_poll_delay 0x20000522 */
/* at: uart4_tx_length 0x20000524, uart4_tx_index 0x20000526, ascii_rx_info_length 0x20000528, ascii_tx_checksum 0x2000052A, modbus_rx_crc 0x2000052C, uart4_ascii_mode 0x200000DE, hex_pair_buffer 0x2000010D, protocol_pack 0x20003D6A, protocol_pending 0x20004161, snapshots 0x200014EA */
/* at: limit_thresholds 0x2000153A, protection_inhibit_flags 0x2000027D, measurement_current_mode 0x2000009F, pack_count 0x2000009E, pack_address 0x20000167 */
/* at: system_soc 0x20000184, system_rated_capacity 0x20000180, measurement_rated_capacity 0x20000186, full_capacity 0x20000168, remaining_capacity 0x20000170, measurement_cell_count 0x20000166 */
/* at: model_code 0x20000077, software_version_text 0x20000062, cycle_count 0x200000A2, auxiliary_voltage_reading 0x20000128, measurement_pack_voltage 0x20000114, measurement_current 0x2000014C */
/* at: primary_soc 0x20000188, displayed_soc 0x20000189, voltage_current_protection_flags 0x20000279, temperature_protection_flags 0x2000027A, power_output_status_flags 0x2000027B, current_direction_latch_flags 0x20000519 */
/* at: hardware_fault_flags 0x2000027C, voltage_current_limit_flags 0x2000027F, temperature_limit_flags 0x20000280, measurement_maximum_cell_voltage 0x20000110, measurement_minimum_cell_voltage 0x20000112 */
/* at: compensated_cell_voltages 0x200010AC, measurement_maximum_temperature 0x2000012E, measurement_minimum_temperature 0x20000130, temperature_readings 0x200011B6, usb_active_class 0x20000051, charge_current_limit 0x20000134 */
/* at: discharge_current_limit 0x20000138, capacity_operating_status 0x200000A0, lower_voltage_current_hold 0x2000013C, upper_voltage_current_hold 0x2000013D, charge_phase_toggle 0x20000098, soc_voltage_hold 0x200000AD */
/* at: protection_thresholds 0x2000156A, recovery_thresholds 0x20001554, protection_debounce_settings 0x20000304, capacity_latch_flags 0x2000027E, current_adc_options 0x200000FC, model_current_class 0x200000FA, cycle_integrated_capacity 0x2000016C, cycle_count_flags 0x200000A9 */
/* at: recovery_soc_reference 0x2000018A, cell_adc_reference_scale 0x20000096, adc_reference_scale 0x20000094, pack_voltage_scale 0x20000158, negative_current_scale 0x20000150, positive_current_scale 0x20000154, log_index 0x2000018C */
/* at: event_log_reserved_word 0x2000018E, date_now 0x20000190, date_start 0x200000B6, date_end 0x200000BC, log_interval 0x200000B4, afe_balance_mask 0x200000CE, os_task_dly_idx 0x2000061E */
/* I follow the UART4 link here: an ASCII command protocol and a small register protocol on
   the same port. ASCII frames start with '~' and end with a carriage return; between them
   every field is hex digits: version 25, address, device type 46, command code, a length
   field with its own checksum nibble, the INFO characters and a four-digit checksum. This
   unit answers them for this pack, and for every pack when the address is 0xFF. Frames that
   start with 0xBB carry a Modbus-style read or write of the peer records, with a CRC.
   Entry points: uart4_receive_byte and uart4_transmit_next_byte (UART4 interrupt) and
   uart4_protocol_task (scheduler). The unit owns the UART4 frame buffer, the Modbus reply
   buffers and the receive and transmit state; it reads snapshots, peer records, thresholds
   and readings, and saves settings through the EEPROM routines.
   Neighbours: serial.c (hex digits, frame checks, serial_send), register_protocol.c (peer
   records), calibration.c and settings_core.c (snapshots, limit banks), eeprom_log.c.
   The dispatcher is the old firmware line's source compiled at -O1; the README says how it
   was matched.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/bms_ascii_protocol.h"
#include "../../include/bms_dates.h"
#include "../../include/bms_measurements.h"
#include "../../include/bms_protection.h"
#include "../../include/bms_protocol.h"
#include "../../include/bms_settings_constants.h"
#include "../../include/bms_thresholds.h"
#include "../../include/tasks.h"

/* One 80-byte measurement snapshot per pack, laid out as snapshot_publish fills it. */
typedef struct {
    uchar warning_present; /* 1 while either limit flag byte is non-zero */
    uchar cell_count;
    ushort cell_voltages[16];
    uchar temperature_count;
    uchar reserved23;
    ushort temperatures[10];
    short current; /* from measurement_current divided by ten (snapshot_publish) */
    ushort pack_voltage;
    ushort remaining_capacity;
    uchar user_defined_count; /* always 3 */
    uchar reserved3f;
    ushort full_capacity;
    ushort cycle_count;
    ushort rated_capacity;
    uchar status_flags[9]; /* the nine flag bytes, as in peer records */
    uchar reserved4f;
} PackSnapshot;

/* This unit's view of the 145-byte peer record (BmsPeerRecord in bms_peer_types.h), with
   the same field names. The flag bytes and temperatures are arrays here because this unit
   loops over them, and the identification is split into its two 21-byte texts.
   match: full_capacity and remaining_capacity are int here (uint in the shared record);
   the unsigned view changes the float conversions that fill them. */
typedef __packed struct {
    ushort reply_seen;
    uchar model_code[21];
    uchar software_version_text[21];
    uchar cell_count;
    ushort cycle_count;
    int auxiliary_voltage;
    ushort pack_voltage;
    int pack_current;
    int full_capacity;
    uint rated_capacity;
    int remaining_capacity;
    uchar soc_primary;
    uchar status_flags[9];
    ushort maximum_cell_voltage;
    ushort minimum_cell_voltage;
    ushort cell_voltages[16];
    ushort maximum_temperature;
    ushort minimum_temperature;
    ushort firmware_version;
    ushort temperatures[5];
    uchar usb_class;
    uchar reserved_132[4];
    uchar soc_secondary;
    ushort charge_current_limit;
    ushort discharge_current_limit;
    uchar participation_state;
    uchar voltage_hold_mode;
    uchar output_phase_toggle;
    uchar soc_voltage_hold;
} PeerRecordView;

/* Settings written to a pack over Modbus, waiting for that pack to apply them.
   Flag bits 0..3 mark the rated capacity, full capacity, remaining capacity and cell count. */
typedef __packed struct {
    uint flags;
    uchar reserved4[4];
    ushort rated_capacity;
    ushort full_capacity;
    ushort remaining_capacity;
    ushort cell_count;
    uchar reserved16[98];
} PendingSettings;

/* This unit's views of the limit and protection thresholds (bms_thresholds.h).
   match: the charge-current fields are unsigned here; the shared layouts' signed fields
   change the loads and shifts that read them. */
typedef struct {
    ushort cell_overvoltage;
    ushort cell_undervoltage;
    ushort pack_overvoltage;
    ushort pack_undervoltage;
    ushort charge_overcurrent;
    short discharge_overcurrent;
    ushort charge_overtemperature;
    ushort discharge_overtemperature;
    ushort charge_undertemperature;
    ushort discharge_undertemperature;
    ushort mosfet_overtemperature;
    ushort ambient_overtemperature;
    ushort ambient_undertemperature;
} LimitThresholdView;
typedef struct {
    ushort cell_overvoltage;
    ushort cell_undervoltage;
    ushort pack_overvoltage;
    ushort pack_undervoltage;
    ushort charge_overcurrent;
    short discharge_overcurrent;
    ushort short_circuit_current;
    ushort charge_overtemperature;
    ushort discharge_overtemperature;
    ushort charge_undertemperature;
    ushort discharge_undertemperature;
    ushort mosfet_overtemperature;
    ushort ambient_overtemperature;
    ushort ambient_undertemperature;
} ProtectionThresholdView;

/* Peer records and pending settings (register_protocol.c), snapshots and thresholds. */
extern PeerRecordView protocol_pack[];
extern PendingSettings protocol_pending[];
extern PackSnapshot snapshots[];
extern LimitThresholdView limit_thresholds;
extern ProtectionThresholdView protection_thresholds;
extern BmsRecoveryThresholds recovery_thresholds;
extern uchar protection_debounce_settings[];

/* Protection and status flag bytes not in the shared headers (measurement_protection.c and
   control_tasks.c define them). */
extern volatile uchar protection_inhibit_flags;
extern volatile uchar capacity_latch_flags;
extern volatile uchar power_output_status_flags, hardware_fault_flags,
    current_direction_latch_flags;

/* Identity, link and configuration. */
extern uchar uart4_ascii_mode;
extern uchar hex_pair_buffer[2];
extern uchar pack_address;
extern uchar pack_count;
extern uchar model_code[], software_version_text[];
extern ushort current_adc_options, model_current_class;
extern ushort afe_balance_mask;
extern uchar usb_active_class;

/* Capacity, state of charge and limits. */
extern ushort measurement_rated_capacity;
extern float full_capacity, remaining_capacity;
extern float system_rated_capacity;
extern ushort system_soc;
extern uchar primary_soc, displayed_soc;
extern ushort cycle_count;
extern float cycle_integrated_capacity;
extern uchar cycle_count_flags, recovery_soc_reference;
extern uchar measurement_current_mode;
extern uint charge_current_limit, discharge_current_limit;
extern uchar capacity_operating_status, lower_voltage_current_hold, upper_voltage_current_hold,
    charge_phase_toggle, soc_voltage_hold;

/* Readings and calibration scales. */
extern uint auxiliary_voltage_reading;
extern ushort compensated_cell_voltages[], temperature_readings[];
extern ushort cell_adc_reference_scale, adc_reference_scale;
extern float pack_voltage_scale, negative_current_scale, positive_current_scale;

/* Event log. */
extern ushort log_index, event_log_reserved_word;
extern uchar log_interval;

/* Window watchdog control register: the service command writes it to force a reset. */
#define WWDG_CR (*(volatile uint *)0x40002C00)

/* UART4 link state, defined in this order. uart4_frame holds a received frame and then the
   ASCII reply built in its place; the Modbus path builds its reply in modbus_reply and sends
   a copy from modbus_tx_frame. */
uchar uart4_frame_ready = 0;
uchar ascii_multi_pack_cid2 = 0;
uchar modbus_peer_index = 0;
uchar uart4_rx_field_count = 0;
uchar uart4_rx_index = 0;
uchar uart4_rx_state = 1;
uchar ascii_tx_pack_index = 0;
uchar modbus_reply_length = 0;
uchar modbus_poll_delay = 0;
ushort uart4_tx_length = 0;
ushort uart4_tx_index = 0;
ushort ascii_rx_info_length = 0;
ushort ascii_tx_checksum = 0;
ushort modbus_rx_crc = 0;
uchar uart4_frame[170];
uchar modbus_tx_frame[151];
uchar modbus_reply[151];
uchar modbus_register_image[130];

/* Serial helpers from serial.c, declared as this unit calls them.
   match: serial.c defines them with int parameters and int results. Declaring the hex
   decoder, the hex encoder or the length writer that way changes this unit's code (each
   tested alone), so these caller views stay local. */
uchar ascii_hex_to_byte(uchar hi, uchar lo);
void ascii_byte_to_hex(uchar value, uchar *out);
int ascii_length_checksum_valid(uchar *frame);
int ascii_frame_checksum_valid(uchar *frame);
void ascii_set_length_field(uchar *frame, ushort length);
void ascii_set_rtn_field(uchar *frame, uchar rtn);
void serial_send(int port, uchar *data, ushort length);

/* Date, EEPROM and settings routines, declared as this unit calls them.
   match: byte_to_bcd returns a wide value here although runtime.c returns uchar; a uchar
   result changes the date cases' code. The word writer returns int, because the cell-count
   command tests its result, and takes a halfword value: the definition's uint value
   changes the code, and the shared bms_eeprom.h view returns void. */
uint byte_to_bcd(uchar value);
uint bcd_to_byte(uint value);
void runtime_publish_date(void);
int eeprom_read_byte(uint addr);
int eeprom_write_value32(ushort addr, float value);
int eeprom_write_word(ushort addr, ushort value);
void settings_clear_counters(void);
void settings_restore_factory_limits(void);
void settings_save_limit_bank(int mode);

/* StdPeriph USART calls, declared without the library header (this unit has no
   StdPeriph include path). */
int USART_GetITStatus(void *usart, ushort it);
void USART_ITConfig(void *usart, ushort it, int state);
#define UART4 ((void *)0x40004C00)
#define UART4_IT_TXE 0x727 /* StdPeriph USART_IT_TXE */
#define UART4_DR (*(volatile ushort *)0x40004C04)

/* match: cid2 is a uchar; an int parameter changes the dispatcher's code. */
void ascii_dispatch_command(uchar cid2);

/* Answers a register read (function 3) of the peer record chosen by the request address.
   The record is laid out as 60 big-endian registers in modbus_register_image: model code and
   version text, cell count, cycle count, auxiliary voltage, pack voltage and current, the
   capacities, SOC, the nine flag bytes, system SOC, cell voltages, system rated capacity and
   the cell voltage extremes. The requested registers follow a two-byte byte count that the
   task fills in. A count of 0 or over 60, or a range that crosses a 60-register boundary,
   gets exception code 0x83. A start register of 60 or more passes those checks and leaves
   reply_index unset before the CRC is placed. */
void uart4_modbus_read_peer_registers(uchar *frame) {
    uchar error = 0;
    ushort first_register, register_count, i, reply_index;
    ushort crc;
    PeerRecordView *peer;
    uint value;

    first_register = frame[3];
    first_register = first_register << 8 | frame[4];
    register_count = frame[5];
    register_count = register_count << 8 | frame[6];
    if (register_count > 60 || register_count == 0)
        error = 1;
    if (first_register + register_count > (first_register / 60 + 1) * 60)
        error = 1;
    if (error == 1) {
        modbus_reply[1] = 0x83;
        crc = protocol_crc16(modbus_reply, 16);
        modbus_reply[2] = crc;
        modbus_reply[3] = crc >> 8;
        return;
    }
    for (i = 0; i < 120; i++)
        modbus_register_image[i] = 0;
    if (first_register < 60) {
        for (i = 0; i < 21; i++)
            modbus_register_image[i] = protocol_pack[modbus_peer_index].model_code[i];
        for (i = 0; i < 21; i++)
            modbus_register_image[i + 21] =
                protocol_pack[modbus_peer_index].software_version_text[i];
        peer = &protocol_pack[modbus_peer_index];
        modbus_register_image[42] = peer->cell_count;
        value = peer->cycle_count;
        modbus_register_image[43] = value >> 8;
        modbus_register_image[44] = value;
        value = peer->auxiliary_voltage;
        modbus_register_image[45] = value >> 24;
        modbus_register_image[46] = value >> 16;
        modbus_register_image[47] = value >> 8;
        modbus_register_image[48] = value;
        value = peer->pack_voltage;
        modbus_register_image[49] = value >> 8;
        modbus_register_image[50] = value;
        value = peer->pack_current;
        modbus_register_image[51] = value >> 24;
        modbus_register_image[52] = value >> 16;
        modbus_register_image[53] = value >> 8;
        modbus_register_image[54] = value;
        value = peer->full_capacity;
        modbus_register_image[55] = value >> 24;
        modbus_register_image[56] = value >> 16;
        modbus_register_image[57] = value >> 8;
        modbus_register_image[58] = value;
        value = peer->rated_capacity;
        modbus_register_image[59] = value >> 24;
        modbus_register_image[60] = value >> 16;
        modbus_register_image[61] = value >> 8;
        modbus_register_image[62] = value;
        value = peer->remaining_capacity;
        modbus_register_image[63] = value >> 24;
        modbus_register_image[64] = value >> 16;
        modbus_register_image[65] = value >> 8;
        modbus_register_image[66] = value;
        modbus_register_image[67] = peer->soc_secondary;
        modbus_register_image[68] = peer->status_flags[0];
        modbus_register_image[69] = peer->status_flags[1];
        modbus_register_image[70] = peer->status_flags[2];
        modbus_register_image[71] = peer->status_flags[3];
        modbus_register_image[72] = peer->status_flags[4];
        modbus_register_image[73] = peer->status_flags[5];
        modbus_register_image[74] = peer->status_flags[6];
        modbus_register_image[75] = peer->status_flags[7];
        modbus_register_image[76] = peer->status_flags[8];
        modbus_register_image[77] = peer->reply_seen;
        modbus_register_image[78] = system_soc;
        for (i = 0; i < 16; i++) {
            modbus_register_image[79 + 2 * i] = (value = peer->cell_voltages[i]) >> 8;
            modbus_register_image[80 + 2 * i] = value;
        }
        value = (uint)system_rated_capacity;
        modbus_register_image[111] = value >> 24;
        modbus_register_image[112] = value >> 16;
        modbus_register_image[113] = value >> 8;
        modbus_register_image[114] = value;
        value = peer->maximum_cell_voltage;
        modbus_register_image[115] = value >> 8;
        modbus_register_image[116] = value;
        value = peer->minimum_cell_voltage;
        modbus_register_image[117] = value >> 8;
        modbus_register_image[118] = value;
        modbus_register_image[119] = 0;
        for (i = first_register * 2, reply_index = 4; i < first_register * 2 + register_count * 2;
             i++, reply_index++)
            modbus_reply[reply_index] = modbus_register_image[i];
    }
    crc = protocol_crc16(modbus_reply, reply_index);
    modbus_reply[reply_index] = crc;
    modbus_reply[reply_index + 1] = crc >> 8;
}

/* Answers a register write (function 6) for the peer at the request address (capped at 15).
   Registers 0 to 2 set a pending rated, full or remaining capacity below 60000, register 3 a
   cell count of 8 to 16, and register 4 is accepted without effect; the peer applies them in
   pending_settings_apply_own. A rejected write sets exception code 0x86. The CRC goes to
   bytes 16 and 17, beyond the eight bytes the task sends. */
void uart4_modbus_write_peer_setting(uchar *frame) {
    uchar peer_index;
    uchar error = 0;
    ushort register_address, value;
    ushort crc;

    peer_index = frame[1];
    if (peer_index > 15)
        peer_index = 15;
    register_address = frame[3];
    register_address = register_address << 8 | frame[4];
    value = frame[5];
    value = value << 8 | frame[6];
    switch (register_address) {
    case 0:
        if (value < 60000) {
            protocol_pending[peer_index].rated_capacity = value;
            protocol_pending[peer_index].flags |= 1;
        } else
            error = 1;
        break;
    case 1:
        if (value < 60000) {
            protocol_pending[peer_index].full_capacity = value;
            protocol_pending[peer_index].flags |= 2;
        } else
            error = 1;
        break;
    case 2:
        if (value < 60000) {
            protocol_pending[peer_index].remaining_capacity = value;
            protocol_pending[peer_index].flags |= 4;
        } else
            error = 1;
        break;
    case 3:
        if (value >= 8 && value <= 16) {
            protocol_pending[peer_index].cell_count = value;
            protocol_pending[peer_index].flags |= 8;
        } else
            error = 1;
        break;
    case 4:
        break;
    default:
        error = 1;
        break;
    }
    if (error) {
        modbus_reply[1] = 0x86;
        crc = protocol_crc16(modbus_reply, 16);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
    } else {
        crc = protocol_crc16(modbus_reply, 16);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
    }
}

/* UART4 receive interrupt step. A '~' starts an ASCII frame and selects ASCII mode: two
   characters each for version, address, device type and command, the four-character length
   field, the INFO characters it announces, the four checksum characters and a carriage
   return, which marks the frame ready unless a reply is still being sent. In the other mode
   a 0xBB byte starts a register frame: address, function, start register, count and CRC; it
   is ready after its ninth byte, and bytes are dropped while a frame waits. */
void uart4_receive_byte(uchar received) {
    if (received == 0x7e || uart4_ascii_mode == 1) {
        uart4_ascii_mode = 1;
        if (received == 0x7e)
            uart4_rx_state = 1;
        switch (uart4_rx_state) {
        case 1:
            if (received == 0x7e) {
                uart4_rx_field_count = 0;
                uart4_rx_index = 1;
                uart4_frame[0] = received;
                uart4_rx_state = 2;
            }
            break;
        case 2:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 2) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 3;
            }
            break;
        case 3:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 2) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 4;
            }
            break;
        case 4:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 2) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 5;
            }
            break;
        case 5:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 2) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 6;
            }
            break;
        case 6:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 4) {
                uart4_rx_field_count = 0;
                if (uart4_frame[9] == '0' && uart4_frame[10] == '0' && uart4_frame[11] == '0' &&
                    uart4_frame[12] == '0')
                    uart4_rx_state = 8;
                else
                    uart4_rx_state = 7;
                ascii_rx_info_length = ascii_hex_to_byte(uart4_frame[11], uart4_frame[12]);
            }
            break;
        case 7:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= ascii_rx_info_length) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 8;
            }
            break;
        case 8:
            uart4_frame[uart4_rx_index++] = received;
            uart4_rx_field_count++;
            if (uart4_rx_field_count >= 4) {
                uart4_rx_field_count = 0;
                uart4_rx_state = 9;
            }
            break;
        case 9:
            if (received == 0x0d) {
                uart4_frame[uart4_rx_index] = received;
                if (uart4_tx_index == 0)
                    uart4_frame_ready = 1;
            }
            uart4_rx_state = 1;
            break;
        }
    } else {
        uart4_ascii_mode = 0;
        if (uart4_frame_ready == 1) {
            uart4_rx_field_count = 0;
            uart4_rx_index = 0;
        } else {
            if (received == 0xbb)
                uart4_rx_state = 1;
            switch (uart4_rx_state) {
            case 1:
                if (received == 0xbb) {
                    uart4_rx_field_count = 0;
                    uart4_rx_index = 1;
                    uart4_frame[0] = received;
                    uart4_rx_state = 2;
                }
                break;
            case 2:
                uart4_frame[uart4_rx_index++] = received;
                uart4_rx_field_count = 0;
                uart4_rx_state = 3;
                break;
            case 3:
                uart4_frame[uart4_rx_index++] = received;
                uart4_rx_field_count = 0;
                uart4_rx_state = 4;
                break;
            case 4:
                uart4_frame[uart4_rx_index++] = received;
                uart4_rx_field_count++;
                if (uart4_rx_field_count >= 2) {
                    uart4_rx_field_count = 0;
                    uart4_rx_state = 5;
                }
                break;
            case 5:
                uart4_frame[uart4_rx_index++] = received;
                uart4_rx_field_count++;
                if (uart4_rx_field_count >= 2) {
                    uart4_rx_field_count = 0;
                    uart4_rx_state = 6;
                }
                break;
            case 6:
                uart4_frame[uart4_rx_index++] = received;
                uart4_rx_field_count++;
                if (uart4_rx_field_count >= 2) {
                    uart4_rx_field_count = 0;
                    uart4_frame_ready = 1;
                }
                break;
            }
        }
    }
}

/* Appends the two digits left in hex_pair_buffer by ascii_byte_to_hex to the frame at *pos.
   match: this and the other reply macros keep the expansions the dispatcher was matched
   with. A do-while form of ASCII_PUT_HEX was tested at -O1 and gives the same bytes
   (levers-o1.tsv); no other form was tested. */
#define ASCII_APPEND_HEX_PAIR(pos)                                                                 \
    do {                                                                                           \
        uart4_frame[(*pos)++] = hex_pair_buffer[0];                                                \
        uart4_frame[(*pos)++] = hex_pair_buffer[1];                                                \
    } while (0)

/* Appends one pack's analog block from its snapshot as hex digits: cell count and voltages,
   temperature count and temperatures, current, pack voltage, remaining capacity, the
   user-defined count (3), full capacity, cycle count and rated capacity. */
void ascii_append_analog_values(uchar pack_index, ushort *position) {
    uchar i;

    ascii_byte_to_hex(snapshots[pack_index].cell_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    for (i = 0; i < snapshots[pack_index].cell_count; i++) {
        ascii_byte_to_hex(snapshots[pack_index].cell_voltages[i] >> 8, hex_pair_buffer);
        ASCII_APPEND_HEX_PAIR(position);
        ascii_byte_to_hex(snapshots[pack_index].cell_voltages[i], hex_pair_buffer);
        ASCII_APPEND_HEX_PAIR(position);
    }
    ascii_byte_to_hex(snapshots[pack_index].temperature_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    for (i = 0; i < snapshots[pack_index].temperature_count; i++) {
        ascii_byte_to_hex(snapshots[pack_index].temperatures[i] >> 8, hex_pair_buffer);
        ASCII_APPEND_HEX_PAIR(position);
        ascii_byte_to_hex(snapshots[pack_index].temperatures[i], hex_pair_buffer);
        ASCII_APPEND_HEX_PAIR(position);
    }
    ascii_byte_to_hex(snapshots[pack_index].current >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].current, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].pack_voltage >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].pack_voltage, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].remaining_capacity >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].remaining_capacity, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].user_defined_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].full_capacity >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].full_capacity, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].cycle_count >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].cycle_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].rated_capacity >> 8, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].rated_capacity, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
}

/* Appends two characters to the frame at *pos: an alarm state "00", "01" or "02". */
#define ASCII_APPEND_PAIR(pos, a, b)                                                               \
    do {                                                                                           \
        uart4_frame[(*pos)++] = (a);                                                               \
        uart4_frame[(*pos)++] = (b);                                                               \
    } while (0)

/* Appends one pack's alarm block: a state per cell and per temperature ("00" normal, "01"
   below the low limit, "02" above the high limit), then the MOSFET and ambient temperatures
   (the last two channels), charge current, pack voltage and discharge current, then the
   nine status bytes. Inhibited alarms read as normal. The cell temperatures use the charge
   limits when measurement_current_mode is 1 and the discharge limits otherwise. */
void ascii_append_alarm_states(uchar pack_index, ushort *position) {
    uchar i;

    ascii_byte_to_hex(snapshots[pack_index].cell_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    for (i = 0; i < snapshots[pack_index].cell_count; i++) {
        if (snapshots[pack_index].cell_voltages[i] > limit_thresholds.cell_overvoltage &&
            !(protection_inhibit_flags & 1))
            ASCII_APPEND_PAIR(position, '0', '2');
        else if (snapshots[pack_index].cell_voltages[i] < limit_thresholds.cell_undervoltage &&
                 !(protection_inhibit_flags & 2))
            ASCII_APPEND_PAIR(position, '0', '1');
        else
            ASCII_APPEND_PAIR(position, '0', '0');
    }
    ascii_byte_to_hex(snapshots[pack_index].temperature_count, hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    for (i = 0; i < snapshots[pack_index].temperature_count - 2; i++) {
        if (measurement_current_mode == 1) {
            if (snapshots[pack_index].temperatures[i] > limit_thresholds.charge_overtemperature &&
                !(protection_inhibit_flags & 0x40))
                ASCII_APPEND_PAIR(position, '0', '2');
            else if (snapshots[pack_index].temperatures[i] <
                         limit_thresholds.charge_undertemperature &&
                     !(protection_inhibit_flags & 0x80))
                ASCII_APPEND_PAIR(position, '0', '1');
            else
                ASCII_APPEND_PAIR(position, '0', '0');
        } else {
            if (snapshots[pack_index].temperatures[i] >
                    limit_thresholds.discharge_overtemperature &&
                !(protection_inhibit_flags & 0x40))
                ASCII_APPEND_PAIR(position, '0', '2');
            else if (snapshots[pack_index].temperatures[i] <
                         limit_thresholds.discharge_undertemperature &&
                     !(protection_inhibit_flags & 0x80))
                ASCII_APPEND_PAIR(position, '0', '1');
            else
                ASCII_APPEND_PAIR(position, '0', '0');
        }
    }
    if (snapshots[pack_index].temperatures[snapshots[pack_index].temperature_count - 2] >
        limit_thresholds.mosfet_overtemperature)
        ASCII_APPEND_PAIR(position, '0', '2');
    else
        ASCII_APPEND_PAIR(position, '0', '0');
    if (snapshots[pack_index].temperatures[snapshots[pack_index].temperature_count - 1] >
        limit_thresholds.ambient_overtemperature)
        ASCII_APPEND_PAIR(position, '0', '2');
    else if (snapshots[pack_index].temperatures[snapshots[pack_index].temperature_count - 1] <
             limit_thresholds.ambient_undertemperature)
        ASCII_APPEND_PAIR(position, '0', '1');
    else
        ASCII_APPEND_PAIR(position, '0', '0');
    if (snapshots[pack_index].current > (float)(limit_thresholds.charge_overcurrent * 100u) &&
        !(protection_inhibit_flags & 0x10))
        ASCII_APPEND_PAIR(position, '0', '2');
    else
        ASCII_APPEND_PAIR(position, '0', '0');
    if (snapshots[pack_index].pack_voltage > limit_thresholds.pack_overvoltage &&
        !(protection_inhibit_flags & 4))
        ASCII_APPEND_PAIR(position, '0', '2');
    else if (snapshots[pack_index].pack_voltage < limit_thresholds.pack_undervoltage &&
             !(protection_inhibit_flags & 8))
        ASCII_APPEND_PAIR(position, '0', '1');
    else
        ASCII_APPEND_PAIR(position, '0', '0');
    if (snapshots[pack_index].current < limit_thresholds.discharge_overcurrent * 100 &&
        !(protection_inhibit_flags & 0x20))
        ASCII_APPEND_PAIR(position, '0', '2');
    else
        ASCII_APPEND_PAIR(position, '0', '0');
    ascii_byte_to_hex(snapshots[pack_index].status_flags[0], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[1], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[2], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[3], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[4], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[5], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[6], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[7], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
    ascii_byte_to_hex(snapshots[pack_index].status_flags[8], hex_pair_buffer);
    ASCII_APPEND_HEX_PAIR(position);
}

/* The request's INFO field starts at frame byte 13. ASCII_INFO_BYTE(i) decodes the byte
   whose two hex digits start i characters into it. */
#define ASCII_INFO_BYTE(i) ascii_hex_to_byte(uart4_frame[13 + (i)], uart4_frame[14 + (i)])
/* Appends one character to the reply at uart4_tx_length. */
#define ASCII_PUT_CHAR(c) (uart4_frame[uart4_tx_length++] = (c))
/* Appends a byte to the reply as two hex digits. */
#define ASCII_PUT_HEX(v)                                                                           \
    {                                                                                              \
        ascii_byte_to_hex((v), hex_pair_buffer);                                                   \
        ASCII_PUT_CHAR(hex_pair_buffer[0]);                                                        \
        ASCII_PUT_CHAR(hex_pair_buffer[1]);                                                        \
    }

/* Builds the reply to one command in uart4_frame and starts sending it. The reply starts
   with '~', version 25, this pack's address, device type 46, a 00 return code and an empty
   length field; each case appends its INFO and may set rtn. Then the return code and the
   length field are filled in and the checksum and carriage return appended. A 42 or 44
   request to address 0xFF with several packs sends this pack's block first; its length field
   covers every pack, and uart4_transmit_next_byte appends the other blocks and the checksum. */
void ascii_dispatch_command(uchar cid2) {
    uchar address;
    uchar rtn = ASCII_RTN_NORMAL;
    ushort i;
    ushort value;
    ushort checksum = 0;
    ushort log_record = 0xffff;
    ushort packed_model_word = 0;

    uart4_tx_length = 0;
    address = ASCII_INFO_BYTE(0);
    ASCII_PUT_CHAR('~');
    ASCII_PUT_CHAR('2');
    ASCII_PUT_CHAR('5');
    ASCII_PUT_HEX(pack_address);
    ASCII_PUT_CHAR('4');
    ASCII_PUT_CHAR('6');
    ASCII_PUT_CHAR('0');
    ASCII_PUT_CHAR('0');
    ASCII_PUT_CHAR('0');
    ASCII_PUT_CHAR('0');
    ASCII_PUT_CHAR('0');
    ASCII_PUT_CHAR('0');
    switch (cid2) {
    /* The frame checksum failed: reply with that return code. */
    case ASCII_CMD_FRAME_CHECKSUM_ERROR:
        rtn = ASCII_RTN_FRAME_CHECKSUM_ERROR;
        break;
    /* The length checksum failed: reply with that return code. */
    case ASCII_CMD_LENGTH_CHECKSUM_ERROR:
        rtn = ASCII_RTN_LENGTH_CHECKSUM_ERROR;
        break;
    /* Pack count: the pack at address 1 reports pack_count, any other pack 01. */
    case ASCII_CMD_GET_PACK_COUNT:
        if (pack_address == 1) {
            ASCII_PUT_HEX(pack_count);
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        break;
    /* Version text: the first 20 characters of software_version_text. */
    case ASCII_CMD_GET_SOFTWARE_VERSION:
        for (i = 0; i < 20; i++)
            ASCII_PUT_HEX(software_version_text[i]);
        break;
    /* Model code: its 20 characters. */
    case ASCII_CMD_GET_MODEL_CODE:
        for (i = 0; i < 20; i++)
            ASCII_PUT_HEX(model_code[i]);
        break;
    /* Set the model code: store 20 characters, save them as ten words from EEPROM 0x3C, then
       reapply the model presets. */
    case ASCII_CMD_SET_MODEL_CODE:
        for (i = 0; i < 20; i++)
            model_code[i] = ASCII_INFO_BYTE(i * 2);
        for (i = 0; i < 10; i++) {
            packed_model_word = (model_code[0 + (i * 2)] * 256) + model_code[0 + (i * 2) + 1];
            eeprom_write_word(0x3c + (i * 2), packed_model_word);
        }
        /* Model presets: the family letter, the variant letter and the rated capacity choose
           current_adc_options, model_current_class and the limit and protection currents;
           the four currents are saved. */
        if (model_code[0] == BMS_MODEL_FAMILY_B) {
            current_adc_options = 0;
            if (model_code[1] == BMS_MODEL_VARIANT_G &&
                measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 1;
                limit_thresholds.discharge_overcurrent = -70;
                limit_thresholds.charge_overcurrent = 70;
                protection_thresholds.discharge_overcurrent = -75;
                protection_thresholds.charge_overcurrent = 75;
            } else if (model_code[1] == BMS_MODEL_VARIANT_Y &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else {
                model_current_class = 2;
                if (measurement_rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
                    limit_thresholds.discharge_overcurrent = -60;
                    limit_thresholds.charge_overcurrent = 60;
                    protection_thresholds.discharge_overcurrent = -65;
                    protection_thresholds.charge_overcurrent = 65;
                } else {
                    limit_thresholds.discharge_overcurrent = -70;
                    limit_thresholds.charge_overcurrent = 70;
                    protection_thresholds.discharge_overcurrent = -75;
                    protection_thresholds.charge_overcurrent = 75;
                }
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        } else if (model_code[0] == BMS_MODEL_FAMILY_D) {
            current_adc_options = 1;
            if (model_code[1] == BMS_MODEL_VARIANT_G &&
                measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 1;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 85;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 90;
            } else if (model_code[1] == BMS_MODEL_VARIANT_Y &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else if ((model_code[1] == BMS_MODEL_VARIANT_W ||
                        model_code[1] == BMS_MODEL_VARIANT_X ||
                        model_code[1] == BMS_MODEL_VARIANT_Z ||
                        model_code[1] == BMS_MODEL_VARIANT_F) &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_EXTENDED) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else {
                model_current_class = 2;
                if (measurement_rated_capacity >= BMS_MODEL_CAPACITY_STANDARD) {
                    limit_thresholds.discharge_overcurrent = -90;
                    limit_thresholds.charge_overcurrent = 85;
                    protection_thresholds.discharge_overcurrent = -95;
                    protection_thresholds.charge_overcurrent = 90;
                } else {
                    limit_thresholds.discharge_overcurrent =
                        -(measurement_rated_capacity / 200 + 10);
                    limit_thresholds.charge_overcurrent = measurement_rated_capacity / 200 + 10;
                    protection_thresholds.discharge_overcurrent =
                        -(measurement_rated_capacity / 200 + 15);
                    protection_thresholds.charge_overcurrent =
                        measurement_rated_capacity / 200 + 15;
                }
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        } else {
            current_adc_options = 0;
            model_current_class = 2;
            if (measurement_rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
                limit_thresholds.discharge_overcurrent = -60;
                limit_thresholds.charge_overcurrent = 60;
                protection_thresholds.discharge_overcurrent = -65;
                protection_thresholds.charge_overcurrent = 65;
            } else {
                limit_thresholds.discharge_overcurrent = -70;
                limit_thresholds.charge_overcurrent = 70;
                protection_thresholds.discharge_overcurrent = -75;
                protection_thresholds.charge_overcurrent = 75;
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        }
        break;
    /* Alarm states: for every pack at address 0xFF (the transmit interrupt appends the other
       packs), for this pack at address 1, otherwise only the address. */
    case ASCII_CMD_GET_ALARM_STATES:
        ASCII_PUT_CHAR('0');
        ASCII_PUT_CHAR('0');
        if (address == 0xff) {
            ASCII_PUT_HEX(pack_count);
            ascii_append_alarm_states(0, &uart4_tx_length);
        } else {
            ASCII_PUT_HEX(address);
            /* match: the two-sided test is kept; address == 1 changes the code. */
            if (address > 0 && address <= 1)
                ascii_append_alarm_states(address - 1, &uart4_tx_length);
        }
        break;
    /* Analog values, addressed as for the alarm states. */
    case ASCII_CMD_GET_ANALOG_VALUES:
        ASCII_PUT_CHAR('0');
        ASCII_PUT_CHAR('0');
        if (address == 0xff) {
            ASCII_PUT_HEX(pack_count);
            ascii_append_analog_values(0, &uart4_tx_length);
        } else {
            ASCII_PUT_HEX(address);
            /* match: the two-sided test is kept; address == 1 changes the code. */
            if (address > 0 && address <= 1)
                ascii_append_analog_values(address - 1, &uart4_tx_length);
        }
        break;
    /* Switch flags: addresses 6 to 13 clear or set one bit of current_direction_latch_flags
       (6/7 bit 5, 8/9 bit 3, 10/11 bit 4, 12/13 bit 0); 11 also moves a set output status
       bit 0 to bit 1 and delays the task. The low seven bits are saved at EEPROM 0x38; the
       reply is the address and the flags. */
    case ASCII_CMD_SET_SWITCH_FLAGS:
        switch (address) {
        case 6:
            current_direction_latch_flags &= 0xdf;
            break;
        case 7:
            current_direction_latch_flags |= 0x20;
            break;
        case 8:
            current_direction_latch_flags &= 0xf7;
            break;
        case 9:
            current_direction_latch_flags |= 0x08;
            break;
        case 10:
            current_direction_latch_flags &= 0xef;
            break;
        case 11:
            current_direction_latch_flags |= 0x10;
            if (power_output_status_flags & 1) {
                power_output_status_flags |= 2;
                os_task_delay((uchar)os_task_dly_idx, 10, 10);
            }
            power_output_status_flags &= 0xfe;
            break;
        case 12:
            current_direction_latch_flags &= 0xfe;
            break;
        case 13:
            current_direction_latch_flags |= 0x01;
            break;
        default:
            rtn = ASCII_RTN_REJECTED;
            break;
        }
        eeprom_write_word(0x38, current_direction_latch_flags & 0x7f);
        ASCII_PUT_HEX(address);
        ASCII_PUT_HEX(current_direction_latch_flags);
        break;
    /* Charge output: address 0 turns it on unless a temperature or hardware fault blocks it,
       clearing the charge protection bits; address 1 turns it off and latches it off unless
       the current is negative or the discharge output is latched off. Replies the output
       status byte. */
    case ASCII_CMD_CHARGE_OUTPUT:
        switch (address) {
        case 0:
            if (!((temperature_protection_flags & 0x70) || (hardware_fault_flags & 0x2c))) {
                if (!(power_output_status_flags & 2)) {
                    current_direction_latch_flags &= ~2;
                    voltage_current_protection_flags &= 0x6a;
                    temperature_protection_flags &= 0xfa;
                    power_output_status_flags |= 2;
                }
            } else {
                rtn = ASCII_RTN_REJECTED;
            }
            break;
        case 1:
            if (measurement_current >= 0 && !(current_direction_latch_flags & 4)) {
                current_direction_latch_flags |= 2;
                power_output_status_flags &= 0xfc;
            } else {
                rtn = ASCII_RTN_REJECTED;
            }
            break;
        default:
            rtn = ASCII_RTN_REJECTED;
            break;
        }
        ASCII_PUT_HEX(power_output_status_flags);
        break;
    /* Discharge output: as the charge output, with the discharge bits; switching off is
       refused while the current is positive or the charge output is latched off. */
    case ASCII_CMD_DISCHARGE_OUTPUT:
        switch (address) {
        case 0:
            if (!((voltage_current_protection_flags & 0x40) ||
                  (temperature_protection_flags & 0x70) || (hardware_fault_flags & 0x2c))) {
                if (!(power_output_status_flags & 4)) {
                    current_direction_latch_flags &= ~4;
                    voltage_current_protection_flags &= 0xd5;
                    temperature_protection_flags &= 0xf5;
                    power_output_status_flags |= 4;
                }
            } else {
                rtn = ASCII_RTN_REJECTED;
            }
            break;
        case 1:
            if (measurement_current <= 0 && !(current_direction_latch_flags & 2)) {
                current_direction_latch_flags |= 4;
                power_output_status_flags &= 0xfb;
            } else {
                rtn = ASCII_RTN_REJECTED;
            }
            break;
        default:
            rtn = ASCII_RTN_REJECTED;
            break;
        }
        ASCII_PUT_HEX(power_output_status_flags);
        break;
    /* Accepted without action. */
    case ASCII_CMD_NO_ACTION_9C:
        break;
    /* Set the cycle count, clear the integrated capacity and cycle flags, and save all three. */
    case ASCII_CMD_SET_CYCLE_COUNT:
        cycle_count = ASCII_INFO_BYTE(0);
        cycle_count = cycle_count << 8;
        cycle_count = cycle_count + ASCII_INFO_BYTE(2);
        cycle_integrated_capacity = 0;
        cycle_count_flags = 0;
        eeprom_write_word(0x0a, cycle_count);
        eeprom_write_word(0x0e, (uint)cycle_integrated_capacity);
        eeprom_write_word(0x10, cycle_count_flags);
        break;
    /* Set the recovery SOC reference: 75 to 96, saved at EEPROM 0x18. */
    case ASCII_CMD_SET_RECOVERY_SOC:
        value = ASCII_INFO_BYTE(0);
        if (value > 74 && value < 97) {
            recovery_soc_reference = value;
            eeprom_write_word(0x18, recovery_soc_reference);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Remaining, full and rated capacity as 16-bit values. */
    case ASCII_CMD_GET_CAPACITIES:
        ASCII_PUT_HEX((uint)remaining_capacity >> 8);
        ASCII_PUT_HEX((uint)remaining_capacity);
        ASCII_PUT_HEX((uint)full_capacity >> 8);
        ASCII_PUT_HEX((uint)full_capacity);
        ASCII_PUT_HEX(measurement_rated_capacity >> 8);
        ASCII_PUT_HEX(measurement_rated_capacity);
        break;
    /* Set and save the remaining, full and rated capacity, then reapply the model presets. */
    case ASCII_CMD_SET_CAPACITIES:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        eeprom_write_word(0x06, value);
        remaining_capacity = value;
        value = ASCII_INFO_BYTE(4);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(6);
        eeprom_write_word(0x08, value);
        full_capacity = value;
        measurement_rated_capacity = ASCII_INFO_BYTE(8);
        measurement_rated_capacity = measurement_rated_capacity << 8;
        measurement_rated_capacity = measurement_rated_capacity + ASCII_INFO_BYTE(10);
        eeprom_write_word(0x0c, measurement_rated_capacity);
        /* Model presets: the family letter, the variant letter and the rated capacity choose
           current_adc_options, model_current_class and the limit and protection currents;
           the four currents are saved. */
        if (model_code[0] == BMS_MODEL_FAMILY_B) {
            current_adc_options = 0;
            if (model_code[1] == BMS_MODEL_VARIANT_G &&
                measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 1;
                limit_thresholds.discharge_overcurrent = -70;
                limit_thresholds.charge_overcurrent = 70;
                protection_thresholds.discharge_overcurrent = -75;
                protection_thresholds.charge_overcurrent = 75;
            } else if (model_code[1] == BMS_MODEL_VARIANT_Y &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else {
                model_current_class = 2;
                if (measurement_rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
                    limit_thresholds.discharge_overcurrent = -60;
                    limit_thresholds.charge_overcurrent = 60;
                    protection_thresholds.discharge_overcurrent = -65;
                    protection_thresholds.charge_overcurrent = 65;
                } else {
                    limit_thresholds.discharge_overcurrent = -70;
                    limit_thresholds.charge_overcurrent = 70;
                    protection_thresholds.discharge_overcurrent = -75;
                    protection_thresholds.charge_overcurrent = 75;
                }
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        } else if (model_code[0] == BMS_MODEL_FAMILY_D) {
            current_adc_options = 1;
            if (model_code[1] == BMS_MODEL_VARIANT_G &&
                measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 1;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 85;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 90;
            } else if (model_code[1] == BMS_MODEL_VARIANT_Y &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_STANDARD) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else if ((model_code[1] == BMS_MODEL_VARIANT_W ||
                        model_code[1] == BMS_MODEL_VARIANT_X ||
                        model_code[1] == BMS_MODEL_VARIANT_Z ||
                        model_code[1] == BMS_MODEL_VARIANT_F) &&
                       measurement_rated_capacity == BMS_MODEL_CAPACITY_EXTENDED) {
                model_current_class = 4;
                limit_thresholds.discharge_overcurrent = -90;
                limit_thresholds.charge_overcurrent = 90;
                protection_thresholds.discharge_overcurrent = -95;
                protection_thresholds.charge_overcurrent = 95;
            } else {
                model_current_class = 2;
                if (measurement_rated_capacity >= BMS_MODEL_CAPACITY_STANDARD) {
                    limit_thresholds.discharge_overcurrent = -90;
                    limit_thresholds.charge_overcurrent = 85;
                    protection_thresholds.discharge_overcurrent = -95;
                    protection_thresholds.charge_overcurrent = 90;
                } else {
                    limit_thresholds.discharge_overcurrent =
                        -(measurement_rated_capacity / 200 + 10);
                    limit_thresholds.charge_overcurrent = measurement_rated_capacity / 200 + 10;
                    protection_thresholds.discharge_overcurrent =
                        -(measurement_rated_capacity / 200 + 15);
                    protection_thresholds.charge_overcurrent =
                        measurement_rated_capacity / 200 + 15;
                }
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        } else {
            current_adc_options = 0;
            model_current_class = 2;
            if (measurement_rated_capacity <= BMS_LOW_CAPACITY_CUTOFF) {
                limit_thresholds.discharge_overcurrent = -60;
                limit_thresholds.charge_overcurrent = 60;
                protection_thresholds.discharge_overcurrent = -65;
                protection_thresholds.charge_overcurrent = 65;
            } else {
                limit_thresholds.discharge_overcurrent = -70;
                limit_thresholds.charge_overcurrent = 70;
                protection_thresholds.discharge_overcurrent = -75;
                protection_thresholds.charge_overcurrent = 75;
            }
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
        }
        break;
    /* Accepted without action. */
    case ASCII_CMD_NO_ACTION_A8:
        break;
    /* Four fixed bytes. */
    case ASCII_CMD_GET_FIXED_A0:
        ASCII_PUT_HEX(0x0c);
        ASCII_PUT_HEX(0x1c);
        ASCII_PUT_HEX(0x00);
        ASCII_PUT_HEX(0x01);
        break;
    /* Accepted without action. */
    case ASCII_CMD_NO_ACTION_AE:
        break;
    /* Five fixed bytes. */
    case ASCII_CMD_GET_FIXED_AF:
        ASCII_PUT_HEX(0xda);
        ASCII_PUT_HEX(0xc0);
        ASCII_PUT_HEX(0x03);
        ASCII_PUT_HEX(0xe8);
        ASCII_PUT_HEX(0x1e);
        break;
    /* Accepted without action. */
    case ASCII_CMD_NO_ACTION_B5:
        break;
    /* Four fixed bytes. */
    case ASCII_CMD_GET_FIXED_B6:
        ASCII_PUT_HEX(0x0d);
        ASCII_PUT_HEX(0x48);
        ASCII_PUT_HEX(0x00);
        ASCII_PUT_HEX(0x1e);
        break;
    /* Set the cell ADC reference: 3251 to 3349, saved at EEPROM 0x1E. */
    case ASCII_CMD_SET_CELL_ADC_REFERENCE:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value > 3250 && value < 3350) {
            cell_adc_reference_scale = value;
            eeprom_write_word(0x1e, cell_adc_reference_scale);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Set the ADC reference: 3251 to 3349, saved at EEPROM 0x1C. */
    case ASCII_CMD_SET_ADC_REFERENCE:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value > 3250 && value < 3350) {
            adc_reference_scale = value;
            eeprom_write_word(0x1c, adc_reference_scale);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Calibrate the pack voltage: scale pack_voltage_scale by the given over the measured
       voltage. A result outside 0.8 to 1.5 resets it to 1.0 and is rejected; either way the
       scale is saved at EEPROM 0x2C. */
    case ASCII_CMD_CALIBRATE_PACK_VOLTAGE:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (measurement_pack_voltage != 0) {
            pack_voltage_scale =
                (float)value / (float)measurement_pack_voltage * pack_voltage_scale;
            if (pack_voltage_scale > 1.5 || pack_voltage_scale < 0.8) {
                pack_voltage_scale = 1.0f;
                rtn = ASCII_RTN_REJECTED;
            }
            eeprom_write_value32(0x2c, pack_voltage_scale);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Calibrate against a negative current: 0 resets the scale to 1.0; 51 to 7999, while the
       measured current is negative, multiplies it by ten times the value over the measured
       magnitude. Results outside 0.8 to 1.5 reset it to 1.0 and are rejected. Saved at EEPROM
       0x24. */
    case ASCII_CMD_CALIBRATE_NEGATIVE_CURRENT:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value == 0) {
            negative_current_scale = 1.0f;
            eeprom_write_value32(
                0x24,
                1.0f); /* match: saves the constant, not the scale; the variable changes the code */
        } else if (value < 8000 && value > 50 && measurement_current < 0) {
            negative_current_scale =
                (float)value * 10.0f / (0 - measurement_current) * negative_current_scale;
            if (negative_current_scale > 1.5 || negative_current_scale < 0.8) {
                negative_current_scale = 1.0f;
                rtn = ASCII_RTN_REJECTED;
            }
            eeprom_write_value32(0x24, negative_current_scale);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Calibrate against a positive current, as for the negative current; saved at EEPROM 0x28. */
    case ASCII_CMD_CALIBRATE_POSITIVE_CURRENT:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value == 0) {
            positive_current_scale = 1.0f;
            eeprom_write_value32(
                0x28,
                1.0f); /* match: saves the constant, not the scale; the variable changes the code */
        } else if (value < 8000 && value > 50 && measurement_current > 0) {
            positive_current_scale =
                (float)value * 10.0f / measurement_current * positive_current_scale;
            if (positive_current_scale > 1.5 || positive_current_scale < 0.8) {
                positive_current_scale = 1.0f;
                rtn = ASCII_RTN_REJECTED;
            }
            eeprom_write_value32(0x28, positive_current_scale);
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Read an event record: 0 is the newest of up to 400 records of 65 bytes; bit 15 of
       log_index marks a full ring. A missing record gets no INFO. */
    case ASCII_CMD_READ_EVENT_LOG:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value < 400) {
            if (log_index & 0x8000)
                log_record = ((log_index & 0x7fff) + (400 - value - 1)) % 400;
            else if (value < log_index)
                log_record = log_index - value - 1;
        }
        if (log_record != 0xffff) {
            for (i = 0; i < 65; i++)
                ASCII_PUT_HEX(eeprom_read_byte(log_record * 65 + i));
        }
        break;
    /* Clear the event log index and its reserved word, save both, and clear the counters. */
    case ASCII_CMD_CLEAR_EVENT_LOG:
        log_index = 0;
        event_log_reserved_word = 0;
        eeprom_write_word(0x14, log_index);
        eeprom_write_word(0x16, event_log_reserved_word);
        settings_clear_counters();
        break;
    /* Set the date: six bytes converted to BCD into date_now; the reply is always rejected. */
    case ASCII_CMD_SET_DATE:
        date_now.d[0] = byte_to_bcd(ASCII_INFO_BYTE(0));
        date_now.d[1] = byte_to_bcd(ASCII_INFO_BYTE(2));
        date_now.d[2] = byte_to_bcd(ASCII_INFO_BYTE(4));
        date_now.d[3] = byte_to_bcd(ASCII_INFO_BYTE(6));
        date_now.d[4] = byte_to_bcd(ASCII_INFO_BYTE(8));
        date_now.d[5] = byte_to_bcd(ASCII_INFO_BYTE(10));
        rtn = ASCII_RTN_REJECTED;
        break;
    /* Date: refresh date_now from the clock and reply its six bytes in binary. */
    case ASCII_CMD_GET_DATE:
        runtime_publish_date();
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[0]));
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[1]));
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[2]));
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[3]));
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[4]));
        ASCII_PUT_HEX(bcd_to_byte(date_now.d[5]));
        break;
    /* Set the log start date (six bytes, stored as BCD). */
    case ASCII_CMD_SET_LOG_START_DATE:
        date_start.d[0] = byte_to_bcd(ASCII_INFO_BYTE(0));
        date_start.d[1] = byte_to_bcd(ASCII_INFO_BYTE(2));
        date_start.d[2] = byte_to_bcd(ASCII_INFO_BYTE(4));
        date_start.d[3] = byte_to_bcd(ASCII_INFO_BYTE(6));
        date_start.d[4] = byte_to_bcd(ASCII_INFO_BYTE(8));
        date_start.d[5] = byte_to_bcd(ASCII_INFO_BYTE(10));
        break;
    /* Log start date. */
    case ASCII_CMD_GET_LOG_START_DATE:
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[0]));
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[1]));
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[2]));
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[3]));
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[4]));
        ASCII_PUT_HEX(bcd_to_byte(date_start.d[5]));
        break;
    /* Set the log end date. */
    case ASCII_CMD_SET_LOG_END_DATE:
        date_end.d[0] = byte_to_bcd(ASCII_INFO_BYTE(0));
        date_end.d[1] = byte_to_bcd(ASCII_INFO_BYTE(2));
        date_end.d[2] = byte_to_bcd(ASCII_INFO_BYTE(4));
        date_end.d[3] = byte_to_bcd(ASCII_INFO_BYTE(6));
        date_end.d[4] = byte_to_bcd(ASCII_INFO_BYTE(8));
        date_end.d[5] = byte_to_bcd(ASCII_INFO_BYTE(10));
        break;
    /* Log end date. */
    case ASCII_CMD_GET_LOG_END_DATE:
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[0]));
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[1]));
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[2]));
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[3]));
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[4]));
        ASCII_PUT_HEX(bcd_to_byte(date_end.d[5]));
        break;
    /* Set the log interval: 1 to 119. */
    case ASCII_CMD_SET_LOG_INTERVAL:
        value = ASCII_INFO_BYTE(0);
        if (value > 0 && value < 120)
            log_interval = value;
        else
            rtn = ASCII_RTN_REJECTED;
        break;
    /* Log interval. */
    case ASCII_CMD_GET_LOG_INTERVAL:
        ASCII_PUT_HEX(log_interval);
        break;
    /* Threshold sets: a first byte of 0 inhibits the alarm; otherwise the inhibit bit is
       cleared and the limit, protection and recovery values and the delay are stored and
       saved. The inhibit flags are saved at EEPROM 0x30 either way. Cell overvoltage here. */
    case ASCII_CMD_SET_CELL_OVERVOLTAGE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x01;
        } else {
            protection_inhibit_flags &= 0xfe;
            limit_thresholds.cell_overvoltage = ASCII_INFO_BYTE(2);
            limit_thresholds.cell_overvoltage = limit_thresholds.cell_overvoltage << 8;
            limit_thresholds.cell_overvoltage =
                limit_thresholds.cell_overvoltage + ASCII_INFO_BYTE(4);
            eeprom_write_word(0x82, limit_thresholds.cell_overvoltage);
            protection_thresholds.cell_overvoltage = ASCII_INFO_BYTE(6);
            protection_thresholds.cell_overvoltage = protection_thresholds.cell_overvoltage << 8;
            protection_thresholds.cell_overvoltage =
                protection_thresholds.cell_overvoltage + ASCII_INFO_BYTE(8);
            eeprom_write_word(0x84, protection_thresholds.cell_overvoltage);
            recovery_thresholds.cell_overvoltage = ASCII_INFO_BYTE(10);
            recovery_thresholds.cell_overvoltage = recovery_thresholds.cell_overvoltage << 8;
            recovery_thresholds.cell_overvoltage =
                recovery_thresholds.cell_overvoltage + ASCII_INFO_BYTE(12);
            eeprom_write_word(0x86, recovery_thresholds.cell_overvoltage);
            protection_debounce_settings[0] = ASCII_INFO_BYTE(14);
            eeprom_write_word(0x88, protection_debounce_settings[0]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    /* Threshold gets: "00" if the alarm is inhibited, "01" if enabled, then the values. */
    case ASCII_CMD_GET_CELL_OVERVOLTAGE:
        if (protection_inhibit_flags & 0x01) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.cell_overvoltage >> 8);
        ASCII_PUT_HEX(limit_thresholds.cell_overvoltage);
        ASCII_PUT_HEX(protection_thresholds.cell_overvoltage >> 8);
        ASCII_PUT_HEX(protection_thresholds.cell_overvoltage);
        ASCII_PUT_HEX(recovery_thresholds.cell_overvoltage >> 8);
        ASCII_PUT_HEX(recovery_thresholds.cell_overvoltage);
        ASCII_PUT_HEX(protection_debounce_settings[0]);
        break;
    /* Cell undervoltage. */
    case ASCII_CMD_SET_CELL_UNDERVOLTAGE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x02;
        } else {
            protection_inhibit_flags &= 0xfd;
            limit_thresholds.cell_undervoltage = ASCII_INFO_BYTE(2);
            limit_thresholds.cell_undervoltage = limit_thresholds.cell_undervoltage << 8;
            limit_thresholds.cell_undervoltage =
                limit_thresholds.cell_undervoltage + ASCII_INFO_BYTE(4);
            eeprom_write_word(0x8a, limit_thresholds.cell_undervoltage);
            protection_thresholds.cell_undervoltage = ASCII_INFO_BYTE(6);
            protection_thresholds.cell_undervoltage = protection_thresholds.cell_undervoltage << 8;
            protection_thresholds.cell_undervoltage =
                protection_thresholds.cell_undervoltage + ASCII_INFO_BYTE(8);
            eeprom_write_word(0x8c, protection_thresholds.cell_undervoltage);
            recovery_thresholds.cell_undervoltage = ASCII_INFO_BYTE(10);
            recovery_thresholds.cell_undervoltage = recovery_thresholds.cell_undervoltage << 8;
            recovery_thresholds.cell_undervoltage =
                recovery_thresholds.cell_undervoltage + ASCII_INFO_BYTE(12);
            eeprom_write_word(0x8e, recovery_thresholds.cell_undervoltage);
            protection_debounce_settings[1] = ASCII_INFO_BYTE(14);
            eeprom_write_word(0x90, protection_debounce_settings[1]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_CELL_UNDERVOLTAGE:
        if (protection_inhibit_flags & 0x02) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.cell_undervoltage >> 8);
        ASCII_PUT_HEX(limit_thresholds.cell_undervoltage);
        ASCII_PUT_HEX(protection_thresholds.cell_undervoltage >> 8);
        ASCII_PUT_HEX(protection_thresholds.cell_undervoltage);
        ASCII_PUT_HEX(recovery_thresholds.cell_undervoltage >> 8);
        ASCII_PUT_HEX(recovery_thresholds.cell_undervoltage);
        ASCII_PUT_HEX(protection_debounce_settings[1]);
        break;
    /* Pack overvoltage. */
    case ASCII_CMD_SET_PACK_OVERVOLTAGE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x04;
        } else {
            protection_inhibit_flags &= 0xfb;
            limit_thresholds.pack_overvoltage = ASCII_INFO_BYTE(2);
            limit_thresholds.pack_overvoltage = limit_thresholds.pack_overvoltage << 8;
            limit_thresholds.pack_overvoltage =
                limit_thresholds.pack_overvoltage + ASCII_INFO_BYTE(4);
            eeprom_write_word(0x92, limit_thresholds.pack_overvoltage);
            protection_thresholds.pack_overvoltage = ASCII_INFO_BYTE(6);
            protection_thresholds.pack_overvoltage = protection_thresholds.pack_overvoltage << 8;
            protection_thresholds.pack_overvoltage =
                protection_thresholds.pack_overvoltage + ASCII_INFO_BYTE(8);
            eeprom_write_word(0x94, protection_thresholds.pack_overvoltage);
            recovery_thresholds.pack_overvoltage = ASCII_INFO_BYTE(10);
            recovery_thresholds.pack_overvoltage = recovery_thresholds.pack_overvoltage << 8;
            recovery_thresholds.pack_overvoltage =
                recovery_thresholds.pack_overvoltage + ASCII_INFO_BYTE(12);
            eeprom_write_word(0x96, recovery_thresholds.pack_overvoltage);
            protection_debounce_settings[2] = ASCII_INFO_BYTE(14);
            eeprom_write_word(0x98, protection_debounce_settings[2]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_PACK_OVERVOLTAGE:
        if (protection_inhibit_flags & 0x04) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.pack_overvoltage >> 8);
        ASCII_PUT_HEX(limit_thresholds.pack_overvoltage);
        ASCII_PUT_HEX(protection_thresholds.pack_overvoltage >> 8);
        ASCII_PUT_HEX(protection_thresholds.pack_overvoltage);
        ASCII_PUT_HEX(recovery_thresholds.pack_overvoltage >> 8);
        ASCII_PUT_HEX(recovery_thresholds.pack_overvoltage);
        ASCII_PUT_HEX(protection_debounce_settings[2]);
        break;
    /* Pack undervoltage. */
    case ASCII_CMD_SET_PACK_UNDERVOLTAGE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x08;
        } else {
            protection_inhibit_flags &= 0xf7;
            limit_thresholds.pack_undervoltage = ASCII_INFO_BYTE(2);
            limit_thresholds.pack_undervoltage = limit_thresholds.pack_undervoltage << 8;
            limit_thresholds.pack_undervoltage =
                limit_thresholds.pack_undervoltage + ASCII_INFO_BYTE(4);
            eeprom_write_word(0x9a, limit_thresholds.pack_undervoltage);
            protection_thresholds.pack_undervoltage = ASCII_INFO_BYTE(6);
            protection_thresholds.pack_undervoltage = protection_thresholds.pack_undervoltage << 8;
            protection_thresholds.pack_undervoltage =
                protection_thresholds.pack_undervoltage + ASCII_INFO_BYTE(8);
            eeprom_write_word(0x9c, protection_thresholds.pack_undervoltage);
            recovery_thresholds.pack_undervoltage = ASCII_INFO_BYTE(10);
            recovery_thresholds.pack_undervoltage = recovery_thresholds.pack_undervoltage << 8;
            recovery_thresholds.pack_undervoltage =
                recovery_thresholds.pack_undervoltage + ASCII_INFO_BYTE(12);
            eeprom_write_word(0x9e, recovery_thresholds.pack_undervoltage);
            protection_debounce_settings[3] = ASCII_INFO_BYTE(14);
            eeprom_write_word(0xa0, protection_debounce_settings[3]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_PACK_UNDERVOLTAGE:
        if (protection_inhibit_flags & 0x08) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.pack_undervoltage >> 8);
        ASCII_PUT_HEX(limit_thresholds.pack_undervoltage);
        ASCII_PUT_HEX(protection_thresholds.pack_undervoltage >> 8);
        ASCII_PUT_HEX(protection_thresholds.pack_undervoltage);
        ASCII_PUT_HEX(recovery_thresholds.pack_undervoltage >> 8);
        ASCII_PUT_HEX(recovery_thresholds.pack_undervoltage);
        ASCII_PUT_HEX(protection_debounce_settings[3]);
        break;
    /* Charge overcurrent: limit and protection currents, each capped at 120, and the delay. */
    case ASCII_CMD_SET_CHARGE_OVERCURRENT:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x10;
        } else {
            protection_inhibit_flags &= 0xef;
            value = ASCII_INFO_BYTE(2);
            value = value << 8;
            value = value + ASCII_INFO_BYTE(4);
            if (value > 120)
                value = 120;
            limit_thresholds.charge_overcurrent = value;
            /* match: the saves read back the stored fields; saving value instead changes the code
               (tested on the limit save). */
            eeprom_write_word(BMS_EEPROM_CHARGE_LIMIT, limit_thresholds.charge_overcurrent);
            value = ASCII_INFO_BYTE(6);
            value = value << 8;
            value = value + ASCII_INFO_BYTE(8);
            if (value > 120)
                value = 120;
            protection_thresholds.charge_overcurrent = value;
            eeprom_write_word(BMS_EEPROM_CHARGE_PROTECTION,
                              protection_thresholds.charge_overcurrent);
            protection_debounce_settings[4] = ASCII_INFO_BYTE(10);
            eeprom_write_word(0xa6, protection_debounce_settings[4]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_CHARGE_OVERCURRENT:
        if (protection_inhibit_flags & 0x10) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.charge_overcurrent >> 8);
        ASCII_PUT_HEX(limit_thresholds.charge_overcurrent);
        ASCII_PUT_HEX(protection_thresholds.charge_overcurrent >> 8);
        ASCII_PUT_HEX(protection_thresholds.charge_overcurrent);
        ASCII_PUT_HEX(protection_debounce_settings[4]);
        break;
    /* Discharge overcurrent: magnitudes capped at 100 (limit) and 120 (protection), stored
       negated, and the delay. */
    case ASCII_CMD_SET_DISCHARGE_OVERCURRENT:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x20;
        } else {
            protection_inhibit_flags &= 0xdf;
            value = ASCII_INFO_BYTE(2);
            value = value << 8;
            value = value + ASCII_INFO_BYTE(4);
            if (value > 100)
                value = 100;
            limit_thresholds.discharge_overcurrent = -value;
            eeprom_write_word(BMS_EEPROM_DISCHARGE_LIMIT, limit_thresholds.discharge_overcurrent);
            value = ASCII_INFO_BYTE(6);
            value = value << 8;
            value = value + ASCII_INFO_BYTE(8);
            if (value > 120)
                value = 120;
            protection_thresholds.discharge_overcurrent = -value;
            eeprom_write_word(BMS_EEPROM_DISCHARGE_PROTECTION,
                              protection_thresholds.discharge_overcurrent);
            protection_debounce_settings[5] = ASCII_INFO_BYTE(10);
            eeprom_write_word(0xac, protection_debounce_settings[5]);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_DISCHARGE_OVERCURRENT:
        if (protection_inhibit_flags & 0x20) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.discharge_overcurrent >> 8);
        ASCII_PUT_HEX(limit_thresholds.discharge_overcurrent);
        ASCII_PUT_HEX(protection_thresholds.discharge_overcurrent >> 8);
        ASCII_PUT_HEX(protection_thresholds.discharge_overcurrent);
        ASCII_PUT_HEX(protection_debounce_settings[5]);
        break;
    /* Charge and discharge overtemperature: limit, protection and recovery values for each. */
    case ASCII_CMD_SET_OVERTEMPERATURE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x40;
        } else {
            protection_inhibit_flags &= 0xbf;
            limit_thresholds.charge_overtemperature = ASCII_INFO_BYTE(2);
            limit_thresholds.charge_overtemperature = limit_thresholds.charge_overtemperature << 8;
            limit_thresholds.charge_overtemperature =
                limit_thresholds.charge_overtemperature + ASCII_INFO_BYTE(4);
            eeprom_write_word(0xae, limit_thresholds.charge_overtemperature);
            protection_thresholds.charge_overtemperature = ASCII_INFO_BYTE(6);
            protection_thresholds.charge_overtemperature =
                protection_thresholds.charge_overtemperature << 8;
            protection_thresholds.charge_overtemperature =
                protection_thresholds.charge_overtemperature + ASCII_INFO_BYTE(8);
            eeprom_write_word(0xb0, protection_thresholds.charge_overtemperature);
            recovery_thresholds.charge_overtemperature = ASCII_INFO_BYTE(10);
            recovery_thresholds.charge_overtemperature = recovery_thresholds.charge_overtemperature
                                                         << 8;
            recovery_thresholds.charge_overtemperature =
                recovery_thresholds.charge_overtemperature + ASCII_INFO_BYTE(12);
            eeprom_write_word(0xb2, recovery_thresholds.charge_overtemperature);
            limit_thresholds.discharge_overtemperature = ASCII_INFO_BYTE(14);
            limit_thresholds.discharge_overtemperature = limit_thresholds.discharge_overtemperature
                                                         << 8;
            limit_thresholds.discharge_overtemperature =
                limit_thresholds.discharge_overtemperature + ASCII_INFO_BYTE(16);
            eeprom_write_word(0xb4, limit_thresholds.discharge_overtemperature);
            protection_thresholds.discharge_overtemperature = ASCII_INFO_BYTE(18);
            protection_thresholds.discharge_overtemperature =
                protection_thresholds.discharge_overtemperature << 8;
            protection_thresholds.discharge_overtemperature =
                protection_thresholds.discharge_overtemperature + ASCII_INFO_BYTE(20);
            eeprom_write_word(0xb6, protection_thresholds.discharge_overtemperature);
            recovery_thresholds.discharge_overtemperature = ASCII_INFO_BYTE(22);
            recovery_thresholds.discharge_overtemperature =
                recovery_thresholds.discharge_overtemperature << 8;
            recovery_thresholds.discharge_overtemperature =
                recovery_thresholds.discharge_overtemperature + ASCII_INFO_BYTE(24);
            eeprom_write_word(0xb8, recovery_thresholds.discharge_overtemperature);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_OVERTEMPERATURE:
        if (protection_inhibit_flags & 0x40) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.charge_overtemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.charge_overtemperature);
        ASCII_PUT_HEX(protection_thresholds.charge_overtemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.charge_overtemperature);
        ASCII_PUT_HEX(recovery_thresholds.charge_overtemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.charge_overtemperature);
        ASCII_PUT_HEX(limit_thresholds.discharge_overtemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.discharge_overtemperature);
        ASCII_PUT_HEX(protection_thresholds.discharge_overtemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.discharge_overtemperature);
        ASCII_PUT_HEX(recovery_thresholds.discharge_overtemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.discharge_overtemperature);
        break;
    /* Charge and discharge undertemperature. */
    case ASCII_CMD_SET_UNDERTEMPERATURE:
        if (ASCII_INFO_BYTE(0) == 0) {
            protection_inhibit_flags |= 0x80;
        } else {
            protection_inhibit_flags &= 0x7f;
            limit_thresholds.charge_undertemperature = ASCII_INFO_BYTE(2);
            limit_thresholds.charge_undertemperature = limit_thresholds.charge_undertemperature
                                                       << 8;
            limit_thresholds.charge_undertemperature =
                limit_thresholds.charge_undertemperature + ASCII_INFO_BYTE(4);
            eeprom_write_word(0xba, limit_thresholds.charge_undertemperature);
            protection_thresholds.charge_undertemperature = ASCII_INFO_BYTE(6);
            protection_thresholds.charge_undertemperature =
                protection_thresholds.charge_undertemperature << 8;
            protection_thresholds.charge_undertemperature =
                protection_thresholds.charge_undertemperature + ASCII_INFO_BYTE(8);
            eeprom_write_word(0xbc, protection_thresholds.charge_undertemperature);
            recovery_thresholds.charge_undertemperature = ASCII_INFO_BYTE(10);
            recovery_thresholds.charge_undertemperature =
                recovery_thresholds.charge_undertemperature << 8;
            recovery_thresholds.charge_undertemperature =
                recovery_thresholds.charge_undertemperature + ASCII_INFO_BYTE(12);
            eeprom_write_word(0xbe, recovery_thresholds.charge_undertemperature);
            limit_thresholds.discharge_undertemperature = ASCII_INFO_BYTE(14);
            limit_thresholds.discharge_undertemperature =
                limit_thresholds.discharge_undertemperature << 8;
            limit_thresholds.discharge_undertemperature =
                limit_thresholds.discharge_undertemperature + ASCII_INFO_BYTE(16);
            eeprom_write_word(0xc0, limit_thresholds.discharge_undertemperature);
            protection_thresholds.discharge_undertemperature = ASCII_INFO_BYTE(18);
            protection_thresholds.discharge_undertemperature =
                protection_thresholds.discharge_undertemperature << 8;
            protection_thresholds.discharge_undertemperature =
                protection_thresholds.discharge_undertemperature + ASCII_INFO_BYTE(20);
            eeprom_write_word(0xc2, protection_thresholds.discharge_undertemperature);
            recovery_thresholds.discharge_undertemperature = ASCII_INFO_BYTE(22);
            recovery_thresholds.discharge_undertemperature =
                recovery_thresholds.discharge_undertemperature << 8;
            recovery_thresholds.discharge_undertemperature =
                recovery_thresholds.discharge_undertemperature + ASCII_INFO_BYTE(24);
            eeprom_write_word(0xc4, recovery_thresholds.discharge_undertemperature);
        }
        eeprom_write_word(0x30, protection_inhibit_flags);
        break;
    case ASCII_CMD_GET_UNDERTEMPERATURE:
        if (protection_inhibit_flags & 0x80) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.charge_undertemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.charge_undertemperature);
        ASCII_PUT_HEX(protection_thresholds.charge_undertemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.charge_undertemperature);
        ASCII_PUT_HEX(recovery_thresholds.charge_undertemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.charge_undertemperature);
        ASCII_PUT_HEX(limit_thresholds.discharge_undertemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.discharge_undertemperature);
        ASCII_PUT_HEX(protection_thresholds.discharge_undertemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.discharge_undertemperature);
        ASCII_PUT_HEX(recovery_thresholds.discharge_undertemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.discharge_undertemperature);
        break;
    /* MOSFET overtemperature: a non-zero first byte stores the three values. No inhibit bit
       changes; capacity_latch_flags is saved at EEPROM 0x3A. */
    case ASCII_CMD_SET_MOSFET_OVERTEMPERATURE:
        if (ASCII_INFO_BYTE(0) != 0) {
            limit_thresholds.mosfet_overtemperature = ASCII_INFO_BYTE(2);
            limit_thresholds.mosfet_overtemperature = limit_thresholds.mosfet_overtemperature << 8;
            limit_thresholds.mosfet_overtemperature =
                limit_thresholds.mosfet_overtemperature + ASCII_INFO_BYTE(4);
            eeprom_write_word(0xc6, limit_thresholds.mosfet_overtemperature);
            protection_thresholds.mosfet_overtemperature = ASCII_INFO_BYTE(6);
            protection_thresholds.mosfet_overtemperature =
                protection_thresholds.mosfet_overtemperature << 8;
            protection_thresholds.mosfet_overtemperature =
                protection_thresholds.mosfet_overtemperature + ASCII_INFO_BYTE(8);
            eeprom_write_word(0xc8, protection_thresholds.mosfet_overtemperature);
            recovery_thresholds.mosfet_overtemperature = ASCII_INFO_BYTE(10);
            recovery_thresholds.mosfet_overtemperature = recovery_thresholds.mosfet_overtemperature
                                                         << 8;
            recovery_thresholds.mosfet_overtemperature =
                recovery_thresholds.mosfet_overtemperature + ASCII_INFO_BYTE(12);
            eeprom_write_word(0xca, recovery_thresholds.mosfet_overtemperature);
        }
        eeprom_write_word(0x3a, capacity_latch_flags);
        break;
    /* "00" if bit 0 of capacity_latch_flags is set, "01" otherwise, then the values. */
    case ASCII_CMD_GET_MOSFET_OVERTEMPERATURE:
        if (capacity_latch_flags & 0x01) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.mosfet_overtemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.mosfet_overtemperature);
        ASCII_PUT_HEX(protection_thresholds.mosfet_overtemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.mosfet_overtemperature);
        ASCII_PUT_HEX(recovery_thresholds.mosfet_overtemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.mosfet_overtemperature);
        break;
    /* Ambient under- and overtemperature, as for the MOSFET command. */
    case ASCII_CMD_SET_AMBIENT_TEMPERATURE:
        if (ASCII_INFO_BYTE(0) != 0) {
            limit_thresholds.ambient_undertemperature = ASCII_INFO_BYTE(2);
            limit_thresholds.ambient_undertemperature = limit_thresholds.ambient_undertemperature
                                                        << 8;
            limit_thresholds.ambient_undertemperature =
                limit_thresholds.ambient_undertemperature + ASCII_INFO_BYTE(4);
            eeprom_write_word(0xce, limit_thresholds.ambient_undertemperature);
            protection_thresholds.ambient_undertemperature = ASCII_INFO_BYTE(6);
            protection_thresholds.ambient_undertemperature =
                protection_thresholds.ambient_undertemperature << 8;
            protection_thresholds.ambient_undertemperature =
                protection_thresholds.ambient_undertemperature + ASCII_INFO_BYTE(8);
            eeprom_write_word(0xf4, protection_thresholds.ambient_undertemperature);
            recovery_thresholds.ambient_undertemperature = ASCII_INFO_BYTE(10);
            recovery_thresholds.ambient_undertemperature =
                recovery_thresholds.ambient_undertemperature << 8;
            recovery_thresholds.ambient_undertemperature =
                recovery_thresholds.ambient_undertemperature + ASCII_INFO_BYTE(12);
            eeprom_write_word(0xf6, recovery_thresholds.ambient_undertemperature);
            limit_thresholds.ambient_overtemperature = ASCII_INFO_BYTE(14);
            limit_thresholds.ambient_overtemperature = limit_thresholds.ambient_overtemperature
                                                       << 8;
            limit_thresholds.ambient_overtemperature =
                limit_thresholds.ambient_overtemperature + ASCII_INFO_BYTE(16);
            eeprom_write_word(0xcc, limit_thresholds.ambient_overtemperature);
            protection_thresholds.ambient_overtemperature = ASCII_INFO_BYTE(18);
            protection_thresholds.ambient_overtemperature =
                protection_thresholds.ambient_overtemperature << 8;
            protection_thresholds.ambient_overtemperature =
                protection_thresholds.ambient_overtemperature + ASCII_INFO_BYTE(20);
            eeprom_write_word(0xf0, protection_thresholds.ambient_overtemperature);
            recovery_thresholds.ambient_overtemperature = ASCII_INFO_BYTE(22);
            recovery_thresholds.ambient_overtemperature =
                recovery_thresholds.ambient_overtemperature << 8;
            recovery_thresholds.ambient_overtemperature =
                recovery_thresholds.ambient_overtemperature + ASCII_INFO_BYTE(24);
            eeprom_write_word(0xf2, recovery_thresholds.ambient_overtemperature);
        }
        eeprom_write_word(0x3a, capacity_latch_flags);
        break;
    /* "00" if bit 1 of capacity_latch_flags is set, "01" otherwise, then the values. */
    case ASCII_CMD_GET_AMBIENT_TEMPERATURE:
        if (capacity_latch_flags & 0x02) {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('0');
        } else {
            ASCII_PUT_CHAR('0');
            ASCII_PUT_CHAR('1');
        }
        ASCII_PUT_HEX(limit_thresholds.ambient_undertemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.ambient_undertemperature);
        ASCII_PUT_HEX(protection_thresholds.ambient_undertemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.ambient_undertemperature);
        ASCII_PUT_HEX(recovery_thresholds.ambient_undertemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.ambient_undertemperature);
        ASCII_PUT_HEX(limit_thresholds.ambient_overtemperature >> 8);
        ASCII_PUT_HEX(limit_thresholds.ambient_overtemperature);
        ASCII_PUT_HEX(protection_thresholds.ambient_overtemperature >> 8);
        ASCII_PUT_HEX(protection_thresholds.ambient_overtemperature);
        ASCII_PUT_HEX(recovery_thresholds.ambient_overtemperature >> 8);
        ASCII_PUT_HEX(recovery_thresholds.ambient_overtemperature);
        break;
    /* Short-circuit current, capped at 120, and its delay. */
    case ASCII_CMD_SET_SHORT_CIRCUIT:
        value = ASCII_INFO_BYTE(0);
        value = value << 8;
        value = value + ASCII_INFO_BYTE(2);
        if (value > 120)
            value = 120;
        protection_thresholds.short_circuit_current = value;
        /* match: saves the stored field; saving value changes the code. */
        eeprom_write_word(0xd0, protection_thresholds.short_circuit_current);
        protection_debounce_settings[6] = ASCII_INFO_BYTE(4);
        eeprom_write_word(0xd2, protection_debounce_settings[6]);
        break;
    case ASCII_CMD_GET_SHORT_CIRCUIT:
        ASCII_PUT_HEX(protection_thresholds.short_circuit_current >> 8);
        ASCII_PUT_HEX(protection_thresholds.short_circuit_current);
        ASCII_PUT_HEX(protection_debounce_settings[6]);
        break;
    /* Eighth protection delay byte: the value is stored as 1 to 10 steps of two. */
    case ASCII_CMD_SET_DELAY_STEPS:
        value = ASCII_INFO_BYTE(0);
        if (value > 19)
            protection_debounce_settings[7] = 10;
        else if (value > 17)
            protection_debounce_settings[7] = 9;
        else if (value > 15)
            protection_debounce_settings[7] = 8;
        else if (value > 13)
            protection_debounce_settings[7] = 7;
        else if (value > 11)
            protection_debounce_settings[7] = 6;
        else if (value > 9)
            protection_debounce_settings[7] = 5;
        else if (value > 7)
            protection_debounce_settings[7] = 4;
        else if (value > 5)
            protection_debounce_settings[7] = 3;
        else if (value > 3)
            protection_debounce_settings[7] = 2;
        else
            protection_debounce_settings[7] = 1;
        eeprom_write_word(0xd4, protection_debounce_settings[7]);
        break;
    /* Eighth protection delay byte, as twice its step count. */
    case ASCII_CMD_GET_DELAY_STEPS:
        if (protection_debounce_settings[7] > 9)
            value = 20;
        else if (protection_debounce_settings[7] > 8)
            value = 18;
        else if (protection_debounce_settings[7] > 7)
            value = 16;
        else if (protection_debounce_settings[7] > 6)
            value = 14;
        else if (protection_debounce_settings[7] > 5)
            value = 12;
        else if (protection_debounce_settings[7] > 4)
            value = 10;
        else if (protection_debounce_settings[7] > 3)
            value = 8;
        else if (protection_debounce_settings[7] > 2)
            value = 6;
        else if (protection_debounce_settings[7] > 1)
            value = 4;
        else
            value = 2;
        ASCII_PUT_HEX(value);
        break;
    /* Restore defaults: clear the bank-0 marker (EEPROM 0x80) and both inhibit bytes, restore
       the factory limits and save bank 0. */
    case ASCII_CMD_RESTORE_DEFAULT_LIMITS:
        eeprom_write_word(0x80, 0);
        eeprom_write_word(0x30, 0);
        eeprom_write_word(0x3a, 0);
        protection_inhibit_flags = 0;
        capacity_latch_flags = 0;
        settings_restore_factory_limits();
        settings_save_limit_bank(0);
        break;
    /* Set the cell count: 8 to 16, saved at EEPROM 0x32; rejected if the write fails. */
    case ASCII_CMD_SET_CELL_COUNT:
        value = ASCII_INFO_BYTE(0);
        if (value <= 16 && value >= 8) {
            if (eeprom_write_word(0x32, value) == 0)
                rtn = ASCII_RTN_REJECTED;
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    /* Service actions by address: 1 sets latch bits 4 and 7, puts the recovery voltages 50
       inside the protection voltages and balances every cell; 2 clears the bank-1 marker
       (EEPROM 0x200) and saves bank 1; 0x50 writes the reset marker, delays the task and
       starts the window watchdog, which resets the chip. */
    case ASCII_CMD_SERVICE:
        if (address == 1) {
            current_direction_latch_flags |= 0x90;
            recovery_thresholds.cell_overvoltage = protection_thresholds.cell_overvoltage - 50;
            recovery_thresholds.cell_undervoltage = protection_thresholds.cell_undervoltage + 50;
            afe_balance_mask = 0;
            for (i = 0; i < measurement_cell_count; i++)
                afe_balance_mask |= 1 << i;
        } else if (address == 2) {
            eeprom_write_word(0x200, 0);
            settings_save_limit_bank(1);
        } else if (address == 0x50) {
            eeprom_write_word(BMS_EEPROM_RESET_MARKER, 0xaaaa);
            os_task_delay((uchar)os_task_dly_idx, 10, 10);
            WWDG_CR = 0x80;
        } else {
            rtn = ASCII_RTN_REJECTED;
        }
        break;
    default:
        rtn = ASCII_RTN_UNKNOWN_COMMAND;
        break;
    }
    ascii_set_rtn_field(uart4_frame, rtn);
    if ((cid2 == ASCII_CMD_GET_ANALOG_VALUES || cid2 == ASCII_CMD_GET_ALARM_STATES) &&
        address == 0xff && pack_count > 1) {
        ascii_set_length_field(uart4_frame, (uart4_tx_length - 17) * pack_count + 4);
        ascii_tx_checksum = 0;
        ascii_multi_pack_cid2 = cid2;
        for (i = 1; i < uart4_tx_length; i++)
            ascii_tx_checksum = uart4_frame[i] + ascii_tx_checksum;
    } else {
        if (uart4_tx_length >= 13)
            ascii_set_length_field(uart4_frame, uart4_tx_length - 13);
        for (i = 1; i < uart4_tx_length; i++)
            checksum = checksum + uart4_frame[i];
        checksum = ~(checksum % 65536) + 1;
        ASCII_PUT_HEX(checksum >> 8);
        ASCII_PUT_HEX(checksum);
        ASCII_PUT_CHAR(0x0d);
    }
    uart4_tx_index = 0;
    USART_ITConfig(UART4, UART4_IT_TXE, 1);
}

/* UART4 transmit interrupt step: sends the next reply byte. When a multi-pack 42 or 44 reply
   runs out, it appends the next pack's block and adds it to the running checksum; after the
   last pack it appends the negated checksum and a carriage return. When any other reply is
   sent, it disables the interrupt. */
void uart4_transmit_next_byte(void) {
    uchar i;
    ushort length;

    if (USART_GetITStatus(UART4, UART4_IT_TXE) != 0) {
        if (uart4_tx_index < uart4_tx_length) {
            UART4_DR = uart4_frame[uart4_tx_index++];
        } else {
            uart4_tx_index = 0;
            uart4_tx_length = 0;
            if (ascii_multi_pack_cid2 == ASCII_CMD_GET_ANALOG_VALUES ||
                ascii_multi_pack_cid2 == ASCII_CMD_GET_ALARM_STATES) {
                ascii_tx_pack_index++;
                if (ascii_tx_pack_index < pack_count) {
                    if (ascii_multi_pack_cid2 == ASCII_CMD_GET_ANALOG_VALUES)
                        ascii_append_analog_values(ascii_tx_pack_index, &uart4_tx_length);
                    else
                        ascii_append_alarm_states(ascii_tx_pack_index, &uart4_tx_length);
                    i = 0;
                    length = uart4_tx_length;
                    for (; i < length; i++)
                        ascii_tx_checksum = uart4_frame[i] + ascii_tx_checksum;
                    if (ascii_tx_pack_index + 1 == pack_count) {
                        ascii_tx_pack_index = 0;
                        ascii_multi_pack_cid2 = 0;
                        ascii_tx_checksum = ~ascii_tx_checksum + 1;
                        ASCII_PUT_HEX(ascii_tx_checksum >> 8);
                        ASCII_PUT_HEX(ascii_tx_checksum);
                        ASCII_PUT_CHAR(0x0d);
                    }
                } else {
                    ascii_tx_pack_index = 0;
                    ascii_multi_pack_cid2 = 0;
                }
            } else {
                ascii_tx_pack_index = 0;
                USART_ITConfig(UART4, UART4_IT_TXE, 0);
            }
        }
    }
}

/* Copies this pack's model code, version text, readings, flag bytes, current limits and
   firmware version (0x0BCE for version 3022) into peer record 0. Only the pack at address 0
   calls it. */
void peer_record_publish_own(void) {
    uchar i;

    for (i = 0; i < 21; i++)
        protocol_pack[0].model_code[i] = model_code[i];
    for (i = 0; i < 21; i++)
        protocol_pack[0].software_version_text[i] = software_version_text[i];
    protocol_pack[0].reply_seen = 1;
    protocol_pack[0].cell_count = measurement_cell_count;
    protocol_pack[0].cycle_count = cycle_count;
    protocol_pack[0].auxiliary_voltage = auxiliary_voltage_reading;
    protocol_pack[0].pack_voltage = measurement_pack_voltage;
    protocol_pack[0].pack_current = measurement_current;
    protocol_pack[0].full_capacity = full_capacity;
    protocol_pack[0].rated_capacity = measurement_rated_capacity;
    protocol_pack[0].remaining_capacity = remaining_capacity;
    protocol_pack[0].soc_primary = primary_soc;
    protocol_pack[0].soc_secondary = displayed_soc;
    protocol_pack[0].status_flags[0] = voltage_current_protection_flags;
    protocol_pack[0].status_flags[1] = temperature_protection_flags;
    protocol_pack[0].status_flags[2] = power_output_status_flags;
    protocol_pack[0].status_flags[3] = current_direction_latch_flags;
    protocol_pack[0].status_flags[4] = hardware_fault_flags;
    protocol_pack[0].status_flags[5] = protection_inhibit_flags;
    protocol_pack[0].status_flags[6] = capacity_latch_flags;
    protocol_pack[0].status_flags[7] = voltage_current_limit_flags;
    protocol_pack[0].status_flags[8] = temperature_limit_flags;
    protocol_pack[0].maximum_cell_voltage = measurement_maximum_cell_voltage;
    protocol_pack[0].minimum_cell_voltage = measurement_minimum_cell_voltage;
    for (i = 0; i < 16; i++)
        protocol_pack[0].cell_voltages[i] = compensated_cell_voltages[i];
    protocol_pack[0].maximum_temperature = measurement_maximum_temperature;
    protocol_pack[0].minimum_temperature = measurement_minimum_temperature;
    protocol_pack[0].firmware_version = 0xbce;
    for (i = 0; i < 5; i++)
        protocol_pack[0].temperatures[i] = temperature_readings[i];
    protocol_pack[0].usb_class = usb_active_class;
    protocol_pack[0].charge_current_limit = charge_current_limit / 10;
    protocol_pack[0].discharge_current_limit = discharge_current_limit / 10;
    protocol_pack[0].participation_state = capacity_operating_status;
    if (lower_voltage_current_hold == 1)
        protocol_pack[0].voltage_hold_mode = 1;
    else if (upper_voltage_current_hold == 1)
        protocol_pack[0].voltage_hold_mode = 2;
    else
        protocol_pack[0].voltage_hold_mode = 0;
    protocol_pack[0].output_phase_toggle = charge_phase_toggle;
    protocol_pack[0].soc_voltage_hold = soc_voltage_hold;
}

/* Applies the pending settings written for this pack (rated, full and remaining capacity,
   cell count), saving each to EEPROM, then clears the other 28 flag bits one by one. */
void pending_settings_apply_own(void) {
    if (protocol_pending[pack_address].flags & 1) {
        protocol_pending[pack_address].flags &= ~1;
        measurement_rated_capacity = protocol_pending[pack_address].rated_capacity;
        eeprom_write_word(0x0c, measurement_rated_capacity);
    }
    if (protocol_pending[pack_address].flags & 2) {
        protocol_pending[pack_address].flags &= ~2;
        full_capacity = protocol_pending[pack_address].full_capacity;
        eeprom_write_word(0x08, full_capacity);
    }
    if (protocol_pending[pack_address].flags & 4) {
        protocol_pending[pack_address].flags &= ~4;
        remaining_capacity = protocol_pending[pack_address].remaining_capacity;
        eeprom_write_word(0x06, remaining_capacity);
    }
    if (protocol_pending[pack_address].flags & 8) {
        protocol_pending[pack_address].flags &= ~8;
        measurement_cell_count = protocol_pending[pack_address].cell_count;
        eeprom_write_word(0x32, measurement_cell_count);
    }
    if (protocol_pending[pack_address].flags & 0x10)
        protocol_pending[pack_address].flags &= ~0x10;
    if (protocol_pending[pack_address].flags & 0x20)
        protocol_pending[pack_address].flags &= ~0x20;
    if (protocol_pending[pack_address].flags & 0x40)
        protocol_pending[pack_address].flags &= ~0x40;
    if (protocol_pending[pack_address].flags & 0x80)
        protocol_pending[pack_address].flags &= ~0x80;
    if (protocol_pending[pack_address].flags & 0x100)
        protocol_pending[pack_address].flags &= ~0x100;
    if (protocol_pending[pack_address].flags & 0x200)
        protocol_pending[pack_address].flags &= ~0x200;
    if (protocol_pending[pack_address].flags & 0x400)
        protocol_pending[pack_address].flags &= ~0x400;
    if (protocol_pending[pack_address].flags & 0x800)
        protocol_pending[pack_address].flags &= ~0x800;
    if (protocol_pending[pack_address].flags & 0x1000)
        protocol_pending[pack_address].flags &= ~0x1000;
    if (protocol_pending[pack_address].flags & 0x2000)
        protocol_pending[pack_address].flags &= ~0x2000;
    if (protocol_pending[pack_address].flags & 0x4000)
        protocol_pending[pack_address].flags &= ~0x4000;
    if (protocol_pending[pack_address].flags & 0x8000)
        protocol_pending[pack_address].flags &= ~0x8000;
    if (protocol_pending[pack_address].flags & 0x10000)
        protocol_pending[pack_address].flags &= ~0x10000;
    if (protocol_pending[pack_address].flags & 0x20000)
        protocol_pending[pack_address].flags &= ~0x20000;
    if (protocol_pending[pack_address].flags & 0x40000)
        protocol_pending[pack_address].flags &= ~0x40000;
    if (protocol_pending[pack_address].flags & 0x80000)
        protocol_pending[pack_address].flags &= ~0x80000;
    if (protocol_pending[pack_address].flags & 0x100000)
        protocol_pending[pack_address].flags &= ~0x100000;
    if (protocol_pending[pack_address].flags & 0x200000)
        protocol_pending[pack_address].flags &= ~0x200000;
    if (protocol_pending[pack_address].flags & 0x400000)
        protocol_pending[pack_address].flags &= ~0x400000;
    if (protocol_pending[pack_address].flags & 0x800000)
        protocol_pending[pack_address].flags &= ~0x800000;
    if (protocol_pending[pack_address].flags & 0x1000000)
        protocol_pending[pack_address].flags &= ~0x1000000;
    if (protocol_pending[pack_address].flags & 0x2000000)
        protocol_pending[pack_address].flags &= ~0x2000000;
    if (protocol_pending[pack_address].flags & 0x4000000)
        protocol_pending[pack_address].flags &= ~0x4000000;
    if (protocol_pending[pack_address].flags & 0x8000000)
        protocol_pending[pack_address].flags &= ~0x8000000;
    if (protocol_pending[pack_address].flags & 0x10000000)
        protocol_pending[pack_address].flags &= ~0x10000000;
    if (protocol_pending[pack_address].flags & 0x20000000)
        protocol_pending[pack_address].flags &= ~0x20000000;
    if (protocol_pending[pack_address].flags & 0x40000000)
        protocol_pending[pack_address].flags &= ~0x40000000;
    if (protocol_pending[pack_address].flags & 0x80000000)
        protocol_pending[pack_address].flags &= ~0x80000000;
}

/* Periodic UART4 task. The pack at address 0 publishes its own peer record and every pack
   applies its pending settings. In ASCII mode a ready frame is checked (frame checksum, then
   length checksum) and dispatched; a failed check is answered with its error code. In the
   other mode a ready frame is handled on every fifth call: with a matching CRC, an address
   below 16 and function 3, 4 or 6, the reply is built and sent with serial_send (function 4
   gets a reply with no register data); otherwise the frame buffer is cleared. */
void uart4_protocol_task(void) {
    uchar crc_ok = 0;
    uchar cid2;
    uchar i;

    if (pack_address == 0)
        peer_record_publish_own();
    pending_settings_apply_own();
    if (uart4_ascii_mode == 1) {
        if (uart4_frame_ready == 0)
            return;
        uart4_frame_ready = 0;
        cid2 = ascii_hex_to_byte(uart4_frame[7], uart4_frame[8]);
        if (ascii_frame_checksum_valid(uart4_frame) == 0)
            ascii_dispatch_command(ASCII_CMD_FRAME_CHECKSUM_ERROR);
        else if (ascii_length_checksum_valid(uart4_frame) == 0)
            ascii_dispatch_command(ASCII_CMD_LENGTH_CHECKSUM_ERROR);
        else
            ascii_dispatch_command(cid2);
        return;
    }
    if (modbus_poll_delay <= 3) {
        modbus_poll_delay++;
        return;
    }
    if (uart4_frame_ready == 0)
        return;
    uart4_frame_ready = 0;
    modbus_poll_delay = 0;
    modbus_rx_crc = protocol_crc16(uart4_frame, 7);
    if ((uchar)modbus_rx_crc == uart4_frame[7] && uart4_frame[8] == modbus_rx_crc >> 8)
        crc_ok = 1;
    if (crc_ok != 1) {
        for (i = 0; i < 170; i++)
            uart4_frame[i] = 0;
        return;
    }
    if (uart4_frame[1] < 0x10) {
        modbus_peer_index = uart4_frame[1];
        if (uart4_frame[2] == BMS_MODBUS_READ_HOLDING_REGISTERS ||
            uart4_frame[2] == BMS_MODBUS_READ_INPUT_REGISTERS ||
            uart4_frame[2] == BMS_MODBUS_WRITE_SINGLE_REGISTER) {
            modbus_reply[0] = uart4_frame[1];
            modbus_reply[1] = uart4_frame[2];
            if (uart4_frame[2] == BMS_MODBUS_READ_HOLDING_REGISTERS)
                uart4_modbus_read_peer_registers(uart4_frame);
            else if (uart4_frame[2] == BMS_MODBUS_READ_INPUT_REGISTERS)
                ;
            else if (uart4_frame[2] == BMS_MODBUS_WRITE_SINGLE_REGISTER)
                uart4_modbus_write_peer_setting(uart4_frame);
            if (uart4_frame[2] == BMS_MODBUS_WRITE_SINGLE_REGISTER) {
                modbus_reply_length = 8;
            } else {
                ushort data_length = (uart4_frame[5] * 256 + uart4_frame[6]) * 2;
                modbus_reply[2] = data_length >> 8;
                modbus_reply[3] = data_length;
                modbus_reply_length = data_length + 6;
            }
            for (i = 0; i < modbus_reply_length; i++)
                modbus_tx_frame[i] = modbus_reply[i];
            serial_send(3, modbus_tx_frame, modbus_reply_length);
            USART_ITConfig(UART4, UART4_IT_TXE, 1);
        } else {
            for (i = 0; i < 170; i++)
                uart4_frame[i] = 0;
        }
    } else {
        for (i = 0; i < 170; i++)
            uart4_frame[i] = 0;
    }
}

/* at: ascii_following_pool 0x080211C8, ascii_watchdog_base 0x40002C00, ascii_uart4_base 0x40004C00 */
