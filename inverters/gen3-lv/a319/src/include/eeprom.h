/* eeprom.h: the I2C EEPROM settings store (drivers/eeprom.c) and the bit-banged I2C under it
   (drivers/i2c.c). The block table is in the header of drivers/eeprom.c.
   name: descriptive (readability step 1). */
#ifndef EEPROM_H
#define EEPROM_H

#include "firmware_types.h"

/* data */
extern uchar eeprom_buf[];
extern uchar eeprom_dirty; /* blocks to save: 1 settings, 2 gains+fault log, 4 events, 8 energy,
                               0x10 header, 0x20/0x40/0x80 blocks 6/7/8 */
extern uchar eeprom_load_failed; /* a block failed at boot: task_monitor retries before saving
                                     defaults */
extern uchar eeprom_load_ok;     /* 1 loaded, 2 tasks started (task_monitor) */
extern uchar eeprom_settings_tag_seen;

/* functions */
uint eeprom_get_be(uchar *buf, int off, int n);
int eeprom_load(int block, int tries);
void eeprom_pack_block(uchar *buf, uchar *data, int len, int tag);
void eeprom_pack_header();
void eeprom_put_be(uchar *buf, int off, uint val, int n);
int eeprom_read_block(int block, uchar *buf, uchar *data, int len, int invert, int offset, int tag);
void eeprom_reload_flagged();
int eeprom_save_dirty(int unused_arg);
int eeprom_unpack_block(uchar *buf, uchar *data, int len, int tag);
uint eeprom_unpack_header();
int eeprom_verify_page(int block, uchar *data, int invert, int offset);
void eeprom_wp_set(int enable);
int eeprom_write_block(int block, uchar *data, int len, int invert, int offset);
int eeprom_write_page(int block, uchar *data, int invert, int offset);
void i2c_bus_recover();
void i2c_delay();
void i2c_gpio_init();
int i2c_read_bit();
uchar i2c_read_byte();
int i2c_read_reg(int device_address, int register_address, uchar *result);
void i2c_scl_dir(int output_enabled);
void i2c_scl_write(int level);
void i2c_sda_dir(int output_enabled);
void i2c_sda_write(int level);
void i2c_send_ack();
void i2c_start();
void i2c_stop();
int i2c_wait_ack();
void i2c_write_bit(int bit_value);
uint i2c_write_byte(uint value);
int i2c_write_reg(int device_address, int register_address, int unused, int value);

#endif
