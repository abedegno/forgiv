/* Complete I2C, EEPROM and event logging source unit. */
/* at: i2c_delay 0x0802217C, i2c_set_sda 0x0802218A, i2c_set_scl 0x080221A0, i2c_read_sda 0x080221B6, i2c_read_ack 0x080221CE, i2c_clock_until_released 0x080221FA, i2c_start 0x08022228, i2c_stop 0x08022254, i2c_read_bit 0x0802227C, i2c_read_byte 0x080222AC, i2c_write_bit 0x080222F0, i2c_write_byte 0x08022324, i2c_send_ack 0x0802236C, eeprom_write_byte 0x08022370, eeprom_read_byte 0x080223BC, eeprom_read_word 0x0802241E, eeprom_write_word 0x08022442, eeprom_read_value32 0x08022466, eeprom_write_value32 0x08022488, eeprom_log_write_record 0x080224AC, date_is_after 0x0802277C, eeprom_log_poll 0x08022816, measure 0x200014D2, date_now 0x2000018C, date_start 0x200000B6, date_end 0x200000BC, log_pending 0x200000B5, log_interval 0x200000B4, device_status 0x20000F9C, flags273 0x20000273, log_index 0x20000188, os_task_dly_idx 0x20000606, log_buffer 0x20001592, log_initialized 0x20000540, log_count 0x20000541, log_position 0x20000542, log_previous_voltage_current_protection 0x20000543, log_previous_temperature_protection 0x20000544, log_previous_fault_flags 0x20000545, log_previous_voltage_current_limits 0x20000546, log_previous_temperature_limits 0x20000547, log_window 0x20000548, alarm271 0x20000271, alarm272 0x20000272, alarm274 0x20000274, alarm277 0x20000277, alarm278 0x20000278 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I keep the software I2C bus, EEPROM access and record logging together.
   The logger owns its byte counts, write position and previous alarm snapshots;
   runtime supplies dates and measurement supplies the published record.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "../../include/bms_i2c_types.h"

void i2c_delay(void) {
    uchar count;
    for (count = 0; count < 80; count++) {
        __nop();
    }
}
void i2c_set_sda(int value) {
    if (value)
        GPIO_SetBits(0x40010c00, 0x80);
    else
        GPIO_ResetBits(0x40010c00, 0x80);
}
void i2c_set_scl(int value) {
    if (value)
        GPIO_SetBits(0x40010c00, 0x40);
    else
        GPIO_ResetBits(0x40010c00, 0x40);
}
int i2c_read_sda(void) {
    i2c_set_sda(1);
    i2c_delay();
    return GPIO_ReadInputDataBit(0x40010c00, 0x80);
}
/* match: I keep the initialized ACK local and its self-assignment; only low SDA sets ACK to one. */
int i2c_read_ack(void) {
    int level = 0, ack = 0;
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    level = i2c_read_sda();
    if (level == 1) {
        ack = ack;
    } else if (level == 0)
        ack = 1;
    i2c_set_scl(0);
    i2c_delay();
    return ack;
}
void i2c_clock_until_released(void) {
    uchar pulse_count = 0;
    int sda_level = 0;
    for (pulse_count = 0; pulse_count < 40; pulse_count++) {
        i2c_set_scl(0);
        i2c_delay();
        i2c_set_scl(1);
        i2c_delay();
        sda_level = i2c_read_sda();
        if (sda_level == 1)
            break;
    }
    i2c_delay();
}
void i2c_start() {

    i2c_set_sda(1);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_sda(0);
    i2c_delay();
    i2c_set_scl(0);
    i2c_delay();
    return;
}

/* STOP condition: SDA rises while SCL is high. */
void i2c_stop() {

    i2c_set_scl(0);
    i2c_set_sda(0);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_sda(1);
    i2c_delay();
    return;
}

int i2c_read_bit(void) {
    int sda_level, bit_value;
    bit_value = 0;
    sda_level = 0;
    i2c_set_scl(0);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    sda_level = i2c_read_sda();
    if (sda_level == 1)
        bit_value = 1;
    else if (sda_level == 0)
        bit_value = 0;
    i2c_set_scl(0);
    i2c_delay();
    return bit_value;
}

uchar i2c_read_byte() {
    uchar value = 0;

    value = 0;
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    value = i2c_read_bit() + (value << 1);
    return value;
}

/* Clocks out one bit. */
void i2c_write_bit(int bit_value) {

    i2c_set_scl(0);
    i2c_set_sda(bit_value);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_scl(0);
    i2c_delay();
    i2c_set_sda(1);
    i2c_delay();
    return;
}

/* Clocks out one byte, MSB first; returns 1 when it was acknowledged. */
uint i2c_write_byte(uint value) {
    uint acknowledged = 0;

    i2c_write_bit(value >> 7);
    i2c_write_bit((value & 0x7f) >> 6);
    i2c_write_bit((value & 0x3f) >> 5);
    i2c_write_bit((value & 0x1f) >> 4);
    i2c_write_bit((value & 0xf) >> 3);
    i2c_write_bit((value & 7) >> 2);
    i2c_write_bit((value & 3) >> 1);
    i2c_write_bit(value & 1);
    acknowledged = i2c_read_ack();
    return acknowledged;
}

/* Sends an ACK (a 0 bit). */
void i2c_send_ack() {

    i2c_write_bit(0);
    return;
}

uint eeprom_write_byte(uint addr, uint value) {
    uint ack = 0;
    os_task_delay((uchar)os_task_dly_idx, 5, 10);
    i2c_clock_until_released();
    i2c_start();
    ack = i2c_write_byte(0xa0);
    ack &= i2c_write_byte(addr >> 8);
    ack &= i2c_write_byte(addr & 255);
    ack &= i2c_write_byte(value);
    i2c_stop();
    return ack;
}
int eeprom_read_byte(uint addr) {
    uint ack = 0;
    uchar value = 0;
    os_task_delay((uchar)os_task_dly_idx, 5, 10);
    i2c_clock_until_released();
    i2c_start();
    ack = i2c_write_byte(0xa0);
    ack &= i2c_write_byte(addr >> 8);
    ack &= i2c_write_byte(addr & 255);
    i2c_stop();
    i2c_start();
    ack &= i2c_write_byte(0xa1);
    if (ack) {
        value = i2c_read_byte();
        i2c_send_ack();
        i2c_stop();
        return value;
    }
    i2c_stop();
    return ack;
}
ushort eeprom_read_word(ushort addr) {
    int low, high;
    low = eeprom_read_byte((ushort)(addr + 0x7801));
    high = eeprom_read_byte((ushort)(addr + 0x7800));
    return high * 256 + low;
}
int eeprom_write_word(ushort addr, uint value) {
    eeprom_write_byte((ushort)(addr + 0x7800), value >> 8);
    eeprom_write_byte((ushort)(addr + 0x7801), value & 255);
    return 1;
}
uint eeprom_read_value32(uint addr) {
    uchar i = 0;
    uint value;
    for (i = 0; i < 4; i++)
        ((uchar *)&value)[i] = eeprom_read_byte((ushort)(addr + i + 0x7800));
    return value;
}
int eeprom_write_value32(uint addr, uint value) {
    uchar i = 0;
    uint temp = value;
    for (i = 0; i < 4; i++)
        eeprom_write_byte((ushort)(addr + i + 0x7800), ((uchar *)&temp)[i]);
    return 1;
}
uchar log_initialized, log_count, log_position, log_previous_voltage_current_protection,
    log_previous_temperature_protection, log_previous_fault_flags,
    log_previous_voltage_current_limits, log_previous_temperature_limits, log_window;
/* Serializes measurement and date fields to the selected EEPROM log bank.
   incremental == 0 writes the complete record; other values advance one byte.
   match: I retain direct measurement reads and the original log-count updates. */
void eeprom_log_write_record(ushort *record_index, int bank, int incremental) {
    uchar element_index;
    ushort record_address;
    if (log_pending) {
        if (bank == 0)
            record_address = (*record_index & 0x7fff) * 65;
        else
            record_address = ((*record_index & 0x7fff) + 400) * 65;
        if (log_initialized == 0 || incremental == 0) {
            log_initialized = 1;
            log_count = 0;
            snapshot_publish();
            runtime_publish_date();
            log_buffer[log_count++] = bcd_to_byte(date_now.d[0]);
            log_buffer[log_count++] = bcd_to_byte(date_now.d[1]);
            log_buffer[log_count++] = bcd_to_byte(date_now.d[2]);
            log_buffer[log_count++] = bcd_to_byte(date_now.d[3]);
            log_buffer[log_count++] = bcd_to_byte(date_now.d[4]);
            log_buffer[log_count++] = bcd_to_byte(date_now.d[5]);
            log_buffer[log_count++] = measure.cells;
            for (element_index = 0; measure.cells > element_index; element_index++) {
                log_buffer[log_count++] = measure.cell[element_index] >> 8;
                log_buffer[log_count++] = measure.cell[element_index];
            }
            log_buffer[log_count++] = measure.temps;
            for (element_index = 0; measure.temps > element_index; element_index++) {
                log_buffer[log_count++] = measure.temp[element_index] >> 8;
                log_buffer[log_count++] = measure.temp[element_index];
            }
            log_buffer[log_count++] = measure.voltage >> 8;
            log_buffer[log_count++] = measure.voltage;
            log_buffer[log_count++] = measure.current >> 8;
            log_buffer[log_count++] = measure.current;
            log_buffer[log_count++] = measure.soc >> 8;
            log_buffer[log_count++] = measure.soc;
            log_buffer[log_count++] = measure.soh >> 8;
            log_buffer[log_count++] = measure.soh;
            log_buffer[log_count++] = measure.alarm70;
            log_buffer[log_count++] = measure.alarm71;
            log_buffer[log_count++] = measure.alarm74;
            if (flags273 & 64)
                log_buffer[log_count++] = measure.alarm77 | 128;
            else
                log_buffer[log_count++] = measure.alarm77;
            log_buffer[log_count++] = measure.alarm78;
        }
        if (incremental == 0) {
            for (element_index = 0; element_index < log_count; element_index++) {
                os_task_delay((uchar)os_task_dly_idx, 5, 10);
                if (eeprom_write_byte((ushort)(record_address + element_index),
                                      log_buffer[element_index]) == 0) {
                    log_pending = 0;
                    log_position = 0;
                    log_initialized = 0;
                    return;
                }
                log_position++;
            }
        } else {
            os_task_delay((uchar)os_task_dly_idx, 5, 10);
            if (eeprom_write_byte((ushort)(record_address + log_position),
                                  log_buffer[log_position]) == 0) {
                log_pending = 0;
                log_position = 0;
                log_initialized = 0;
                return;
            }
            log_position++;
        }
        if (log_position >= log_count) {
            log_pending = 0;
            log_position = 0;
            log_initialized = 0;
            (*record_index)++;
            if (bank == 0) {
                if ((*record_index & 0x7fff) > 399)
                    *record_index = 0x8000;
                eeprom_write_word(0x14, *record_index);
                return;
            }
            if ((*record_index & 0x7fff) != 0)
                *record_index = 0x8000;
        }
    }
}
/* Lexicographic strict comparison of six date bytes.
   Equal dates return zero; this is a strict ordering test. */
int date_is_after(date_t candidate, date_t reference) {
    if ((uint)candidate.d[0] > reference.d[0])
        return 1;
    if ((uint)candidate.d[0] < reference.d[0])
        return 0;
    if ((uint)candidate.d[1] > reference.d[1])
        return 1;
    if ((uint)candidate.d[1] < reference.d[1])
        return 0;
    if ((uint)candidate.d[2] > reference.d[2])
        return 1;
    if ((uint)candidate.d[2] < reference.d[2])
        return 0;
    if ((uint)candidate.d[3] > reference.d[3])
        return 1;
    if ((uint)candidate.d[3] < reference.d[3])
        return 0;
    if ((uint)candidate.d[4] > reference.d[4])
        return 1;
    if ((uint)candidate.d[4] < reference.d[4])
        return 0;
    if ((uint)candidate.d[5] > reference.d[5])
        return 1;
    return 0;
}
/* Detects time-window and alarm changes then advances record writing. */
void eeprom_log_poll(void) {
    if (!log_pending) {
        if (device_status[0xa9]) {
            runtime_publish_date();
            if (log_window == 0) {
                runtime_timer_start(0x15, 400);
                if (date_is_after(date_now, date_start))
                    log_window = 1;
            } else {
                if (date_is_after(date_now, date_end) || date_is_after(date_start, date_end)) {
                    runtime_timer_start(0x15, 400);
                    log_window = 0;
                } else {
                    runtime_timer_start(0x15, (uint)log_interval * 2400);
                    log_pending = 1;
                }
            }
        }
        if (log_previous_voltage_current_protection != alarm271 ||
            log_previous_temperature_protection != (alarm272 & 0x7f)) {
            log_previous_voltage_current_protection = alarm271;
            log_previous_temperature_protection = alarm272 & 0x7f;
            log_pending = 1;
        } else if (log_previous_fault_flags != (alarm274 & 0x3f)) {
            log_previous_fault_flags = alarm274 & 0x3f;
            log_pending = 1;
        } else if (log_previous_voltage_current_limits != (alarm277 & 0x7f) ||
                   log_previous_temperature_limits != (alarm278 & 0x7f)) {
            log_previous_voltage_current_limits = alarm277 & 0x7f;
            log_previous_temperature_limits = alarm278 & 0x7f;
            log_pending = 1;
        }
    }
    eeprom_log_write_record(&log_index, 0, 1);
}
