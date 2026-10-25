/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow register-function decoding, pack requests and inverter replies here.
   This unit owns the CRC tables and seven packed peer records.
   Measurement shares the packed record layout and keeps its six-record extern view.
   match: I keep the CRC body with its callers because that affects saved-register code.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
/* at: protocol_crc16 0x08024CB8, protocol_initialize 0x08024CE2, pack_send_request 0x08024CE4, pack_handle_register_write 0x08024D22, pack_reply_register_read 0x08024DC6, pack_dispatch_frame 0x08025426, pack_send_pending_settings 0x08025506, pack_store_register_reply 0x0802570E, pack_protocol_task 0x08025B94, inverter_reply_register_write 0x08025EEC, inverter_reply_input_registers 0x08025F40, inverter_reply_register_read 0x08026034, inverter_dispatch_frame 0x080260D0, inverter_reply_read_error 0x080261BE, pack_update_summary 0x0802624C, pack_balance_and_control 0x0802661C */
/* at: g_2000004F 0x2000004F, g_20000051 0x20000051, g_20000077 0x20000077, g_20000098 0x20000098, g_200000A0 0x200000A0, g_200000A2 0x200000A2, g_200000AD 0x200000AD, g_200000F4 0x200000F6, measurement_maximum_cell_voltage 0x20000110, measurement_minimum_cell_voltage 0x20000112, measurement_pack_voltage 0x20000114, g_20000124 0x20000128, measurement_maximum_temperature 0x2000012E, measurement_minimum_temperature 0x20000130, g_20000130 0x20000134, g_20000134 0x20000138, g_20000138 0x2000013C, g_20000139 0x2000013D, measurement_current 0x2000014C, measurement_cell_count 0x20000166, g_20000163 0x20000167, g_20000164 0x20000168, g_2000016C 0x20000170, g_20000182 0x20000186, g_20000184 0x20000188, g_20000185 0x20000189, g_20000271 0x20000279, g_20000272 0x2000027A, g_20000273 0x2000027B, g_20000274 0x2000027C, g_20000275 0x2000027D, g_20000276 0x2000027E, g_20000277 0x2000027F, g_20000278 0x20000280, g_2000050D 0x20000519, g_2000058C 0x200005A4, g_20000A9C 0x20000AB4, g_20000BC9 0x20000BE1, g_20001094 0x200010AC, g_2000119E 0x200011B6, g_20003C5E 0x20003C76, g_20003C5F 0x20003C77, g_20003C60 0x20003C78, g_20003C61 0x20003C79, g_20003C62 0x20003C7A, g_20003C63 0x20003C7B, g_20003C64 0x20003C7C, g_20003C65 0x20003C7D, g_20003C66 0x20003C7E, g_20003C67 0x20003C7F, g_20003C68 0x20003C80, g_20003C69 0x20003C81, g_20003C6A 0x20003C82, g_20003C6B 0x20003C83, g_20003C6C 0x20003C84, g_20003C6D 0x20003C85, g_20003C6E 0x20003C86, g_20003C73 0x20003C8B, g_20003C74 0x20003C8C, g_20003C75 0x20003C8D, g_20003C76 0x20003C8E, g_20003C77 0x20003C8F, g_20003C78 0x20003C90, g_20003C79 0x20003C91, g_20003C7A 0x20003C92, g_20003C7B 0x20003C93, g_20003C7C 0x20003C94, g_20003C7D 0x20003C95, g_20003C7E 0x20003C96, g_20003C7F 0x20003C97, g_20003C80 0x20003C98, g_20003C81 0x20003C99, g_20003C82 0x20003C9A, g_20003C83 0x20003C9B, g_20003C84 0x20003C9C, g_20003C85 0x20003C9D, g_20003C86 0x20003C9E, g_20003C87 0x20003C9F, g_20003C88 0x20003CA0, g_20003C89 0x20003CA1, g_20003C8A 0x20003CA2, g_20003C8B 0x20003CA3, g_20003C8C 0x20003CA4, g_20003C8D 0x20003CA5, g_20003C8E 0x20003CA6, g_20003C8F 0x20003CA7, g_20003C90 0x20003CA8, g_20003C91 0x20003CA9, g_20003C92 0x20003CAA, g_20003C93 0x20003CAB, g_20003C94 0x20003CAC, g_20003C95 0x20003CAD, g_20003C96 0x20003CAE, g_20003C97 0x20003CAF, g_20003C98 0x20003CB0, g_20003C99 0x20003CB1, g_20003C9A 0x20003CB2, g_20003C9B 0x20003CB3, g_20003C9C 0x20003CB4, g_20003C9D 0x20003CB5, g_20003C9E 0x20003CB6, g_20003C9F 0x20003CB7, g_20003CA0 0x20003CB8, g_20003CA1 0x20003CB9, g_20003CA2 0x20003CBA, g_20003CA3 0x20003CBB, g_20003CA4 0x20003CBC, g_20003CA5 0x20003CBD, g_20003CA6 0x20003CBE, g_20003CA7 0x20003CBF, g_20003CA8 0x20003CC0, g_20003CA9 0x20003CC1, g_20003CAA 0x20003CC2, g_20003CAB 0x20003CC3, g_20003CAC 0x20003CC4, g_20003CAD 0x20003CC5, g_20003CAE 0x20003CC6, g_20003CAF 0x20003CC7, g_20003CB0 0x20003CC8, g_20003CB1 0x20003CC9, g_20003CB2 0x20003CCA, g_20003CB3 0x20003CCB, g_20003CB4 0x20003CCC, g_20003CB5 0x20003CCD, g_20003CB6 0x20003CCE, g_20003CB7 0x20003CCF, g_20003CB8 0x20003CD0, g_20003CB9 0x20003CD1, g_20003CBA 0x20003CD2, g_20003CBB 0x20003CD3, g_20003CBC 0x20003CD4, g_20003CBD 0x20003CD5, g_20003CC2 0x20003CDA, g_20003CC3 0x20003CDB, g_20003CC4 0x20003CDC, g_20003CC5 0x20003CDD, g_20003CC6 0x20003CDE, g_20003CC7 0x20003CDF, g_20003CC8 0x20003CE0, g_20003CC9 0x20003CE1, g_20003CCA 0x20003CE2, g_20003CCB 0x20003CE3, g_20003CCC 0x20003CE4, g_20003CCD 0x20003CE5, g_20003CCE 0x20003CE6, g_20003CCF 0x20003CE7, g_20003CD0 0x20003CE8, g_20003CD1 0x20003CE9, g_20003CD2 0x20003CEA, g_20003CD3 0x20003CEB, g_20003CD4 0x20003CEC, g_20003CD5 0x20003CED, g_20003CE0 0x20003CF8, g_20003D40 0x20003D58, g_20004149 0x20004161, g_20004467 0x2000447F, g_200044FE 0x20004516 */
#include "../../include/ghidra_types.h"
#include "../../include/bms_measurements.h"
#include "../../include/bms_protocol.h"
#include "../../include/bms_byte_order.h"
#include <string.h>
typedef __packed struct {
    uchar b0 : 1;
    uchar b1 : 1;
    uchar b2 : 1;
    uchar b3 : 1;
    uchar b4 : 1;
    uchar b5 : 1;
    uchar b6 : 1;
    uchar b7 : 1;
} ProtocolByteBits;
typedef __packed struct {
    ushort b0 : 1;
    ushort b1 : 1;
    ushort b2 : 1;
    ushort b3 : 1;
    ushort b4 : 1;
    ushort b5 : 1;
    ushort b6 : 1;
    ushort b7 : 1;
    ushort b8 : 1;
    ushort b9 : 1;
    ushort b10 : 1;
    ushort b11 : 1;
    ushort b12 : 1;
    ushort b13 : 1;
    ushort b14 : 1;
    ushort b15 : 1;
} ProtocolWordBits;
typedef __packed struct {
    ProtocolByteBits low, high;
} ProtocolWordBytes;
typedef __packed union {
    ushort value;
    ProtocolWordBits bits;
    ProtocolWordBytes bytes;
} ProtocolFlagWord;
typedef __packed struct {
    ProtocolFlagWord word;
    uchar spare;
} ProtocolFlag;

typedef struct {
    ushort ready, pending, error;
    uchar reserved[10];
} ProtocolStat;
typedef __packed struct {
    uint flags;
    uint spare;
    ushort x8, xA, xC, xE;
    uchar pad[98];
} ProtocolParams;
typedef struct {
    uchar bytes[16];
    ushort reserved16, poll18, uart20, reserved22, reserved24, crc26;
    ushort phase28;
    ushort ticks30, soc32, settle34, wait36, crc38;
    uint sum40, soc44, clear48;
    ushort fault52[6];
} ProtocolControl;
/* at: g_2000058C 0x200005A4 */
uchar g_2000058C = 0;
/* at: protocol_byte1 0x200005A5 */
uchar protocol_byte1 = 0;
/* at: protocol_byte2 0x200005A6 */
uchar protocol_byte2 = 0;
/* at: protocol_byte3 0x200005A7 */
uchar protocol_byte3 = 0;
/* at: protocol_byte4 0x200005A8 */
uchar protocol_byte4 = 8;
/* at: g_20000591 0x200005A9 */
uchar g_20000591 = 0;
/* at: protocol_byte6 0x200005AA */
uchar protocol_byte6 = 0;
/* at: protocol_byte7 0x200005AB */
uchar protocol_byte7 = 1;
/* at: protocol_byte8 0x200005AC */
uchar protocol_byte8 = 0;
/* at: protocol_byte9 0x200005AD */
uchar protocol_byte9 = 0;
/* at: protocol_byte10 0x200005AE */
uchar protocol_byte10 = 0;
/* at: protocol_byte11 0x200005AF */
uchar protocol_byte11 = 0;
/* at: protocol_byte12 0x200005B0 */
uchar protocol_byte12 = 0;
/* at: protocol_byte13 0x200005B1 */
uchar protocol_byte13 = 0;
/* at: protocol_byte14 0x200005B2 */
uchar protocol_byte14 = 8;
/* at: protocol_byte15 0x200005B3 */
uchar protocol_byte15 = 1;
/* at: protocol_reserved16 0x200005B4 */
ushort protocol_reserved16 = 0;
/* at: protocol_poll18 0x200005B6 */
ushort protocol_poll18 = 0;
/* at: protocol_uart20 0x200005B8 */
ushort protocol_uart20 = 0;
/* at: protocol_reserved22 0x200005BA */
ushort protocol_reserved22 = 0;
/* at: protocol_reserved24 0x200005BC */
ushort protocol_reserved24 = 0;
/* at: protocol_crc26 0x200005BE */
ushort protocol_crc26 = 0;
/* at: protocol_phase28 0x200005C0 */
ushort protocol_phase28 = 0;
/* at: protocol_ticks30 0x200005C2 */
ushort protocol_ticks30 = 0;
/* at: protocol_soc32 0x200005C4 */
ushort protocol_soc32 = 0;
/* at: protocol_settle34 0x200005C6 */
ushort protocol_settle34 = 0;
/* at: protocol_wait36 0x200005C8 */
ushort protocol_wait36 = 0;
/* at: protocol_crc38 0x200005CA */
ushort protocol_crc38 = 0;
/* at: protocol_sum40 0x200005CC */
uint protocol_sum40 = 0;
/* at: protocol_soc44 0x200005D0 */
uint protocol_soc44 = 0;
/* at: protocol_clear48 0x200005D4 */
uint protocol_clear48 = 0;
/* at: protocol_fault52 0x200005D8 */
ushort protocol_fault52[6] = {2000, 2000, 2000, 2000, 2000, 2000};

#include "../../include/stdperiph_types.h"
#undef CONCAT11
#define CONCAT11(a, b) ((b) | (a) << 8)
/* at: protocol_timers 0x20003BE8, protocol_clear 0x20003BF4, protocol_scratch 0x20003C76 */
ushort protocol_timers[6];
uchar protocol_clear[130];
uchar protocol_scratch[130];
#define protocol_scratch_data (protocol_scratch)
/* at: protocol_stats 0x20003CF8 */
ProtocolStat protocol_stats[6];
/* at: protocol_flags 0x20003D58 */
ProtocolFlag protocol_flags[6];
/* at: protocol_pack 0x20003D6A */
#include "../../include/bms_peer_types.h"
BmsPeerRecord protocol_pack[7];
#define protocol_records (protocol_pack)

/* at: protocol_pending 0x20004161 */
ProtocolParams protocol_pending[7];
/* at: protocol_out 0x2000447F */
uchar protocol_out[151];
/* at: protocol_reply 0x20004516 */
uchar protocol_reply[152];
void sub_08014F94(int, undefined4, undefined2);
void board_set_pa8(int);
int __aeabi_f2iz(float);
uint __aeabi_f2uiz(float);
ushort os_task_delay(int, undefined2, undefined2);

/* at: crc16_tbl_hi 0x08030BFC, crc16_tbl_lo 0x08030CFC */
const uchar crc16_tbl_hi[256] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
};
const uchar crc16_tbl_lo[256] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
    0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
    0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
    0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
    0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
    0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
    0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
    0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
    0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40,
};

#define c_080307B8_v (*(uchar *)(c_080307B8 + 0))

ushort protocol_crc16(byte *data, ushort length) {
    uint table_idx, crc_hi;
    uchar crc_lo;
    crc_hi = 0xff;
    crc_lo = 0xff;
    while (length--) {
        table_idx = *data++ ^ crc_lo;
        crc_lo = crc16_tbl_hi[table_idx] ^ crc_hi;
        crc_hi = crc16_tbl_lo[table_idx];
    }
    return crc_lo | crc_hi << 8;
}
#undef c_080307B8_v

void protocol_initialize(void) {

    return;
}

#define DAT_2000058c (g_2000058C)
extern uchar g_20000BC9[];
#define DAT_20000bc9 (*(uchar *)(g_20000BC9 + 0))

void pack_send_request(int peer_address, int function_code, int register_address,
                       int value_or_count) {
    int v1;
    uchar v2;
    int v4;
    int v5;
    ushort crc;
    v5 = &DAT_2000058c;
    v4 = 8;
    protocol_byte4 = v4;
    v4 = &DAT_20000bc9;
    *(uchar *)v4 = peer_address;
    *(uchar *)(v4 + 1) = function_code;
    *(uchar *)(v4 + 2) = (uint)register_address >> 8;
    *(uchar *)(v4 + 3) = register_address;
    *(uchar *)(v4 + 4) = (uint)value_or_count >> 8;
    *(uchar *)(v4 + 5) = value_or_count;
    crc = protocol_reserved16 = protocol_crc16(v4, 6);
    *(uchar *)(v4 + 6) = crc >> 8;
    *(uchar *)(v4 + 7) = crc;
    board_set_pa8(1);
    v2 = protocol_byte4;
    v1 = v4;
    sub_08014F94((0), v1, v2);
}
#undef DAT_2000058c
#undef DAT_20000bc9

extern uchar g_200000F4[];
#define DAT_200000f4 (*(ushort *)(g_200000F4 + 0))
extern uchar g_20000163;
#define DAT_20000163 (g_20000163)
#define DAT_2000058c (g_2000058C)

static __inline uint protocol_be16(const uchar *p) {
    uchar high = p[0];
    return p[1] | ((uint)high << 8);
}
static __inline ProtocolParams *protocol_params(uint index) {
    ProtocolParams *p = protocol_pending;
    return p + index;
}
void pack_handle_register_write(uchar *frame) {
    ushort reg;
    ushort val;

    reg = frame[2];
    reg = reg << 8 | frame[3];
    val = frame[4];
    val = val << 8 | frame[5];
    switch (reg) {
    case 1:
        protocol_flags[g_20000163].word.value = val;
        if (protocol_flags[g_20000163].word.bits.b4 || protocol_flags[g_20000163].word.bits.b5)
            protocol_byte10++;
        break;
    case 2:
        if (val < 60000) {
            protocol_pending[g_20000163].x8 = val;
            protocol_pending[g_20000163].flags |= 1;
        }
        break;
    case 3:
        if (val < 60000) {
            protocol_pending[g_20000163].xA = val;
            protocol_pending[g_20000163].flags |= 2;
        }
        break;
    case 4:
        if (val < 60000) {
            protocol_pending[g_20000163].xC = val;
            protocol_pending[g_20000163].flags |= 4;
        }
        break;
    case 5:
        if (val >= 8 && val <= 16) {
            protocol_pending[g_20000163].xE = val;
            protocol_pending[g_20000163].flags |= 8;
        }
        break;
    case 6:
        if (val <= 1)
            DAT_200000f4 = val;
        break;
    }
}
#undef DAT_200000f4
#undef DAT_20000163
#undef DAT_2000058c

extern uchar g_20000051;
#define g_20000051_v (g_20000051)
extern uchar g_20000077;
#define g_20000077_v (g_20000077)
extern uchar g_20000098;
#define g_20000098_v (g_20000098)
extern uchar g_200000A0;
#define g_200000A0_v (g_200000A0)
extern ushort g_200000A2;
#define g_200000A2_v (g_200000A2)
extern uchar g_200000AD;
#define g_200000AD_v (g_200000AD)
extern int g_20000124;
#define g_20000124_v (g_20000124)
extern uint g_20000130;
#define g_20000130_v (g_20000130)
extern uint g_20000134;
#define g_20000134_v (g_20000134)
extern uchar g_20000138;
#define g_20000138_v (g_20000138)
extern uchar g_20000139;
#define g_20000139_v (g_20000139)
extern float g_20000164;
#define g_20000164_v (g_20000164)
extern float g_2000016C;
#define g_2000016C_v (g_2000016C)
extern ushort g_20000182;
#define g_20000182_v (g_20000182)
extern uchar g_20000184;
#define g_20000184_v (g_20000184)
extern uchar g_20000185;
#define g_20000185_v (g_20000185)
extern uchar g_20000271;
#define g_20000271_v (g_20000271)
extern uchar g_20000272;
#define g_20000272_v (g_20000272)
extern uchar g_20000273;
#define g_20000273_v (g_20000273)
extern uchar g_20000274;
#define g_20000274_v (g_20000274)
extern uchar g_20000275;
#define g_20000275_v (g_20000275)
extern uchar g_20000276;
#define g_20000276_v (g_20000276)
extern uchar g_20000277;
#define g_20000277_v (g_20000277)
extern uchar g_20000278;
#define g_20000278_v (g_20000278)
extern uchar g_2000050D;
#define g_2000050D_v (g_2000050D)
#define DAT_2000058d (protocol_byte1)
extern ushort g_20001094;
#define g_20001094_v (g_20001094)
extern uchar g_2000119E[];
#define DAT_200011a0 (*(ushort *)(g_2000119E + 2))
#define DAT_200011a2 (*(ushort *)(g_2000119E + 4))
#define DAT_200011a4 (*(ushort *)(g_2000119E + 6))
#define DAT_200011a6 (*(ushort *)(g_2000119E + 8))
#define g_2000119E_v (*(ushort *)(g_2000119E + 0))
#define DAT_20003c5e (*(uchar *)((protocol_scratch_data + 0) + 0))
#define DAT_20003c5f (*(uchar *)((protocol_scratch_data + 1) + 0))
#define DAT_20003c60 (*(uchar *)((protocol_scratch_data + 2) + 0))
#define DAT_20003c61 (*(uchar *)((protocol_scratch_data + 3) + 0))
#define DAT_20003c62 (*(uchar *)((protocol_scratch_data + 4) + 0))
#define DAT_20003c63 (*(uchar *)((protocol_scratch_data + 5) + 0))
#define DAT_20003c64 (*(uchar *)((protocol_scratch_data + 6) + 0))
#define DAT_20003c65 (*(uchar *)((protocol_scratch_data + 7) + 0))
#define DAT_20003c66 (*(uchar *)((protocol_scratch_data + 8) + 0))
#define DAT_20003c67 (*(uchar *)((protocol_scratch_data + 9) + 0))
#define DAT_20003c68 (*(uchar *)((protocol_scratch_data + 10) + 0))
#define DAT_20003c69 (*(uchar *)((protocol_scratch_data + 11) + 0))
#define DAT_20003c6a (*(uchar *)((protocol_scratch_data + 12) + 0))
#define DAT_20003c6b (*(uchar *)((protocol_scratch_data + 13) + 0))
#define DAT_20003c6c (*(uchar *)((protocol_scratch_data + 14) + 0))
#define DAT_20003c6d (*(uchar *)((protocol_scratch_data + 15) + 0))
#define DAT_20003c6e (*(uchar *)((protocol_scratch_data + 16) + 0))
#define DAT_20003c73 (*(uchar *)((protocol_scratch_data + 21) + 0))
#define DAT_20003c74 (*(uchar *)((protocol_scratch_data + 22) + 0))
#define DAT_20003c75 (*(uchar *)((protocol_scratch_data + 23) + 0))
#define DAT_20003c76 (*(uchar *)((protocol_scratch_data + 24) + 0))
#define DAT_20003c77 (*(uchar *)((protocol_scratch_data + 25) + 0))
#define DAT_20003c78 (*(uchar *)((protocol_scratch_data + 26) + 0))
#define DAT_20003c79 (*(uchar *)((protocol_scratch_data + 27) + 0))
#define DAT_20003c7a (*(uchar *)((protocol_scratch_data + 28) + 0))
#define DAT_20003c7b (*(uchar *)((protocol_scratch_data + 29) + 0))
#define DAT_20003c7c (*(uchar *)((protocol_scratch_data + 30) + 0))
#define DAT_20003c7d (*(uchar *)((protocol_scratch_data + 31) + 0))
#define DAT_20003c7e (*(uchar *)((protocol_scratch_data + 32) + 0))
#define DAT_20003c7f (*(uchar *)((protocol_scratch_data + 33) + 0))
#define DAT_20003c80 (*(uchar *)((protocol_scratch_data + 34) + 0))
#define DAT_20003c81 (*(uchar *)((protocol_scratch_data + 35) + 0))
#define DAT_20003c82 (*(uchar *)((protocol_scratch_data + 36) + 0))
#define DAT_20003c83 (*(uchar *)((protocol_scratch_data + 37) + 0))
#define DAT_20003c84 (*(uchar *)((protocol_scratch_data + 38) + 0))
#define DAT_20003c85 (*(uchar *)((protocol_scratch_data + 39) + 0))
#define DAT_20003c86 (*(uchar *)((protocol_scratch_data + 40) + 0))
#define DAT_20003c87 (*(uchar *)((protocol_scratch_data + 41) + 0))
#define DAT_20003c88 (*(uchar *)((protocol_scratch_data + 42) + 0))
#define DAT_20003c89 (*(uchar *)((protocol_scratch_data + 43) + 0))
#define DAT_20003c8a (*(uchar *)((protocol_scratch_data + 44) + 0))
#define DAT_20003c8b (*(uchar *)((protocol_scratch_data + 45) + 0))
#define DAT_20003c8c (*(uchar *)((protocol_scratch_data + 46) + 0))
#define DAT_20003c8d (*(uchar *)((protocol_scratch_data + 47) + 0))
#define DAT_20003c8e (*(uchar *)((protocol_scratch_data + 48) + 0))
#define DAT_20003c8f (*(uchar *)((protocol_scratch_data + 49) + 0))
#define DAT_20003c90 (*(uchar *)((protocol_scratch_data + 50) + 0))
#define DAT_20003c91 (*(uchar *)((protocol_scratch_data + 51) + 0))
#define DAT_20003c92 (*(uchar *)((protocol_scratch_data + 52) + 0))
#define DAT_20003c93 (*(uchar *)((protocol_scratch_data + 53) + 0))
#define DAT_20003c94 (*(uchar *)((protocol_scratch_data + 54) + 0))
#define DAT_20003c95 (*(uchar *)((protocol_scratch_data + 55) + 0))
#define DAT_20003c96 (*(uchar *)((protocol_scratch_data + 56) + 0))
#define DAT_20003c97 (*(uchar *)((protocol_scratch_data + 57) + 0))
#define DAT_20003c98 (*(uchar *)((protocol_scratch_data + 58) + 0))
#define DAT_20003c99 (*(uchar *)((protocol_scratch_data + 59) + 0))
#define DAT_20003c9a (*(uchar *)((protocol_scratch_data + 60) + 0))
#define DAT_20003c9b (*(uchar *)((protocol_scratch_data + 61) + 0))
#define DAT_20003c9c (*(uchar *)((protocol_scratch_data + 62) + 0))
#define DAT_20003c9d (*(uchar *)((protocol_scratch_data + 63) + 0))
#define DAT_20003c9e (*(uchar *)((protocol_scratch_data + 64) + 0))
#define DAT_20003c9f (*(uchar *)((protocol_scratch_data + 65) + 0))
#define DAT_20003ca0 (*(uchar *)((protocol_scratch_data + 66) + 0))
#define DAT_20003ca1 (*(uchar *)((protocol_scratch_data + 67) + 0))
#define DAT_20003ca2 (*(uchar *)((protocol_scratch_data + 68) + 0))
#define DAT_20003ca3 (*(uchar *)((protocol_scratch_data + 69) + 0))
#define DAT_20003ca4 (*(uchar *)((protocol_scratch_data + 70) + 0))
#define DAT_20003ca5 (*(uchar *)((protocol_scratch_data + 71) + 0))
#define DAT_20003ca6 (*(uchar *)((protocol_scratch_data + 72) + 0))
#define DAT_20003ca7 (*(uchar *)((protocol_scratch_data + 73) + 0))
#define DAT_20003ca8 (*(uchar *)((protocol_scratch_data + 74) + 0))
#define DAT_20003ca9 (*(uchar *)((protocol_scratch_data + 75) + 0))
#define DAT_20003caa (*(uchar *)((protocol_scratch_data + 76) + 0))
#define DAT_20003cab (*(uchar *)((protocol_scratch_data + 77) + 0))
#define DAT_20003cac (*(uchar *)((protocol_scratch_data + 78) + 0))
#define DAT_20003cad (*(uchar *)((protocol_scratch_data + 79) + 0))
#define DAT_20003cae (*(uchar *)((protocol_scratch_data + 80) + 0))
#define DAT_20003caf (*(uchar *)((protocol_scratch_data + 81) + 0))
#define DAT_20003cb0 (*(uchar *)((protocol_scratch_data + 82) + 0))
#define DAT_20003cb1 (*(uchar *)((protocol_scratch_data + 83) + 0))
#define DAT_20003cb2 (*(uchar *)((protocol_scratch_data + 84) + 0))
#define DAT_20003cb3 (*(uchar *)((protocol_scratch_data + 85) + 0))
#define DAT_20003cb4 (*(uchar *)((protocol_scratch_data + 86) + 0))
#define DAT_20003cb5 (*(uchar *)((protocol_scratch_data + 87) + 0))
#define DAT_20003cb6 (*(uchar *)((protocol_scratch_data + 88) + 0))
#define DAT_20003cb7 (*(uchar *)((protocol_scratch_data + 89) + 0))
#define DAT_20003cb8 (*(uchar *)((protocol_scratch_data + 90) + 0))
#define DAT_20003cb9 (*(uchar *)((protocol_scratch_data + 91) + 0))
#define DAT_20003cba (*(uchar *)((protocol_scratch_data + 92) + 0))
#define DAT_20003cbb (*(uchar *)((protocol_scratch_data + 93) + 0))
#define DAT_20003cbc (*(uchar *)((protocol_scratch_data + 94) + 0))
#define DAT_20003cbd (*(uchar *)((protocol_scratch_data + 95) + 0))
#define DAT_20003cc2 (*(uchar *)((protocol_scratch_data + 100) + 0))
#define DAT_20003cc3 (*(uchar *)((protocol_scratch_data + 101) + 0))
#define DAT_20003cc4 (*(uchar *)((protocol_scratch_data + 102) + 0))
#define DAT_20003cc5 (*(uchar *)((protocol_scratch_data + 103) + 0))
#define DAT_20003cc6 (*(uchar *)((protocol_scratch_data + 104) + 0))
#define DAT_20003cc7 (*(uchar *)((protocol_scratch_data + 105) + 0))
#define DAT_20003cc8 (*(uchar *)((protocol_scratch_data + 106) + 0))
#define DAT_20003cc9 (*(uchar *)((protocol_scratch_data + 107) + 0))
#define DAT_20003cca (*(uchar *)((protocol_scratch_data + 108) + 0))
#define DAT_20003ccb (*(uchar *)((protocol_scratch_data + 109) + 0))
#define DAT_20003ccc (*(uchar *)((protocol_scratch_data + 110) + 0))
#define DAT_20003ccd (*(uchar *)((protocol_scratch_data + 111) + 0))
#define DAT_20003cce (*(uchar *)((protocol_scratch_data + 112) + 0))
#define DAT_20003ccf (*(uchar *)((protocol_scratch_data + 113) + 0))
#define DAT_20003cd0 (*(uchar *)((protocol_scratch_data + 114) + 0))
#define DAT_20003cd1 (*(uchar *)((protocol_scratch_data + 115) + 0))
#define DAT_20003cd2 (*(uchar *)((protocol_scratch_data + 116) + 0))
#define DAT_20003cd3 (*(uchar *)((protocol_scratch_data + 117) + 0))
#define DAT_20003cd4 (*(uchar *)((protocol_scratch_data + 118) + 0))
#define DAT_20003cd5 (*(uchar *)((protocol_scratch_data + 119) + 0))
#define DAT_200044ff (*(uchar *)((protocol_reply + 0) + 1))
#define DAT_20004500 (*(uchar *)((protocol_reply + 0) + 2))
#define DAT_20004501 (*(uchar *)((protocol_reply + 0) + 3))
#define g_200044FE_v (*(uchar *)((protocol_reply + 0) + 0))

static __inline uint protocol_word(ushort v) {
    return v;
}
void pack_reply_register_read(int request_address) {
    uchar *cell_bytes;
    ushort index;
    uchar high;
    ushort cell_voltage;
    uchar invalid_request;
    uint work_value;
    ushort reply_length;
    uint start_register;
    uint register_count;

    work_value = 0;
    reply_length = 0;
    invalid_request = false;
    high = *(uchar *)(request_address + 2);
    start_register = *(uchar *)(request_address + 3) | ((uint)high << 8);
    high = *(uchar *)(request_address + 4);
    register_count = *(uchar *)(request_address + 5) | ((uint)high << 8);
    if ((register_count > 0x3c) || (register_count == 0)) {
        invalid_request = true;
    }
    if ((start_register / 0x3c + 1) * 0x3c < start_register + register_count) {
        invalid_request = true;
    }
    if (invalid_request == 1) {
        DAT_200044ff = 0x83;
        work_value = protocol_crc16(&g_200044FE_v, 0x10);
        DAT_20004500 = (undefined1)work_value;
        DAT_20004501 = (undefined1)(work_value >> 8);
        DAT_2000058d = 1;
    } else {
        do {
            (&DAT_20003c5e)[work_value] = 0;
            work_value = work_value + 1 & 0xffff;
        } while (work_value < 0x78);
        if (start_register < 0x3c) {
            work_value = 0;
            do {
                (&DAT_20003c5e)[work_value] = (&g_20000077_v)[work_value];
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 0x15);
            DAT_20003c73 = 0;
            DAT_20003c74 = (undefined1)((ushort)g_2000119E_v >> 8);
            DAT_20003c75 = (undefined1)g_2000119E_v;
            DAT_20003c76 = (undefined1)((ushort)DAT_200011a0 >> 8);
            DAT_20003c77 = (undefined1)DAT_200011a0;
            DAT_20003c78 = (undefined1)((ushort)DAT_200011a2 >> 8);
            DAT_20003c79 = (undefined1)DAT_200011a2;
            DAT_20003c7a = (undefined1)((ushort)DAT_200011a4 >> 8);
            DAT_20003c7b = (undefined1)DAT_200011a4;
            DAT_20003c7c = (undefined1)((ushort)DAT_200011a6 >> 8);
            DAT_20003c7d = (undefined1)DAT_200011a6;
            DAT_20003c7e = g_20000051_v;
            if (g_20000138_v == '\x01') {
                DAT_20003c7f = 1;
            } else if (g_20000139_v == '\x01') {
                DAT_20003c7f = 2;
            } else {
                DAT_20003c7f = 0;
            }
            DAT_20003c80 = g_20000098_v;
            DAT_20003c81 = g_200000AD_v;
            DAT_20003c82 = (undefined1)(g_20000130_v / 10 >> 8);
            DAT_20003c83 = (undefined1)(g_20000130_v / 10);
            DAT_20003c84 = (undefined1)(g_20000134_v / 10 >> 8);
            DAT_20003c85 = (undefined1)(g_20000134_v / 10);
            DAT_20003c86 = g_200000A0_v;
            DAT_20003c87 = g_20000184_v;
            DAT_20003c88 = measurement_cell_count;
            DAT_20003c89 = (undefined1)((ushort)g_200000A2_v >> 8);
            DAT_20003c8a = (undefined1)g_200000A2_v;
            DAT_20003c8b = (undefined1)((uint)g_20000124_v >> 0x18);
            DAT_20003c8c = (undefined1)((uint)g_20000124_v >> 0x10);
            DAT_20003c8d = (undefined1)((uint)g_20000124_v >> 8);
            DAT_20003c8e = (undefined1)g_20000124_v;
            DAT_20003c8f = (undefined1)((ushort)measurement_pack_voltage >> 8);
            DAT_20003c90 = (undefined1)measurement_pack_voltage;
            work_value = __aeabi_f2iz(measurement_current);
            DAT_20003c91 = (undefined1)(work_value >> 0x18);
            DAT_20003c92 = (undefined1)(work_value >> 0x10);
            DAT_20003c93 = (undefined1)(work_value >> 8);
            DAT_20003c94 = (undefined1)work_value;
            work_value = __aeabi_f2uiz(g_20000164_v);
            DAT_20003c95 = (undefined1)(work_value >> 0x18);
            DAT_20003c96 = (undefined1)(work_value >> 0x10);
            DAT_20003c97 = (undefined1)(work_value >> 8);
            DAT_20003c98 = (undefined1)work_value;
            work_value = protocol_word(g_20000182_v);
            DAT_20003c99 = work_value >> 24;
            DAT_20003c9a = work_value >> 16;
            DAT_20003c9b = work_value >> 8;
            DAT_20003c9c = work_value;
            work_value = __aeabi_f2uiz(g_2000016C_v);
            DAT_20003c9d = (undefined1)(work_value >> 0x18);
            DAT_20003c9e = (undefined1)(work_value >> 0x10);
            DAT_20003c9f = (undefined1)(work_value >> 8);
            DAT_20003ca0 = (undefined1)work_value;
            DAT_20003ca1 = g_20000185_v;
            DAT_20003ca2 = g_20000271_v;
            DAT_20003ca3 = g_20000272_v;
            DAT_20003ca4 = g_20000273_v;
            DAT_20003ca5 = g_2000050D_v;
            DAT_20003ca6 = g_20000274_v;
            DAT_20003ca7 = g_20000275_v;
            DAT_20003ca8 = g_20000276_v;
            DAT_20003ca9 = g_20000277_v;
            DAT_20003caa = g_20000278_v;
            DAT_20003cab = 0xb;
            DAT_20003cac = 0xce;
            work_value = 0;
            do {
                cell_bytes = &DAT_20003c5e + work_value * 2;
                cell_voltage = (&g_20001094_v)[work_value];
                cell_bytes[79] = (ushort)cell_voltage >> 8;
                cell_bytes[80] = cell_voltage;
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 0x10);
            DAT_20003ccd = (undefined1)((ushort)measurement_maximum_temperature >> 8);
            DAT_20003cce = (undefined1)measurement_maximum_temperature;
            DAT_20003ccf = (undefined1)((ushort)measurement_minimum_temperature >> 8);
            DAT_20003cd0 = (undefined1)measurement_minimum_temperature;
            DAT_20003cd1 = (undefined1)((ushort)measurement_maximum_cell_voltage >> 8);
            DAT_20003cd2 = (undefined1)measurement_maximum_cell_voltage;
            DAT_20003cd3 = (undefined1)((ushort)measurement_minimum_cell_voltage >> 8);
            DAT_20003cd4 = (undefined1)measurement_minimum_cell_voltage;
            DAT_20003cd5 = 0;
            index = (ushort)(start_register * 2);
            reply_length = 4;
            for (; index < start_register * 2 + register_count * 2; index++, reply_length++) {
                (&g_200044FE_v)[reply_length] = (&DAT_20003c5e)[index];
            }
        } else if (start_register < 0x78) {
            DAT_20003c5e = 0;
            DAT_20003c5f = 0;
            DAT_20003c60 = 0;
            DAT_20003c61 = 0;
            DAT_20003c62 = 0;
            DAT_20003c63 = 0;
            DAT_20003c64 = 0;
            DAT_20003c65 = 0;
            DAT_20003c66 = 0;
            DAT_20003c67 = 0;
            DAT_20003c68 = 0;
            DAT_20003c69 = 0;
            DAT_20003c6a = 0;
            DAT_20003c6b = 0;
            DAT_20003c6c = 0;
            DAT_20003c6d = 0;
            work_value = 0;
            do {
                (&DAT_20003c6e)[work_value] = 0;
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 10);
            work_value = 0;
            do {
                (&DAT_20003c78)[work_value] = 0;
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 10);
            DAT_20003c82 = 0;
            DAT_20003c83 = 0;
            DAT_20003c84 = 0;
            DAT_20003c85 = 0;
            DAT_20003c86 = 0;
            DAT_20003c87 = 0;
            DAT_20003c88 = 0;
            DAT_20003c89 = 0;
            DAT_20003c8a = 0;
            DAT_20003c8b = 0;
            DAT_20003c8c = 0;
            DAT_20003c8d = 0;
            DAT_20003c8e = 0;
            DAT_20003c8f = 0;
            DAT_20003c90 = 0;
            DAT_20003c91 = 0;
            DAT_20003c92 = 0;
            DAT_20003c93 = 0;
            DAT_20003c94 = 0;
            DAT_20003c95 = 0;
            DAT_20003c96 = 0;
            DAT_20003c97 = 0;
            DAT_20003c98 = 0;
            DAT_20003c99 = 0;
            DAT_20003c9a = 0;
            DAT_20003c9b = 0;
            DAT_20003c9c = 0;
            DAT_20003c9d = 0;
            DAT_20003c9e = 0;
            DAT_20003c9f = 0;
            DAT_20003ca0 = 0;
            DAT_20003ca1 = 0;
            DAT_20003ca2 = 0;
            DAT_20003ca3 = 0;
            DAT_20003ca4 = 0;
            DAT_20003ca5 = 0;
            DAT_20003ca6 = 0;
            DAT_20003ca7 = 0;
            DAT_20003ca8 = 0;
            DAT_20003ca9 = 0;
            DAT_20003caa = 0;
            DAT_20003cab = 0;
            DAT_20003cac = 0;
            DAT_20003cad = 0;
            DAT_20003cae = 0;
            DAT_20003caf = 0;
            DAT_20003cb0 = 0;
            DAT_20003cb1 = 0;
            DAT_20003cb2 = 0;
            DAT_20003cb3 = 0;
            DAT_20003cb4 = 0;
            DAT_20003cb5 = 0;
            DAT_20003cb6 = 0;
            DAT_20003cb7 = 0;
            DAT_20003cb8 = 0;
            DAT_20003cb9 = 0;
            DAT_20003cba = 0;
            DAT_20003cbb = 0;
            DAT_20003cbc = 0;
            DAT_20003cbd = 0;
            DAT_20003cc2 = 0;
            DAT_20003cc3 = 0;
            DAT_20003cc4 = 0;
            DAT_20003cc5 = 0;
            DAT_20003cc6 = 0;
            DAT_20003cc7 = 0;
            DAT_20003cc8 = 0;
            DAT_20003cc9 = 0;
            DAT_20003cca = 0;
            DAT_20003ccb = 0;
            DAT_20003ccc = 0;
            DAT_20003ccd = 0;
            DAT_20003cce = 0;
            DAT_20003ccf = 0;
            DAT_20003cd0 = 0;
            DAT_20003cd1 = 0;
            DAT_20003cd2 = 0;
            DAT_20003cd3 = 0;
            DAT_20003cd4 = 0;
            DAT_20003cd5 = 0;
            index = (ushort)(start_register * 2 - 0x78);
            reply_length = 4;
            for (; (int)index < (int)(start_register * 2 + register_count * 2 + -0x78);
                 index++, reply_length++) {
                (&g_200044FE_v)[reply_length] = (&DAT_20003c5e)[index];
            }
        } else if (start_register < 0xb4) {
            DAT_20003c5e = 0;
            DAT_20003c5f = 0;
            DAT_20003c60 = 0;
            DAT_20003c61 = 0;
            DAT_20003c62 = 0;
            DAT_20003c63 = 0;
            DAT_20003c64 = 0;
            DAT_20003c65 = 0;
            DAT_20003c66 = 0;
            DAT_20003c67 = 0;
            DAT_20003c68 = 0;
            DAT_20003c69 = 0;
            DAT_20003c6a = 0;
            DAT_20003c6b = 0;
            DAT_20003c6c = 0;
            DAT_20003c6d = 0;
            work_value = 0;
            do {
                (&DAT_20003c6e)[work_value] = 0;
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 10);
            work_value = 0;
            do {
                (&DAT_20003c78)[work_value] = 0;
                work_value = work_value + 1 & 0xffff;
            } while (work_value < 10);
            DAT_20003c82 = 0;
            DAT_20003c83 = 0;
            DAT_20003c84 = 0;
            DAT_20003c85 = 0;
            DAT_20003c86 = 0;
            DAT_20003c87 = 0;
            DAT_20003c88 = 0;
            DAT_20003c89 = 0;
            DAT_20003c8a = 0;
            DAT_20003c8b = 0;
            DAT_20003c8c = 0;
            DAT_20003c8d = 0;
            DAT_20003c8e = 0;
            DAT_20003c8f = 0;
            DAT_20003c90 = 0;
            DAT_20003c91 = 0;
            DAT_20003c92 = 0;
            DAT_20003c93 = 0;
            DAT_20003c94 = 0;
            DAT_20003c95 = 0;
            DAT_20003c96 = 0;
            DAT_20003c97 = 0;
            DAT_20003c98 = 0;
            DAT_20003c99 = 0;
            DAT_20003c9a = 0;
            DAT_20003c9b = 0;
            DAT_20003c9c = 0;
            DAT_20003c9d = 0;
            DAT_20003c9e = 0;
            DAT_20003c9f = 0;
            DAT_20003ca0 = 0;
            DAT_20003ca1 = 0;
            DAT_20003ca2 = 0;
            DAT_20003ca3 = 0;
            DAT_20003ca4 = 0;
            DAT_20003ca5 = 0;
            DAT_20003ca6 = 0;
            DAT_20003ca7 = 0;
            DAT_20003ca8 = 0;
            DAT_20003ca9 = 0;
            DAT_20003caa = 0;
            DAT_20003cab = 0;
            DAT_20003cac = 0;
            DAT_20003cad = 0;
            DAT_20003cae = 0;
            DAT_20003caf = 0;
            DAT_20003cb0 = 0;
            DAT_20003cb1 = 0;
            DAT_20003cb2 = 0;
            DAT_20003cb3 = 0;
            DAT_20003cb4 = 0;
            DAT_20003cb5 = 0;
            DAT_20003cb6 = 0;
            DAT_20003cb7 = 0;
            DAT_20003cb8 = 0;
            DAT_20003cb9 = 0;
            DAT_20003cba = 0;
            DAT_20003cbb = 0;
            DAT_20003cbc = 0;
            DAT_20003cbd = 0;
            DAT_20003cc2 = 0;
            DAT_20003cc3 = 0;
            DAT_20003cc4 = 0;
            DAT_20003cc5 = 0;
            DAT_20003cc6 = 0;
            DAT_20003cc7 = 0;
            DAT_20003cc8 = 0;
            DAT_20003cc9 = 0;
            DAT_20003cca = 0;
            DAT_20003ccb = 0;
            DAT_20003ccc = 0;
            DAT_20003ccd = 0;
            DAT_20003cce = 0;
            DAT_20003ccf = 0;
            DAT_20003cd0 = 0;
            DAT_20003cd1 = 0;
            DAT_20003cd2 = 0;
            DAT_20003cd3 = 0;
            DAT_20003cd4 = 0;
            DAT_20003cd5 = 0;
            index = (ushort)(start_register * 2 - 0xf0);
            reply_length = 4;
            for (; (int)index < (int)(start_register * 2 + register_count * 2 + -0xf0);
                 index++, reply_length++) {
                (&g_200044FE_v)[reply_length] = (&DAT_20003c5e)[index];
            }
        }
        if (reply_length > 0x96) {
            reply_length = 0x96;
        }
        work_value = protocol_crc16(&g_200044FE_v, reply_length);
        (&g_200044FE_v)[reply_length] = (char)work_value;
        (&DAT_200044ff)[reply_length] = (char)(work_value >> 8);
    }
    return;
}
#undef g_20000051_v
#undef g_20000077_v
#undef g_20000098_v
#undef g_200000A0_v
#undef g_200000A2_v
#undef g_200000AD_v
#undef g_20000124_v
#undef g_20000130_v
#undef g_20000134_v
#undef g_20000138_v
#undef g_20000139_v
#undef g_20000164_v
#undef g_2000016C_v
#undef g_20000182_v
#undef g_20000184_v
#undef g_20000185_v
#undef g_20000271_v
#undef g_20000272_v
#undef g_20000273_v
#undef g_20000274_v
#undef g_20000275_v
#undef g_20000276_v
#undef g_20000277_v
#undef g_20000278_v
#undef g_2000050D_v
#undef DAT_2000058d
#undef g_20001094_v
#undef DAT_200011a0
#undef DAT_200011a2
#undef DAT_200011a4
#undef DAT_200011a6
#undef g_2000119E_v
#undef DAT_20003c5e
#undef DAT_20003c5f
#undef DAT_20003c60
#undef DAT_20003c61
#undef DAT_20003c62
#undef DAT_20003c63
#undef DAT_20003c64
#undef DAT_20003c65
#undef DAT_20003c66
#undef DAT_20003c67
#undef DAT_20003c68
#undef DAT_20003c69
#undef DAT_20003c6a
#undef DAT_20003c6b
#undef DAT_20003c6c
#undef DAT_20003c6d
#undef DAT_20003c6e
#undef DAT_20003c73
#undef DAT_20003c74
#undef DAT_20003c75
#undef DAT_20003c76
#undef DAT_20003c77
#undef DAT_20003c78
#undef DAT_20003c79
#undef DAT_20003c7a
#undef DAT_20003c7b
#undef DAT_20003c7c
#undef DAT_20003c7d
#undef DAT_20003c7e
#undef DAT_20003c7f
#undef DAT_20003c80
#undef DAT_20003c81
#undef DAT_20003c82
#undef DAT_20003c83
#undef DAT_20003c84
#undef DAT_20003c85
#undef DAT_20003c86
#undef DAT_20003c87
#undef DAT_20003c88
#undef DAT_20003c89
#undef DAT_20003c8a
#undef DAT_20003c8b
#undef DAT_20003c8c
#undef DAT_20003c8d
#undef DAT_20003c8e
#undef DAT_20003c8f
#undef DAT_20003c90
#undef DAT_20003c91
#undef DAT_20003c92
#undef DAT_20003c93
#undef DAT_20003c94
#undef DAT_20003c95
#undef DAT_20003c96
#undef DAT_20003c97
#undef DAT_20003c98
#undef DAT_20003c99
#undef DAT_20003c9a
#undef DAT_20003c9b
#undef DAT_20003c9c
#undef DAT_20003c9d
#undef DAT_20003c9e
#undef DAT_20003c9f
#undef DAT_20003ca0
#undef DAT_20003ca1
#undef DAT_20003ca2
#undef DAT_20003ca3
#undef DAT_20003ca4
#undef DAT_20003ca5
#undef DAT_20003ca6
#undef DAT_20003ca7
#undef DAT_20003ca8
#undef DAT_20003ca9
#undef DAT_20003caa
#undef DAT_20003cab
#undef DAT_20003cac
#undef DAT_20003cad
#undef DAT_20003cae
#undef DAT_20003caf
#undef DAT_20003cb0
#undef DAT_20003cb1
#undef DAT_20003cb2
#undef DAT_20003cb3
#undef DAT_20003cb4
#undef DAT_20003cb5
#undef DAT_20003cb6
#undef DAT_20003cb7
#undef DAT_20003cb8
#undef DAT_20003cb9
#undef DAT_20003cba
#undef DAT_20003cbb
#undef DAT_20003cbc
#undef DAT_20003cbd
#undef DAT_20003cc2
#undef DAT_20003cc3
#undef DAT_20003cc4
#undef DAT_20003cc5
#undef DAT_20003cc6
#undef DAT_20003cc7
#undef DAT_20003cc8
#undef DAT_20003cc9
#undef DAT_20003cca
#undef DAT_20003ccb
#undef DAT_20003ccc
#undef DAT_20003ccd
#undef DAT_20003cce
#undef DAT_20003ccf
#undef DAT_20003cd0
#undef DAT_20003cd1
#undef DAT_20003cd2
#undef DAT_20003cd3
#undef DAT_20003cd4
#undef DAT_20003cd5
#undef DAT_200044ff
#undef DAT_20004500
#undef DAT_20004501
#undef g_200044FE_v

extern uchar g_20000163;
extern ushort g_20000606;
extern uchar g_20000A9C[];
#define DAT_200044ff (*(uchar *)((protocol_reply + 0) + 1))
#define DAT_20004500 (*(uchar *)((protocol_reply + 0) + 2))
#define DAT_20004501 (*(sbyte *)((protocol_reply + 0) + 3))
#define g_200044FE_v (*(uchar *)((protocol_reply + 0) + 0))

void pack_dispatch_frame(byte *frame, uchar length) {
    uchar mine = 0;
    uchar ok = 0;
    uint crc;
    uint i;
    uchar fc;

    crc = protocol_crc16(frame, 6);
    protocol_reserved24 = crc;
    if ((crc & 0xff) == frame[7] && frame[6] == crc >> 8)
        ok = 1;
    if (ok == 1) {
        if (frame[0] == g_20000163 || frame[0] == 0)
            mine = 1;
        if (mine == 1) {
            protocol_reply[0] = g_20000163;
            protocol_reply[1] = frame[1];
            fc = frame[1];
            if (frame[1] >= BMS_MODBUS_READ_HOLDING_REGISTERS &&
                fc <= BMS_MODBUS_WRITE_SINGLE_REGISTER) {
                if (frame[1] == BMS_MODBUS_READ_HOLDING_REGISTERS)
                    pack_reply_register_read((int)frame);
                else if (frame[1] == BMS_MODBUS_WRITE_SINGLE_REGISTER) {
                    pack_handle_register_write(frame);
                    g_2000058C = 1;
                } else
                    g_2000058C = 1;
            } else
                g_2000058C = 1;
            if (g_2000058C == 1) {
                g_2000058C = 0;
                return;
            }
            if (frame[1] == BMS_MODBUS_WRITE_SINGLE_REGISTER)
                protocol_byte2 = 8;
            else {
                crc = (g_20000A9C[4] * 256 + g_20000A9C[5]) * 2 & 0xffff;
                protocol_reply[2] = crc >> 8;
                protocol_reply[3] = crc;
                protocol_byte2 = crc + 6;
            }
            for (i = 0; i < protocol_byte2; i = i + 1 & 0xff)
                protocol_out[i] = protocol_reply[i];
            board_set_pa8(1);
            os_task_delay((uint)(byte)g_20000606, 10, 5);
            sub_08014F94(0, protocol_out, protocol_byte2);
        }
    }
}
#undef DAT_200044ff
#undef DAT_20004500
#undef DAT_20004501
#undef g_200044FE_v

#define DAT_2000059a (protocol_byte14)
#define DAT_2000059b (protocol_byte15)
#define DAT_200005b2 (protocol_crc38)
extern uchar g_20000BC9[];
#define g_20000BC9_v (*(uchar *)(g_20000BC9 + 0))

void pack_send_pending_settings(void) {
    uint crc;
    ushort reg, value;
    DAT_2000059a = 8;
    g_20000BC9[0] = DAT_2000059b;
    g_20000BC9[1] = BMS_MODBUS_WRITE_SINGLE_REGISTER;
    if (protocol_pending[DAT_2000059b].flags != 0) {
        if (protocol_pending[DAT_2000059b].flags & 1) {
            protocol_pending[DAT_2000059b].flags &= ~1U;
            reg = 2;
            value = protocol_pending[DAT_2000059b].x8;
            protocol_pending[DAT_2000059b].x8 = 0;
        } else if (protocol_pending[DAT_2000059b].flags & 2) {
            protocol_pending[DAT_2000059b].flags &= ~2U;
            reg = 3;
            value = protocol_pending[DAT_2000059b].xA;
            protocol_pending[DAT_2000059b].xA = 0;
        } else if (protocol_pending[DAT_2000059b].flags & 4) {
            protocol_pending[DAT_2000059b].flags &= ~4U;
            reg = 4;
            value = protocol_pending[DAT_2000059b].xC;
            protocol_pending[DAT_2000059b].xC = 0;
        } else if (protocol_pending[DAT_2000059b].flags & 8) {
            protocol_pending[DAT_2000059b].flags &= ~8U;
            reg = 5;
            value = protocol_pending[DAT_2000059b].xE;
            protocol_pending[DAT_2000059b].xE = 0;
        } else if (protocol_pending[DAT_2000059b].flags & 16) {
            protocol_pending[DAT_2000059b].flags &= ~16U;
            reg = 6;
            value = 1;
        } else if (protocol_pending[DAT_2000059b].flags & (1U << 5))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 5);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 6))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 6);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 7))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 7);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 8))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 8);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 9))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 9);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 10))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 10);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 11))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 11);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 12))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 12);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 13))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 13);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 14))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 14);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 15))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 15);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 16))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 16);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 17))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 17);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 18))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 18);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 19))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 19);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 20))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 20);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 21))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 21);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 22))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 22);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 23))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 23);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 24))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 24);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 25))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 25);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 26))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 26);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 27))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 27);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 28))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 28);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 29))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 29);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 30))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 30);
        else if (protocol_pending[DAT_2000059b].flags & (1U << 31))
            protocol_pending[DAT_2000059b].flags &= ~(1U << 31);
    } else {
        if (++DAT_2000059b >= 6)
            DAT_2000059b = 1;
    }
    g_20000BC9[2] = reg >> 8;
    g_20000BC9[3] = reg;
    g_20000BC9[4] = value >> 8;
    g_20000BC9[5] = value;
    crc = protocol_crc16(g_20000BC9, 6);
    DAT_200005b2 = crc;
    g_20000BC9[6] = crc >> 8;
    g_20000BC9[7] = crc;
    board_set_pa8(1);
    sub_08014F94(0, g_20000BC9, DAT_2000059a);
}

#undef DAT_2000059a
#undef DAT_2000059b
#undef DAT_200005b2
#undef g_20000BC9_v

extern uchar g_2000004F;
#define g_2000004F_v (g_2000004F)
#define DAT_200005a2 (protocol_reserved22)
#define g_20003CE0_v (*(uchar *)((protocol_stats + 0) + 0))
#define protocol_pack_value (*(ushort *)(((uchar *)protocol_pack) + 0))

void pack_store_register_reply(byte *frame, ushort data_length) {
    uint work_value;

    work_value = protocol_crc16(frame, data_length);
    DAT_200005a2 = (undefined2)work_value;
    if ((work_value & 0xff) == frame[data_length] && frame[data_length + 1] == work_value >> 8) {
        if ((uint)(frame[1] - BMS_MODBUS_READ_HOLDING_REGISTERS) <= 1) {
            *(undefined2 *)(&g_20003CE0_v + (uint)g_2000004F_v * 0x10) = 1;
            if (*frame >= 6) {
                *frame = 6;
            }
            if (*frame <= 1) {
                *frame = 1;
            }
            protocol_records[*frame].reply_seen = 1;
            work_value = 0;
            do {
                protocol_records[*frame].identification[work_value] = frame[work_value + 4];
                work_value = work_value + 1 & 0xff;
            } while (work_value < 0x15);
            protocol_records[*frame].temperature_0 = BMS_READ_BE16(frame, 0x1a);
            protocol_records[*frame].temperature_1 = BMS_READ_BE16(frame, 0x1c);
            protocol_records[*frame].temperature_2 = BMS_READ_BE16(frame, 0x1e);
            protocol_records[*frame].mosfet_temperature = BMS_READ_BE16(frame, 0x20);
            protocol_records[*frame].ambient_temperature = BMS_READ_BE16(frame, 0x22);
            protocol_records[*frame].usb_class = frame[0x24];
            protocol_records[*frame].voltage_hold_mode = frame[0x25];
            protocol_records[*frame].output_phase_toggle = frame[0x26];
            protocol_records[*frame].soc_voltage_hold = frame[0x27];
            protocol_records[*frame].charge_current_limit = BMS_READ_BE16(frame, 0x28);
            protocol_records[*frame].discharge_current_limit = BMS_READ_BE16(frame, 0x2a);
            protocol_records[*frame].participation_state = frame[0x2c];
            protocol_records[*frame].soc_primary = frame[0x2d];
            protocol_records[*frame].cell_count = frame[0x2e];
            protocol_records[*frame].cycle_count = BMS_READ_BE16(frame, 0x2f);
            protocol_records[*frame].auxiliary_voltage = BMS_READ_BE32(frame, 0x31);
            protocol_records[*frame].pack_voltage = BMS_READ_BE16(frame, 0x35);
            protocol_records[*frame].pack_current = BMS_READ_BE32(frame, 0x37);
            protocol_records[*frame].full_capacity = BMS_READ_BE32(frame, 0x3b);
            protocol_records[*frame].rated_capacity = BMS_READ_BE32(frame, 0x3f);
            protocol_records[*frame].remaining_capacity = BMS_READ_BE32(frame, 0x43);
            protocol_records[*frame].soc_secondary = frame[0x47];
            protocol_records[*frame].voltage_current_protection_flags = frame[0x48];
            protocol_records[*frame].temperature_protection_flags = frame[0x49];
            protocol_records[*frame].output_status_flags = frame[0x4a];
            protocol_records[*frame].current_direction_latch_flags = frame[0x4b];
            protocol_records[*frame].hardware_fault_flags = frame[0x4c];
            protocol_records[*frame].protection_inhibit_flags = frame[0x4d];
            protocol_records[*frame].capacity_latch_flags = frame[0x4e];
            protocol_records[*frame].voltage_current_limit_flags = frame[0x4f];
            protocol_records[*frame].temperature_limit_flags = frame[0x50];
            protocol_records[*frame].firmware_version = BMS_READ_BE16(frame, 0x51);
            work_value = 0;
            do {
                protocol_records[*frame].cell_voltages[work_value] =
                    (ushort)frame[work_value * 2 + 0x54] +
                    (ushort)frame[work_value * 2 + 0x53] * 0x100;
                work_value = work_value + 1 & 0xff;
            } while (work_value < 0x10);
            protocol_records[*frame].maximum_temperature = BMS_READ_BE16(frame, 0x73);
            protocol_records[*frame].minimum_temperature = BMS_READ_BE16(frame, 0x75);
            protocol_records[*frame].maximum_cell_voltage = BMS_READ_BE16(frame, 0x77);
            protocol_records[*frame].minimum_cell_voltage = BMS_READ_BE16(frame, 0x79);
        }
    } else {
        *(undefined2 *)(&g_20003CE0_v + (uint)g_2000004F_v * 0x10) = 0;
    }
    return;
}
#undef g_2000004F_v
#undef DAT_200005a2
#undef g_20003CE0_v
#undef protocol_pack_value

/* at: g_2000004F 0x2000004F, g_2000005A 0x2000005A, g_2000005B 0x2000005B, g_2000005C 0x2000005C, g_2000009F 0x2000009F, g_200000AA 0x200000AA, g_200000D5 0x200000D5, g_200000D8 0x200000D8, g_200000D9 0x200000D9, g_200000DA 0x200000DA, g_200000DC 0x200000DC, g_200000DF 0x200000DF, g_200000E1 0x200000E1, g_200000EE 0x200000EE, g_200000F8 0x200000FA, g_200000FC 0x20000100, g_200000FE 0x20000102, g_20000100 0x20000104, measurement_pack_voltage 0x20000114, g_20000124 0x20000128, g_2000013C 0x20000140, g_2000013D 0x20000141, measurement_current 0x2000014C, g_20000163 0x20000167, g_2000017C 0x20000180, g_20000180 0x20000184, g_20000182 0x20000186, g_20000185 0x20000189, g_20000276 0x2000027E, g_2000058C 0x200005A4, g_20000606 0x2000061E, g_20000A9C 0x20000AB4, g_20000BC9 0x20000BE1, g_20000C60 0x20000C78, ((uchar*)protocol_timers) 0x20003BE8, protocol_clear 0x20003BF4, protocol_flags 0x20003D58, protocol_pack 0x20003D6A, g_2000748C 0x200074A4 */
void GPIO_ResetBits(void *, ushort);
uint os_flags_pend(ushort);
void sub_08014AC8(int, uint);
undefined4 sub_08014F36(int, undefined1 *);
void firmware_relay_poll(void);
void board_set_pa8(int);
void sub_08014F94(int, undefined4, undefined2);
undefined4 *__aeabi_memclr(undefined4 *, uint);
float __aeabi_ui2f(uint);
void pack_update_peer_participation_flags(void);

#define DAT_40010c00 (*(volatile uchar *)0x40010C00)
#define DAT_40011000 (*(volatile uchar *)0x40011000)
#define DAT_40011400 (*(volatile uchar *)0x40011400)
extern uchar g_2000004F;
#define DAT_2000004f (g_2000004F)
extern uchar g_2000005A;
#define DAT_2000005a (g_2000005A)
extern uchar g_2000005B;
#define DAT_2000005b (g_2000005B)
extern uchar g_2000005C;
#define DAT_2000005c (g_2000005C)
extern ushort g_200000DC;
#define DAT_200000dc (g_200000DC)
extern uchar g_200000DF;
#define DAT_200000df (g_200000DF)
extern uchar g_200000E1;
#define DAT_200000e1 (g_200000E1)
extern uchar g_200000EE;
#define DAT_200000ee (g_200000EE)
extern ushort g_200000FC;
#define DAT_200000fc (*(volatile ushort *)&g_200000FC)
extern ushort g_200000FE;
#define DAT_200000fe (g_200000FE)
extern uchar g_20000163;
#define DAT_20000163 (g_20000163)
extern uchar g_20000276;
#define DAT_20000276 (*(volatile uchar *)&g_20000276)
#define DAT_20000591 (g_20000591)
#define DAT_20000592 (protocol_byte6)
#define DAT_20000593 (protocol_byte7)
#define DAT_20000594 (protocol_byte8)
#define DAT_20000595 (protocol_byte9)
#define DAT_2000059e (protocol_poll18)
#define DAT_200005a0 (protocol_uart20)
extern uchar g_20000A9C[];
#define protocol_stats (*(uchar *)(((uchar *)protocol_timers) + 272))
#define protocol_pack (*(ushort *)((uchar *)protocol_flags + 18))
extern uchar g_2000748C[];
#define DAT_200074be (*(ushort *)(g_2000748C + 50))
#define DAT_200074c0 (*(ushort *)(g_2000748C + 52))

#undef protocol_stats
#undef protocol_pack
typedef __packed struct {
    ushort b0 : 1;
    ushort b1 : 1;
    ushort b2 : 1;
    ushort b3 : 1;
    ushort b4 : 1;
    ushort b5 : 1;
    ushort b6 : 1;
    ushort b7 : 1;
    ushort b8 : 1;
    ushort b9 : 1;
    ushort b10 : 1;
    ushort b11 : 1;
    ushort b12 : 1;
    ushort b13 : 1;
    ushort b14 : 1;
    ushort b15 : 1;
    uchar spare;
} ProtocolFlagBits;
void pack_protocol_task(void) {
    uchar i;
    ushort state;
    board_set_pa8(0);
    sub_08014AC8(0, 150);
    protocol_initialize();
    for (i = 0; i < 6; i++)
        protocol_timers[i] = 0;
    while (1) {
        os_flags_pend(0);
        if (DAT_200000ee != 1) {
            DAT_200074c0 = 0;
            DAT_200074be = 50;
        } else if (DAT_200000e1 == 1 || DAT_200000df == 1) {
            DAT_200000fc = 0;
            sub_08014AC8(0, 150);
            if (DAT_20000163 == 0 && DAT_200000e1 == 1)
                firmware_relay_poll();
            DAT_200074c0 = 0;
            DAT_200074be = 200;
        } else {
            if (DAT_20000163 == 0) {
                while (sub_08014F36(0, &g_20000591) == 0) {
                    switch (state = DAT_200000fc) {
                    case 0:
                        if (DAT_20000591 == DAT_20000593) {
                            g_20000A9C[0] = DAT_20000591;
                            DAT_200000fc = 1;
                        } else {
                            ((ProtocolStat *)protocol_stats)[DAT_2000004f].ready = 0;
                            DAT_200000fc = 0;
                        }
                        break;
                    case 1:
                        if (DAT_20000591 == 3 || DAT_20000591 == 4 || DAT_20000591 == 6 ||
                            DAT_20000591 == 16) {
                            g_20000A9C[1] = DAT_20000591;
                            DAT_200000fc = 2;
                        } else {
                            ((ProtocolStat *)protocol_stats)[DAT_2000004f].ready = 0;
                            DAT_200000fc = 0;
                        }
                        break;
                    case 3:
                        g_20000A9C[3] = DAT_20000591;
                        DAT_200000fe = DAT_20000591 + g_20000A9C[2] * 256;
                        DAT_200000fc = 4;
                        break;
                    default: {
                        ushort frame_length;
                        g_20000A9C[state] = DAT_20000591;
                        state = state + 1;
                        DAT_200000fc = state;
                        frame_length = DAT_200000fe;
                        if (state >= frame_length + 6) {
                            pack_store_register_reply(g_20000A9C, (uchar)(frame_length + 4));
                            DAT_200000fc = 0;
                        }
                        if (DAT_200000fc > 300)
                            DAT_200000fc = 0;
                    } break;
                    }
                    DAT_20000592 = 0;
                    DAT_200005a0 = 0;
                }
                if (DAT_2000059e++ > 3) {
                    DAT_2000059e = 0;
                    if (++DAT_20000595 >= 6)
                        DAT_20000595 = 0;
                    DAT_2000004f = DAT_20000593;
                    if (DAT_2000004f > 6)
                        DAT_2000004f = 5;
                    if (DAT_2000004f <= 1)
                        DAT_2000004f = 1;
                    if (((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b3 &&
                        !((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b6) {
                        ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b6 = 1;
                        ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b3 = 0;
                        pack_send_request(
                            DAT_20000593, BMS_MODBUS_WRITE_SINGLE_REGISTER, 1,
                            ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.value);
                        ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b0 = 0;
                        ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b1 = 0;
                        continue;
                    } else if (DAT_20000595 == 5) {
                        pack_send_pending_settings();
                        continue;
                    } else {
                        ((ProtocolFlag *)protocol_flags)[DAT_2000004f].word.bits.b6 = 0;
                        pack_send_request(DAT_20000593, BMS_MODBUS_READ_HOLDING_REGISTERS, 0, 60);
                        ((ProtocolStat *)protocol_stats)[DAT_2000004f].pending = 1;
                        DAT_20000594 = 0;
                    }
                }

                {
                    uint previous_tick = DAT_20000594;
                    uint next_tick = previous_tick + 1;
                    DAT_20000594 = next_tick;
                    if (previous_tick >= 2) {
                        uchar slot;
                        ProtocolStat *stat;
                        ushort pending;
                        uint lifetime_diagnostic = 0;
                        slot = DAT_2000004f;
                        lifetime_diagnostic =
                            (uint)((uchar *)(stat = ((ProtocolStat *)protocol_stats) + slot) !=
                                   ((uchar *)protocol_stats) + (next_tick & 1));
                        pending = stat->pending;
                        if (pending == 1) {
                            DAT_20000594 = 0;
                            if (++DAT_20000593 >= 6)
                                DAT_20000593 = 1;
                            if (stat->ready == 1) {
                                stat->error = 0;
                                stat->pending = 0;
                                stat->ready = 0;
                                DAT_20000592 = 0;
                                protocol_timers[slot] = 0;
                            }
                        }
                    }
                }
                for (i = 1; i < 6; i++)
                    if (protocol_timers[i]++ >= 2400) {
                        protocol_timers[i] = 0;
                        DAT_20000594 = 0;
                        ((ProtocolStat *)protocol_stats)[i].error = 0;
                        ((ProtocolStat *)protocol_stats)[i].pending = 0;
                        protocol_records[i].reply_seen = 0;
                        ((ProtocolStat *)protocol_stats)[i].ready = 0;
                    }
                DAT_20000592++;

            } else {
                while (sub_08014F36(0, &g_20000591) == 0) {
                    switch (state = DAT_200000fc) {
                    case 0:
                        if (DAT_20000591 == DAT_20000163) {
                            g_20000A9C[0] = DAT_20000591;
                            DAT_200000fc = 1;
                        } else
                            DAT_200000fc = 0;
                        break;
                    case 1:
                        if (DAT_20000591 == 3 || DAT_20000591 == 4 || DAT_20000591 == 16 ||
                            DAT_20000591 == 17) {
                            g_20000A9C[1] = DAT_20000591;
                            DAT_200000fc = 2;
                        } else if (DAT_20000591 == 6) {
                            g_20000A9C[1] = DAT_20000591;
                            DAT_200000fc = 2;
                        } else
                            DAT_200000fc = 0;
                        break;
                    default:
                        DAT_200000dc = 0;
                        DAT_200000fe = 8;
                        g_20000A9C[state] = DAT_20000591;
                        state = state + 1;
                        DAT_200000fc = state;
                        if (state >= 8) {
                            pack_dispatch_frame(g_20000A9C, (uchar)state);
                            DAT_200000fc = 0;
                        }
                        if (DAT_200000fc > 300)
                            DAT_200000fc = 0;
                        break;
                    }
                    DAT_20000592 = 0;
                }
                if (DAT_20000163 && DAT_200000dc++ >= 1200) {
                    DAT_200000dc = 1200;
                    ((volatile ProtocolByteBits *)&DAT_20000276)->b3 = 1;
                    ((volatile ProtocolByteBits *)&DAT_20000276)->b4 = 1;
                    GPIO_ResetBits(&DAT_40011000, 256);
                    DAT_2000005a = 0;
                    GPIO_ResetBits(&DAT_40011400, 4);
                    DAT_2000005c = 0;
                    GPIO_ResetBits(&DAT_40010c00, 8);
                    DAT_2000005b = 0;
                }
                DAT_20000592++;
            }
            DAT_200074c0 = 0;
            DAT_200074be = 50;
        }
    }
}

#undef DAT_40010c00
#undef DAT_40011000
#undef DAT_40011400
#undef DAT_2000004f
#undef DAT_2000005a
#undef DAT_2000005b
#undef DAT_2000005c
#undef DAT_200000dc
#undef DAT_200000df
#undef DAT_200000e1
#undef DAT_200000ee
#undef DAT_200000fc
#undef DAT_200000fe
#undef DAT_20000163
#undef DAT_20000276
#undef DAT_20000591
#undef DAT_20000592
#undef DAT_20000593
#undef DAT_20000594
#undef DAT_20000595
#undef DAT_2000059e
#undef DAT_200005a0
#undef protocol_stats
#undef protocol_pack
#undef DAT_200074be
#undef DAT_200074c0

extern uchar g_20000C60[];
#define DAT_20000c62 (*(uchar *)(g_20000C60 + 2))
#define DAT_20000c6c (*(uchar *)(g_20000C60 + 12))
#define DAT_20000c70 (*(uchar *)(g_20000C60 + 16))
#define DAT_20000c71 (*(uchar *)(g_20000C60 + 17))
#define g_20000C60_v (*(uchar *)(g_20000C60 + 0))

void inverter_reply_register_write(uchar *frame) {
    uchar error = 0, i;
    uint crc;
    for (i = 0; i < 10; i++)
        g_20000C60[i + 2] = 0;
    for (i = 0; i < 4; i++)
        g_20000C60[i + 12] = ((uchar *)frame)[i + 2];
    if (error) {
        g_20000C60[1] = 0x86;
        crc = protocol_crc16(g_20000C60, 16);
        g_20000C60[16] = crc;
        g_20000C60[17] = crc >> 8;
        protocol_byte1 = 1;
    } else {
        crc = protocol_crc16(g_20000C60, 16);
        g_20000C60[16] = crc;
        g_20000C60[17] = crc >> 8;
    }
}
#undef DAT_20000c62
#undef DAT_20000c6c
#undef DAT_20000c70
#undef DAT_20000c71
#undef g_20000C60_v

#define DAT_2000058d (protocol_byte1)
extern uchar g_20000C60[];
#define DAT_20000c61 (*(uchar *)(g_20000C60 + 1))
#define DAT_20000c62 (*(uchar *)(g_20000C60 + 2))
#define DAT_20000c6c (*(uchar *)(g_20000C60 + 12))
#define DAT_20000c70 (*(uchar *)(g_20000C60 + 16))
#define DAT_20000c71 (*(uchar *)(g_20000C60 + 17))
#define g_20000C60_v (*(uchar *)(g_20000C60 + 0))

void inverter_reply_input_registers(uchar *frame) {
    ushort i = 0;
    ushort length = 0;
    uchar error = 0;
    uint start, count, crc;
    start = protocol_be16(frame + 2);
    count = protocol_be16(frame + 4);
    if (count > 60 || count == 0)
        error = 1;
    if ((start / 60 + 1) * 60 < start + count)
        error = 1;
    for (i = 0; i < 10; i++)
        g_20000C60[i + 2] = 0;
    for (i = 0; i < 4; i++)
        g_20000C60[i + 12] = ((uchar *)frame)[i + 2];
    if (error != 1) {
        for (i = 0; i < 120; i++)
            protocol_clear[i] = 0;
        if (length > 150)
            length = 150;
        crc = protocol_crc16(g_20000C60, length);
        g_20000C60[length] = crc;
        g_20000C60[length + 1] = crc >> 8;
    } else {
        g_20000C60[1] = 132;
        crc = protocol_crc16(g_20000C60, 16);
        g_20000C60[16] = crc;
        g_20000C60[17] = crc >> 8;
        protocol_byte1 = 1;
    }
}
#undef DAT_2000058d
#undef DAT_20000c61
#undef DAT_20000c62
#undef DAT_20000c6c
#undef DAT_20000c70
#undef DAT_20000c71
#undef g_20000C60_v

#define DAT_2000058d (protocol_byte1)
extern uchar g_20000C60[];
#define DAT_20000c61 (*(uchar *)(g_20000C60 + 1))
#define DAT_20000c62 (*(uchar *)(g_20000C60 + 2))
#define DAT_20000c6c (*(uchar *)(g_20000C60 + 12))
#define DAT_20000c70 (*(uchar *)(g_20000C60 + 16))
#define DAT_20000c71 (*(uchar *)(g_20000C60 + 17))
#define g_20000C60_v (*(uchar *)(g_20000C60 + 0))

void inverter_reply_register_read(uchar *frame) {
    ushort i = 0;
    ushort length = 0;
    uchar error = 0;
    uint start, count, crc;
    start = protocol_be16(frame + 2);
    count = protocol_be16(frame + 4);
    if (count > 60 || count == 0)
        error = 1;
    if ((start / 60 + 1) * 60 < start + count)
        error = 1;
    for (i = 0; i < 10; i++)
        g_20000C60[i + 2] = 0;
    for (i = 0; i < 4; i++)
        g_20000C60[i + 12] = ((uchar *)frame)[i + 2];
    if (error != 1) {
        for (i = 0; i < 120; i++)
            protocol_clear[i] = 0;
        if (length > 150)
            length = 150;
        crc = protocol_crc16(g_20000C60, length);
        g_20000C60[length] = crc;
        g_20000C60[length + 1] = crc >> 8;
    } else {
        g_20000C60[1] = 131;
        crc = protocol_crc16(g_20000C60, 16);
        g_20000C60[16] = crc;
        g_20000C60[17] = crc >> 8;
        protocol_byte1 = 1;
    }
}
#undef DAT_2000058d
#undef DAT_20000c61
#undef DAT_20000c62
#undef DAT_20000c6c
#undef DAT_20000c70
#undef DAT_20000c71
#undef g_20000C60_v

extern uchar g_2000004F;
#define DAT_2000004f (g_2000004F)
extern uchar g_20000100;
#define DAT_2000058c (g_2000058C)
#define DAT_2000058d (protocol_byte1)
extern ushort g_20000606;
#define os_task_dly_idx (g_20000606)
extern uchar g_20000BC9[];
#define g_20000BC9_v (*(uchar *)(g_20000BC9 + 0))
extern uchar g_20000C60[];
#define DAT_20000c61 (*(uchar *)(g_20000C60 + 1))
#define g_20000C60_v (*(uchar *)(g_20000C60 + 0))

void inverter_dispatch_frame(byte *frame) {
    uchar mine = 0;
    uchar ok = 0;
    uint crc;
    uint i;

    crc = protocol_crc16(frame, 6);
    protocol_crc26 = crc;
    if ((crc & 0xff) == frame[7] && frame[6] == crc >> 8)
        ok = 1;
    if (ok == 1) {
        if (frame[0] == g_2000004F || frame[0] == 0)
            mine = 1;
        if (mine == 1) {
            g_20000C60[0] = g_2000004F;
            g_20000C60[1] = frame[1];
            if (frame[1] >= BMS_MODBUS_READ_HOLDING_REGISTERS &&
                frame[1] <= BMS_MODBUS_WRITE_SINGLE_REGISTER) {
                if (frame[1] == BMS_MODBUS_READ_HOLDING_REGISTERS)
                    inverter_reply_register_read(frame);
                else if (frame[1] == BMS_MODBUS_READ_INPUT_REGISTERS)
                    inverter_reply_input_registers(frame);
                else if (frame[1] == BMS_MODBUS_WRITE_SINGLE_REGISTER) {
                    inverter_reply_register_write(frame);
                    if (frame[0] == 0)
                        g_2000058C = 1;
                } else
                    g_2000058C = 1;
            } else
                g_2000058C = 1;
            if (g_2000058C == 1) {
                g_2000058C = 0;
                return;
            }
            if (g_20000C60[1] == BMS_MODBUS_WRITE_SINGLE_REGISTER)
                g_20000100 = 0x12;
            else
                g_20000100 = g_20000C60[15] * 2 + 0x12;
            if (protocol_byte1 == 1) {
                g_20000100 = 0x12;
                protocol_byte1 = 0;
            }
            if (g_20000100 > 0x96)
                g_20000100 = 0x96;
            for (i = 0; i < g_20000100; i = i + 1 & 0xff)
                g_20000BC9[i] = g_20000C60[i];
            board_set_pa8(1);
            os_task_delay((uint)(byte)os_task_dly_idx, 3, 5);
            sub_08014F94(0, g_20000BC9, g_20000100);
        }
    }
}
#undef DAT_2000004f
#undef DAT_2000058c
#undef DAT_2000058d
#undef os_task_dly_idx
#undef g_20000BC9_v
#undef DAT_20000c61
#undef g_20000C60_v

#define DAT_2000058d (protocol_byte1)
extern uchar g_20000C60[];
#define DAT_20000c61 (*(uchar *)(g_20000C60 + 1))
#define DAT_20000c62 (*(uchar *)(g_20000C60 + 2))
#define DAT_20000c6c (*(uchar *)(g_20000C60 + 12))
#define DAT_20000c70 (*(uchar *)(g_20000C60 + 16))
#define DAT_20000c71 (*(uchar *)(g_20000C60 + 17))
#define g_20000C60_v (*(uchar *)(g_20000C60 + 0))

void inverter_reply_read_error(uchar *frame) {
    ushort i = 0;
    ushort length = 0;
    uchar error = 0;
    uint start, count, crc;
    start = protocol_be16(frame + 2);
    count = protocol_be16(frame + 4);
    if (count > 60 || count == 0)
        error = 1;
    if ((start / 60 + 1) * 60 < start + count)
        error = 1;
    for (i = 0; i < 10; i++)
        g_20000C60[i + 2] = 0;
    for (i = 0; i < 4; i++)
        g_20000C60[i + 12] = ((uchar *)frame)[i + 2];
    if (error != 1) {

        if (length > 150)
            length = 150;
        crc = protocol_crc16(g_20000C60, length);
        g_20000C60[length] = crc;
        g_20000C60[length + 1] = crc >> 8;
    } else {
        g_20000C60[1] = 132;
        crc = protocol_crc16(g_20000C60, 16);
        g_20000C60[16] = crc;
        g_20000C60[17] = crc >> 8;
        protocol_byte1 = 1;
    }
}
#undef DAT_2000058d
#undef DAT_20000c61
#undef DAT_20000c62
#undef DAT_20000c6c
#undef DAT_20000c70
#undef DAT_20000c71
#undef g_20000C60_v

extern uchar g_200000AA, g_200000D5, g_200000D8, g_200000D9, g_2000013C, g_2000013D, g_20000163,
    g_20000185;
extern ushort g_200000F8, g_20000180, g_20000182;
extern float g_2000017C;
#define ABS(x) ((x) >= 0 ? (x) : -(x))
void pack_update_summary(void) {
    uchar n = 0;
    int cur = 0;
    ushort cnt;
    uchar nalarm;
    uchar nprot;
    ushort soc;
    uchar i;
    short d;
    ushort lim;

    if (g_20000163 > 6)
        g_20000163 = 5;
    if (g_20000163 == 0)
        *(volatile uchar *)&g_20000163 = 0;
    if (g_20000163 == 0) {
        if (protocol_clear48++ > 500) {
            protocol_clear48 = 0;
            for (i = 1; i < 6; i++) {
                if (protocol_records[i].reply_seen == 0)
                    memset(&protocol_records[i], 0, sizeof(protocol_records[i]));
            }
        }
        cnt = 0;
        nalarm = 0;
        nprot = 0;
        protocol_sum40 = 0;
        soc = 0;
        protocol_byte11 = 0;
        for (i = 0; i < 6; i++) {
            if (protocol_records[i].participation_state) {
                protocol_sum40 += protocol_records[i].rated_capacity;
                soc += protocol_records[i].soc_secondary;
                n++;
                cur += protocol_records[i].pack_current;
            }
            if (protocol_records[i].participation_state) {
                cnt++;
                if (((protocol_records[i].voltage_current_limit_flags & 8) == 8) ||
                    ((protocol_records[i].voltage_current_limit_flags & 2) == 2) ||
                    ((protocol_records[i].voltage_current_protection_flags & 8) == 8) ||
                    ((protocol_records[i].voltage_current_protection_flags & 2) == 2) ||
                    ((protocol_records[i].reserved_134 & 8) == 8) ||
                    ((protocol_records[i].reserved_132 & 2) == 2)) {
                    nalarm++;
                    if (protocol_records[i].voltage_current_limit_flags & 0x0a)
                        protocol_records[i].reserved_134 =
                            protocol_records[i].voltage_current_limit_flags;
                    if (protocol_records[i].voltage_current_protection_flags & 0x0a)
                        protocol_records[i].reserved_132 =
                            protocol_records[i].voltage_current_protection_flags;
                }
                if (((protocol_records[i].voltage_current_limit_flags & 4) == 4) ||
                    ((protocol_records[i].voltage_current_limit_flags & 1) == 1) ||
                    ((protocol_records[i].voltage_current_protection_flags & 4) == 4) ||
                    ((protocol_records[i].voltage_current_protection_flags & 1) == 1) ||
                    ((protocol_records[i].reserved_134 & 4) == 4) ||
                    ((protocol_records[i].reserved_132 & 1) == 1)) {
                    nprot++;
                    if (protocol_records[i].voltage_current_limit_flags & 5)
                        protocol_records[i].reserved_134 =
                            protocol_records[i].voltage_current_limit_flags;
                    if (protocol_records[i].voltage_current_protection_flags & 5)
                        protocol_records[i].reserved_132 =
                            protocol_records[i].voltage_current_protection_flags;
                }
                if (!((protocol_records[i].output_status_flags & 2) == 2) &&
                    !((protocol_records[i].output_status_flags & 4) == 4))
                    protocol_byte11++;
            }
        }
        if (nalarm >= cnt) {
            g_200000D8 = 1;
            g_200000D9 = 0;
        } else
            g_200000D8 = 0;
        if (nprot >= cnt) {
            g_200000D9 = 1;
            g_200000D8 = 0;
        } else
            g_200000D9 = 0;
        if (n == 0 || g_200000D5 == 0) {
            g_2000017C = g_20000182;
            soc = g_20000185;
            g_20000180 = g_20000185;
        } else {
            g_2000017C = protocol_sum40;
            soc = soc / n;
        }
        protocol_soc44 = soc;
        if (protocol_soc44 > 100)
            protocol_soc44 = 100;
        if (g_200000F8 & 1) {
            if (protocol_soc44 > 9) {
                if (protocol_soc44 >= 96)
                    soc = 100;
                else
                    soc = 4 + ((protocol_soc44 - 9) * 283 >> 8);
            } else if (protocol_soc44 == 0)
                soc = 0;
            else
                soc = 4 - ((9 - protocol_soc44) * 114 >> 8);
        } else if (g_200000F8 & 2) {
            if (protocol_soc44 > 15) {
                if (protocol_soc44 >= 95)
                    soc = 100;
                else
                    soc = 4 + ((protocol_soc44 - 15) * 307 >> 8);
            } else if (protocol_soc44 == 0)
                soc = 0;
            else
                soc = 4 - ((15 - protocol_soc44) * 68 >> 8);
        } else if (g_200000F8 & 4) {
            if (protocol_soc44 > 4) {
                if (protocol_soc44 >= 97)
                    soc = 100;
                else
                    soc = 4 + ((protocol_soc44 - 4) * 264 >> 8);
            } else if (protocol_soc44 == 0)
                soc = 0;
            else
                soc = 4 - ((4 - protocol_soc44) * 299 >> 8);
        }
        if (n == 0)
            g_20000180 = soc;
        if ((g_2000013D == 1 && protocol_soc32 == 100) ||
            (g_2000013C == 1 && protocol_soc32 == 0)) {
            g_20000180 = protocol_soc32;
        } else {
            d = soc - g_20000180;
            if (ABS(d) > 1 || protocol_phase28 == 1) {
                if (protocol_phase28 == 0) {
                    g_20000180 = d / ABS(d) + g_20000180;
                    protocol_phase28 = 1;
                    protocol_ticks30 = 0;
                } else if (protocol_phase28 == 1) {
                    if (ABS(d) > 5)
                        lim = 1200;
                    else if (ABS(d) > 1)
                        lim = 1500;
                    else
                        lim = 6000;
                    if (cur > 5000 && d < 0)
                        protocol_ticks30 = 0;
                    else if (cur < -5000 && d > 0)
                        protocol_ticks30 = 0;
                    if (protocol_ticks30++ > lim) {
                        g_20000180 = d / ABS(d) + g_20000180;
                        protocol_ticks30 = 0;
                    }
                    if (soc == g_20000180)
                        protocol_phase28 = 0;
                }
            } else {
                g_20000180 = soc;
                protocol_phase28 = 0;
                protocol_ticks30 = 0;
            }
            protocol_soc32 = g_20000180;
        }
    } else {
        g_200000D8 = 1;
        g_200000D9 = 1;
        g_2000017C = g_20000182;
        g_20000180 = g_20000185;
    }
    if (g_20000180 >= 100) {
        if (g_200000AA == 0)
            g_20000180 = 100;
        else
            g_20000180 = 99;
    }
}

extern uchar g_2000005A, g_2000005B, g_2000005C, g_2000009F, g_200000DA;
#define F276 (*(volatile uchar *)&g_20000276)
void pack_update_peer_participation_flags(void);
#define FL(i) (&protocol_flags[i].word.bits)
void pack_balance_and_control(void) {
    uchar i;
    uchar j;
    ushort low;

    if (g_20000163 != 0) {
        if (FL(g_20000163)->b0) {
            FL(g_20000163)->b0 = 0;
            F276 &= ~8;
            F276 &= ~0x10;
        }
        if (FL(g_20000163)->b1) {
            FL(g_20000163)->b1 = 0;
            F276 &= ~8;
            F276 &= ~0x10;
        }
        if (FL(g_20000163)->b4) {
            FL(g_20000163)->b4 = 0;
            F276 |= 8;
            F276 |= 0x10;
            GPIO_ResetBits((void *)0x40011000, 0x100);
            g_2000005A = 0;
            GPIO_ResetBits((void *)0x40011400, 4);
            g_2000005C = 0;
            GPIO_ResetBits((void *)0x40010C00, 8);
            g_2000005B = 0;
        }
        if (FL(g_20000163)->b5) {
            FL(g_20000163)->b5 = 0;
            F276 |= 8;
            F276 |= 0x10;
            GPIO_ResetBits((void *)0x40011000, 0x100);
            g_2000005A = 0;
            GPIO_ResetBits((void *)0x40011400, 4);
            g_2000005C = 0;
            GPIO_ResetBits((void *)0x40010C00, 8);
            g_2000005B = 0;
        }
        if (FL(g_20000163)->b7 && (F276 & 1)) {
            FL(g_20000163)->b7 = 0;
            F276 |= 2;
        }
    } else if (protocol_settle34++ < 2000) {
        F276 |= 0x18;
    } else {
        protocol_settle34 = 2000;
        if (g_200000DA == 1) {
            if (protocol_byte13 == 0) {
                F276 |= 0x18;
                for (i = 1; i < 6; i++) {
                    FL(i)->b0 = 0;
                    FL(i)->b1 = 0;
                    FL(i)->b4 = 1;
                    FL(i)->b5 = 1;
                    FL(i)->b3 = 1;
                }
                protocol_wait36 = 0;
                protocol_byte13 = 1;
            } else if (protocol_byte13 == 1) {
                for (i = 1; i < 6; i++) {
                    if (FL(i)->b3)
                        return;
                }
                if (protocol_wait36++ >= 4000) {
                    protocol_wait36 = 0;
                    protocol_byte13 = 2;
                }
            } else if (protocol_byte13 == 2) {
                if (measurement_pack_voltage >= 25000)
                    low = measurement_pack_voltage;
                else
                    low = 25000;
                for (i = 0; i < 6; i++) {
                    if (protocol_records[i].pack_voltage < low &&
                        protocol_records[i].pack_voltage >= 25000)
                        low = protocol_records[i].pack_voltage;
                }
                if (low == measurement_pack_voltage || low == 25000) {
                    F276 &= ~8;
                    F276 &= ~0x10;
                } else {
                    for (i = 1; i < 6; i++) {
                        if (protocol_records[i].pack_voltage == low) {
                            FL(i)->b0 = 1;
                            FL(i)->b3 = 1;
                            FL(i)->b4 = 0;
                            FL(i)->b5 = 0;
                        }
                    }
                }
                protocol_byte13 = 3;
            } else if (protocol_byte13 == 3) {
                if (measurement_pack_voltage >= 25000)
                    low = measurement_pack_voltage;
                else
                    low = 25000;
                for (i = 0; i < 6; i++) {
                    if (protocol_records[i].pack_voltage < low &&
                        protocol_records[i].pack_voltage >= 25000)
                        low = protocol_records[i].pack_voltage;
                }
                if (low == measurement_pack_voltage || low == 25000) {
                    F276 &= ~8;
                    F276 &= ~0x10;
                } else {
                    for (i = 1; i < 6; i++) {
                        if (protocol_records[i].pack_voltage == low) {
                            if (i != protocol_byte12) {
                                FL(i)->b0 = 1;
                                FL(i)->b3 = 1;
                                FL(i)->b4 = 0;
                                FL(i)->b5 = 0;
                            }
                            protocol_byte12 = i;
                        }
                    }
                }
            } else {
                F276 |= 0x18;
                for (i = 1; i < 6; i++) {
                    FL(i)->b0 = 0;
                    FL(i)->b1 = 0;
                    FL(i)->b4 = 1;
                    FL(i)->b5 = 1;
                    FL(i)->b3 = 1;
                }
                protocol_byte13 = 0;
            }
        } else {
            F276 &= ~8;
            F276 &= ~0x10;
            protocol_byte13 = 0;
            g_200000DA = 0;
            for (i = 1; i < 6; i++) {
                if (protocol_records[i].auxiliary_voltage < -5000) {
                    for (j = 1; j < 6; j++) {
                        FL(j)->b4 = 0;
                        FL(j)->b5 = 0;
                        FL(j)->b0 = 0;
                        FL(j)->b1 = 0;
                        FL(j)->b3 = 1;
                    }
                    F276 |= 0x18;
                    protocol_byte3 = 1;
                }
            }
            if (protocol_byte3 != 1) {
                for (i = 1; i < 6; i++) {
                    FL(i)->b4 = 0;
                    FL(i)->b5 = 0;
                    switch (g_2000009F) {
                    case 0:
                        if (((protocol_records[i].pack_voltage < g_20000124 + 1000 &&
                              protocol_records[i].pack_voltage > g_20000124) ||
                             (protocol_records[i].pack_voltage > g_20000124 - 1000 &&
                              protocol_records[i].pack_voltage < g_20000124)) &&
                            (((protocol_records[i].capacity_latch_flags & 8) == 8) ||
                             ((protocol_records[i].capacity_latch_flags & 0x10) == 0x10))) {
                            FL(i)->b0 = 1;
                            FL(i)->b3 = 1;
                        }
                        break;
                    case 1:
                        if (measurement_current < 1000.0f) {
                            if (((protocol_records[i].pack_voltage < g_20000124 + 1000 &&
                                  protocol_records[i].pack_voltage > g_20000124) ||
                                 (protocol_records[i].pack_voltage > g_20000124 - 1000 &&
                                  protocol_records[i].pack_voltage < g_20000124)) &&
                                (((protocol_records[i].capacity_latch_flags & 8) == 8) ||
                                 ((protocol_records[i].capacity_latch_flags & 0x10) == 0x10))) {
                                FL(i)->b0 = 1;
                                FL(i)->b3 = 1;
                            }
                        } else {
                            if (protocol_records[i].pack_voltage > g_20000124 - 1000 &&
                                protocol_records[i].pack_voltage < g_20000124 &&
                                (((protocol_records[i].capacity_latch_flags & 8) == 8) ||
                                 ((protocol_records[i].capacity_latch_flags & 0x10) == 0x10))) {
                                FL(i)->b0 = 1;
                                FL(i)->b3 = 1;
                            }
                        }
                        break;
                    case 2:
                        if (measurement_current > -1000.0f) {
                            if (((protocol_records[i].pack_voltage < g_20000124 + 1000 &&
                                  protocol_records[i].pack_voltage > g_20000124) ||
                                 (protocol_records[i].pack_voltage > g_20000124 - 1000 &&
                                  protocol_records[i].pack_voltage < g_20000124)) &&
                                (((protocol_records[i].capacity_latch_flags & 8) == 8) ||
                                 ((protocol_records[i].capacity_latch_flags & 0x10) == 0x10))) {
                                FL(i)->b1 = 1;
                                FL(i)->b3 = 1;
                            }
                        } else {
                            if (protocol_records[i].pack_voltage < g_20000124 + 1000 &&
                                protocol_records[i].pack_voltage > g_20000124 &&
                                (((protocol_records[i].capacity_latch_flags & 8) == 8) ||
                                 ((protocol_records[i].capacity_latch_flags & 0x10) == 0x10))) {
                                FL(i)->b1 = 1;
                                FL(i)->b3 = 1;
                            }
                        }
                        break;
                    }
                    if ((protocol_records[i].pack_voltage < g_20000124 - 4000 ||
                         protocol_records[i].pack_voltage > g_20000124 + 4000) &&
                        ((protocol_records[i].capacity_latch_flags & 8) == 0 ||
                         (protocol_records[i].capacity_latch_flags & 0x10) == 0) &&
                        protocol_records[i].participation_state != 2) {
                        protocol_fault52[i]++;
                        if (protocol_fault52[i] >= 2000) {
                            protocol_fault52[i] = 2000;
                            FL(i)->b0 = 0;
                            FL(i)->b1 = 0;
                            FL(i)->b4 = 1;
                            FL(i)->b5 = 1;
                            FL(i)->b3 = 1;
                        }
                    } else
                        protocol_fault52[i] = 0;
                }
                pack_update_peer_participation_flags();
                if (FL(g_20000163)->b7 && (F276 & 1)) {
                    FL(g_20000163)->b7 = 0;
                    F276 |= 2;
                }
            }
        }
    }
}
