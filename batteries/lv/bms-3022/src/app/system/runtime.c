/* at: crc_table_initialize 0x08015184 */
/* at: g_2000002E 0x2000002E, g_20000030 0x20000030, g_20000032 0x20000032, g_20000034 0x20000034, g_20000036 0x20000036, g_20000038 0x20000038,  g_20000024 0x20000024, g_20000025 0x20000025, g_20000026 0x20000026, g_20000027 0x20000027, g_20000028 0x20000028, g_20000029 0x20000029, g_2000002A 0x2000002A, g_2000002B 0x2000002B, g_2000002C 0x2000002C, g_2000004C 0x2000004C, g_20000050 0x20000050, g_20000052 0x20000052, g_20000053 0x20000053, g_20000054 0x20000054, g_20000058 0x20000058, g_2000005A 0x2000005A, g_2000005B 0x2000005B, g_2000005C 0x2000005C, g_2000005D 0x2000005D, g_20000060 0x20000060, g_20000061 0x20000061, g_2000008C 0x2000008C, g_20000090 0x20000090, g_20000092 0x20000092, g_2000009D 0x2000009D, g_200000B5 0x200000B5, g_200000D2 0x200000D2, g_200000D6 0x200000D6, g_200000DF 0x200000DF, g_200000E1 0x200000E1, g_200000EA 0x200000EA, g_200000EC 0x200000EC, g_200000EE 0x200000EE, g_200000EF 0x200000EF, g_200000F0 0x200000F2, g_200000F2 0x200000F4, g_200000F4 0x200000F6, g_20000101 0x20000105, g_20000144 0x20000148, g_20000160 0x20000164, g_20000163 0x20000167, g_2000016C 0x20000170, g_20000184 0x20000188, g_20000188 0x2000018C, date_now 0x20000190, g_20000273 0x2000027B, g_2000058F 0x200005A7, g_20000606 0x2000061E, g_2000069C 0x200006B4, g_20000F9C 0x20000FB4, g_20004149 0x20004161, g_2000748C 0x200074A4 */
/* Runtime CRC, software timers, GPIO indications and shutdown task. */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow software timers, BCD helpers, RTC publication and shutdown checks here. Serial, logging and chain tasks use these services.
   Entry examples: crc_table_initialize.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/bms_protocol.h"
#include "../../include/stdperiph_types.h"
#include "../../include/bms_timers.h"
#include "../../include/bms_dates.h"
uchar g_20000024 = 0, g_20000025 = 0, g_20000026 = 0, g_20000027 = 0, g_20000028 = 0,
      g_20000029 = 0, g_2000002A = 0, g_2000002B = 0, g_2000002C = 0;
ushort g_2000002E = 0, g_20000030 = 0, g_20000032 = 0, g_20000034 = 0, g_20000036 = 0,
       g_20000038 = 0;
uint bit_reflect(uint, int);
void GPIO_ResetBits(void *, ushort);
void GPIO_SetBits(void *, ushort);
undefined1 afe_write_register(short, ushort);
void board_set_pb9(int);
uchar GPIO_ReadInputDataBit(void *, ushort);
void RCC_AHBPeriphClockCmd(uint, uchar);
undefined4 RTC_Get(void);
undefined4 RTC_Init(void);
void board_set_pb5(int);
uint __aeabi_f2uiz(float);
void os_task_delay(int, ushort, ushort);
void snapshot_publish(void);
undefined4 eeprom_write_word(short, uint);
void eeprom_log_write_record(ushort *, int, int);
void board_set_pb2(int);
void board_set_pa8(int);
uint os_flags_pend(ushort);
void firmware_clear_marker_and_reset(void);
void eeprom_log_poll(void);
void runtime_update_pb9_pulse(void);
void runtime_read_pack_address(void);
void runtime_timers_tick(void);
void runtime_update_soc_indicators(void);
void runtime_timer_start(int param_1, uint param_2);
uchar byte_to_bcd(uchar value);
void runtime_timer_stop(int param_1);
void runtime_update_high_current_latch(void);
void runtime_check_reset_requests(void);
void runtime_slow_tick(void);
static __inline void runtime_system_reset(void);
uint hex_digit_to_ascii(uint param_1);
void runtime_task(void);
void crc_update(uint *crc, uchar *data, ushort length);
void runtime_check_shutdown_voltage(void);
void runtime_publish_date(void);
void crc_table_initialize(uint polynomial);
uchar bcd_to_byte(uchar value);

extern uchar g_2000069C[];
#define g_2000069C_v (*(uint *)(g_2000069C + 0))

void crc_table_initialize(uint polynomial) {
    int i, j;
    uint value;
    polynomial = bit_reflect(polynomial, 32);
    for (i = 0; i <= 255; i++) {
        value = i;
        for (j = 0; j < 8; j++) {
            if (value & 1)
                value = (value >> 1) ^ polynomial;
            else
                value >>= 1;
        }
        ((uint *)g_2000069C)[i] = value;
    }
}
#undef g_2000069C_v

extern uchar g_2000069C[];
#define g_2000069C_v (*(uint *)(g_2000069C + 0))

void crc_update(uint *crc, uchar *data, ushort length) {
    ushort i;
    for (i = 0; i < length; i++)
        *crc = ((uint *)g_2000069C)[*data++ ^ (uchar)*crc] ^ (*crc >> 8);
}
#undef g_2000069C_v

uchar bcd_to_byte(uchar value) {
    uint high = (value >> 4) * 10;
    return high + (value & 15);
}

uchar byte_to_bcd(uchar value) {
    uint high = (value / 10) * 16;
    return high + value % 10;
}

uint hex_digit_to_ascii(uint nibble) {

    if (nibble < 10) {
        return nibble + 0x30 & 0xff;
    }
    return nibble + 0x37 & 0xff;
}

extern uchar g_2000008C;
#define timer_tick_pending (g_2000008C)

void runtime_timers_tick(void) {
    uchar i;
    timer_tick_pending = 1;
    for (i = 0; i < 31; i++) {
        if (g_20000F9C[i].enabled) {
            if (g_20000F9C[i].left > 1)
                g_20000F9C[i].left--;
            else {
                g_20000F9C[i].enabled = 0;
                g_20000F9C[i].elapsed = 1;
            }
        }
    }
}
#undef timer_tick_pending

void runtime_timer_start(int timer_index, uint duration) {
    g_20000F9C[timer_index].elapsed = 0;
    g_20000F9C[timer_index].left = duration;
    g_20000F9C[timer_index].enabled = 1;
    return;
}

void runtime_timer_stop(int timer_index) {
    g_20000F9C[timer_index].enabled = 0;
    g_20000F9C[timer_index].elapsed = 0;
    return;
}

#define gpio_a_port (*(volatile uchar *)0x40010800)
#define gpio_c_port (*(volatile uchar *)0x40011000)

#define low_soc_led_phase (g_20000025)
extern uchar g_20000184;
#define state_of_charge (g_20000184)

/* Drives SOC indication levels and the low-SOC blink phase. */
void runtime_update_soc_indicators(void) {

    if (state_of_charge <= 100 && state_of_charge >= 75) {
        GPIO_SetBits((void *)&gpio_a_port, 0x8000);
        GPIO_SetBits((void *)&gpio_c_port, 0x1000);
        afe_write_register(3, 3);
        return;
    }
    if (state_of_charge >= 50) {
        GPIO_SetBits((void *)&gpio_a_port, 0x8000);
        GPIO_SetBits((void *)&gpio_c_port, 0x1000);
        afe_write_register(3, 1);
        return;
    }
    if (state_of_charge >= 25) {
        GPIO_SetBits((void *)&gpio_a_port, 0x8000);
        GPIO_SetBits((void *)&gpio_c_port, 0x1000);
        afe_write_register(3, 0);
        return;
    }
    if (state_of_charge >= 5) {
        GPIO_SetBits((void *)&gpio_a_port, 0x8000);
        GPIO_ResetBits((void *)&gpio_c_port, 0x1000);
        afe_write_register(3, 0);
        return;
    }
    if (low_soc_led_phase == '\0') {
        low_soc_led_phase = 1;
        GPIO_ResetBits((void *)&gpio_a_port, 0x8000);
    } else {
        low_soc_led_phase = 0;
        GPIO_SetBits((void *)&gpio_a_port, 0x8000);
    }
    GPIO_ResetBits((void *)&gpio_c_port, 0x1000);
    afe_write_register(3, 0);
    return;
}
#undef gpio_a_port
#undef gpio_c_port
#undef low_soc_led_phase
#undef state_of_charge

#define DAT_2000002e (g_2000002E)
extern volatile uchar g_2000009D;
#define DAT_2000009d (g_2000009D)
extern uchar g_2000058F;
#define DAT_2000058f (g_2000058F)

/* Drives PB9 for the selected short or long pulse duration. */
void runtime_update_pb9_pulse(void) {
    if (!DAT_2000009d) {
        if (DAT_2000058f == 1) {
            if (DAT_2000002e < 1500) {
                DAT_2000002e++;
                board_set_pb9(1);
            } else
                board_set_pb9(0);
        } else {
            if (DAT_2000002e < 15) {
                DAT_2000002e++;
                board_set_pb9(1);
            } else
                board_set_pb9(0);
        }
    }
}
#undef DAT_2000002e
#undef DAT_2000009d
#undef DAT_2000058f

extern uchar g_20000101[];
#define rtc_second (*(uchar *)(g_20000101 + 0))
#define rtc_minute (*(uchar *)(g_20000101 + 1))
#define rtc_hour (*(uchar *)(g_20000101 + 2))
#define rtc_day (*(uchar *)(g_20000101 + 3))
#define rtc_month (*(uchar *)(g_20000101 + 4))
#define rtc_year (*(uchar *)(g_20000101 + 5))

void runtime_publish_date(void) {
    date_now.d[0] = rtc_year;
    date_now.d[1] = rtc_month;
    date_now.d[2] = rtc_day;
    date_now.d[3] = rtc_hour;
    date_now.d[4] = rtc_minute;
    date_now.d[5] = rtc_second;
    return;
}
#undef rtc_second
#undef rtc_minute
#undef rtc_hour
#undef rtc_day
#undef rtc_month
#undef rtc_year

#define gpio_b_port (*(volatile uchar *)0x40010C00)
extern uchar g_200000EE;
#define DAT_200000ee (g_200000EE)
extern uchar g_20000163;
#define pack_address (g_20000163)

void runtime_read_pack_address(void) {
    uchar pin_level;

    DAT_200000ee = 1;
    pin_level = GPIO_ReadInputDataBit((void *)&gpio_b_port, 0x1000);
    if (pin_level == '\0') {
        pack_address = 1;
        return;
    }
    pin_level = GPIO_ReadInputDataBit((void *)&gpio_b_port, 0x2000);
    if (pin_level == '\0') {
        pack_address = 2;
        return;
    }
    pin_level = GPIO_ReadInputDataBit((void *)&gpio_b_port, 0x4000);
    if (pin_level == '\0') {
        pack_address = 3;
        return;
    }
    pin_level = GPIO_ReadInputDataBit((void *)&gpio_b_port, 0x8000);
    if (pin_level != '\0') {
        pack_address = 0;
        return;
    }
    pack_address = 4;
    return;
}
#undef gpio_b_port
#undef DAT_200000ee
#undef pack_address

#define DAT_20000030 (g_20000030)
#define DAT_20000032 (g_20000032)
extern uchar g_20000092;
#define DAT_20000092 (g_20000092)
extern float g_20000144;
#define DAT_20000144 (g_20000144)

/* Debounces the high-current state using separate on/off counters. */
void runtime_update_high_current_latch(void) {

    if (DAT_20000144 >= 10000.0f) {
        DAT_20000030 = DAT_20000030 + 1;
        if (DAT_20000030 >= 240) {
            DAT_20000030 = 0;
            DAT_20000092 = 1;
        }
        return;
    }
    DAT_20000032 = DAT_20000032 + 1;
    if (DAT_20000032 >= 120) {
        DAT_20000032 = 0;
        DAT_20000092 = 0;
    }
    DAT_20000030 = 0;
    return;
}
#undef DAT_20000030
#undef DAT_20000032
#undef DAT_20000092
#undef DAT_20000144

#define DAT_20000024 (g_20000024)
#define DAT_20000026 (g_20000026)
#define DAT_20000027 (g_20000027)
#define DAT_20000028 (g_20000028)
#define DAT_20000029 (g_20000029)
#define DAT_2000002a (g_2000002A)
extern uchar g_2000004C;
#define DAT_2000004c (g_2000004C)
extern uchar g_20000050;
#define DAT_20000050 (g_20000050)
extern uchar g_20000053;
#define DAT_20000053 (g_20000053)
extern uchar g_20000054;
#define DAT_20000054 (g_20000054)
extern ushort g_20000058;
#define DAT_20000058 (g_20000058)
extern uchar g_20000060;
#define DAT_20000060 (g_20000060)
extern volatile uchar g_20000061;
#define DAT_20000061 (g_20000061)
extern volatile uchar g_2000009D;
#define DAT_2000009d (g_2000009D)
extern ushort g_200000D2;
#define DAT_200000d2 (g_200000D2)
extern ushort g_200000D6;
#define DAT_200000d6 (g_200000D6)

/* Updates indicators, elapsed counters, USB restart state and RTC maintenance. */
void runtime_slow_tick(void) {
    if (!DAT_2000002a && !DAT_2000009d && !DAT_20000061) {
        DAT_2000002a = 1;
        board_set_pb5(1);
    } else {
        DAT_2000002a = 0;
        board_set_pb5(0);
    }
    runtime_update_soc_indicators();
    runtime_read_pack_address();
    runtime_update_high_current_latch();
    if (DAT_200000d6 < 20)
        DAT_200000d6++;
    if (DAT_200000d2 < 320)
        DAT_200000d2++;
    else
        DAT_200000d2 = 0;
    if (DAT_20000060 < 10)
        DAT_20000060++;
    if (DAT_20000050 >= 2) {
        if (DAT_20000050 < 20)
            DAT_20000050++;
    }
    if (DAT_20000053 != 8) {
        if (DAT_20000058 >= 1440) {
            DAT_20000058 = 0;
            DAT_20000050 = 1;
            RCC_AHBPeriphClockCmd(0x1000, 0);
            DAT_20000027 = 0;
        }
        if (DAT_20000050 == 1) {
            if (DAT_20000027 < 13)
                DAT_20000027++;
            else {
                DAT_20000027 = 0;
                DAT_20000050 = 2;
                DAT_20000058 = 0;
            }
        }
    } else {
        DAT_20000050 = 0;
        DAT_20000058 = 0;
    }
    if (!DAT_20000024) {
        if (RTC_Init()) {
            DAT_2000004c = 1;
            DAT_20000024 = 1;
        } else {
            DAT_20000029++;
            if (DAT_20000029 > 1) {
                DAT_20000024 = 1;
                DAT_20000029 = 0;
                DAT_2000004c = 1;
            }
        }
    }
    if (DAT_20000054) {
        if (DAT_20000026 < 120)
            DAT_20000026++;
        else {
            DAT_20000026 = 0;
            DAT_20000054 = 0;
        }
    } else {
        DAT_20000026 = 0;
        if (DAT_20000028) {
            DAT_20000028 = 0;
            RTC_Get();
        } else
            DAT_20000028 = 1;
    }
}
#undef DAT_20000024
#undef DAT_20000026
#undef DAT_20000027
#undef DAT_20000028
#undef DAT_20000029
#undef DAT_2000002a
#undef DAT_2000004c
#undef DAT_20000050
#undef DAT_20000053
#undef DAT_20000054
#undef DAT_20000058
#undef DAT_20000060
#undef DAT_20000061
#undef DAT_2000009d
#undef DAT_200000d2
#undef DAT_200000d6

#define gpio_b_port (*(volatile uchar *)0x40010C00)
#define gpio_c_port (*(volatile uchar *)0x40011000)
#define gpio_d_port (*(volatile uchar *)0x40011400)

#define shutdown_delay_ticks (g_20000034)
#define shutdown_ticks (g_20000036)
extern uchar g_2000005A;
#define gpio_c8_flag (g_2000005A)
extern uchar g_2000005B;
#define gpio_b3_flag (g_2000005B)
extern uchar g_2000005C;
#define gpio_d2_flag (g_2000005C)
extern uchar g_2000005D;
#define DAT_2000005d (g_2000005D)
extern ushort g_20000090;
#define DAT_20000090 (g_20000090)
extern uchar g_200000B5;
#define log_write_pending (g_200000B5)
extern uchar g_200000DF;
#define DAT_200000df (g_200000DF)
extern uchar g_200000E1;
#define DAT_200000e1 (g_200000E1)
extern ushort g_200000EA;
#define DAT_200000ea (g_200000EA)
extern ushort g_200000EC;
#define DAT_200000ec (g_200000EC)
extern uchar g_200000EF;
#define DAT_200000ef (g_200000EF)
extern ushort g_200000F2;
#define DAT_200000f2 (g_200000F2)
extern ushort g_200000F4;
#define DAT_200000f4 (g_200000F4)
extern uchar g_20000163;
#define pack_address (g_20000163)
extern float g_2000016C;
#define remaining_capacity (g_2000016C)
extern ushort g_20000188;
#define log_record_index (g_20000188)
extern uchar g_20000273;
#define DAT_20000273 (g_20000273)
extern ushort g_20000606;
#define os_task_dly_idx (g_20000606)
#define DAT_20000ffd (*(volatile uchar *)((uchar *)g_20000F9C + 97))
extern uint g_20004149;
#define g_20004149_v (g_20004149)

static __inline ushort next_count(ushort value) {
    return value + 1;
}
/* Persists capacity and disables outputs before requested resets. */
void runtime_check_reset_requests(void) {
    uint *pack_flags;
    uchar i;
    uint capacity_word;

    i = 0;
    if ((pack_address == '\0') && (DAT_200000df == '\x01')) {
        DAT_200000ec = DAT_200000ec + 1;
        if (9000 < DAT_200000ec) {
            DAT_200000ec = 0;
            afe_write_register(6, 0x10);
            capacity_word = (uint)remaining_capacity;
            eeprom_write_word(6, capacity_word & 0xffff);
            afe_write_register(6, 0x10);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
            disableFIQinterrupts();
            /* WARNING: Subroutine does not return */
            runtime_system_reset();
        }
    } else {
        DAT_200000ec = 0;
    }
    if (pack_address == '\0') {
        shutdown_delay_ticks = 9000;
    } else {
        shutdown_delay_ticks = 6000;
    }
    if ((DAT_200000ef == '\x01') && (DAT_200000e1 == '\x01')) {
        shutdown_delay_ticks = 500;
    }
    if (DAT_200000e1 == '\x01') {
        if (++DAT_200000ea > shutdown_delay_ticks) {
            DAT_200000ea = 0;
            afe_write_register(6, 0x10);
            capacity_word = (uint)remaining_capacity;
            eeprom_write_word(6, capacity_word & 0xffff);
            afe_write_register(6, 0x10);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
            disableFIQinterrupts();
            /* WARNING: Subroutine does not return */
            runtime_system_reset();
        }
    } else {
        DAT_200000ea = 0;
    }

    if (pack_address == '\0') {
        if ((DAT_200000f2 != 1) && (DAT_20000ffd != '\x01')) {
            shutdown_ticks = 0;
            return;
        }
        if ((shutdown_ticks = next_count(shutdown_ticks)) == 1) {
            runtime_timer_stop(0x1a);
            log_write_pending = 1;
            DAT_20000273 = DAT_20000273 | 0x40;
            snapshot_publish();
            afe_write_register(6, 0x10);
            eeprom_write_word(0x1a, (uint)DAT_20000090);
            eeprom_write_word(6, ((uint)remaining_capacity) & 0xffff);
            eeprom_log_write_record(&log_record_index, 0, 0);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
            GPIO_ResetBits((void *)&gpio_d_port, 4);
            gpio_d2_flag = 0;
            GPIO_ResetBits((void *)&gpio_b_port, 8);
            gpio_b3_flag = 0;
            GPIO_ResetBits((void *)&gpio_c_port, 0x100);
            gpio_c8_flag = 0;
            DAT_2000005d = 0;
            board_set_pb2(0);
            board_set_pa8(0);
            afe_write_register(6, 0x10);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
            do {
                pack_flags = (uint *)((int)&g_20004149_v + i * 0x72);
                *pack_flags = *pack_flags | 0x10;
                i++;
            } while (i < 7);
        }
        if (shutdown_ticks >= 1000) {
            DAT_200000f2 = 0;
            shutdown_ticks = 0;
            disableFIQinterrupts();
            runtime_system_reset();
        }
    } else {
        if ((DAT_200000f2 != 1) && (DAT_20000ffd != '\x01')) {
            shutdown_ticks = 0;
            return;
        }
        if ((shutdown_ticks = next_count(shutdown_ticks)) == 1) {
            runtime_timer_stop(0x1a);
            log_write_pending = 1;
            DAT_20000273 = DAT_20000273 | 0x40;
            snapshot_publish();
            eeprom_log_write_record(&log_record_index, 0, 0);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
            GPIO_ResetBits((void *)&gpio_d_port, 4);
            gpio_d2_flag = 0;
            GPIO_ResetBits((void *)&gpio_b_port, 8);
            gpio_b3_flag = 0;
            GPIO_ResetBits((void *)&gpio_c_port, 0x100);
            gpio_c8_flag = 0;
            DAT_2000005d = 0;
            board_set_pb2(0);
            board_set_pa8(0);
            afe_write_register(6, 0x10);
            eeprom_write_word(0x1a, (uint)DAT_20000090);
            capacity_word = (uint)remaining_capacity;
            eeprom_write_word(6, capacity_word & 0xffff);
            afe_write_register(6, 0x10);
            os_task_delay((uint)(byte)os_task_dly_idx, 100, 10);
        }
        if (shutdown_ticks >= 200) {
            DAT_200000f4 = 0;
            shutdown_ticks = 0;
            disableFIQinterrupts();
            /* WARNING: Subroutine does not return */
            runtime_system_reset();
        }
    }
}
#undef gpio_b_port
#undef gpio_c_port
#undef gpio_d_port
#undef shutdown_delay_ticks
#undef shutdown_ticks
#undef gpio_c8_flag
#undef gpio_b3_flag
#undef gpio_d2_flag
#undef DAT_2000005d
#undef DAT_20000090
#undef log_write_pending
#undef DAT_200000df
#undef DAT_200000e1
#undef DAT_200000ea
#undef DAT_200000ec
#undef DAT_200000ef
#undef DAT_200000f2
#undef DAT_200000f4
#undef pack_address
#undef remaining_capacity
#undef log_record_index
#undef DAT_20000273
#undef os_task_dly_idx
#undef DAT_20000ffd
#undef g_20004149_v

#define DAT_2000002b (g_2000002B)
extern volatile uchar g_2000009D;
#define DAT_2000009d (g_2000009D)
extern ushort g_20000160;
#define DAT_20000160 (g_20000160)

/* Debounces the voltage-window condition that requests shutdown. */
void runtime_check_shutdown_voltage(void) {
    if ((uint)(DAT_20000160 - 0xf08) <= 0xf8) {
        if (DAT_2000002b >= 100) {
            DAT_2000002b = 0;
            DAT_2000009d |= 2;
        } else
            DAT_2000002b++;
    } else
        DAT_2000002b = 0;
}
#undef DAT_2000002b
#undef DAT_2000009d
#undef DAT_20000160

#define DAT_2000002c (g_2000002C)
#define DAT_20000038 (g_20000038)
extern uchar g_20000052;
#define DAT_20000052 (g_20000052)
extern uchar g_200000DF;
#define DAT_200000df (g_200000DF)
extern uchar g_200000E1;
#define DAT_200000e1 (g_200000E1)
extern uchar g_200000F0;
#define DAT_200000f0 (g_200000F0)
extern uchar g_20000163;
#define pack_address (g_20000163)
extern uchar g_2000748C[];
#define DAT_200074d2 (*(ushort *)(g_2000748C + 70))
#define DAT_200074d4 (*(ushort *)(g_2000748C + 72))

/* Runs logging, periodic maintenance, pack summary and reset checks. */
void runtime_task(void) {

    while (1) {
        os_flags_pend(0);
        eeprom_log_poll();
        runtime_update_pb9_pulse();
        runtime_check_shutdown_voltage();
        if (DAT_2000002c < 0x18) {
            DAT_2000002c = DAT_2000002c + 1;
        } else {
            DAT_2000002c = 0;
            runtime_slow_tick();
            if (((DAT_200000f0 == '\x01') && (pack_address == '\0')) && (DAT_20000038++ > 20)) {
                DAT_20000038 = 0;
                DAT_200000e1 = 1;
                DAT_200000df = 1;
                DAT_200000f0 = '\0';
            }
        }
        pack_update_summary();
        if (DAT_20000052 != '\0') {
            firmware_clear_marker_and_reset();
        }
        runtime_check_reset_requests();
        DAT_200074d4 = 0;
        DAT_200074d2 = 0x14;
    }
}
#undef DAT_2000002c
#undef DAT_20000038
#undef DAT_20000052
#undef DAT_200000df
#undef DAT_200000e1
#undef DAT_200000f0
#undef pack_address
#undef DAT_200074d2
#undef DAT_200074d4

#define system_reset_control (*(volatile uint *)0xE000ED0C)

static __inline void runtime_system_reset(void) {

    system_reset_control = (system_reset_control) & 0x700 | 0x5fa0004;
    DataSynchronizationBarrier(0xf);
    do {
        /* WARNING: Do nothing block with infinite loop */
    } while (true);
}
#undef system_reset_control
