/* 3021/3022 HR1 recovery enable and stage telemetry. */
/* at: heating_inhibit 0x200000FE, heating_enabled 0x20000196, heating_stage_bytes 0x20000197 */
extern unsigned char heating_inhibit;
extern unsigned char heating_enabled;
#include "../../include/bms_heating.h"
/* at: battery_heating 0x20000196 */
/* at: coordination_initialize_registers 0x08021530 */
/* section: .constdata 0x08030BDC */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow inter-pack framing, register publication and coordination here.
   This unit owns packet progress, CRC state and cached control bytes;
   serial transport, runtime timers and register_protocol.c are its neighbours.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/bms_timers.h"
#include "../../include/bms_dates.h"
/* at: g_200039A8 0x200039C0, g_20000077 0x20000077, g_2000017C 0x20000180 */
/* at: date_now 0x20000190, g_20000148 0x2000014C, g_20001578 0x20001590 */
/* at: g_200000E8 0x200000E8, g_200000E9 0x200000E9, g_2000013B 0x2000013F */
/* at: g_2000050C 0x20000518, g_2000013C 0x20000140, g_200000CE 0x200000CE */
/* at: g_2000009D 0x2000009D, g_2000013D 0x20000141, g_20000273 0x2000027B */
/* at: g_20000271 0x20000279, g_200000D9 0x200000D9, g_200000D8 0x200000D8 */
/* at: g_20000524 0x20000530, g_20003D52 0x20003D6A, g_20000272 0x2000027A */
/* at: g_20000180 0x20000184, g_20000110 0x20000114, g_2000119E 0x200011B6 */
/* at: g_20001522 0x2000153A, g_2000013E 0x20000142, g_20000140 0x20000144 */
/* at: g_20001094 0x200010AC */
extern ushort g_200039A8[];
extern uchar g_20000077[], g_20000524[], g_20003D52[];
extern float g_2000017C, g_20000148;
extern uchar g_200000E8, g_200000E9, g_2000013B, g_2000050C, g_2000013C;
extern uchar g_2000009D, g_2000013D, g_200000D9, g_200000D8;
extern volatile uchar g_20000273, g_20000271, g_20000272;
extern ushort g_200000CE, g_20000180, g_20000110, g_2000013E, g_20000140;
extern ushort g_2000119E[], g_20001522[], g_20001094[], g_20001578[];
extern void runtime_publish_date(void);
extern uint bcd_to_byte(uint value);
extern uint eeprom_write_word(uint offset, uint value);
extern uint __aeabi_f2uiz(float value);
extern int __aeabi_f2iz(float value);

extern uchar cached_discharge, cached_charge;

/* Initializes the published register bank and model identification. */
void coordination_initialize_registers(void) {
    uchar i = 0;
    do {
        g_200039A8[i] = 0xffff;
    } while (++i < 200);
    g_200039A8[0] = 0x65;
    i = 0;
    do {
        g_200039A8[i + 5] = (g_20000077[2 * i] << 8) + g_20000077[2 * i + 1];
    } while (++i < 5);
    g_200039A8[11] = __aeabi_f2uiz(g_2000017C / 100.0f);
    g_200039A8[12] = 0x30;
}

/* Publishes timestamp, pack status, limits and cell samples. */
void coordination_update_registers(void) {
    /* match: Acquire the date view after publication; volatile reads preserve
       the native load order. The original declaration spelling is unknown. */
    volatile date_t *date;
    uint stamp;
    ushort status = 0;
    float current;
    uchar i;
    uint maximum;
    runtime_publish_date();
    date = &date_now;
    stamp = bcd_to_byte(date->d[0]) * 16;
    stamp = (stamp + bcd_to_byte(date->d[1])) * 32;
    stamp = (stamp + bcd_to_byte(date->d[2])) * 32;
    stamp = (stamp + bcd_to_byte(date->d[3])) * 64;
    stamp = (stamp + bcd_to_byte(date->d[4])) * 64;
    stamp += bcd_to_byte(date->d[5]);
    g_200039A8[11] = __aeabi_f2uiz(g_2000017C / 100.0f);
    g_200039A8[13] = 0xbce;
    if (g_200000E8)
        g_200000E9 = 1;
    else
        g_200000E9 = 0;
    g_200039A8[14] = g_200000E9;
    if (g_2000013B != 0) status = 1;
    if (battery_heating.positive_current_stage != 0) {
        status |= 2;
    } else if (battery_heating.negative_current_stage != 0) {
        status |= 4;
    }
    g_200039A8[15] = status;
    g_200039A8[16] = g_2000050C;
    g_200039A8[17] = stamp;
    g_200039A8[18] = stamp >> 16;
    current = g_20000148;
    if (current > 0.0f)
        status = 2;
    else if (current < 0.0f)
        status = 3;
    else
        status = 1;
    status |= 4;
    if (g_2000013C)
        status &= ~4;
    if (g_200000CE == 0)
        status |= 8;
    if (g_2000009D)
        status |= 16;
    if (g_2000013D && !g_2000013C)
        status |= 32;
    if (g_20000273 & 4)
        status |= 64;
    if (g_20000273 & 2)
        status |= 128;
    g_200039A8[19] = status;
    if (g_20000271 & 32)
        status = 1;
    else
        status = 0;
    if (g_20000271 & 64)
        status |= 2;
    if (g_200000D9 == 1 || cached_discharge == 1) {
        status |= 4;
        cached_discharge = g_200000D9;
        cached_charge = 0;
        i = 0;
        do {
            if (g_20003D52[i * 145 + 0x8d]) {
                g_20003D52[i * 145 + 0x86] = 0;
                g_20003D52[i * 145 + 0x84] = 0;
            }
        } while (++i < 6);
    }
    if (g_200000D8 == 1 || cached_charge == 1) {
        status |= 8;
        cached_charge = g_200000D8;
        cached_discharge = 0;
        i = 0;
        do {
            if (g_20003D52[i * 145 + 0x8d]) {
                g_20003D52[i * 145 + 0x86] = 0;
                g_20003D52[i * 145 + 0x84] = 0;
            }
        } while (++i < 6);
    }
    if (g_20000272 & 2)
        status |= 16;
    if (g_20000272 & 1)
        status |= 32;
    if (g_20000272 & 8)
        status |= 64;
    if (g_20000272 & 4)
        status |= 128;
    g_200039A8[20] = status;
    g_200039A8[21] = g_20000180;
    g_200039A8[22] = g_20000110 / 10;
    g_200039A8[23] = __aeabi_f2iz(current / 10.0f);
    maximum = g_2000119E[0];
    i = 0;
    do {
        if (maximum < g_2000119E[i])
            maximum = g_2000119E[i];
    } while (++i < 4);
    g_200039A8[24] = ((int)maximum - 0xaaa) / 10;
    g_200039A8[25] = g_20001522[4] * 100;
    g_200039A8[26] = g_2000013E;
    g_200039A8[27] = g_20000140;
    i = 0;
    do {
        g_200039A8[i + 0x71] = g_20001094[i];
    } while (++i < 16);
}

/* Increments an EEPROM event count and publishes its timestamp. */
void coordination_record_event(uint channel) {
    uint stamp;
    runtime_publish_date();
    stamp = bcd_to_byte(date_now.d[0]) * 16;
    stamp = (stamp + bcd_to_byte(date_now.d[1])) * 32;
    stamp = (stamp + bcd_to_byte(date_now.d[2])) * 32;
    stamp = (stamp + bcd_to_byte(date_now.d[3])) * 64;
    stamp = (stamp + bcd_to_byte(date_now.d[4])) * 64;
    stamp += bcd_to_byte(date_now.d[5]);
    if (channel == 0) {
        g_20001578[0]++;
        eeprom_write_word(0x500, g_20001578[0]);
        g_200039A8[32] = g_20001578[0];
        g_200039A8[33] = stamp;
        g_200039A8[34] = stamp >> 16;
    } else if (channel == 1) {
        g_20001578[1]++;
        eeprom_write_word(0x502, g_20001578[1]);
        g_200039A8[35] = g_20001578[1];
        g_200039A8[36] = stamp;
        g_200039A8[37] = stamp >> 16;
    } else if (channel == 2) {
        g_20001578[2]++;
        eeprom_write_word(0x504, g_20001578[2]);
        g_200039A8[48] = g_20001578[2];
        g_200039A8[50] = stamp;
        g_200039A8[51] = stamp >> 16;
    } else if (channel == 3) {
        g_20001578[3]++;
        eeprom_write_word(0x506, g_20001578[3]);
        g_200039A8[49] = g_20001578[3];
        g_200039A8[52] = stamp;
        g_200039A8[53] = stamp >> 16;
    } else if (channel == 4) {
        g_20001578[4]++;
        eeprom_write_word(0x508, g_20001578[4]);
        g_200039A8[54] = g_20001578[4];
        g_200039A8[56] = stamp;
        g_200039A8[57] = stamp >> 16;
    } else if (channel == 5) {
        g_20001578[5]++;
        eeprom_write_word(0x50A, g_20001578[5]);
        g_200039A8[55] = g_20001578[5];
        g_200039A8[58] = stamp;
        g_200039A8[59] = stamp >> 16;
    } else if (channel == 6) {
        g_20001578[6]++;
        eeprom_write_word(0x50C, g_20001578[6]);
        g_200039A8[64] = g_20001578[6];
        g_200039A8[66] = stamp;
        g_200039A8[67] = stamp >> 16;
    } else if (channel == 7) {
        g_20001578[7]++;
        eeprom_write_word(0x50E, g_20001578[7]);
        g_200039A8[65] = g_20001578[7];
        g_200039A8[68] = stamp;
        g_200039A8[69] = stamp >> 16;
    } else if (channel == 8) {
        g_20001578[8]++;
        eeprom_write_word(0x510, g_20001578[8]);
        g_200039A8[70] = g_20001578[8];
        g_200039A8[72] = stamp;
        g_200039A8[73] = stamp >> 16;
    } else if (channel == 9) {
        g_20001578[9]++;
        eeprom_write_word(0x512, g_20001578[9]);
        g_200039A8[71] = g_20001578[9];
        g_200039A8[74] = stamp;
        g_200039A8[75] = stamp >> 16;
    }
    if (channel == 10) {
        g_20001578[10]++;
        eeprom_write_word(0x514, g_20001578[10]);
        g_200039A8[80] = g_20001578[10];
        g_200039A8[81] = stamp;
        g_200039A8[82] = stamp >> 16;
    } else if (channel == 11) {
        g_20001578[11]++;
        eeprom_write_word(0x516, g_20001578[11]);
        g_200039A8[83] = g_20001578[11];
        g_200039A8[84] = stamp;
        g_200039A8[85] = stamp >> 16;
    } else if (channel == 12) {
        g_20001578[12]++;
        eeprom_write_word(0x518, g_20001578[12]);
        g_200039A8[96] = g_20001578[12];
        g_200039A8[97] = stamp;
        g_200039A8[98] = stamp >> 16;
    }
}

/* Complete chain protocol unit. The first three functions also have an independent prefix proof. */
/* at: coordination_crc16 0x08021A48, coordination_receive_byte 0x08021A96, coordination_transmit_interrupt 0x08021D3A, coordination_reply_registers 0x08021D7E, coordination_reply_paged_registers 0x08021EB8, coordination_poll 0x0802216A */
/* at: g_20000040 0x20000040, g_2000004D 0x2000004D, g_200000DA 0x200000DA, g_200000DF 0x200000DF, g_200000E0 0x200000E0, g_200000E1 0x200000E1, g_200000E8 0x200000E8, g_200000E9 0x200000E9, g_200000F0 0x200000F2, g_200000F2 0x200000F4, g_20000163 0x20000167, g_20000279 0x20000281, g_20000524 0x20000530, g_20000606 0x2000061E, g_20000F9C 0x20000FB4, g_20003843 0x2000385B, g_20003844 0x2000385C, g_200039A8 0x200039C0, g_20003B38 0x20003B50, g_20003D52 0x20003D6A */
/* at: firmware_update_payload 0x2000164F */
/* at: chain_tx 0x200038C0 */
/* at: tx_length 0x20000530, tx_sent 0x20000531, packet_done 0x20000532, packet_valid 0x20000533, exception_code 0x20000534, packet_timeout 0x20000535, reserved_052a 0x20000536, register_count 0x20000538, start_register 0x2000053A, rx_phase_count 0x2000053C, rx_length 0x2000053E, rx_state 0x20000540, packet_crc 0x20000542, update_prefix 0x20000544, reserved_053b 0x20000547, uart_packets 0x20000548 */
/* at: cached_discharge 0x20000536, cached_charge 0x20000537 */

#include "../../include/ghidra_types.h"

#include "../../include/stdperiph_types.h"

/* Initialized UART state; all 28 decoded bytes are zero. */
uchar tx_length = 0;
uchar tx_sent = 0;
uchar packet_done = 0;
uchar packet_valid = 0;
uchar exception_code = 0;
uchar packet_timeout = 0;
uchar cached_discharge = 0;
uchar cached_charge = 0;
ushort register_count = 0;
ushort start_register = 0;
volatile ushort rx_phase_count = 0;
ushort rx_length = 0;
volatile ushort rx_state = 0;
ushort packet_crc = 0;
uchar update_prefix[3] = {0};
uint uart_packets = 0;
/* Buffers owned by this source unit, verified with the complete linked object. */
uchar g_20003844[100];
uchar chain_tx[256];
ushort g_200039A8[200];
uchar g_20003B38[130];
void __aeabi_memcpy8(void *, const void *, uint);
void runtime_timer_start(int, undefined4);
uchar USART_GetITStatus(void *, ushort);
void USART_ITConfig(void *, ushort, uchar);
void os_task_delay(int, ushort, ushort);
void board_set_pb2(int);
void runtime_timer_stop(int);
void coordination_reply_paged_registers(uchar *request, uint function_code);
void coordination_transmit_interrupt();
uint coordination_crc16(byte *data, uint length);
void coordination_poll();
void coordination_receive_byte(uchar received_byte);
void coordination_reply_registers(int function_code);

extern uchar c_08030798[];

/* Computes nibble-table CRC16 over a byte buffer.
   match: The sixteen-entry nibble table remains a local initializer. */
uint coordination_crc16(byte *data, uint length) {
    ushort byte_index = 0;
    uint crc = 0xffff;
    ushort nibble_table[16] = {0x0,    0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
                               0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400};
    uchar received;
    uint half_crc;

    for (; byte_index < length; byte_index = byte_index + 1 & 0xffff) {
        received = *data++;
        half_crc = (uint)nibble_table[(received ^ crc) & 0xf] ^ crc >> 4;
        crc = (uint)nibble_table[(half_crc ^ received >> 4) & 0xf] ^ half_crc >> 4;
    }
    return crc;
}

extern uchar g_20000040[];
#define DAT_20000040 (*(int *)(g_20000040 + 0))
extern uchar g_200000DF[];
#define DAT_200000df (*(uchar *)(g_200000DF + 0))
extern uchar g_200000E0[];
#define DAT_200000e0 (*(uchar *)(g_200000E0 + 0))
/* at: fw_length_ready 0x200000E5, fw_received 0x200000EC */
extern uchar fw_length_ready;
extern uchar g_20000163[];
extern ushort fw_received;
#define DAT_200000e5 (*(uchar *)(&fw_length_ready))
#define DAT_200000ec (*(ushort *)(&fw_received))
#define DAT_20000163 (*(uchar *)(g_20000163 + 0))
extern uchar g_20000524[];
#define DAT_20000526 (packet_done)
#define DAT_20000527 (packet_valid)
#define DAT_2000052c (register_count)
#define DAT_20000530 (rx_phase_count)
#define DAT_20000532 (rx_length)
#define DAT_20000534 (rx_state)
#define DAT_20000536 (packet_crc)
#define DAT_2000053c (uart_packets)
#define DAT_2000105d (g_20000F9C[24].elapsed)
extern uchar g_20003843[];
extern uchar g_20003844[];
#define DAT_20003845 (*(uchar *)(g_20003844 + 1))
#define DAT_20003848 (*(uchar *)(g_20003844 + 4))
#define DAT_20003849 (*(uchar *)(g_20003844 + 5))
#define g_20003844_v (*(uchar *)(g_20003844 + 0))

extern uchar firmware_update_payload[];
/* Parses firmware-prefix and register frames one received byte at a time.
   match: I retain the selected-prefix pointer assignment inside the state test. */
void coordination_receive_byte(uchar received_byte) {
    uint expected_address;

    uchar received_char;
    ushort position;
    ushort received_length;
    ushort byte_index;
    ushort checksum;

    uchar *header;
    ushort state;
    checksum = 0;
    byte_index = 0;
    if (DAT_2000105d != '\0') {
        DAT_20000534 = 0;
        DAT_20000532 = 0;
    }
    runtime_timer_start(0x18, 5);
    received_char = (char)received_byte;
    if ((DAT_200000df == '\x01') && (DAT_200000e0 == '\0')) {
        if ((header = update_prefix, (position = (uint)DAT_20000530) == 0)) {
            if ((received_byte == 0x48) || (received_byte == 0x45)) {
                header[0] = received_char;
                DAT_20000530 = 1;
            } else {
                DAT_20000530 = 0;
            }
        } else if (position == 1) {
            if ((received_byte == 0x45) || (received_byte == 0x4e)) {
                update_prefix[1] = received_char;
                DAT_20000530 = 2;
            } else {
                DAT_20000530 = 0;
            }
        } else if (position == 2) {
            if (received_byte == 0x44) {
                update_prefix[2] = received_byte;
                DAT_20000530 = 3;
            } else {
                DAT_20000530 = 0;
            }
            DAT_200000ec = 0;
        } else {
            firmware_update_payload[position - 3] = received_char;
            if (((update_prefix[0] == 'E') && (update_prefix[1] == 'N')) &&
                (update_prefix[2] == 'D')) {
                DAT_20000530 = ++position;
                if (position >= 7) {
                    DAT_20000040 = (uint)firmware_update_payload[0] * 0x1000000 +
                                   (uint)firmware_update_payload[1] * 0x10000 +
                                   (uint)firmware_update_payload[2] * 0x100 +
                                   (uint)firmware_update_payload[3];
                    DAT_200000e5 = 1;
                    DAT_20000530 = 0;
                }
            } else {
                received_length = ++position;
                {
                    uint frame_limit = 0x405;
                    DAT_20000530 = received_length;
                    if (received_length >= frame_limit) {
                        do {
                            checksum = checksum + firmware_update_payload[byte_index];
                            byte_index = byte_index + 1 & 0xffff;
                        } while (byte_index < 0x400);
                        if (received_length > 0x40d) {
                            DAT_20000530 = 0x40d;
                        }
                        received_length = DAT_20000530;
                        DAT_20000536 = firmware_update_payload[received_length - 5];
                        DAT_20000536 <<= 8;
                        DAT_20000536 += firmware_update_payload[received_length - 4];
                        if (DAT_20000536 == checksum) {
                            DAT_200000e0 = '\x01';
                        }
                        DAT_20000530 = 0;
                    }
                }
            }
        }
    } else {
        state = DAT_20000534;
        switch (state) {
        case 0:
            expected_address = DAT_20000163 + 1;

            if (received_byte == expected_address || (received_byte <= 6)) {
                DAT_20000530 = 0;
                DAT_20000532 = 1;
                g_20003844_v = received_char;
                DAT_20000534 = state + 1;
                if (received_byte > 2) {
                    DAT_2000053c = DAT_2000053c + 1;
                }
            } else {
                DAT_20000534 = 6;
            }
            break;
        case 1:
            byte_index = DAT_20000532++;
            (&g_20003844_v)[byte_index] = received_char;
            DAT_20000534 = state + 1;
            if (received_byte == 4) {
                DAT_2000053c = DAT_2000053c + 1;
            }
            break;
        case 2:
            byte_index = DAT_20000532++;
            (&g_20003844_v)[byte_index] = received_char;
            if (++DAT_20000530 >= 2) {
                DAT_20000530 = 0;
                DAT_20000534 = state + 1;
            }
            break;
        case 3:
            byte_index = DAT_20000532++;
            (&g_20003844_v)[byte_index] = received_char;
            if (++DAT_20000530 >= 2) {
                DAT_20000530 = 0;
                DAT_2000052c = DAT_20003848;
                DAT_2000052c <<= 8;
                DAT_2000052c += DAT_20003849;
                if (DAT_20003845 == '\x10') {
                    DAT_20000534 = 5;
                } else {
                    DAT_20000534 = 4;
                }
            }
            break;
        case 5:
            byte_index = DAT_20000532++;
            (&g_20003844_v)[byte_index] = received_char;
            if ((uint)++DAT_20000530 >= (uint)DAT_2000052c * 2 + 1) {
                DAT_20000530 = 0;
                DAT_20000534 = 4;
            }
            break;
        case 4:
            received_length = (uint)DAT_20000532;
            byte_index = received_length + 1;
            position = byte_index & 0xffff;
            DAT_20000532 = (ushort)byte_index;
            (&g_20003844_v)[received_length] = received_char;
            if (++DAT_20000530 >= 2) {
                DAT_20000530 = 0;
                DAT_20000526 = 1;
                DAT_20000534 = 0;
                DAT_20000536 = g_20003844[position - 1];
                DAT_20000536 <<= 8;
                DAT_20000536 += g_20003844[position - 2];
                if ((coordination_crc16((byte *)&g_20003844_v, position - 2 & 0xffff)) ==
                    DAT_20000536) {
                    DAT_20000527 = 1;
                }
            }
            break;
        }
    }
    return;
}
#undef DAT_20000040
#undef DAT_200000df
#undef DAT_200000e0
#undef DAT_200000e5
#undef DAT_200000ec
#undef DAT_20000163
#undef DAT_20000526
#undef DAT_20000527
#undef DAT_2000052c
#undef DAT_20000530
#undef DAT_20000532
#undef DAT_20000534
#undef DAT_20000536
#undef DAT_2000053c
#undef DAT_2000105d
#undef DAT_20003845
#undef DAT_20003848
#undef DAT_20003849
#undef g_20003844_v

#define DAT_40004804 (*(volatile ushort *)0x40004804)
extern uchar g_2000004D[];
#define DAT_2000004d (*(uchar *)(g_2000004D + 0))
extern uchar g_20000524[];
extern uchar g_20003844[];

/* Feeds UART4 transmit data until the queued frame is complete. */
void coordination_transmit_interrupt() {
    if (USART_GetITStatus((void *)0x40004800, 0x727)) {
        if (tx_sent < tx_length) {
            DAT_40004804 = chain_tx[tx_sent++];
        } else {
            USART_ITConfig((void *)0x40004800, 0x727, 0);
            tx_sent = 0;
            DAT_2000004d = 1;
        }
    }
}

#undef DAT_40004804
#undef DAT_2000004d

extern uchar g_200000DF[];
#define DAT_200000df (*(uchar *)(g_200000DF + 0))
extern uchar g_200000E8;
#define DAT_200000e8 (*(uchar *)(&g_200000E8 + 0))
extern uchar g_200000E9;
#define DAT_200000e9 (*(uchar *)(&g_200000E9 + 0))
extern uchar g_20000163[];
#define DAT_20000163 (*(uchar *)(g_20000163 + 0))
extern uchar g_20000524[];
extern uchar g_20000606[];
#define os_task_dly_idx (*(ushort *)(g_20000606 + 0))
extern uchar g_20003844[];
extern ushort g_200039A8[];
#define g_200039A8_v (*(ushort *)(g_200039A8 + 0))

/* Builds normal or exception register responses and queues transmission. */
void coordination_reply_registers(int function_code) {

    uint enabled;
    uint i;
    uchar next;
    ushort start, count, value;
    uint crc;

    tx_length = 0;
    chain_tx[tx_length++] = DAT_20000163 + 1;
    chain_tx[tx_length++] = function_code;
    i = 0;
    do {
        os_task_delay((uchar)os_task_dly_idx, 6, 10);
        i = (i + 1) & 0xff;
    } while (i < 5);
    if (function_code == 3) {
        chain_tx[tx_length++] = register_count * 2;
        if (register_count > 199)
            register_count = 199;
        for (i = (uchar)start_register; i < start_register + register_count; i = (i + 1) & 0xff) {
            chain_tx[tx_length++] = (&g_200039A8_v)[i] >> 8;
            chain_tx[tx_length++] = (&g_200039A8_v)[i];
        }
        enabled = 1;
        if (DAT_200000e9) {
            DAT_200000df = enabled;
            DAT_200000e8 = 0;
            DAT_200000e9 = 0;
        }
    } else {
        if (tx_length > 254)
            tx_length = 254;
        chain_tx[tx_length++] = exception_code;
    }
    if (tx_length > 253)
        tx_length = 253;
    crc = coordination_crc16(chain_tx, tx_length);
    value = tx_length;
    next = value + 1;
    chain_tx[value] = crc;
    crc >>= 8;
    value = next;
    next++;
    tx_length = next;
    chain_tx[value] = crc;
    board_set_pb2(1);
    os_task_delay((uchar)os_task_dly_idx, 5, 10);
    tx_sent = 0;
    USART_ITConfig((void *)0x40004800, 0x727, 1);
}

#undef DAT_200000df
#undef DAT_200000e8
#undef DAT_200000e9
#undef DAT_20000163
#undef os_task_dly_idx
#undef g_200039A8_v

#define DAT_40004800 (*(volatile ushort *)0x40004800)
extern uchar g_20000524[];
#define DAT_20000524 (tx_length)
#define DAT_20000525 (tx_sent)
extern uchar g_20000606[];
#define os_task_dly_idx (*(ushort *)(g_20000606 + 0))
extern uchar g_20003844[];
#define DAT_20003846 (*(uchar *)(g_20003844 + 2))
#define DAT_20003847 (*(uchar *)(g_20003844 + 3))
#define g_20003844_v (*(uchar *)(g_20003844 + 0))
extern uchar g_20003B38[];
#define DAT_20003b4d (*(uchar *)(g_20003B38 + 21))
#define DAT_20003b4e (*(uchar *)(g_20003B38 + 22))
#define DAT_20003b4f (*(uchar *)(g_20003B38 + 23))
#define DAT_20003b50 (*(uchar *)(g_20003B38 + 24))
#define DAT_20003b51 (*(uchar *)(g_20003B38 + 25))
#define DAT_20003b52 (*(uchar *)(g_20003B38 + 26))
#define DAT_20003b53 (*(uchar *)(g_20003B38 + 27))
#define DAT_20003b54 (*(uchar *)(g_20003B38 + 28))
#define DAT_20003b55 (*(uchar *)(g_20003B38 + 29))
#define DAT_20003b56 (*(uchar *)(g_20003B38 + 30))
#define DAT_20003b57 (*(uchar *)(g_20003B38 + 31))
#define DAT_20003b58 (*(uchar *)(g_20003B38 + 32))
#define DAT_20003b59 (*(uchar *)(g_20003B38 + 33))
#define DAT_20003b5a (*(uchar *)(g_20003B38 + 34))
#define DAT_20003b5b (*(uchar *)(g_20003B38 + 35))
#define DAT_20003b5c (*(uchar *)(g_20003B38 + 36))
#define DAT_20003b5d (*(uchar *)(g_20003B38 + 37))
#define DAT_20003b5e (*(uchar *)(g_20003B38 + 38))
#define DAT_20003b5f (*(uchar *)(g_20003B38 + 39))
#define DAT_20003b60 (*(uchar *)(g_20003B38 + 40))
#define DAT_20003b61 (*(uchar *)(g_20003B38 + 41))
#define DAT_20003b62 (*(uchar *)(g_20003B38 + 42))
#define DAT_20003b63 (*(uchar *)(g_20003B38 + 43))
#define DAT_20003b64 (*(uchar *)(g_20003B38 + 44))
#define DAT_20003b65 (*(uchar *)(g_20003B38 + 45))
#define DAT_20003b66 (*(uchar *)(g_20003B38 + 46))
#define DAT_20003b67 (*(uchar *)(g_20003B38 + 47))
#define DAT_20003b68 (*(uchar *)(g_20003B38 + 48))
#define DAT_20003b69 (*(uchar *)(g_20003B38 + 49))
#define DAT_20003b6a (*(uchar *)(g_20003B38 + 50))
#define DAT_20003b6b (*(uchar *)(g_20003B38 + 51))
#define DAT_20003b6c (*(uchar *)(g_20003B38 + 52))
#define DAT_20003b6d (*(uchar *)(g_20003B38 + 53))
#define DAT_20003b6e (*(uchar *)(g_20003B38 + 54))
#define DAT_20003b6f (*(uchar *)(g_20003B38 + 55))
#define DAT_20003b70 (*(uchar *)(g_20003B38 + 56))
#define DAT_20003b71 (*(uchar *)(g_20003B38 + 57))
#define DAT_20003b72 (*(uchar *)(g_20003B38 + 58))
#define DAT_20003b73 (*(uchar *)(g_20003B38 + 59))
#define DAT_20003b74 (*(uchar *)(g_20003B38 + 60))
#define DAT_20003b75 (*(uchar *)(g_20003B38 + 61))
#define DAT_20003b76 (*(uchar *)(g_20003B38 + 62))
#define DAT_20003b77 (*(uchar *)(g_20003B38 + 63))
#define DAT_20003b78 (*(uchar *)(g_20003B38 + 64))
#define DAT_20003b79 (*(uchar *)(g_20003B38 + 65))
#define DAT_20003b7a (*(uchar *)(g_20003B38 + 66))
#define DAT_20003b7b (*(uchar *)(g_20003B38 + 67))
#define DAT_20003b7c (*(uchar *)(g_20003B38 + 68))
#define DAT_20003b7d (*(uchar *)(g_20003B38 + 69))
#define DAT_20003b7e (*(uchar *)(g_20003B38 + 70))
#define DAT_20003b7f (*(uchar *)(g_20003B38 + 71))
#define DAT_20003b80 (*(uchar *)(g_20003B38 + 72))
#define DAT_20003b81 (*(uchar *)(g_20003B38 + 73))
#define DAT_20003b82 (*(uchar *)(g_20003B38 + 74))
#define DAT_20003b83 (*(uchar *)(g_20003B38 + 75))
#define DAT_20003b84 (*(uchar *)(g_20003B38 + 76))
#define DAT_20003b85 (*(uchar *)(g_20003B38 + 77))
#define DAT_20003b86 (*(uchar *)(g_20003B38 + 78))
#define DAT_20003b87 (*(uchar *)(g_20003B38 + 79))
#define DAT_20003b88 (*(uchar *)(g_20003B38 + 80))
#define DAT_20003ba8 (*(uchar *)(g_20003B38 + 112))
#define DAT_20003ba9 (*(uchar *)(g_20003B38 + 113))
#define DAT_20003baa (*(uchar *)(g_20003B38 + 114))
#define DAT_20003bab (*(uchar *)(g_20003B38 + 115))
#define DAT_20003bac (*(uchar *)(g_20003B38 + 116))
#define DAT_20003bad (*(uchar *)(g_20003B38 + 117))
#define DAT_20003bae (*(uchar *)(g_20003B38 + 118))
#define DAT_20003baf (*(uchar *)(g_20003B38 + 119))
#define g_20003B38_v (*(uchar *)(g_20003B38 + 0))
extern uchar g_20003D52[];

static __inline void chain_word(ushort value, uchar *dst) {
    dst[0] = value >> 8;
    dst[1] = value;
}
/* Checks a maximum sixty-register page and serializes the selected data.
   match: reply_length is assigned in the supported first-page branch.
   The out-of-page C path leaves it unset; I retain this as an open reconstruction question. */
void coordination_reply_paged_registers(uchar *request, uint function_code) {
    uint word_value;
    uchar invalid_request;
    uint buffer_index;
    int temperature_word;
    uint dword_value;
    uint frame_word;
    uint reply_length;
    ushort delay_count;
    uint index;
    uint register_count;

    ushort copy_index;
    uchar *record;
    invalid_request = false;
    chain_tx[0] = g_20003844_v;
    chain_tx[1] = function_code;
    chain_tx[2] = DAT_20003846;
    chain_tx[3] = DAT_20003847;
    delay_count = 0;
    do {
        os_task_delay((uint)(byte)os_task_dly_idx, 6, 10);
        delay_count = delay_count + 1;
    } while (delay_count < 5);
    frame_word = request[2];
    frame_word = (frame_word << 8) | request[3];
    register_count = request[4];
    register_count = (register_count << 8) | request[5];
    if ((register_count > 0x3c) || (register_count == 0)) {
        invalid_request = true;
    }
    if ((frame_word / 0x3c + 1) * 0x3c < frame_word + register_count) {
        invalid_request = true;
    }
    if (invalid_request == true) {
        frame_word = coordination_crc16(&chain_tx[0], 1);
        chain_tx[2] = (char)frame_word;
        chain_tx[3] = (char)(frame_word >> 8);
        DAT_20000525 = 0;
        DAT_20000524 = 4;
        return;
    }
    index = g_20003844_v - 1 & 0xff;
    if (index >= 6) {
        index = 6;
    }
    buffer_index = 0;
    do {
        (&g_20003B38_v)[buffer_index] = 0;
        buffer_index = buffer_index + 1 & 0xffff;
    } while (buffer_index < 0x78);
    if (frame_word < 0x3c) {
        buffer_index = 0;
        record = g_20003D52;
        record += index * 145;
        do {
            (&g_20003B38_v)[buffer_index] = record[2 + buffer_index];
            buffer_index = buffer_index + 1 & 0xffff;
        } while (buffer_index < 0x15);
        DAT_20003b4d = 0;
        temperature_word = *(ushort *)(record + 121) - 0xaaa;
        DAT_20003b4e = (undefined1)((uint)temperature_word >> 8);
        DAT_20003b4f = (undefined1)temperature_word;
        temperature_word = *(ushort *)(record + 123) - 0xaaa;
        DAT_20003b50 = (undefined1)((uint)temperature_word >> 8);
        DAT_20003b51 = (undefined1)temperature_word;
        temperature_word = *(ushort *)(record + 125) - 0xaaa;
        DAT_20003b52 = (undefined1)((uint)temperature_word >> 8);
        DAT_20003b53 = (undefined1)temperature_word;
        temperature_word = *(ushort *)(record + 127) - 0xaaa;
        DAT_20003b54 = (undefined1)((uint)temperature_word >> 8);
        DAT_20003b55 = (undefined1)temperature_word;
        temperature_word = *(ushort *)(record + 129) - 0xaaa;
        DAT_20003b56 = (undefined1)((uint)temperature_word >> 8);
        DAT_20003b57 = (undefined1)temperature_word;
        word_value = *(ushort *)(record + 0);
        DAT_20003b58 = (uchar)(word_value >> 8);
        DAT_20003b59 = (uchar)word_value;
        word_value = record[131];
        DAT_20003b5a = (uchar)(word_value >> 8);
        DAT_20003b5b = (uchar)word_value;
        DAT_20003b5c = 0;
        DAT_20003b5d = 0;
        DAT_20003b5e = 0;
        DAT_20003b5f = 0;
        DAT_20003b60 = 0;
        DAT_20003b61 = 0;
        DAT_20003b62 = record[44];
        word_value = *(ushort *)(record + 45);
        DAT_20003b63 = (uchar)(word_value >> 8);
        DAT_20003b64 = (uchar)word_value;
        dword_value = *(undefined4 *)(record + 47);
        DAT_20003b65 = (undefined1)((uint)dword_value >> 0x18);
        DAT_20003b66 = (undefined1)((uint)dword_value >> 0x10);
        DAT_20003b67 = (undefined1)((uint)dword_value >> 8);
        DAT_20003b68 = (undefined1)dword_value;
        word_value = *(ushort *)(record + 51);
        DAT_20003b69 = (uchar)(word_value >> 8);
        DAT_20003b6a = (uchar)word_value;
        dword_value = *(undefined4 *)(record + 53);
        DAT_20003b6b = (undefined1)((uint)dword_value >> 0x18);
        DAT_20003b6c = (undefined1)((uint)dword_value >> 0x10);
        DAT_20003b6d = (undefined1)((uint)dword_value >> 8);
        DAT_20003b6e = (undefined1)dword_value;
        dword_value = *(undefined4 *)(record + 57);
        DAT_20003b6f = (undefined1)((uint)dword_value >> 0x18);
        DAT_20003b70 = (undefined1)((uint)dword_value >> 0x10);
        DAT_20003b71 = (undefined1)((uint)dword_value >> 8);
        DAT_20003b72 = (undefined1)dword_value;
        dword_value = *(undefined4 *)(record + 61);
        DAT_20003b73 = (undefined1)((uint)dword_value >> 0x18);
        DAT_20003b74 = (undefined1)((uint)dword_value >> 0x10);
        DAT_20003b75 = (undefined1)((uint)dword_value >> 8);
        DAT_20003b76 = (undefined1)dword_value;
        dword_value = *(undefined4 *)(record + 65);
        DAT_20003b77 = (undefined1)((uint)dword_value >> 0x18);
        DAT_20003b78 = (undefined1)((uint)dword_value >> 0x10);
        DAT_20003b79 = (undefined1)((uint)dword_value >> 8);
        DAT_20003b7a = (undefined1)dword_value;
        DAT_20003b7b = record[136];
        DAT_20003b7c = record[70];
        DAT_20003b7d = record[71];
        DAT_20003b7e = record[72];
        DAT_20003b7f = record[73];
        DAT_20003b80 = record[74];
        DAT_20003b81 = record[75];
        DAT_20003b82 = record[76];
        DAT_20003b83 = record[77];
        DAT_20003b84 = record[78];
        word_value = *(ushort *)(record + 119);
        DAT_20003b85 = (uchar)(word_value >> 8);
        DAT_20003b86 = (uchar)word_value;
        DAT_20003b87 = 0;
        index = 0;
        do {
            chain_word(*(ushort *)(record + 83 + index * 2), (&DAT_20003b88) + index * 2);
            index = index + 1 & 0xffff;
        } while (index < 0x10);
        temperature_word = *(ushort *)(record + 115) - 0xaaa;
        DAT_20003ba8 = (undefined1)((uint)temperature_word >> 8);
        DAT_20003ba9 = (undefined1)temperature_word;
        temperature_word = *(ushort *)(record + 117) - 0xaaa;
        DAT_20003baa = (undefined1)((uint)temperature_word >> 8);
        DAT_20003bab = (undefined1)temperature_word;
        word_value = *(ushort *)(record + 79);
        DAT_20003bac = (uchar)(word_value >> 8);
        DAT_20003bad = (uchar)word_value;
        word_value = *(ushort *)(record + 81);
        DAT_20003bae = (uchar)(word_value >> 8);
        DAT_20003baf = (uchar)word_value;
        copy_index = frame_word * 2;
        reply_length = 4;
        while (copy_index < frame_word * 2 + register_count * 2) {
            chain_tx[reply_length] = (&g_20003B38_v)[copy_index];
            copy_index++;
            reply_length = reply_length + 1 & 0xffff;
        }
    }
    if (reply_length > 0xfd) {
        reply_length = 0xfd;
    }
    frame_word = coordination_crc16(&chain_tx[0], reply_length);
    (&chain_tx[0])[reply_length] = (char)frame_word;
    (&chain_tx[1])[reply_length] = (char)(frame_word >> 8);
    DAT_20000524 = (char)reply_length + '\x02';
    board_set_pb2(1);
    os_task_delay((uint)(byte)os_task_dly_idx, 5, 10);
    DAT_20000525 = 0;
    USART_ITConfig(&DAT_40004800, 0x727, '\x01');
    return;
}
#undef DAT_40004800
#undef DAT_20000524
#undef DAT_20000525
#undef os_task_dly_idx
#undef DAT_20003846
#undef DAT_20003847
#undef g_20003844_v
#undef DAT_20003b4d
#undef DAT_20003b4e
#undef DAT_20003b4f
#undef DAT_20003b50
#undef DAT_20003b51
#undef DAT_20003b52
#undef DAT_20003b53
#undef DAT_20003b54
#undef DAT_20003b55
#undef DAT_20003b56
#undef DAT_20003b57
#undef DAT_20003b58
#undef DAT_20003b59
#undef DAT_20003b5a
#undef DAT_20003b5b
#undef DAT_20003b5c
#undef DAT_20003b5d
#undef DAT_20003b5e
#undef DAT_20003b5f
#undef DAT_20003b60
#undef DAT_20003b61
#undef DAT_20003b62
#undef DAT_20003b63
#undef DAT_20003b64
#undef DAT_20003b65
#undef DAT_20003b66
#undef DAT_20003b67
#undef DAT_20003b68
#undef DAT_20003b69
#undef DAT_20003b6a
#undef DAT_20003b6b
#undef DAT_20003b6c
#undef DAT_20003b6d
#undef DAT_20003b6e
#undef DAT_20003b6f
#undef DAT_20003b70
#undef DAT_20003b71
#undef DAT_20003b72
#undef DAT_20003b73
#undef DAT_20003b74
#undef DAT_20003b75
#undef DAT_20003b76
#undef DAT_20003b77
#undef DAT_20003b78
#undef DAT_20003b79
#undef DAT_20003b7a
#undef DAT_20003b7b
#undef DAT_20003b7c
#undef DAT_20003b7d
#undef DAT_20003b7e
#undef DAT_20003b7f
#undef DAT_20003b80
#undef DAT_20003b81
#undef DAT_20003b82
#undef DAT_20003b83
#undef DAT_20003b84
#undef DAT_20003b85
#undef DAT_20003b86
#undef DAT_20003b87
#undef DAT_20003b88
#undef DAT_20003ba8
#undef DAT_20003ba9
#undef DAT_20003baa
#undef DAT_20003bab
#undef DAT_20003bac
#undef DAT_20003bad
#undef DAT_20003bae
#undef DAT_20003baf
#undef g_20003B38_v

extern uchar g_200000DA[];
#define DAT_200000da (*(uchar *)(g_200000DA + 0))
extern uchar g_200000E1[];
#define DAT_200000e1 (*(uchar *)(g_200000E1 + 0))
extern uchar g_200000E8;
#define DAT_200000e8 (*(uchar *)(&g_200000E8 + 0))
extern uchar g_200000F0[];
#define DAT_200000f0 (*(uchar *)(g_200000F0 + 0))
extern uchar g_200000F2[];
#define DAT_200000f2 (*(ushort *)(g_200000F2 + 0))
extern uchar g_20000163[];
#define DAT_20000163 (*(uchar *)(g_20000163 + 0))
extern uchar g_20000279[];
#define DAT_20000279 (*(volatile uchar *)(g_20000279 + 0))
extern uchar g_20000524[];
#define DAT_20000526 (packet_done)
#define DAT_20000527 (packet_valid)
#define DAT_20000528 (exception_code)
#define DAT_20000529 (packet_timeout)
#define DAT_2000052e (start_register)
extern uchar g_20000606[];
#define os_task_dly_idx (*(ushort *)(g_20000606 + 0))
#define DAT_20000fcd (g_20000F9C[6].elapsed)
extern uchar g_20003844[];
#define DAT_20003845 (*(uchar *)(g_20003844 + 1))
#define DAT_20003846 (*(uchar *)(g_20003844 + 2))
#define DAT_20003847 (*(uchar *)(g_20003844 + 3))
#define DAT_20003848 (*(uchar *)(g_20003844 + 4))
#define DAT_20003849 (*(uchar *)(g_20003844 + 5))

/* Checks packet timeout and dispatches register functions3,4,6. */
void coordination_poll() {
    uint function_code;
    ushort register_value;
    uchar delay_count;

    if (DAT_20000527 == '\0') {
        if (((DAT_20000fcd != '\0') && ((DAT_20000279 & 1) == 0)) && (DAT_20000163 == '\0')) {
            DAT_20000279 = DAT_20000279 | 1;
        }
        if (DAT_20000529 == '\x01') {
            return;
        }
        delay_count = 0;
        do {
            os_task_delay((uint)(byte)os_task_dly_idx, 5, 10);
            delay_count = delay_count + 1;
        } while (delay_count < 6);
        if (DAT_20000527 == '\0' && DAT_20000529 == '\0') {
            DAT_20000529 = '\x01';
            runtime_timer_start(6, 0x4b0);
        }
    } else {
        DAT_20000529 = '\0';
        DAT_20000279 = DAT_20000279 & 0xfe;
        runtime_timer_stop(6);
    }
    if (DAT_20000526 == '\0') {
        return;
    }
    DAT_20000526 = 0;
    DAT_20000528 = 0;
    function_code = (uint)DAT_20003845;
    switch (DAT_20000527) {
    default: {
        DAT_20000527 = '\0';
        DAT_2000052e = DAT_20003846;
        DAT_2000052e <<= 8;
        DAT_2000052e += DAT_20003847;
        switch (function_code) {
        default: {

            function_code = function_code + 0x80 & 0xff;
            DAT_20000528 = 1;

        } break;
        case 3: {

            if (DAT_2000052e > 0x80) {
                function_code = 0x83;
                DAT_20000528 = 2;
            }

        } break;
        case 4: {

            if (DAT_2000052e > 0x80) {
                function_code = 0x84;
                DAT_20000528 = 4;
            }

        } break;
        case 6: {

            register_value = ((uint)DAT_20003848 << 8) + DAT_20003849;
            switch (DAT_2000052e) {
            case 0:
                break;
            case 1:
                if (register_value <= 1) {
                    DAT_200000da = register_value;
                    heating_inhibit = 0;
                } else if (register_value & 2) {
                    heating_inhibit = 1;
                } else {
                    heating_inhibit = 0;
                }
                if (register_value & 4) {
                    heating_enabled = 1;
                } else {
                    heating_enabled = 0;
                }
                break;
            case 2:
                if (DAT_200000e1 == '\0') {
                    if (register_value < 10) {
                        DAT_200000e8 = register_value;
                    } else {
                        DAT_200000e8 = 0;
                    }
                } else {
                    DAT_200000e8 = 0;
                }
                break;
            case 3:
                if (register_value <= 1) {
                    DAT_200000f0 = register_value;
                }
                break;
            case 4:
                if (register_value <= 1) {
                    DAT_200000f2 = register_value;
                }
            }

        } break;
        }

    } break;
    case 0: {
        function_code = function_code + 0x80 & 0xff;
        DAT_20000528 = 3;
    } break;
    }

    if (function_code == 4 || function_code == 0x84) {
        coordination_reply_paged_registers(g_20003844, function_code);
    } else {
        coordination_reply_registers(function_code);
    }
    return;
}
#undef DAT_200000da
#undef DAT_200000e1
#undef DAT_200000e8
#undef DAT_200000f0
#undef DAT_200000f2
#undef DAT_20000163
#undef DAT_20000279
#undef DAT_20000526
#undef DAT_20000527
#undef DAT_20000528
#undef DAT_20000529
#undef DAT_2000052e
#undef os_task_dly_idx
#undef DAT_20000fcd
#undef DAT_20003845
#undef DAT_20003846
#undef DAT_20003847
#undef DAT_20003848
#undef DAT_20003849
