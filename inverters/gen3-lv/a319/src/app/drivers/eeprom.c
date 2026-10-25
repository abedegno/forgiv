// clang-format off
/* at: eeprom_wp_set 0x0801760C, eeprom_put_be 0x0801762A, eeprom_get_be 0x0801767C, eeprom_pack_header 0x080176C6, eeprom_unpack_header 0x08017724, eeprom_pack_block 0x08017780, eeprom_unpack_block 0x080177F2, eeprom_write_page 0x0801785A, eeprom_verify_page 0x080178C4, eeprom_write_block 0x0801793C, eeprom_save_dirty 0x080179C8, eeprom_read_block 0x08017C3A, eeprom_load 0x08017DBA */
/* at: eeprom_settings_tag_seen 0x200001A9, eeprom_dirty 0x20000260, os_task_dly_idx 0x200004D8, eeprom_buf 0x20000EC4, event_log 0x20000FC4, fault_log 0x200019C3, cfg_settings 0x20001ABA, cal_gains 0x20001B8D, energy_totals 0x20001BD1, cfg_block6 0x20001C8E, cfg_block7 0x20001D88, cfg_block8 0x20001E82 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* eeprom.c: the settings store in a 24Cxx-type I2C EEPROM (device address 0xA0, 16-bit word
   address, 8-byte pages), bit-banged through i2c.c, with PB8 as the write-protect line.

   Entry points: eeprom_load (cfg_load_all at boot, eeprom_reload_flagged in task_monitor),
   eeprom_save_dirty (task_monitor), eeprom_wp_set (cfg_load_all).
   Data: eeprom_buf[256] (the image of one block on its way to or from the chip),
   eeprom_settings_tag_seen. The dirty mask eeprom_dirty is defined in board.c's data but this
   file clears its bits.
   Neighbours: drivers/i2c.c below it, board.c (cfg_load_all) and control/monitor.c (the save
   loop) above it; crc16_modbus from the Modbus slave.

   The blocks (a block number is the high byte of the EEPROM address; dirty bit; RAM copy; tag):

     addr   dirty  RAM                         bytes  tag
     0x100  0x01   cfg_settings (block 1)      0xD3   0x01  (plus a CRC of bytes 0-5 at byte 6)
     0x1E0  0x10   header copy: HR1, HR2, HR30 0x20   0x20  (eeprom_pack_header)
     0x200  0x02   cal_gains (block 2)         0x44   0x41
     0x280  0x02   fault_log 0x200019C3   0x60
     0x300  0x04   event_log (block 3)         0xFA   0x80
     0x400  0x08   energy_totals (block 4)     0xBD   0xC2
     0x500  0x20   cfg_block6 0x20001C8E   0xAA
     0x600  0x40   cfg_block7 0x20001D88   0xAC
     0x700  0x80   cfg_block8 0x20001E82   0xAF

   On the chip a block is [length][tag][data][CRC16 big-endian], length = data + 4, and every
   byte is stored inverted (0xFF - b), so an erased chip reads as zeros. Block 1 also carries a
   CRC of its first 6 bytes at bytes 6-7 (over the settings bytes +0x04/+0x05, which the code
   otherwise leaves unused), so the reader can check the length byte before reading the rest; a
   stored length different from today's marks block 1 dirty, so an older, shorter block is loaded
   and then rewritten in the new length (inference: this is how a firmware update keeps the
   settings).

   name: likely (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/eeprom.h"
#include "../../include/energy.h"
#include "../../include/faults.h"
#include "../../include/modbus.h"
#include "../../include/settings.h"
#include "../../include/stdperiph.h"
#include "../../include/tasks.h"

/* Drives PB8, the EEPROM write-protect line. eeprom_write_block sets it to 1 around each page
   write, so 1 enables writing on this board (inference: the line is inverted between the MCU and
   the chip, or the chip's WP is active low). */
void eeprom_wp_set(int enable) {

    if (enable == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x100);
    } else {
        GPIO_SetBits(GPIOB_BASE, 0x100);
    }
    return;
}

/* Stores val big-endian as n = 1, 2 or 4 bytes at buf + off. */
void eeprom_put_be(uchar *buf, int off, uint val, int n) {
    switch (n) {
    case 1:
        buf[off] = val;
        return;
    case 2:
        buf[off] = (ushort)val / 256;
        *(buf + off + 1) = (ushort)val % 256;
        return;
    case 4:
        buf[off] = val >> 24;
        *(buf + off + 1) = val >> 16;
        *(buf + off + 2) = val >> 8;
        *(buf + off + 3) = val;
        return;
    }
}

/* Reads a big-endian value of n = 1, 2 or 4 bytes at buf + off (0 for another n). */
uint eeprom_get_be(uchar *buf, int off, int n) {
    uint v;

    switch (n) {
    case 1:
        v = buf[off];
        break;
    case 2:
        v = (ushort)(*(buf + off + 1) + buf[off] * 256);
        break;
    case 4:
        v = (ushort)(*(buf + off + 1) + buf[off] * 256);
        v = *(buf + off + 3) + *(buf + off + 2) * 256 + (v << 16);
        break;
    default:
        v = 0;
        break;
    }
    return v;
}

/* Builds the 32-byte header copy in eeprom_buf: 0x20, 0x20, HR1, HR2, the Modbus address, and
   the CRC16 of bytes 0-29 at 30. It lets cfg_load_all recover the model and address when block 1
   is lost. */
void eeprom_pack_header() {
    uint crc;

    eeprom_put_be(eeprom_buf, 0, 0x20, 1);
    eeprom_put_be(eeprom_buf, 1, 0x20, 1);
    eeprom_put_be(eeprom_buf, 2, (uint)cfg_settings.inverter_module_h, 2);
    eeprom_put_be(eeprom_buf, 4, (uint)cfg_settings.inverter_module_l, 2);
    eeprom_put_be(eeprom_buf, 6, (uint)cfg_settings.modbus_address, 1);
    crc = crc16_modbus(&eeprom_buf[0], 0x1e);
    eeprom_put_be(eeprom_buf, 0x1e, crc, 2);
    return;
}

/* Checks the header copy in eeprom_buf (first byte 0x20, CRC) and, when good, restores HR1, HR2
   and the Modbus address. Returns 1 when used. */
uint eeprom_unpack_header() {
    uint unused_value1;
    uint crc;
    uint unused_value2;

    if (!(eeprom_buf[0] == ' ')) {
        return 0;
    }
    crc = crc16_modbus(eeprom_buf, 0x1e);
    if (!((eeprom_get_be(eeprom_buf, 0x1e, 2) & 0xffff) == crc)) {
        return 0;
    }
    cfg_settings.inverter_module_h = (ushort)eeprom_get_be(eeprom_buf, 2, 2);
    cfg_settings.inverter_module_l = (ushort)eeprom_get_be(eeprom_buf, 4, 2);
    cfg_settings.modbus_address = (uchar)eeprom_get_be(eeprom_buf, 6, 1);
    return 1;
}

/* Builds a block image at buf: length and tag, the len - 4 data bytes from the RAM copy, and the
   CRC16 of all but the last two bytes at the end. For tag 1 it also writes the CRC of bytes 0-5
   into bytes 6-7. */
void eeprom_pack_block(uchar *buf, uchar *data, int len, int tag) {
    uchar i;
    int unused_value;

    for (i = 0; len - 4 > i; i++) {
        *(buf + i + 2) = data[i];
    }
    eeprom_put_be(buf, 0, len, 1);
    eeprom_put_be(buf, 1, tag, 1);
    if (tag == 1) {
        eeprom_put_be(buf, 6, crc16_modbus(buf, 6), 2);
    }
    eeprom_put_be(buf, (uchar)(len - 2), crc16_modbus(buf, (ushort)(len - 2)), 2);
}

/* Checks a block image (CRC, length byte, tag) and copies its data to the RAM copy. Returns 1
   when copied. When block 1 is expected it sets eeprom_settings_tag_seen once the CRC and length
   pass, before the tag byte is checked. */
int eeprom_unpack_block(uchar *buf, uchar *data, int len, int tag) {
    uchar i;
    int crc;

    crc = crc16_modbus(buf, (ushort)(len - 2));
    if ((ushort)eeprom_get_be(buf, (uchar)(len - 2), 2) != crc) {
        return 0;
    }
    if (*buf != len) {
        return 0;
    }
    if (tag == 1) {
        eeprom_settings_tag_seen = 1;
    }
    if (buf[1] != tag) {
        return 0;
    }
    for (i = 0; len - 4 > i; i++) {
        data[i] = *(buf + i + 2);
    }
    return 1;
}

/* Writes 8 bytes to one EEPROM page (block, offset), inverted when inv is 1. Offsets that are
   not a multiple of 8 are ignored. Returns 1 when every byte was acknowledged. */
int eeprom_write_page(int block, uchar *data, int invert, int offset) {
    int ack_ok;
    int i;

    i = 0;
    ack_ok = 0;
    if ((offset & 7) == 0) {
        i2c_start();
        ack_ok = i2c_write_byte(0xa0);
        ack_ok = ack_ok & i2c_write_byte(block);
        ack_ok = ack_ok & i2c_write_byte(offset);
        for (i = 0; i < 8;) {
            if (invert == 0) {
                ack_ok = ack_ok & i2c_write_byte(*data);
            } else {
                ack_ok = ack_ok & i2c_write_byte(0xff - *data);
            }
            data++;
            i = i + 1 & 0xff;
        }
        i2c_stop();
    }
    return ack_ok;
}

/* Reads one 8-byte page back and compares it with buf (inverted when inv is 1). Returns 1 when
   equal. */
int eeprom_verify_page(int block, uchar *data, int invert, int offset) {
    uchar i;
    int match_ok;

    match_ok = 0;
    i2c_start();
    match_ok = i2c_write_byte(0xa0);
    match_ok = match_ok & i2c_write_byte(block);
    match_ok = match_ok & i2c_write_byte(offset);
    i2c_stop();
    i2c_start();
    match_ok = match_ok & i2c_write_byte(0xa1);
    for (i = 0; i < 8; i++) {
        if (invert == 0) {
            if (i2c_read_byte() != data[i]) {
                match_ok = 0;
            }
        } else {
            if (0xff - i2c_read_byte() != data[i]) {
                match_ok = 0;
            }
        }
        if (i < 7) {
            i2c_send_ack();
        }
    }
    i2c_stop();
    return match_ok;
}

/* Writes len bytes as 8-byte pages, with a 10-tick pause before each, then reads every page
   back. Returns 0 at the first page that fails; on a failed write it returns with the
   write-protect line still set to 1. */
int eeprom_write_block(int block, uchar *data, int len, int invert, int offset) {
    ushort page_off;
    int ok;

    i2c_bus_recover();
    ok = 1;
    page_off = 0;
    while (page_off < len) {
        os_task_delay((uchar)os_task_dly_idx, 0xa, 0x14);
        eeprom_wp_set(1);
        if ((eeprom_write_page(block, data + page_off, invert, (uchar)(offset + page_off)) & ok) ==
            0) {
            return 0;
        }
        eeprom_wp_set(0);
        page_off = page_off + 8;
    }
    page_off = 0;
    while (page_off < len) {
        os_task_delay((uchar)os_task_dly_idx, 0xa, 0x14);
        if ((eeprom_verify_page(block, data + page_off, invert, (uchar)(offset + page_off)) & ok) ==
            0) {
            return 0;
        }
        page_off = page_off + 8;
    }
    return (uchar)ok;
}

/* Writes every block whose bit is set in eeprom_dirty and clears the bit of each block written
   and verified. The header copy (0x10) is written after block 1. Block 2 holds two images (gains
   and fault log) and its bit is cleared only when both were written. Returns what is still dirty
   (0 when all went well). Its argument is not used. */
int eeprom_save_dirty(int unused_arg) {
    int block_len;
    int fault_len;
    ushort i;
    uchar blocks_saved;

    blocks_saved = 0;
    for (i = 0; i <= 0xff; i++) {
        eeprom_buf[i] = 0;
    }
    i2c_bus_recover();
    if ((eeprom_dirty & 1) != 0) {
        block_len = 0xd3;
        if (block_len + 0x24 > 0xfe) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], (uchar *)&cfg_settings, (uchar)(block_len + 4), 1);
        if (eeprom_write_block(1, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xfe;
        }
    }
    if ((eeprom_dirty & 0x10) != 0) {
        eeprom_pack_header();
        if (eeprom_write_block(1, &eeprom_buf[0], 0x20, 1, 0xe0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xef;
        }
    }
    if ((eeprom_dirty & 2) != 0) {
        block_len = 0x44;
        fault_len = 0x2d;
        eeprom_pack_block(&eeprom_buf[0], (uchar *)&cal_gains, (uchar)(block_len + 4), 0x41);
        if (eeprom_write_block(2, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            blocks_saved++;
        }
        if (block_len + 4 + fault_len + 4 > 0xfe || block_len > 0x80 || fault_len > 0x80) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], &fault_log[0], (uchar)(fault_len + 4), 0x60);
        if (eeprom_write_block(2, &eeprom_buf[0], (uchar)(fault_len + 4), 1, 0x80) != 0) {
            blocks_saved++;
        }
        if (blocks_saved == 2) {
            eeprom_dirty = eeprom_dirty & 0xfd;
        }
    }
    if ((eeprom_dirty & 4) != 0) {
        eeprom_pack_block(&eeprom_buf[0], &event_log[0], 0xfe, 0x80);
        if (eeprom_write_block(3, &eeprom_buf[0], 0xfe, 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xfb;
        }
    }
    if ((eeprom_dirty & 8) != 0) {
        block_len = 0xbd;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], (uchar *)&energy_totals, (uchar)(block_len + 4), 0xc2);
        if (eeprom_write_block(4, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xf7;
        }
    }
    if ((eeprom_dirty & 0x20) != 0) {
        block_len = 0xfa;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], &cfg_block6.hr322, (uchar)(block_len + 4), 0xaa);
        if (eeprom_write_block(5, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xdf;
        }
    }
    if ((eeprom_dirty & 0x40) != 0) {
        block_len = 0xfa;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], &cfg_block7[0], (uchar)(block_len + 4), 0xac);
        if (eeprom_write_block(6, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0xbf;
        }
    }
    if ((eeprom_dirty & 0x80) != 0) {
        block_len = 0xf0;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        eeprom_pack_block(&eeprom_buf[0], &cfg_block8[0], (uchar)(block_len + 4), 0xaf);
        if (eeprom_write_block(7, &eeprom_buf[0], (uchar)(block_len + 4), 1, 0) != 0) {
            eeprom_dirty = eeprom_dirty & 0x7f;
        }
    }
    return eeprom_dirty;
}

/* Reads one block image from the chip into buf and unpacks it into the RAM copy. For the
   settings (tag 1) it reads the first 8 bytes, checks their CRC, takes the stored length (and
   marks block 1 dirty when it differs from the expected one) and reads the rest; other blocks
   are read at their fixed length, and tag 0x20 is the header copy. Returns 1 when the block was
   taken. Every byte is acknowledged, the last one too (the usual I2C read ends with a NACK). */
int eeprom_read_block(int block, uchar *buf, uchar *data, int len, int invert, int offset,
                      int tag) {
    uchar i;
    int ack_ok;
    uchar stored_len;
    int crc;

    i2c_bus_recover();
    for (i = 0; i < len; i++) {
        buf[i] = 0;
    }
    *(buf + len - 2) = 0xef;
    *(buf + len - 1) = 0xfe;
    ack_ok = 0;
    i2c_start();
    ack_ok = i2c_write_byte(0xa0);
    ack_ok = ack_ok & i2c_write_byte(block);
    ack_ok = ack_ok & i2c_write_byte(offset);
    i2c_stop();
    i2c_start();
    ack_ok = ack_ok & i2c_write_byte(0xa1);
    if (ack_ok == 0) {
        return ack_ok;
    }
    if (tag == 1) {
        for (i = 0; i < 8; i++) {
            if (invert == 0) {
                buf[i] = i2c_read_byte();
            } else {
                buf[i] = 0xff - i2c_read_byte();
            }
            i2c_send_ack();
        }
        crc = crc16_modbus(buf, 6);
        if ((ushort)eeprom_get_be(buf, 6, 2) != crc) {
            return 0;
        }
        stored_len = *buf;
        if (stored_len != len) {
            eeprom_dirty = eeprom_dirty | 1;
        }
        if (stored_len < 0xff) {
            for (i = 8; i < stored_len; i++) {
                if (invert == 0) {
                    buf[i] = i2c_read_byte();
                } else {
                    buf[i] = 0xff - i2c_read_byte();
                }
                if (i < stored_len) {
                    i2c_send_ack();
                }
            }
        } else {
            return 0;
        }
        i2c_stop();
        if (ack_ok == 1) {
            if (stored_len > len) {
                return 0;
            }
            ack_ok = eeprom_unpack_block(buf, data, stored_len & 0xff, tag);
        }
    } else {
        for (i = 0; i < len; i++) {
            if (invert == 0) {
                buf[i] = i2c_read_byte();
            } else {
                buf[i] = 0xff - i2c_read_byte();
            }
            if (i < len) {
                i2c_send_ack();
            }
        }
        i2c_stop();
        if (ack_ok == 1) {
            if (tag == 0x20) {
                ack_ok = eeprom_unpack_header();
            } else {
                ack_ok = eeprom_unpack_block(buf, data, len, tag);
            }
        }
    }
    return ack_ok;
}

/* Loads block n (1-8; 5 is the header copy) from the EEPROM into RAM with up to tries attempts.
   Blocks 6, 7 and 8 live at EEPROM pages 5, 6 and 7. Returns 1 when loaded, 0 when not; block 2
   returns a mask, 1 for the gains and 2 for the fault log. For n == 5 the retry counter attempt is
   used without being set to 0 first, so the number of tries depends on what was on the stack
   (docs/findings.md). */
int eeprom_load(int block, int tries) {
    int block_len;
    uchar attempt;
    int fault_len;
    int loaded_mask;
    int i;

    loaded_mask = 0;
    i2c_bus_recover();
    for (i = 0; i <= 0xff; i = (ushort)(i + 1)) {
        eeprom_buf[i] = 0;
    }
    if (block == 1) {
        block_len = 0xd3;
        if (block_len + 0x24 > 0xfe) {
            return 0;
        }
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(block, &eeprom_buf[0], (uchar *)&cfg_settings,
                                  (uchar)(block_len + 4), 1, 0, 1) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 5) {
        while (attempt++ < tries) {
            if (eeprom_read_block(1, &eeprom_buf[0], &eeprom_buf[0], 0x20, 1, 0xe0, 0x20) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 2) {
        attempt = 0;
        block_len = 0x44;
        fault_len = 0x2d;
        if (block_len + 4 + fault_len + 4 > 0xfe || block_len > 0x80 || fault_len > 0x80) {
            return 0;
        }
        while (attempt++ < tries) {
            if (eeprom_read_block(block, &eeprom_buf[0], (uchar *)&cal_gains,
                                  (uchar)(block_len + 4), 1, 0, 0x41) == 1) {
                loaded_mask = loaded_mask | 1;
            }
            if (eeprom_read_block(block, &eeprom_buf[0], &fault_log[0], (uchar)(fault_len + 4), 1,
                                  0x80, 0x60) == 1) {
                loaded_mask = loaded_mask | 2;
            }
            if (loaded_mask == 3) {
                return loaded_mask;
            }
        }
        return loaded_mask;
    }
    if (block == 3) {
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(block, &eeprom_buf[0], &event_log[0], 0xfe, 1, 0, 0x80) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 4) {
        block_len = 0xbd;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(block, &eeprom_buf[0], (uchar *)&energy_totals,
                                  (uchar)(block_len + 4), 1, 0, 0xc2) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 6) {
        block_len = 0xfa;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(5, &eeprom_buf[0], &cfg_block6.hr322, (uchar)(block_len + 4), 1,
                                  0, 0xaa) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 7) {
        block_len = 0xfa;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(6, &eeprom_buf[0], &cfg_block7[0], (uchar)(block_len + 4), 1, 0,
                                  0xac) == 1) {
                return 1;
            }
        }
        return 0;
    }
    if (block == 8) {
        block_len = 0xf0;
        if (block_len + 4 > 0xfe) {
            return 0;
        }
        attempt = 0;
        while (attempt++ < tries) {
            if (eeprom_read_block(7, &eeprom_buf[0], &cfg_block8[0], (uchar)(block_len + 4), 1, 0,
                                  0xaf) == 1) {
                return 1;
            }
        }
        return 0;
    }
    return 0;
}
