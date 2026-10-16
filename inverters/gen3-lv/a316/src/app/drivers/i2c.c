// clang-format off
/* at: i2c_delay 0x08023144 */
/* at: i2c_sda_dir 0x08023154, i2c_scl_dir 0x08023196, i2c_sda_write 0x080231D8, i2c_scl_write 0x080231FC, i2c_wait_ack 0x08023220, i2c_bus_recover 0x08023262, i2c_start 0x080232AC, i2c_stop 0x080232D8, i2c_read_bit 0x08023300, i2c_read_byte 0x0802334A, i2c_write_bit 0x080233A4, i2c_write_byte 0x080233D8, i2c_send_ack 0x08023426, i2c_write_reg 0x08023430, i2c_read_reg 0x08023464 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* i2c.c: a bit-banged I2C master on PB6 (SCL) and PB7 (SDA) for the EEPROM. Lines are driven
   push-pull and switched to input with pull-up to read (GPIO mode 0x10 output, 0x48 input
   pull-up); one bit takes a few i2c_delay calls, a few hundred kHz at most (inference).

   Entry points: i2c_start, i2c_stop, i2c_write_byte, i2c_read_byte, i2c_send_ack,
   i2c_bus_recover (all from eeprom.c). Dead: i2c_write_reg, i2c_read_reg.
   Data: none.
   Neighbours: modbus_slave.c before it (i2c_delay could be that file's tail; nothing ties it
   either way), board.c after it; eeprom.c is the only user.

   name: likely (map/filenames.tsv). Two parts joined, the joined file compiles to the same
   bytes. */
#define OWN_GPIO_ReadInputDataBit /* match: declared below as bool(): this file compiled against that, not the header's int(int,int) */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/eeprom.h"
#include "../../include/stdperiph.h"

/* Busy-waits 60 NOPs: the I2C bit timing. */
void i2c_delay() {
    uchar delay_count;

    for (delay_count = 0; delay_count < 0x3c; delay_count = delay_count + 1) {
        __nop();
    }
    return;
}

bool GPIO_ReadInputDataBit();

/* SDA (PB7) as input with pull-up (0) or push-pull output (1). */
void i2c_sda_dir(int output_enabled) {
    GPIO_InitTypeDef gpio_config;

    if (output_enabled == 0) {
        gpio_config.GPIO_Pin = 0x80;
        gpio_config.GPIO_Speed = 0x3;
        gpio_config.GPIO_Mode = 0x48;
        GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    } else if (output_enabled == 1) {
        gpio_config.GPIO_Pin = 0x80;
        gpio_config.GPIO_Speed = 0x3;
        gpio_config.GPIO_Mode = 0x10;
        GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    }
    return;
}

/* SCL (PB6) as input with pull-up (0) or push-pull output (1). */
void i2c_scl_dir(int output_enabled) {
    GPIO_InitTypeDef gpio_config;

    if (output_enabled == 0) {
        gpio_config.GPIO_Pin = 0x40;
        gpio_config.GPIO_Speed = 0x3;
        gpio_config.GPIO_Mode = 0x48;
        GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    } else if (output_enabled == 1) {
        gpio_config.GPIO_Pin = 0x40;
        gpio_config.GPIO_Speed = 0x3;
        gpio_config.GPIO_Mode = 0x10;
        GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    }
    return;
}

/* Drives SDA high (1) or low (0). */
void i2c_sda_write(int level) {

    i2c_sda_dir(1);
    if (level == 1) {
        GPIO_SetBits(GPIOB_BASE, 0x80);
    } else if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x80);
    }
    return;
}

/* Drives SCL high (1) or low (0). */
void i2c_scl_write(int level) {

    i2c_scl_dir(1);
    if (level == 1) {
        GPIO_SetBits(GPIOB_BASE, 0x40);
    } else if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x40);
    }
    return;
}

/* Clocks the acknowledge bit; returns 1 when the slave pulled SDA low (ACK), 0 for NACK. */
int i2c_wait_ack() {
    int sda_level = 0;
    int acknowledged;

    acknowledged = 0;
    i2c_sda_dir(0);
    i2c_delay();
    i2c_scl_write(1);
    i2c_delay();
    sda_level = GPIO_ReadInputDataBit(GPIOB_BASE, 0x80);
    if (sda_level == 1) {
        acknowledged = acknowledged;
    } else if (sda_level == 0) {
        acknowledged = 1;
    }
    i2c_scl_write(0);
    i2c_sda_dir(1);
    i2c_delay();
    return acknowledged;
}

/* Clocks SCL up to four times until SDA reads high, to free a slave stuck in a read. */
void i2c_bus_recover() {
    uchar pulse_count = 0;
    uchar sda_level = 0;

    i2c_sda_dir(0);
    for (pulse_count = 0; pulse_count < 4; pulse_count = pulse_count + 1) {
        i2c_scl_write(0);
        i2c_delay();
        i2c_scl_write(1);
        i2c_delay();
        sda_level = GPIO_ReadInputDataBit(GPIOB_BASE, 0x80);
        if (sda_level == 1)
            break;
    }
    i2c_sda_dir(1);
    i2c_delay();
    return;
}

/* START condition: SDA falls while SCL is high. */
void i2c_start() {

    i2c_sda_write(1);
    i2c_delay();
    i2c_scl_write(1);
    i2c_delay();
    i2c_sda_write(0);
    i2c_delay();
    i2c_scl_write(0);
    i2c_delay();
    return;
}

/* STOP condition: SDA rises while SCL is high. */
void i2c_stop() {

    i2c_scl_write(0);
    i2c_sda_write(0);
    i2c_delay();
    i2c_scl_write(1);
    i2c_delay();
    i2c_sda_write(1);
    i2c_delay();
    return;
}

/* Clocks in one bit. */
int i2c_read_bit() {
    int sda_level;
    int bit_value;

    bit_value = 0;
    sda_level = 0;
    i2c_sda_dir(0);
    i2c_scl_write(0);
    i2c_delay();
    i2c_scl_write(1);
    i2c_delay();
    sda_level = GPIO_ReadInputDataBit(GPIOB_BASE, 0x80);
    if (sda_level == 1) {
        bit_value = 1;
    } else {
        if (sda_level == 0) {
            bit_value = 0;
        }
    }
    i2c_scl_write(0);
    i2c_sda_dir(1);
    i2c_delay();
    return bit_value;
}

/* Clocks in one byte, MSB first (no acknowledge; the caller sends it). */
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

    i2c_scl_write(0);
    i2c_sda_write(bit_value);
    i2c_delay();
    i2c_scl_write(1);
    i2c_delay();
    i2c_scl_write(0);
    i2c_delay();
    i2c_sda_write(1);
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
    acknowledged = i2c_wait_ack();
    return acknowledged;
}

/* Sends an ACK (a 0 bit). */
void i2c_send_ack() {

    i2c_write_bit(0);
    return;
}

/* Dead. Writes a byte to a register of an I2C device (address byte, register, value). */
int i2c_write_reg(int device_address, int register_address, int unused, int value) {
    int acknowledged;

    acknowledged = 0;
    i2c_start();
    acknowledged = i2c_write_byte(device_address);
    acknowledged = acknowledged & i2c_write_byte(register_address);
    acknowledged = acknowledged & i2c_write_byte(value);
    i2c_stop();
    return acknowledged;
}

/* Dead. Reads a byte from a register of an I2C device. */
int i2c_read_reg(int device_address, int register_address, uchar *result) {
    int acknowledged;

    acknowledged = 0;
    *result = 0;
    __nop();
    i2c_start();
    acknowledged = i2c_write_byte(device_address);
    acknowledged = acknowledged & i2c_write_byte(register_address);
    i2c_stop();
    i2c_start();
    acknowledged = acknowledged & i2c_write_byte(device_address | 1);
    if (acknowledged == 1) {
        *result = i2c_read_byte();
    }
    i2c_stop();
    return acknowledged;
}
