/* build: ascii-compose */
/* Nine functions compile as C; the dispatcher is composed from ascii_dispatcher.inc. */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* at: sub_0801D424 0x0801D424, sub_0801D606 0x0801D606, sub_0801D6E4 0x0801D6E4, sub_0801D8C0 0x0801D8C0, sub_0801DB3A 0x0801DB3A, sub_0801DF70 0x0801DF70, sub_08020D52 0x08020D52, sub_08020E40 0x08020E40, sub_08020FA4 0x08020FA4, sub_0802126C 0x0802126C */
/* at: pace_buf 0x200035EA, modbus_tx 0x20003694, modbus_resp 0x2000372B, modbus_regs 0x200037C2 */
/* at: rx_ready 0x2000050E, tx_cid2 0x2000050F, mb_addr 0x20000510, rx_count 0x20000511, rx_idx 0x20000512, ascii_rx_state 0x20000513, tx_pack 0x20000514, mb_len 0x20000515, mb_wait 0x20000516 */
/* at: tx_len 0x20000518, tx_idx 0x2000051A, rx_info_len 0x2000051C, tx_sum 0x2000051E, mb_crc 0x20000520, pace_mode 0x200000DE, hexbuf 0x20000109, packs 0x20003D52, cfgs 0x20004149, measure 0x200014D2 */
/* at: limits 0x20001522, alarm_mask 0x20000275, charging 0x2000009F, pack_count 0x2000009E, own_addr 0x20000163 */
/* at: g_20000180 0x20000180, g_2000017C 0x2000017C, g_20000182 0x20000182, g_20000164 0x20000164, g_2000016C 0x2000016C, g_20000162 0x20000162 */
/* at: g_20000077 0x20000077, g_20000062 0x20000062, g_200000A2 0x200000A2, g_20000124 0x20000124, g_20000110 0x20000110, g_20000148 0x20000148 */
/* at: g_20000184 0x20000184, g_20000185 0x20000185, g_20000271 0x20000271, g_20000272 0x20000272, g_20000273 0x20000273, g_2000050D 0x2000050D */
/* at: g_20000274 0x20000274, g_20000276 0x20000276, g_20000277 0x20000277, g_20000278 0x20000278, g_2000010C 0x2000010C, g_2000010E 0x2000010E */
/* at: g_20001094 0x20001094, g_2000012A 0x2000012A, g_2000012C 0x2000012C, g_2000119E 0x2000119E, g_20000051 0x20000051, g_20000130 0x20000130 */
/* at: g_20000134 0x20000134, g_200000A0 0x200000A0, g_20000138 0x20000138, g_20000139 0x20000139, g_20000098 0x20000098, g_200000AD 0x200000AD */
/* at: prot 0x20001552, recov 0x2000153C, delays 0x200002F8, alarm_mask2 0x20000276, g_200000FA 0x200000FA, g_200000F8 0x200000F8, g_20000168 0x20000168, g_200000A9 0x200000A9 */
/* at: g_20000186 0x20000186, g_20000096 0x20000096, g_20000094 0x20000094, g_20000154 0x20000154, g_2000014C 0x2000014C, g_20000150 0x20000150, log_index 0x20000188 */
/* at: g_2000018A 0x2000018A, date_now 0x2000018C, date_start 0x200000B6, date_end 0x200000BC, log_interval 0x200000B4, g_200000CE 0x200000CE, os_task_dly_idx 0x20000606 */
/* The pack-to-pack link on UART4: an ASCII protocol (Pylontech v2.5-family framing, CID1 0x46)
   and a small Modbus RTU slave on the same port. */
#include "../../include/ghidra_types.h"


typedef struct {
    uchar reserved0;
    uchar cells;
    ushort cell[16];
    uchar temps;
    uchar reserved23;
    ushort temp[10];
    short current;
    ushort voltage;
    ushort remain;
    uchar user;
    uchar reserved3f;
    ushort full;
    ushort cycles;
    ushort design;
    uchar alarm[9];
    uchar reserved4f;
} measure_t;

typedef __packed struct {
    ushort online;
    uchar serial[21];
    uchar model[21];
    uchar f2c;
    ushort f2d;
    uint f2f;
    ushort f33;
    int f35;
    int f39;
    uint f3d;
    int f41;
    uchar f45;
    uchar alarm[9];
    ushort f4f;
    ushort f51;
    ushort cell[16];
    ushort f73;
    ushort f75;
    ushort version;
    ushort temp[5];
    uchar f83;
    uchar f84[4];
    uchar f88;
    ushort f89;
    ushort f8b;
    uchar f8d;
    uchar f8e;
    uchar f8f;
    uchar f90;
} pack_t;

typedef __packed struct {
    uint flags;
    uchar f4[4];
    ushort v8;
    ushort va;
    ushort vc;
    ushort ve;
    uchar f10[98];
} cfg_t;

typedef struct { uchar c[2]; } hex_t;

extern uchar pace_mode;
extern uchar hexbuf[2];
extern pack_t packs[];
extern cfg_t cfgs[];
extern measure_t measure[];
typedef struct {
    ushort cell_ov, cell_uv, pack_ov, pack_uv, chg_oc;
    short dsg_oc;
    ushort chg_ot, dsg_ot, chg_ut, dsg_ut, mos_ot, env_ot, env_ut;
} limits_t;
extern limits_t limits;
typedef struct {
    ushort cell_ov, cell_uv, pack_ov, pack_uv, chg_oc;
    short dsg_oc;
    ushort short_cur, chg_ot, dsg_ot, chg_ut, dsg_ut, mos_ot, env_ot, env_ut;
} prot_t;
extern prot_t prot;
typedef struct {
    ushort cell_ov, cell_uv, pack_ov, pack_uv, chg_ot, dsg_ot, chg_ut, dsg_ut, mos_ot, env_ot, env_ut;
} recov_t;
extern recov_t recov;
extern uchar delays[];
extern uchar alarm_mask2;
extern ushort g_200000FA, g_200000F8;
extern float g_20000168;
extern uchar g_200000A9, g_20000186;
extern ushort g_20000096, g_20000094;
extern float g_20000154, g_2000014C, g_20000150;
extern ushort log_index, g_2000018A;
extern uchar date_now[6], date_start[6], date_end[6];
extern uchar log_interval;
extern ushort g_200000CE;
extern ushort os_task_dly_idx;
#define WWDG_CR (*(volatile uint *)0x40002C00)
extern uchar alarm_mask;
extern uchar charging;
extern uchar pack_count;
extern uchar own_addr;
extern uchar g_20000180;
extern float g_2000017C;
extern ushort g_20000182;
extern float g_20000164, g_2000016C;
extern uchar g_20000162;
extern uchar g_20000077[], g_20000062[];
extern ushort g_200000A2;
extern uint g_20000124;
extern ushort g_20000110;
extern float g_20000148;
extern uchar g_20000184, g_20000185;
extern volatile uchar g_20000271, g_20000272, g_20000273, g_20000274, g_20000277, g_20000278, g_2000050D;
extern uchar g_20000276;
extern ushort g_2000010C, g_2000010E;
extern ushort g_20001094[], g_2000119E[];
extern ushort g_2000012A, g_2000012C;
extern uchar g_20000051;
extern uint g_20000130, g_20000134;
extern uchar g_200000A0, g_20000138, g_20000139, g_20000098, g_200000AD;

uchar rx_ready = 0;
uchar tx_cid2 = 0;
uchar mb_addr = 0;
uchar rx_count = 0;
uchar rx_idx = 0;
uchar ascii_rx_state = 1;
uchar tx_pack = 0;
uchar mb_len = 0;
uchar mb_wait = 0;
ushort tx_len = 0;
ushort tx_idx = 0;
ushort rx_info_len = 0;
ushort tx_sum = 0;
ushort mb_crc = 0;
uchar pace_buf[170];
uchar modbus_tx[151];
uchar modbus_resp[151];
uchar modbus_regs[130];

uchar sub_08014FD6(uchar hi, uchar lo);
uchar sub_080151EA(uchar value);
uint sub_080151D8(uint value);
void sub_0801535E(void);
int sub_080223BC(uint addr);
int sub_08022488(ushort addr, float value);
void sub_080172B2(void);
void sub_08016F1E(void);
void sub_080179EA(int mode);
void os_task_delay(int idx, ushort first, ushort period);
void sub_08015034(uchar value, uchar *out);
int sub_08015058(uchar *frame);
int sub_0801508A(uchar *frame);
void sub_080150EE(uchar *frame, ushort length);
void sub_0801512E(uchar *frame, uchar rtn);
void sub_08014F94(int port, uchar *data, ushort length);
ushort sub_08024874(uchar *data, ushort length);
int sub_08022442(ushort addr, ushort value);
int USART_GetITStatus(void *usart, ushort it);
void USART_ITConfig(void *usart, ushort it, int state);
float __aeabi_ui2f(uint);
#define UART4 ((void *)0x40004C00)
#define UART4_DR (*(volatile ushort *)0x40004C04)

void sub_0801DF70(uchar cid2);

void sub_0801D424(uchar *f)
{
    uchar err = 0;
    ushort start, count, i, n;
    ushort crc;
    pack_t *p;
    uint v;

    start = f[3];
    start = start << 8 | f[4];
    count = f[5];
    count = count << 8 | f[6];
    if (count > 60 || count == 0) err = 1;
    if (start + count > (start / 60 + 1) * 60) err = 1;
    if (err == 1) {
        modbus_resp[1] = 0x83;
        crc = sub_08024874(modbus_resp, 16);
        modbus_resp[2] = crc;
        modbus_resp[3] = crc >> 8;
        return;
    }
    for (i = 0; i < 120; i++) modbus_regs[i] = 0;
    if (start < 60) {
        for (i = 0; i < 21; i++) modbus_regs[i] = packs[mb_addr].serial[i];
        for (i = 0; i < 21; i++) modbus_regs[i + 21] = packs[mb_addr].model[i];
        p = &packs[mb_addr];
        modbus_regs[42] = p->f2c;
        v = p->f2d;
        modbus_regs[43] = v >> 8;
        modbus_regs[44] = v;
        v = p->f2f;
        modbus_regs[45] = v >> 24;
        modbus_regs[46] = v >> 16;
        modbus_regs[47] = v >> 8;
        modbus_regs[48] = v;
        v = p->f33;
        modbus_regs[49] = v >> 8;
        modbus_regs[50] = v;
        v = p->f35;
        modbus_regs[51] = v >> 24;
        modbus_regs[52] = v >> 16;
        modbus_regs[53] = v >> 8;
        modbus_regs[54] = v;
        v = p->f39;
        modbus_regs[55] = v >> 24;
        modbus_regs[56] = v >> 16;
        modbus_regs[57] = v >> 8;
        modbus_regs[58] = v;
        v = p->f3d;
        modbus_regs[59] = v >> 24;
        modbus_regs[60] = v >> 16;
        modbus_regs[61] = v >> 8;
        modbus_regs[62] = v;
        v = p->f41;
        modbus_regs[63] = v >> 24;
        modbus_regs[64] = v >> 16;
        modbus_regs[65] = v >> 8;
        modbus_regs[66] = v;
        modbus_regs[67] = p->f88;
        modbus_regs[68] = p->alarm[0];
        modbus_regs[69] = p->alarm[1];
        modbus_regs[70] = p->alarm[2];
        modbus_regs[71] = p->alarm[3];
        modbus_regs[72] = p->alarm[4];
        modbus_regs[73] = p->alarm[5];
        modbus_regs[74] = p->alarm[6];
        modbus_regs[75] = p->alarm[7];
        modbus_regs[76] = p->alarm[8];
        modbus_regs[77] = p->online;
        modbus_regs[78] = g_20000180;
        for (i = 0; i < 16; i++) {
            modbus_regs[79 + 2 * i] = (v = p->cell[i]) >> 8;
            modbus_regs[80 + 2 * i] = v;
        }
        v = (uint)g_2000017C;
        modbus_regs[111] = v >> 24;
        modbus_regs[112] = v >> 16;
        modbus_regs[113] = v >> 8;
        modbus_regs[114] = v;
        v = p->f4f;
        modbus_regs[115] = v >> 8;
        modbus_regs[116] = v;
        v = p->f51;
        modbus_regs[117] = v >> 8;
        modbus_regs[118] = v;
        modbus_regs[119] = 0;
        for (i = start * 2, n = 4; i < start * 2 + count * 2; i++, n++)
            modbus_resp[n] = modbus_regs[i];
    }
    crc = sub_08024874(modbus_resp, n);
    modbus_resp[n] = crc;
    modbus_resp[n + 1] = crc >> 8;
}

void sub_0801D606(uchar *f)
{
    uchar a;
    uchar err = 0;
    ushort reg, val;
    ushort crc;

    a = f[1];
    if (a > 15) a = 15;
    reg = f[3];
    reg = reg << 8 | f[4];
    val = f[5];
    val = val << 8 | f[6];
    switch (reg) {
    case 0:
        if (val < 60000) { cfgs[a].v8 = val; cfgs[a].flags |= 1; }
        else err = 1;
        break;
    case 1:
        if (val < 60000) { cfgs[a].va = val; cfgs[a].flags |= 2; }
        else err = 1;
        break;
    case 2:
        if (val < 60000) { cfgs[a].vc = val; cfgs[a].flags |= 4; }
        else err = 1;
        break;
    case 3:
        if (val >= 8 && val <= 16) { cfgs[a].ve = val; cfgs[a].flags |= 8; }
        else err = 1;
        break;
    case 4:
        break;
    default:
        err = 1;
        break;
    }
    if (err) {
        modbus_resp[1] = 0x86;
        crc = sub_08024874(modbus_resp, 16);
        modbus_resp[16] = crc;
        modbus_resp[17] = crc >> 8;
    } else {
        crc = sub_08024874(modbus_resp, 16);
        modbus_resp[16] = crc;
        modbus_resp[17] = crc >> 8;
    }
}

void sub_0801D6E4(uchar c)
{
    if (c == 0x7e || pace_mode == 1) {
        pace_mode = 1;
        if (c == 0x7e) ascii_rx_state = 1;
        switch (ascii_rx_state) {
        case 1:
            if (c == 0x7e) { rx_count = 0; rx_idx = 1; pace_buf[0] = c; ascii_rx_state = 2; }
            break;
        case 2:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 3; }
            break;
        case 3:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 4; }
            break;
        case 4:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 5; }
            break;
        case 5:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 6; }
            break;
        case 6:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 4) {
                rx_count = 0;
                if (pace_buf[9] == '0' && pace_buf[10] == '0' && pace_buf[11] == '0' && pace_buf[12] == '0')
                    ascii_rx_state = 8;
                else
                    ascii_rx_state = 7;
                rx_info_len = sub_08014FD6(pace_buf[11], pace_buf[12]);
            }
            break;
        case 7:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= rx_info_len) { rx_count = 0; ascii_rx_state = 8; }
            break;
        case 8:
            pace_buf[rx_idx++] = c;
            rx_count++;
            if (rx_count >= 4) { rx_count = 0; ascii_rx_state = 9; }
            break;
        case 9:
            if (c == 0x0d) {
                pace_buf[rx_idx] = c;
                if (tx_idx == 0) rx_ready = 1;
            }
            ascii_rx_state = 1;
            break;
        }
    } else {
        pace_mode = 0;
        if (rx_ready == 1) {
            rx_count = 0;
            rx_idx = 0;
        } else {
            if (c == 0xbb) ascii_rx_state = 1;
            switch (ascii_rx_state) {
            case 1:
                if (c == 0xbb) { rx_count = 0; rx_idx = 1; pace_buf[0] = c; ascii_rx_state = 2; }
                break;
            case 2:
                pace_buf[rx_idx++] = c;
                rx_count = 0;
                ascii_rx_state = 3;
                break;
            case 3:
                pace_buf[rx_idx++] = c;
                rx_count = 0;
                ascii_rx_state = 4;
                break;
            case 4:
                pace_buf[rx_idx++] = c;
                rx_count++;
                if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 5; }
                break;
            case 5:
                pace_buf[rx_idx++] = c;
                rx_count++;
                if (rx_count >= 2) { rx_count = 0; ascii_rx_state = 6; }
                break;
            case 6:
                pace_buf[rx_idx++] = c;
                rx_count++;
                if (rx_count >= 2) { rx_count = 0; rx_ready = 1; }
                break;
            }
        }
    }
}

#define PUT_HEX(pos) do { pace_buf[(*pos)++] = hexbuf[0]; pace_buf[(*pos)++] = hexbuf[1]; } while (0)

void sub_0801D8C0(uchar n, ushort *pos)
{
    uchar i;

    sub_08015034(measure[n].cells, hexbuf);
    PUT_HEX(pos);
    for (i = 0; i < measure[n].cells; i++) {
        sub_08015034(measure[n].cell[i] >> 8, hexbuf);
        PUT_HEX(pos);
        sub_08015034(measure[n].cell[i], hexbuf);
        PUT_HEX(pos);
    }
    sub_08015034(measure[n].temps, hexbuf);
    PUT_HEX(pos);
    for (i = 0; i < measure[n].temps; i++) {
        sub_08015034(measure[n].temp[i] >> 8, hexbuf);
        PUT_HEX(pos);
        sub_08015034(measure[n].temp[i], hexbuf);
        PUT_HEX(pos);
    }
    sub_08015034(measure[n].current >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].current, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].voltage >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].voltage, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].remain >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].remain, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].user, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].full >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].full, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].cycles >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].cycles, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].design >> 8, hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].design, hexbuf);
    PUT_HEX(pos);
}

#define PUT2(pos, a, b) do { pace_buf[(*pos)++] = (a); pace_buf[(*pos)++] = (b); } while (0)

void sub_0801DB3A(uchar n, ushort *pos)
{
    uchar i;

    sub_08015034(measure[n].cells, hexbuf);
    PUT_HEX(pos);
    for (i = 0; i < measure[n].cells; i++) {
        if (measure[n].cell[i] > limits.cell_ov && !(alarm_mask & 1)) PUT2(pos, '0', '2');
        else if (measure[n].cell[i] < limits.cell_uv && !(alarm_mask & 2)) PUT2(pos, '0', '1');
        else PUT2(pos, '0', '0');
    }
    sub_08015034(measure[n].temps, hexbuf);
    PUT_HEX(pos);
    for (i = 0; i < measure[n].temps - 2; i++) {
        if (charging == 1) {
            if (measure[n].temp[i] > limits.chg_ot && !(alarm_mask & 0x40)) PUT2(pos, '0', '2');
            else if (measure[n].temp[i] < limits.chg_ut && !(alarm_mask & 0x80)) PUT2(pos, '0', '1');
            else PUT2(pos, '0', '0');
        } else {
            if (measure[n].temp[i] > limits.dsg_ot && !(alarm_mask & 0x40)) PUT2(pos, '0', '2');
            else if (measure[n].temp[i] < limits.dsg_ut && !(alarm_mask & 0x80)) PUT2(pos, '0', '1');
            else PUT2(pos, '0', '0');
        }
    }
    if (measure[n].temp[measure[n].temps - 2] > limits.mos_ot) PUT2(pos, '0', '2');
    else PUT2(pos, '0', '0');
    if (measure[n].temp[measure[n].temps - 1] > limits.env_ot) PUT2(pos, '0', '2');
    else if (measure[n].temp[measure[n].temps - 1] < limits.env_ut) PUT2(pos, '0', '1');
    else PUT2(pos, '0', '0');
    if (measure[n].current > (float)(limits.chg_oc * 100u) && !(alarm_mask & 0x10)) PUT2(pos, '0', '2');
    else PUT2(pos, '0', '0');
    if (measure[n].voltage > limits.pack_ov && !(alarm_mask & 4)) PUT2(pos, '0', '2');
    else if (measure[n].voltage < limits.pack_uv && !(alarm_mask & 8)) PUT2(pos, '0', '1');
    else PUT2(pos, '0', '0');
    if (measure[n].current < limits.dsg_oc * 100 && !(alarm_mask & 0x20)) PUT2(pos, '0', '2');
    else PUT2(pos, '0', '0');
    sub_08015034(measure[n].alarm[0], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[1], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[2], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[3], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[4], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[5], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[6], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[7], hexbuf);
    PUT_HEX(pos);
    sub_08015034(measure[n].alarm[8], hexbuf);
    PUT_HEX(pos);
}

#define HB(i) sub_08014FD6(pace_buf[i], pace_buf[(i) + 1])
#define PUTC(c) (pace_buf[tx_len++] = (c))
#define PUTHEX(v) do { sub_08015034((v), hexbuf); PUTC(hexbuf[0]); PUTC(hexbuf[1]); } while (0)

void sub_08020D52(void)
{
    uchar i;
    ushort len;

    if (USART_GetITStatus(UART4, 0x727) != 0) {
        if (tx_idx < tx_len) {
            UART4_DR = pace_buf[tx_idx++];
        } else {
            tx_idx = 0;
            tx_len = 0;
            if (tx_cid2 == 0x42 || tx_cid2 == 0x44) {
                tx_pack++;
                if (tx_pack < pack_count) {
                    if (tx_cid2 == 0x42) sub_0801D8C0(tx_pack, &tx_len);
                    else sub_0801DB3A(tx_pack, &tx_len);
                    i = 0;
                    len = tx_len;
                    for (; i < len; i++) tx_sum = pace_buf[i] + tx_sum;
                    if (tx_pack + 1 == pack_count) {
                        tx_pack = 0;
                        tx_cid2 = 0;
                        tx_sum = ~tx_sum + 1;
                        sub_08015034(tx_sum >> 8, hexbuf);
                        pace_buf[tx_len++] = hexbuf[0];
                        pace_buf[tx_len++] = hexbuf[1];
                        sub_08015034(tx_sum, hexbuf);
                        pace_buf[tx_len++] = hexbuf[0];
                        pace_buf[tx_len++] = hexbuf[1];
                        pace_buf[tx_len++] = 0x0d;
                    }
                } else {
                    tx_pack = 0;
                    tx_cid2 = 0;
                }
            } else {
                tx_pack = 0;
                USART_ITConfig(UART4, 0x727, 0);
            }
        }
    }
}

void sub_08020E40(void)
{
    uchar i;

    for (i = 0; i < 21; i++) packs[0].serial[i] = g_20000077[i];
    for (i = 0; i < 21; i++) packs[0].model[i] = g_20000062[i];
    packs[0].online = 1;
    packs[0].f2c = g_20000162;
    packs[0].f2d = g_200000A2;
    packs[0].f2f = g_20000124;
    packs[0].f33 = g_20000110;
    packs[0].f35 = g_20000148;
    packs[0].f39 = g_20000164;
    packs[0].f3d = g_20000182;
    packs[0].f41 = g_2000016C;
    packs[0].f45 = g_20000184;
    packs[0].f88 = g_20000185;
    packs[0].alarm[0] = g_20000271;
    packs[0].alarm[1] = g_20000272;
    packs[0].alarm[2] = g_20000273;
    packs[0].alarm[3] = g_2000050D;
    packs[0].alarm[4] = g_20000274;
    packs[0].alarm[5] = alarm_mask;
    packs[0].alarm[6] = g_20000276;
    packs[0].alarm[7] = g_20000277;
    packs[0].alarm[8] = g_20000278;
    packs[0].f4f = g_2000010C;
    packs[0].f51 = g_2000010E;
    for (i = 0; i < 16; i++) packs[0].cell[i] = g_20001094[i];
    packs[0].f73 = g_2000012A;
    packs[0].f75 = g_2000012C;
    packs[0].version = 0xbcc;
    for (i = 0; i < 5; i++) packs[0].temp[i] = g_2000119E[i];
    packs[0].f83 = g_20000051;
    packs[0].f89 = g_20000130 / 10;
    packs[0].f8b = g_20000134 / 10;
    packs[0].f8d = g_200000A0;
    if (g_20000138 == 1) packs[0].f8e = 1;
    else if (g_20000139 == 1) packs[0].f8e = 2;
    else packs[0].f8e = 0;
    packs[0].f8f = g_20000098;
    packs[0].f90 = g_200000AD;
}

void sub_08020FA4(void)
{
    if (cfgs[own_addr].flags & 1) {
        cfgs[own_addr].flags &= ~1;
        g_20000182 = cfgs[own_addr].v8;
        sub_08022442(0x0c, g_20000182);
    }
    if (cfgs[own_addr].flags & 2) {
        cfgs[own_addr].flags &= ~2;
        g_20000164 = cfgs[own_addr].va;
        sub_08022442(0x08, g_20000164);
    }
    if (cfgs[own_addr].flags & 4) {
        cfgs[own_addr].flags &= ~4;
        g_2000016C = cfgs[own_addr].vc;
        sub_08022442(0x06, g_2000016C);
    }
    if (cfgs[own_addr].flags & 8) {
        cfgs[own_addr].flags &= ~8;
        g_20000162 = cfgs[own_addr].ve;
        sub_08022442(0x32, g_20000162);
    }
    if (cfgs[own_addr].flags & 0x10) cfgs[own_addr].flags &= ~0x10;
    if (cfgs[own_addr].flags & 0x20) cfgs[own_addr].flags &= ~0x20;
    if (cfgs[own_addr].flags & 0x40) cfgs[own_addr].flags &= ~0x40;
    if (cfgs[own_addr].flags & 0x80) cfgs[own_addr].flags &= ~0x80;
    if (cfgs[own_addr].flags & 0x100) cfgs[own_addr].flags &= ~0x100;
    if (cfgs[own_addr].flags & 0x200) cfgs[own_addr].flags &= ~0x200;
    if (cfgs[own_addr].flags & 0x400) cfgs[own_addr].flags &= ~0x400;
    if (cfgs[own_addr].flags & 0x800) cfgs[own_addr].flags &= ~0x800;
    if (cfgs[own_addr].flags & 0x1000) cfgs[own_addr].flags &= ~0x1000;
    if (cfgs[own_addr].flags & 0x2000) cfgs[own_addr].flags &= ~0x2000;
    if (cfgs[own_addr].flags & 0x4000) cfgs[own_addr].flags &= ~0x4000;
    if (cfgs[own_addr].flags & 0x8000) cfgs[own_addr].flags &= ~0x8000;
    if (cfgs[own_addr].flags & 0x10000) cfgs[own_addr].flags &= ~0x10000;
    if (cfgs[own_addr].flags & 0x20000) cfgs[own_addr].flags &= ~0x20000;
    if (cfgs[own_addr].flags & 0x40000) cfgs[own_addr].flags &= ~0x40000;
    if (cfgs[own_addr].flags & 0x80000) cfgs[own_addr].flags &= ~0x80000;
    if (cfgs[own_addr].flags & 0x100000) cfgs[own_addr].flags &= ~0x100000;
    if (cfgs[own_addr].flags & 0x200000) cfgs[own_addr].flags &= ~0x200000;
    if (cfgs[own_addr].flags & 0x400000) cfgs[own_addr].flags &= ~0x400000;
    if (cfgs[own_addr].flags & 0x800000) cfgs[own_addr].flags &= ~0x800000;
    if (cfgs[own_addr].flags & 0x1000000) cfgs[own_addr].flags &= ~0x1000000;
    if (cfgs[own_addr].flags & 0x2000000) cfgs[own_addr].flags &= ~0x2000000;
    if (cfgs[own_addr].flags & 0x4000000) cfgs[own_addr].flags &= ~0x4000000;
    if (cfgs[own_addr].flags & 0x8000000) cfgs[own_addr].flags &= ~0x8000000;
    if (cfgs[own_addr].flags & 0x10000000) cfgs[own_addr].flags &= ~0x10000000;
    if (cfgs[own_addr].flags & 0x20000000) cfgs[own_addr].flags &= ~0x20000000;
    if (cfgs[own_addr].flags & 0x40000000) cfgs[own_addr].flags &= ~0x40000000;
    if (cfgs[own_addr].flags & 0x80000000) cfgs[own_addr].flags &= ~0x80000000;
}

void sub_0802126C(void)
{
    uchar ok = 0;
    uchar cid2;
    uchar i;

    if (own_addr == 0) sub_08020E40();
    sub_08020FA4();
    if (pace_mode == 1) {
        if (rx_ready == 0) return;
        rx_ready = 0;
        cid2 = sub_08014FD6(pace_buf[7], pace_buf[8]);
        if (sub_0801508A(pace_buf) == 0) sub_0801DF70(2);
        else if (sub_08015058(pace_buf) == 0) sub_0801DF70(3);
        else sub_0801DF70(cid2);
        return;
    }
    if (mb_wait <= 3) { mb_wait++; return; }
    if (rx_ready == 0) return;
    rx_ready = 0;
    mb_wait = 0;
    mb_crc = sub_08024874(pace_buf, 7);
    if ((uchar)mb_crc == pace_buf[7] && pace_buf[8] == mb_crc >> 8) ok = 1;
    if (ok != 1) {
        for (i = 0; i < 170; i++) pace_buf[i] = 0;
        return;
    }
    if (pace_buf[1] < 0x10) {
        mb_addr = pace_buf[1];
        if (pace_buf[2] == 3 || pace_buf[2] == 4 || pace_buf[2] == 6) {
            modbus_resp[0] = pace_buf[1];
            modbus_resp[1] = pace_buf[2];
            if (pace_buf[2] == 3) sub_0801D424(pace_buf);
            else if (pace_buf[2] == 4) ;
            else if (pace_buf[2] == 6) sub_0801D606(pace_buf);
            if (pace_buf[2] == 6) {
                mb_len = 8;
            } else {
                ushort n = (pace_buf[5] * 256 + pace_buf[6]) * 2;
                modbus_resp[2] = n >> 8;
                modbus_resp[3] = n;
                mb_len = n + 6;
            }
            for (i = 0; i < mb_len; i++) modbus_tx[i] = modbus_resp[i];
            sub_08014F94(3, modbus_tx, mb_len);
            USART_ITConfig(UART4, 0x727, 1);
        } else {
            for (i = 0; i < 170; i++) pace_buf[i] = 0;
        }
    } else {
        for (i = 0; i < 170; i++) pace_buf[i] = 0;
    }
}

/* at: ascii_following_pool 0x08021070, ascii_watchdog_base 0x40002C00, ascii_uart4_base 0x40004C00 */
