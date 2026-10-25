// clang-format off
/* at: USBH_USR_Init 0x0802A2D8, USBH_USR_DeviceAttached 0x0802A2E6, USBH_USR_UnrecoveredError 0x0802A2E8, USBH_USR_DeviceDisconnected 0x0802A2EA, USBH_USR_ResetDevice 0x0802A31A, USBH_USR_DeviceSpeedDetected 0x0802A31C, USBH_USR_Device_DescAvailable 0x0802A326, USBH_USR_DeviceAddressAssigned 0x0802A328, USBH_USR_Configuration_DescAvailable 0x0802A32A, USBH_USR_Manufacturer_String 0x0802A356, USBH_USR_Product_String 0x0802A358, USBH_USR_SerialNum_String 0x0802A35A, USBH_USR_EnumerationDone 0x0802A35C, USBH_USR_DeviceNotSupported 0x0802A364, USBH_USR_UserInput 0x0802A366, USBH_USR_OverCurrentDetected 0x0802A36A, usb_disk_getfree 0x0802A36C, usb_check_update_files 0x0802A3B6, USBH_USR_MSC_Application 0x0802A54C, USBH_USR_DeInit 0x0802A628, usb_read_msn_file 0x0802A630, dsp_fw_update_process 0x0802A652, arm_fw_update_arm_bootflag 0x0802AB6C, bms_fw_update_process 0x0802AC12 */
/* at: bms_update_crc32 0x200000F0, cfg_changed_flags 0x2000019C, save_req_pending 0x200001A8, fault_led_on 0x200001B3, dsp_fw_version 0x20000244, eeprom_dirty 0x20000260, usb_device_inserted 0x200002EE, arm_update_state 0x20000307, dsp_flash_step 0x20000308, dsp_flash_reply 0x20000309, usb_class_code 0x2000030A, usb_storage_initialized 0x2000030D, usb_cdc_configured 0x2000030E, usb_sn_written 0x20000312, bms_chip_version 0x2000039A, dsp_flash_ok 0x200003A6, bms_update_last 0x200003A7, bms_update_pending 0x200003A8, bms_update_failed 0x200003AA, USBH_USR_ApplicationState 0x200003DC, usb_usr_init_done 0x20000430, dsp_update_timeout 0x20000432, bms_update_state 0x20000448, bms_update_cur 0x20000450, bms_update_end 0x20000454, bms_update_block 0x2000045C, bms_update_end_count 0x20000460, bms_update_timeout 0x20000462, os_task_dly_idx 0x200004D8, usb_line_stuck 0x2000050C, UserCb 0x2000055C, dsp_tx_frame 0x20001346, uart_rx_buf 0x2000175C, cfg_serial_and_settings 0x20001ACD, cal_gains 0x20001B8D, usb_fatfs 0x20002388, fil_tmp 0x200025B8, fil_msn 0x200027E0, usb_file_buf 0x20002A08, fil_arm_image 0x20002E08, fil_update 0x20003030, USB_OTG_Core 0x20003480, USB_Host 0x20003A30, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* usbh_usr.c: ST's USB host user callbacks (USR_cb, in slot order), and GivEnergy's USB stick
   application around them: firmware updates of the DSP, the ARM and the battery from files on a
   stick, and setting the serial number.

   Entry points: the USBH_USR_* callbacks through USR_cb (USBH_USR_MSC_Application runs the stick
   jobs); dsp_fw_update_process, arm_fw_update_arm_bootflag and bms_fw_update_process from
   task_dsp_rx; usb_read_msn_file. Dead: usb_disk_getfree.
   Data: the FatFs objects fil_update, fil_arm_image, fil_msn, fil_tmp, usb_fatfs, the file
   buffer usb_file_buf; the update state (dsp_flash_step, dsp_update_*, arm_update_state,
   bms_update_*).
   Neighbours: usb_bsp.c before, rtc.c after; src/lib/fatfs (f_open, f_read, f_unlink),
   comm/dsp_link.c (the updates send on UART4 and reuse dsp_tx_frame), the host library.

   Files on the stick:
     0:DSPStore.bin  DSP image: taken when its version (first word, big-endian) differs from the
                     DSP's and has the same hundreds (or the DSP reports 0)
     0:ARMStore.bin  header "version (little-endian), 0x5566". Version above 1000: a battery
                     image, relayed to the BMS through the DSP when HR46 (BMS chip version) is
                     non-zero and no relay has failed; version below 1000: an ARM image, when it
                     differs from the word at 0x08013FFC and has the same hundreds
     0:SN.txt        10 characters: becomes the inverter serial number
     0:MSN.txt       factory mode (inference): its presence deletes the image files after use

   name: original. ST's example usbh_usr.c with the application added (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/battery.h"
#include "../../include/board.h"
#include "../../include/clib.h"
#include "../../include/dsp_link.h"
#include "../../include/eeprom.h"
#include "../../include/faults.h"
#include "../../include/ff.h"
#include "../../include/schedule.h"
#include "../../include/settings.h"
#include "../../include/stdperiph.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"
#include "../../include/usb.h"
#include "../../include/modbus.h"

/* Host user init: marks that it ran once. */
void USBH_USR_Init() {

    if (usb_usr_init_done == '\0') {
        usb_usr_init_done = '\x01';
    }
    return;
}

/* Does nothing. */
void USBH_USR_DeviceAttached() { return; }

/* Does nothing. */
void USBH_USR_UnrecoveredError() { return; }

/* A device was unplugged: abandons a DSP or ARM update in progress, sets the class back to CDC
   (2) and clears the "serial number written" indication. */
void USBH_USR_DeviceDisconnected() {

    arm_update_state = 0;
    dsp_flash_step = 0;
    usb_storage_initialized = 0;
    usb_cdc_configured = 0;
    usb_device_inserted = 0;
    usb_class_code = 2;
    if (usb_sn_written == '\x01') {
        usb_sn_written = '\0';
        fault_led_on = 0;
    }
    return;
}

/* Does nothing. */
void USBH_USR_ResetDevice() { return; }

/* Does nothing (a NOP for speeds other than 0 and 1). */
void USBH_USR_DeviceSpeedDetected(int speed) {
    if (speed != 0 && speed != 1) {
        __nop();
    }
}

/* Does nothing. */
void USBH_USR_Device_DescAvailable() { return; }

/* Does nothing. */
void USBH_USR_DeviceAddressAssigned() { return; }

/* Takes the class of the device's interface (byte 5 of the interface descriptor): 8 mass
   storage, 2 CDC (then starts CDC reception). */
void USBH_USR_Configuration_DescAvailable(int unused_host, int config_desc, int unused_len) {
    volatile int unused_value;
    uchar *p;

    unused_value = 0;
    p = (uchar *)config_desc;
    if (p[5] == 8) {
        usb_class_code = 8;
    } else if (p[5] == 2) {
        usb_class_code = 2;
        CDC_StartReception(&USB_OTG_Core[0]);
    }
}

/* Does nothing. */
void USBH_USR_Manufacturer_String() { return; }

/* Does nothing. */
void USBH_USR_Product_String() { return; }

/* Does nothing. */
void USBH_USR_SerialNum_String() { return; }

/* Enumeration done: points UserCb.Receive (the CDC receive callback) at modbus_slave_process
   (0x08022C51), so dongle frames go to the Modbus slave. */
void USBH_USR_EnumerationDone() {

    *(uint *)&UserCb[4] = (uint)modbus_slave_process;
    return;
}

/* Does nothing. */
void USBH_USR_DeviceNotSupported() { return; }

/* Always "go on" (1). */
uint USBH_USR_UserInput() { return 1; }

/* Does nothing. */
void USBH_USR_OverCurrentDetected() { return; }

/* Dead. Total and free space of a FatFs volume in KB (512-byte sectors). */
int usb_disk_getfree(int path, int *free_kb, int *total_kb) {
    int result;
    int free_sectors;
    int total_sectors;
    FATFS *fs_info;
    int free_clusters;

    free_clusters = 0;
    free_sectors = 0;
    total_sectors = 0;
    result = f_getfree(path, &free_clusters, &fs_info);
    if (result == 0) {
        total_sectors = (fs_info->n_fatent - 2) * fs_info->csize;
        free_sectors = fs_info->csize * free_clusters;
        *free_kb = (uint)total_sectors >> 1;
        *total_kb = (uint)free_sectors >> 1;
    }
    return result;
}

#define c_08013FFC ((uchar *)0x08013FFC)

/* Looks at the update files on a newly mounted stick (rules in the file header) and starts at
   most one DSP update (dsp_flash_step = 1), one ARM update (arm_update_state = 1) or one battery
   relay (bms_update_pending, which the DSP acknowledges on page 4 byte 16). When an ARMStore.bin
   is refused, bms_update_failed is cleared, so the next stick can try again. With MSN.txt and
   nothing started, it deletes both image files. */
void usb_check_update_files() {
    int read_result;
    ushort expected_version;
    int version_data;
    int update_found;
    int bytes_read;

    bytes_read = 0;
    update_found = 0;
    if (dsp_flash_step == 0) {
        if (f_open(&fil_update, "0:DSPStore.bin", 1) == 0) {
            read_result = f_read(&fil_update, &usb_file_buf[0], 4, &bytes_read);
            if (read_result == 0) {
                if (usb_file_buf[1] + usb_file_buf[0] * 0x100 != dsp_fw_version &&
                    (dsp_fw_version / 0x64 == (usb_file_buf[1] + usb_file_buf[0] * 0x100) / 0x64 ||
                     dsp_fw_version == 0)) {
                    f_close(&fil_update);
                    dsp_flash_step = 1;
                    update_found = 1;
                } else {
                    f_close(&fil_update);
                }
            } else {
                f_close(&fil_update);
            }
        }
    }
    if (arm_update_state == 0 && f_open(&fil_arm_image, "0:ARMStore.bin", 1) == 0) {
        version_data = &(*(uchar *)(c_08013FFC + 0));
        expected_version = *(int *)version_data;
        read_result = f_read(&fil_arm_image, &usb_file_buf[0], 4, &bytes_read);
        if (read_result == 0 && (ushort)(usb_file_buf[2] + usb_file_buf[3] * 0x100) == 0x5566) {
            if (usb_file_buf[0] + usb_file_buf[1] * 0x100 > 0x3e8 && bms_update_failed == 0 &&
                bms_chip_version != 0) {
                f_close(&fil_arm_image);
                bms_update_pending = 1;
                update_found = 1;
            } else {
                if (usb_file_buf[0] + usb_file_buf[1] * 0x100 != expected_version &&
                    usb_file_buf[0] + usb_file_buf[1] * 0x100 < 0x3e8 &&
                    expected_version / 0x64 == (usb_file_buf[0] + usb_file_buf[1] * 0x100) / 0x64 &&
                    bms_update_failed == 0) {
                    f_close(&fil_arm_image);
                    arm_update_state = 1;
                    update_found = 1;
                } else {
                    f_close(&fil_arm_image);
                    bms_update_failed = 0;
                }
            }
        } else {
            f_close(&fil_arm_image);
        }
    }
    if (f_open(&fil_msn, "0:MSN.txt", 1) == 0) {
        f_close(&fil_msn);
        if (update_found == 0) {
            if (f_open(&fil_tmp, "0:DSPStore.bin", 1) == 0) {
                f_close(&fil_tmp);
                f_unlink("0:DSPStore.bin");
            }
            if (f_open(&fil_tmp, "0:ARMStore.bin", 1) == 0) {
                f_close(&fil_tmp);
                f_unlink("0:ARMStore.bin");
            }
        }
    }
}
#undef c_08013FFC

/* The class application, called by the host state machine. For CDC (the dongle): asks for the
   line coding once, and again when the host core reports the line stuck. For a stick: mounts it,
   checks the update files and, when no update starts, reads SN.txt: when its 10 characters
   differ from the serial, they are copied in with strcpy, saved, and the fault LED lit to show
   it. f_read does not add a terminator and strcpy writes one more byte than the 10: the zero (or
   whatever follows in usb_file_buf) lands on settings +0x1D, the low byte of HR138
   (docs/findings.md). */
int USBH_USR_MSC_Application() {
    int read_result;
    int bytes_read;

    bytes_read = 0;
    switch (USBH_USR_ApplicationState) {
    case 0:
        if (usb_class_code == 2) {
            usb_device_inserted = 2;
            if (usb_cdc_configured == 0) {
                CDC_ChangeStateToIssueSetConfig(&USB_OTG_Core[0], &USB_Host[0]);
            }
            if (usb_line_stuck != 0) {
                usb_line_stuck = 0;
                CDC_ChangeStateToIssueSetConfig(&USB_OTG_Core[0], &USB_Host[0]);
            }
            usb_cdc_configured = 1;
        } else if (usb_class_code == 8) {
            usb_device_inserted = 8;
            if (usb_storage_initialized == 0) {
                if (f_mount(&usb_fatfs[0], "0:", 0) != 0) {
                    return -1;
                }
                usb_check_update_files();
                if (dsp_flash_step == 0 && arm_update_state == 0) {
                    if (f_open(&fil_tmp, "0:SN.txt", 1) == 0) {
                        read_result = f_read(&fil_tmp, &usb_file_buf[0], 0xa, &bytes_read);
                        if (read_result == 0 &&
                            strcmp(&usb_file_buf[0], &cfg_serial_and_settings[0]) != 0) {
                            usb_sn_written = 1;
                            strcpy(&cfg_serial_and_settings[0], &usb_file_buf[0]);
                            eeprom_dirty = eeprom_dirty | 1;
                            save_req_pending = 1;
                            fault_led_on = 1;
                        }
                        f_close(&fil_tmp);
                    }
                    __nop();
                }
                usb_storage_initialized = 1;
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/* Resets the application state. */
void USBH_USR_DeInit() {

    USBH_USR_ApplicationState = 0;
    return;
}

/* Opens and reads MSN.txt (10 bytes) and closes it. Called during long waits in the updates;
   inference: it keeps the stick and the file system awake. */
void usb_read_msn_file() {
    int bytes_read;

    if (f_open(&fil_msn, "0:MSN.txt", 1) == 0) {
        f_read(&fil_msn, &usb_file_buf[0], 0xa, &bytes_read);
    }
    f_close(&fil_msn);
}

/* The DSP firmware update over UART4, one step per call from task_dsp_rx (every 10 ticks). The
   frames start 0x0A and end with a 16-bit byte sum; the DSP answers 0x0A ... 0x4F ("O"):

     step 1      wait 4 x 1000 ticks, stop task 2 (task_dsp_tx)
     step 2      "W" (0x0A 0x00 0x57): enter the loader          -> wait for the answer (0xA2)
     step 3      open DSPStore.bin, "S" with the start address   -> 0xA3
     step 4      "D" with a block number and 128 bytes           -> 0xA4, then step 4 again
                 until the file is sent (step 5)
     step 0/5    close, show the result on the LEDs, "E" (0x0A 0x00 0x45): end

   No answer for 2000 calls restarts from step 2. Inference: the DSP's boot loader protocol. */
void dsp_fw_update_process() {
    uchar j = 0;
    uchar i = 0;
    uint sum = 0;
    int res;
    uint br;

    if (dsp_flash_reply == 1) {
        dsp_update_timeout = 0;
        dsp_flash_reply = 0;
        switch (dsp_flash_step) {
        case 0xa2:
            if (uart_rx_buf[3][0] == 0xa && uart_rx_buf[3][2] == 0x4f) {
                dsp_flash_step = 3;
                uart_rx_buf[3][0] = 0;
                uart_rx_buf[3][2] = 0;
                dsp_update_timeout = 0;
            }
            break;
        case 0xa3:
            if (uart_rx_buf[3][0] == 0xa && uart_rx_buf[3][2] == 0x4f) {
                dsp_flash_step = 4;
                uart_rx_buf[3][0] = 0;
                uart_rx_buf[3][2] = 0;
            }
            break;
        case 0xa4:
            if (uart_rx_buf[3][0] == 0xa && uart_rx_buf[3][2] == 0x4f) {
                dsp_flash_step = 4;
                if (dsp_update_block == 0) {
                    dsp_flash_step = 5;
                }
                uart_rx_buf[3][0] = 0;
                uart_rx_buf[3][2] = 0;
            }
            break;
        }
        uart_rx_reset(3, 0x2d);
    } else if (dsp_flash_step != 0 && dsp_flash_step != 5) {
        dsp_update_timeout = dsp_update_timeout + 1;
        if (dsp_update_timeout > 2000) {
            f_close(&fil_update);
            dsp_update_timeout = 0;
            dsp_flash_step = 2;
            dsp_update_cur = 0;
            dsp_update_end = 0;
            dsp_update_chunk = 0x80;
            dsp_update_block = 0;
        }
        if (dsp_update_timeout % 140 == 0) {
            usb_read_msn_file();
        }
        if (dsp_update_timeout % 200 == 0) {
            led_set(1, 0);
            led_set(2, 1);
        } else if (dsp_update_timeout % 100 == 0) {
            led_set(1, 1);
            led_set(2, 0);
        }
    }
    switch (dsp_flash_step) {
    case 1:
        for (j = 0; j < 4; j++) {
            os_task_delay((uchar)os_task_dly_idx, 1000, 60);
            usb_read_msn_file();
        }
        task_tbl[TASK_DSP_TX].period = 0;
        dsp_flash_step = 2;
        os_task_delay((uchar)os_task_dly_idx, 1000, 60);
        usb_read_msn_file();
        break;
    case 2:
        dsp_tx_frame[0] = 0xa;
        dsp_tx_frame[1] = 0;
        dsp_tx_frame[2] = 0x57;
        for (i = 0; i < 3; i++) {
            sum = sum + dsp_tx_frame[i];
        }
        dsp_tx_frame[3] = sum >> 8;
        dsp_tx_frame[4] = sum;
        uart_tx_start(3, dsp_tx_frame, 5);
        dsp_flash_step = 0xa2;
        uart_rx_reset(3, 0x2d);
        break;
    case 3:
        res = f_open(&fil_update, "0:DSPStore.bin", 1);
        if (res == 0) {
            dsp_tx_frame[0] = 0xa;
            dsp_tx_frame[1] = 4;
            dsp_tx_frame[2] = 0x53;
            dsp_update_cur = dsp_update_base;
            dsp_update_end = dsp_update_base + (fil_update.obj.objsize + 1) / 2;
            dsp_update_block = 0;
            dsp_tx_frame[3] = dsp_update_base >> 24;
            dsp_tx_frame[4] = dsp_update_base >> 16;
            dsp_tx_frame[5] = dsp_update_base >> 8;
            dsp_tx_frame[6] = dsp_update_base;
            for (i = 0; i < 7; i++) {
                sum = sum + dsp_tx_frame[i];
            }
            dsp_tx_frame[7] = sum >> 8;
            dsp_tx_frame[8] = sum;
            uart_tx_start(3, dsp_tx_frame, 9);
            uart_rx_reset(3, 0x2d);
            dsp_flash_step = 0xa3;
        } else {
            return;
        }
        break;
    case 4:
        for (i = 0; i < 0x80; i++) {
            usb_file_buf[i] = 0xff;
        }
        dsp_tx_frame[0] = 0xa;
        dsp_tx_frame[1] = 0x82;
        dsp_tx_frame[2] = 0x44;
        dsp_tx_frame[3] = dsp_update_block >> 8;
        dsp_tx_frame[4] = dsp_update_block;
        dsp_update_block = dsp_update_block + 1;
        if ((dsp_update_block & 1) == 0) {
            led_set(1, 0);
            led_set(2, 1);
        } else if ((dsp_update_block & 1) != 0) {
            led_set(1, 1);
            led_set(2, 0);
        }
        if (dsp_update_cur + 0x40 >= dsp_update_end) {
            dsp_update_chunk = (dsp_update_end - dsp_update_cur) * 2;
            dsp_update_block = 0;
        } else {
            dsp_update_chunk = 0x80;
        }
        dsp_update_cur = dsp_update_cur + 0x40;
        if (dsp_update_chunk <= 2) {
            dsp_update_chunk = 2;
        }
        res = f_read(&fil_update, usb_file_buf, dsp_update_chunk, &br);
        if (res != 0) {
            f_close(&fil_update);
            dsp_update_timeout = 0;
            dsp_flash_step = 2;
            dsp_update_cur = 0;
            dsp_update_end = 0;
            dsp_update_chunk = 0x80;
            dsp_update_block = 0;
            return;
        }
        for (i = 0; i < 0x80; i++) {
            dsp_tx_frame[i + 5] = usb_file_buf[i];
        }
        for (i = 0; i < 0x85; i++) {
            sum = sum + dsp_tx_frame[i];
        }
        dsp_tx_frame[0x85] = sum >> 8;
        dsp_tx_frame[0x86] = sum;
        uart_tx_start(3, dsp_tx_frame, 0x87);
        uart_rx_reset(3, 0x2d);
        dsp_flash_step = 0xa4;
        break;
    case 0:

    case 5:
        f_close(&fil_update);
        if (arm_update_state == 0) {
            led_set(1, 0);
            led_set(2, 1);
            fault_led_on = 1;
            led_fault_set(1);
        }
        os_task_delay((uchar)os_task_dly_idx, 300, 60);
        if (dsp_flash_step == 0 || arm_update_state == 2) {
            dsp_tx_frame[0] = 0xa;
            dsp_tx_frame[1] = 0;
            dsp_tx_frame[2] = 0x45;
            for (i = 0; i < 3; i++) {
                sum = sum + dsp_tx_frame[i];
            }
            dsp_tx_frame[3] = sum >> 8;
            dsp_tx_frame[4] = sum;
            uart_tx_start(3, dsp_tx_frame, 5);
            uart_rx_reset(3, 0x2d);
        }
        break;
    }
}

#define c_08013FFC ((uchar *)0x08013FFC)

/* The ARM self-update: after a 2000-tick pause it writes 0 over the version word at 0x08013FFC
   (just below the application, in the bootloader's area, inference) and, when that worked, sets
   arm_update_state = 2. On the next start the bootloader sees version 0 and takes the image from
   the stick (inference). In state 2 it also sets cfg_changed_flags bit 0. */
void arm_fw_update_arm_bootflag() {
    volatile int unused_value2;
    volatile int unused_value1;

    unused_value2 = 0;
    unused_value1 = 0;
    if (arm_update_state == 2) {
        cfg_changed_flags = cfg_changed_flags | 1;
    }
    if (arm_update_state != 1) {
        return;
    }
    os_task_delay((uchar)os_task_dly_idx, 0x7d0, 0x3c);
    usb_read_msn_file();
    OS_ENTER_CRITICAL();
    FLASH_Unlock();
    FLASH_ClearFlag(0x35);
    FLASH_ProgramWord(&(*(uchar *)(c_08013FFC + 0)), 0);
    OS_EXIT_CRITICAL();
    if (*(int *)(c_08013FFC + 0) != 0) {
        return;
    }
    arm_update_state = 2;
    return;
}
#undef c_08013FFC

/* The battery firmware relay: streams ARMStore.bin (a battery image) to the DSP on UART4, which
   passes it to the BMS (inference). One step per call from task_dsp_rx (every 100 ticks):

     state 0     open the file, start the CRC-32
     state 1     read 1 KB, send "BMS" 1 "HED" + 1024 bytes + 16-bit byte sum + "E" (1034
                 bytes); a block of only 0xFF bytes is not sent; then wait for "OK" (0xA1)
     0xA1        on "OK" the next block or the end; no answer for 300 calls: failed
     0xA2        "BMS" 1 "END" + the inverted CRC-32, sent at the 3rd and 7th call
     state 2     a read error: failed after 300 calls

   A failure sets the stored "update failed" word (block 2 +0x34). Three faults are in this code
   (docs/findings.md): the CRC-32 table is never built, so the CRC sent is a constant; the second
   END frame inverts the CRC again; and a skipped 0xFF block gets no answer, which ends in a
   timeout. */
void bms_fw_update_process() {
    volatile int l = 0;
    ushort i = 0;
    int open_result;
    ushort checksum = 0;
    uint br;

    switch (bms_update_state) {
    case 0:
        open_result = f_open(&fil_update, "0:ARMStore.bin", 1);
        if (open_result == 0) {
            bms_update_cur = bms_update_base;
            bms_update_end = bms_update_base + (fil_update.obj.objsize + 1) / 2;
            bms_update_block = 0;
            bms_update_state = 1;
            crc32_init(&bms_update_crc32);
        }
        break;
    case 1:
        for (i = 0; i < 0x400; i++) {
            usb_file_buf[i] = 0xff;
        }
        dsp_tx_frame[0] = 0x42;
        dsp_tx_frame[1] = 0x4d;
        dsp_tx_frame[2] = 0x53;
        dsp_tx_frame[3] = 1;
        dsp_tx_frame[4] = 0x48;
        dsp_tx_frame[5] = 0x45;
        dsp_tx_frame[6] = 0x44;
        if ((uint)(bms_update_cur + 0x200) >= bms_update_end) {
            bms_update_chunk = bms_update_end - bms_update_cur << 1;
            bms_update_block = 0;
            bms_update_last = 1;
        } else {
            bms_update_chunk = 0x400;
            bms_update_last = 0;
        }
        bms_update_cur = bms_update_cur + 0x200;
        if ((uint)bms_update_chunk <= 2) {
            bms_update_chunk = 2;
        }
        open_result = f_read(&fil_update, &usb_file_buf[0], bms_update_chunk, &br);
        if (open_result != 0) {
            f_close(&fil_update);
            bms_update_cur = 0;
            bms_update_end = 0;
            bms_update_chunk = 0x400;
            bms_update_block = 0;
            bms_update_state = 2;
            bms_update_timeout = 0;
            return;
        }
        crc32_update(&bms_update_crc32, &usb_file_buf[0], (ushort)bms_update_chunk);
        for (i = 0; i < 0x400; i++) {
            dsp_tx_frame[i + 7] = usb_file_buf[i];
        }
        for (i = 7; i < 0x407; i++) {
            checksum = dsp_tx_frame[i] + checksum;
        }
        dsp_tx_frame[1031] = checksum >> 8;
        dsp_tx_frame[1032] = checksum;
        dsp_tx_frame[1033] = 0x45;
        for (i = 0; i < 0x400; i++) {
            if (usb_file_buf[i] != 0xff) {
                uart_tx_start(3, &dsp_tx_frame[0], 0x40a);
                uart_rx_reset(3, 0x2d);
                break;
            }
        }
        bms_update_state = 0xa1;
        bms_update_timeout = 0;
        break;
    case 2:
        if (bms_update_timeout <= 0x12c) {
            bms_update_timeout = bms_update_timeout + 1;
        } else {
            bms_update_state = 0xa2;
            bms_update_end_count = 0;
            bms_update_timeout = 0x190;
            cal_gains.bms_update_failed = 1;
            eeprom_dirty = eeprom_dirty | 2;
        }
        break;
    case 160:
        break;
    case 161:
        if (dsp_flash_ok == 1) {
            bms_update_timeout = 0;
            dsp_flash_ok = 0;
            if (bms_update_last == 0) {
                bms_update_state = 1;
            } else {
                bms_update_state = 0xa2;
                bms_update_end_count = 0;
            }
            uart_rx_buf[3][0] = 0;
            uart_rx_buf[3][2] = 0;
        } else {
            if (bms_update_timeout <= 0x12c) {
                bms_update_timeout = bms_update_timeout + 1;
            } else {
                bms_update_state = 0xa2;
                bms_update_end_count = 0;
                bms_update_timeout = 0x190;
                cal_gains.bms_update_failed = 1;
                eeprom_dirty = eeprom_dirty | 2;
            }
        }
        break;
    case 162:
        bms_update_end_count = bms_update_end_count + 1;
        if (bms_update_end_count >= 0xa) {
            bms_update_end_count = 0xa;
        }
        if ((bms_update_end_count == 3 || bms_update_end_count == 7) &&
            bms_update_timeout != 0x190) {
            bms_update_crc32 = ~bms_update_crc32;
            dsp_tx_frame[0] = 0x42;
            dsp_tx_frame[1] = 0x4d;
            dsp_tx_frame[2] = 0x53;
            dsp_tx_frame[3] = 1;
            dsp_tx_frame[4] = 0x45;
            dsp_tx_frame[5] = 0x4e;
            dsp_tx_frame[6] = 0x44;
            dsp_tx_frame[7] = bms_update_crc32 >> 0x18;
            dsp_tx_frame[8] = bms_update_crc32 >> 0x10;
            dsp_tx_frame[9] = (uint)((ushort)bms_update_crc32) >> 8;
            dsp_tx_frame[10] = (uchar)bms_update_crc32;
            uart_tx_start(3, &dsp_tx_frame[0], 0x40a);
        }
        if (bms_update_end_count == 1 && f_open(&fil_msn, "0:MSN.txt", 1) == 0) {
            f_close(&fil_msn);
            f_unlink("0:ARMStore.bin");
        }
        break;
    default:
    }
    return;
}
