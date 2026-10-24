/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow USB host callbacks and firmware-file handling here; the filesystem and link transport provide the data.
   Entry examples: usb_user_initialize, usb_user_device_attached, usb_user_unrecovered_error.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
/* USB host user application (USR_cb) and the firmware-update relay. Proved whole. */
/* at: usb_user_initialize 0x08015AAC, usb_user_device_attached 0x08015ABA, usb_user_unrecovered_error 0x08015ABC, usb_user_device_disconnected 0x08015ABE, usb_user_reset_device 0x08015AD8, usb_user_speed_detected 0x08015ADA, usb_user_device_descriptor 0x08015ADC, usb_user_address_assigned 0x08015ADE, usb_user_configuration 0x08015AE0, usb_user_manufacturer_string 0x08015AFA, usb_user_product_string 0x08015AFC, usb_user_serial_number 0x08015AFE, usb_user_enumeration_done 0x08015B00, usb_user_device_not_supported 0x08015B08, usb_user_input 0x08015B0A, usb_user_overcurrent 0x08015B0E, usb_get_filesystem_space 0x08015B10, usb_validate_firmware_file 0x08015B3E, usb_user_application 0x08015C6A, usb_user_deinitialize 0x08016034, firmware_clear_marker_and_reset 0x0801603E, firmware_find_first_ready_peer 0x080160BC, firmware_find_next_ready_peer 0x080160E2, firmware_relay_poll 0x0801610E, sub_0802F040 0x0802F040, f_getfree 0x0802B872, f_open 0x0802B094, f_read 0x0802B24E, f_unlink 0x0802B9F4, crc32_init 0x08015158, crc_update 0x080151B2, f_close 0x0802B5EA, sub_0802F0D2 0x0802F0D2, f_mount 0x0802B04C, board_set_pb2 0x08023B9A, strcpy 0x0802F100, f_lseek 0x0802B60A, f_write 0x0802B3B2, USART_ITConfig 0x08029D5A, sub_08014AC8 0x08014AC8, board_set_pa8 0x08023BB6, sub_08014F94 0x08014F94, f_rename 0x0802BBEA, afe_write_register 0x080244B8, __aeabi_f2uiz 0x08030020, eeprom_write_word 0x08022442, os_task_delay 0x0802A346, OS_ENTER_CRITICAL 0x0802A098, FLASH_Unlock 0x08028026, FLASH_ClearFlag 0x0802846A, FLASH_ProgramWord 0x080281DE, OS_EXIT_CRITICAL 0x0802A09C, board_set_pb5 0x08023BD2, usr_appstate 0x20000198, usr_init 0x20000199, usr_flag 0x2000019A, usr_wait 0x2000019B, usr_end_count 0x2000019C, usr_file_wait 0x2000019E, usr_reply 0x200001A0, usr_relay_state 0x200001A2, usr_pack 0x200001A4, usr_last_pack 0x200001A6, usr_relay_wait 0x200001A8, usr_relay_retries 0x200001AA, usr_file_offset 0x200001AC, usr_file_chunk 0x200001B0, usr_relay_base 0x200001B4, usr_relay_cur 0x200001B8, usr_relay_end 0x200001BC, usr_relay_chunk 0x200001C0, usr_relay_block 0x200001C4, USR_cb 0x200001C8, g_usr_20000198 0x20000198, g_usr_20000051 0x20000051, g_usr_20000052 0x20000052, g_usr_20000053 0x20000053, g_usr_20000055 0x20000055, g_usr_20000056 0x20000056, g_usr_20000674 0x20000674, g_usr_20001E4C 0x20001E4C, g_usr_200028CC 0x200028CC, g_usr_08013FFC 0x08013FFC, g_usr_2000003C 0x2000003C, g_usr_20000040 0x20000040, g_usr_200000E7 0x200000E7, g_usr_20001A41 0x20001A41, g_usr_2000224C 0x2000224C, g_usr_2000247C 0x2000247C, g_usr_200026A4 0x200026A4, g_usr_20002D1C 0x20002D1C, g_usr_200032CC 0x200032CC, g_usr_20000044 0x20000044, g_usr_20000163 0x20000163, g_usr_20000048 0x20000048, g_usr_200000DF 0x200000DF, g_usr_200000E0 0x200000E0, g_usr_200000E1 0x200000E1, g_usr_200000E2 0x200000E2, g_usr_200038A8 0x200038A8, g_usr_200000E5 0x200000E5, g_usr_200000E6 0x200000E6, g_usr_200000E9 0x200000E9, g_usr_200000EF 0x200000EF, g_usr_200001A0 0x200001A0, g_usr_200000FC 0x200000FC, g_usr_08015EE8 0x08015EE8, g_usr_20001637 0x20001637, g_usr_200015D3 0x200015D3, g_usr_20001605 0x20001605, g_usr_20000524 0x20000524, g_usr_20000525 0x20000525, g_usr_200000D5 0x200000D5, g_usr_20000606 0x20000606, g_usr_2000016C 0x2000016C, g_usr_20003D52 0x20003D52, g_usr_20002AF4 0x20002AF4, g_usr_200000E3 0x200000E3, g_usr_200000E4 0x200000E4, g_usr_20000CF7 0x20000CF7 */
#include "bms_usb_usr_types.h"
uchar g_usr_20001E4C[0x400];
FATFS g_usr_2000224C;
FIL g_usr_2000247C;
FIL g_usr_200026A4;
FIL g_usr_200028CC;
FIL g_usr_20002AF4;
static __inline void put_status(uchar *p, uchar value) {
    *(uchar *)p = value;
}
uchar usr_appstate = 0;
uchar usr_init = 0;
uchar usr_flag = 0;
uchar usr_wait = 0;
uchar usr_end_count = 0;
ushort usr_file_wait = 0;
uchar usr_reply[2] = {0};
ushort usr_relay_state = 0;
ushort usr_pack = 0;
ushort usr_last_pack = 0;
ushort usr_relay_wait = 0;
ushort usr_relay_retries = 0;
uint usr_file_offset = 0;
uint usr_file_chunk = 0x400;
uint usr_relay_base = 0;
uint usr_relay_cur = 0;
uint usr_relay_end = 0;
uint usr_relay_chunk = 0x400;
uint usr_relay_block = 0;
#define DAT_20000199 usr_init

/* Latch the first USB host initialization. */
void usb_user_initialize(void)

{
    if (DAT_20000199 == '\0') {
        DAT_20000199 = '\x01';
    }
    return;
}

#undef DAT_20000199

/* No application action on device attachment. */
void usb_user_device_attached(void)

{
    return;
}

/* No application action for an unrecovered host error. */
void usb_user_unrecovered_error(void)

{
    return;
}

#define g_20000051 g_usr_20000051
#define g_20000052 g_usr_20000052
#define g_20000053 g_usr_20000053
#define g_20000055 g_usr_20000055
#define g_20000056 g_usr_20000056

/* DeviceDisconnected callback clears transfer state and selects class2. */
void usb_user_device_disconnected(void)

{
    g_20000052 = 0;
    g_20000055 = 0;
    g_20000056 = 0;
    g_20000051 = 0;
    g_20000053 = 2;
    return;
}

#undef g_20000051
#undef g_20000052
#undef g_20000053
#undef g_20000055
#undef g_20000056

/* No application action during device reset. */
void usb_user_reset_device(void)

{
    return;
}

/* No application action when device speed is reported. */
void usb_user_speed_detected(void)

{
    return;
}

/* No application action when the device descriptor arrives. */
void usb_user_device_descriptor(void)

{
    return;
}

/* No application action when the USB address is assigned. */
void usb_user_address_assigned(void)

{
    return;
}

#define g_20000053 g_usr_20000053

/* ConfigurationDescAvailable callback selects interface class8 or2. */
void usb_user_configuration(undefined4 configuration_descriptor, int interface_descriptor)

{
    char interface_class;

    interface_class = *(char *)(interface_descriptor + 5);
    if (interface_class == '\b') {
        g_20000053 = 8;
    } else if (interface_class == '\x02') {
        g_20000053 = interface_class;
        sub_0802F040(g_usr_20002D1C);
        return;
    }
    return;
}

#undef g_20000053

/* No application action for the manufacturer string. */
void usb_user_manufacturer_string(void)

{
    return;
}

/* No application action for the product string. */
void usb_user_product_string(void)

{
    return;
}

/* No application action for the serial-number string. */
void usb_user_serial_number(void)

{
    return;
}

#define DAT_20000678 (*(uint *)(g_usr_20000674 + 4))

/* EnumerationDone callback installs the receive-handler entry. */
void usb_user_enumeration_done(void)

{
    DAT_20000678 = 0x8025c8d;
    return;
}

#undef DAT_20000678

/* No application action for an unsupported device. */
void usb_user_device_not_supported(void)

{
    return;
}

/* UserInput callback returns the original value1. */
undefined4 usb_user_input(void)

{
    return 1;
}

/* No application action in the host overcurrent callback. */
void usb_user_overcurrent(void)

{
    return;
}

/* Returns total and free KiB from FatFS cluster counts.
   The first output receives total KiB and the second receives free KiB; I name their actual writes. */
int usb_get_filesystem_space(int path, int *total_kib, int *free_kib) {
    int result, free_sectors, total_sectors;
    FATFS *fs_info;
    int free_clusters = 0;
    result = f_getfree(path, &free_clusters, &fs_info);
    if (result == 0) {
        total_sectors = (fs_info->n_fatent - 2) * fs_info->csize;
        free_sectors = fs_info->csize * free_clusters;
        *total_kib = (uint)total_sectors >> 1;
        *free_kib = (uint)free_sectors >> 1;
    }
    return result;
}
#define g_20001E4C (*(uchar *)(g_usr_20001E4C + 0))
#define g_200028CC g_usr_200028CC
#define DAT_08013ffc g_usr_08013FFC
#define DAT_20001e4e (*(uchar *)(g_usr_20001E4C + 2))
#define DAT_20001e4f (*(uchar *)(g_usr_20001E4C + 3))
#define g_2000003C g_usr_2000003C
#define DAT_20001e4d (*(uchar *)(g_usr_20001E4C + 1))
#define g_20000040 g_usr_20000040
#define DAT_200028d8 (g_usr_200028CC.obj.objsize)
#define DAT_20001e55 (*(uchar *)(g_usr_20001E4C + 9))
#define DAT_20001e52 (*(uchar *)(g_usr_20001E4C + 6))
#define DAT_20001e54 (*(uchar *)(g_usr_20001E4C + 8))
#define DAT_20001e53 (*(uchar *)(g_usr_20001E4C + 7))
#define g_20000052 g_usr_20000052
#define g_200000E7 g_usr_200000E7
/* Checks firmware version, magic and CRC before accepting the file.
   I retain version/CRC rejection, short-read handling and file-removal branches as matched. */
void usb_validate_firmware_file(void) {
    int read_result;
    ushort expected_version;
    ushort i;
    uint br = 0;
    uint chunk = 0x400;
    if (g_20000052 == 0 && f_open(&g_200028CC, "0:BMS_ARM.bin", 1) == 0) {
        expected_version = (ushort)g_usr_08013FFC;
        read_result = f_read(&g_200028CC, g_usr_20001E4C, chunk, &br);
        if (read_result == 0 && (ushort)(DAT_20001e4e + DAT_20001e4f * 0x100) == 0x5566) {
            if (g_20001E4C + DAT_20001e4d * 0x100 != expected_version) {
                crc32_init(&g_2000003C);
                crc_update(&g_2000003C, g_usr_20001E4C, chunk);
                for (i = 1; i < (DAT_200028d8 + 0x3ff) >> 10; i++) {
                    if (((DAT_200028d8 + 0x3ff) >> 10) - 1 == i) {
                        chunk = DAT_200028d8 & 0x3ff;
                        if (chunk == 0)
                            chunk = 0x400;
                    } else
                        chunk = 0x400;
                    f_read(&g_200028CC, g_usr_20001E4C, chunk, &br);
                    if (((DAT_200028d8 + 0x3ff) >> 10) - 1 != i)
                        crc_update(&g_2000003C, g_usr_20001E4C, chunk);
                }
                {
                    uint w = g_usr_20001E4C[9] + (uint)g_usr_20001E4C[6] * 0x1000000U +
                             (g_usr_20001E4C[7] * 0x10000 + g_usr_20001E4C[8] * 0x100);
                    g_20000040 = w;
                }
                g_2000003C = ~g_2000003C;
                if (g_2000003C == g_20000040)
                    g_20000052 = 1;
                else {
                    g_200000E7 = 0;
                    f_unlink("0:BMS_ARM.bin");
                }
            } else {
                g_200000E7 = 0;
                f_unlink("0:BMS_ARM.bin");
            }
        }
        f_close(&g_200028CC);
    }
}
#undef g_20001E4C
#undef g_200028CC
#undef DAT_08013ffc
#undef DAT_20001e4e
#undef DAT_20001e4f
#undef g_2000003C
#undef DAT_20001e4d
#undef g_20000040
#undef DAT_200028d8
#undef DAT_20001e55
#undef DAT_20001e52
#undef DAT_20001e54
#undef DAT_20001e53
#undef g_20000052
#undef g_200000E7
#define g_20001A41 (*(uchar *)(g_usr_20001A41 + 0))
#define g_20001E4C (*(uchar *)(g_usr_20001E4C + 0))
#define g_2000224C g_usr_2000224C
#define g_2000247C g_usr_2000247C
#define g_200026A4 g_usr_200026A4
#define g_20002D1C (*(uchar *)(g_usr_20002D1C + 0))
#define g_200032CC (*(uchar *)(g_usr_200032CC + 0))
#define g_20000044 g_usr_20000044
#define g_20000163 g_usr_20000163
#define g_20000048 g_usr_20000048
#define g_200000DF g_usr_200000DF
#define g_200000E0 g_usr_200000E0
#define g_200000E1 g_usr_200000E1
#define g_200000E2 g_usr_200000E2
#define g_200038A8 (*(uchar *)(g_usr_200038A8 + 0))
#define g_20000051 g_usr_20000051
#define g_200000E5 g_usr_200000E5
#define g_20000053 g_usr_20000053
#define g_200000E6 g_usr_200000E6
#define g_200000E7 g_usr_200000E7
#define g_20000055 g_usr_20000055
#define g_20000056 g_usr_20000056
#define g_200000E9 g_usr_200000E9
#define g_200000EF g_usr_200000EF
#define g_20000198 usr_appstate
#define g_200001A0 usr_reply[0]
#define g_200000FC g_usr_200000FC
#define DAT_200001ac usr_file_offset
#define DAT_2000019a usr_flag
#define DAT_200038a9 (*(uchar *)(g_usr_200038A8 + 1))
#define DAT_200001a1 usr_reply[1]
#define DAT_2000019e usr_file_wait
#define DAT_200001b0 usr_file_chunk
#define DAT_200026b0 (g_usr_200026A4.obj.objsize)
#define DAT_20001e55 (*(uchar *)(g_usr_20001E4C + 9))
#define DAT_20001e52 (*(uchar *)(g_usr_20001E4C + 6))
#define DAT_20001e53 (*(uchar *)(g_usr_20001E4C + 7))
#define DAT_20001e54 (*(uchar *)(g_usr_20001E4C + 8))
#define DAT_2000163b (*(uchar *)(g_usr_20001637 + 4))
#define g_200015D3 (*(uchar *)(g_usr_200015D3 + 0))
#define g_20001605 (*(uchar *)(g_usr_20001605 + 0))
#define g_20000524 g_usr_20000524
#define g_20001637 (*(uchar *)(g_usr_20001637 + 0))
#define g_20000525 g_usr_20000525
/* Services CDC or mass-storage firmware receive and file operations. */
int usb_user_application(void) {
    int result;
    uint bytes = 0;
    uint br;
    ushort i = 0;
    if (g_20000198 == 0) {
        if (g_20000053 == 2) {
            g_20000051 = 2;
            if (g_20000056 == 0)
                sub_0802F0D2(&g_20002D1C, &g_200032CC);
            g_20000056 = 1;
        } else if (g_20000053 == 8) {
            g_20000051 = 8;
            if (g_20000055 == 0) {
                if (f_mount(&g_2000224C, "0:", 0) != 0)
                    return -1;
                usb_validate_firmware_file();
                g_20000055 = 1;
            }
            if (g_20000163 == 0) {
                if (g_200000E0 == 1) {
                    board_set_pb2(1);
                    strcpy((char *)&g_20001605, "0:ZH01.bin");
                    if (DAT_200001ac == 0)
                        f_unlink(&g_20001605);
                    result = f_open(&g_2000247C, &g_20001605, 0x12);
                    if (result == 0)
                        result = f_lseek(&g_2000247C, DAT_200001ac);
                    if (result == 0) {
                        f_write(&g_2000247C, &g_20001637, 0x400, &br);
                        g_20000524 = 2;
                        g_200038A8 = 0x4f;
                        DAT_200038a9 = 0x4b;
                        g_200000E0 = 0;
                        g_200000E7 = 5;
                        DAT_200001ac += 0x400;
                    } else {
                        DAT_2000019a = 0;
                        g_200000E0 = 0;
                        put_status(&g_20000524, 2);
                        g_200038A8 = 0x4e;
                        DAT_200038a9 = 0x4f;
                        g_200000E7 = 0;
                    }
                    f_close(&g_2000247C);
                    if (g_200000E0 == 0) {
                        for (i = 0; i < 0x400; i++)
                            (&g_20001637)[i] = 0;
                        g_200000E0 = 0;
                        g_20000525 = 0;
                        USART_ITConfig(0x40004800, 0x727, 1);
                    }
                } else if (g_200000E5 == 1) {
                    g_200000E5 = 0;
                    g_200000DF = 0;
                    g_200000E9 = 0;
                    g_200000E1 = 1;
                    g_200000FC = 0;
                    sub_08014AC8(0, 150);
                }
            } else if (g_200000E2 == 1) {
                board_set_pa8(1);
                for (i = 0; i < 0x400; i++)
                    (&g_20001A41)[i] = (&DAT_2000163b)[i];
                strcpy((char *)&g_20001605, "0:ZH02.bin");
                strcpy((char *)&g_200015D3, "0:BMS_ARM.bin");
                if (DAT_200001ac == 0) {
                    f_unlink(&g_200015D3);
                    f_unlink(&g_20001605);
                }
                result = f_open(&g_2000247C, &g_20001605, 0x12);
                if (result == 0)
                    result = f_lseek(&g_2000247C, DAT_200001ac);
                if (result == 0) {
                    f_write(&g_2000247C, &g_20001A41, 0x400, &br);
                    g_200001A0 = 0x4f;
                    DAT_200001a1 = 0x4b;
                    g_200000E2 = 0;
                    DAT_200001ac += 0x400;
                    g_200000E7 = 1;
                } else {
                    DAT_2000019a = 0;
                    g_200000E2 = 0;
                    g_200001A0 = 0x4e;
                    DAT_200001a1 = 0x4f;
                    g_200000E7 = 2;
                }
                f_close(&g_2000247C);
                if (g_200000E2 == 0) {
                    for (i = 0; i < 0x400; i++) {
                        (&DAT_2000163b)[i] = 0;
                        (&g_20001A41)[i] = 0;
                    }
                    g_200000E2 = 0;
                    g_20000525 = 0;
                    sub_08014F94(0, &g_200001A0, 2);
                }
            } else if (g_200000E6 == 1 && DAT_2000019e++ > 200) {
                DAT_2000019e = 200;
                if (f_open(&g_200026A4, &g_20001605, 1) == 0) {
                    crc32_init(&g_20000048);
                    for (i = 0; i < (DAT_200026b0 + 0x3ff) >> 10; i++) {
                        if (((DAT_200026b0 + 0x3ff) >> 10) - 1 == i) {
                            DAT_200001b0 = DAT_200026b0 & 0x3ff;
                            if (DAT_200001b0 == 0)
                                DAT_200001b0 = 0x400;
                        } else
                            DAT_200001b0 = 0x400;
                        f_read(&g_200026A4, g_usr_20001E4C, DAT_200001b0, &bytes);
                        if (((DAT_200026b0 + 0x3ff) >> 10) - 1 != i)
                            crc_update(&g_20000048, g_usr_20001E4C, DAT_200001b0);
                    }
                    g_20000048 = ~g_20000048;
                    g_20000044 = DAT_20001e55 + DAT_20001e52 * 0x1000000U +
                                 (DAT_20001e53 * 0x10000 + DAT_20001e54 * 0x100);
                    if (g_20000048 == g_20000044) {
                        f_unlink(&g_200015D3);
                        f_rename(&g_20001605, &g_200015D3);
                    }
                }
                f_close(&g_2000247C);
                f_unlink(&g_20001605);
                g_200001A0 = 0x4f;
                DAT_200001a1 = 0x4b;
                g_200000E6 = 0;
                g_20000525 = 0;
                sub_08014F94(0, &g_200001A0, 2);
                g_200000EF = 1;
            }
        }
    }
    return 0;
}
#undef g_20001A41
#undef g_20001E4C
#undef g_2000224C
#undef g_2000247C
#undef g_200026A4
#undef g_20002D1C
#undef g_200032CC
#undef g_20000044
#undef g_20000163
#undef g_20000048
#undef g_200000DF
#undef g_200000E0
#undef g_200000E1
#undef g_200000E2
#undef g_200038A8
#undef g_20000051
#undef g_200000E5
#undef g_20000053
#undef g_200000E6
#undef g_200000E7
#undef g_20000055
#undef g_20000056
#undef g_200000E9
#undef g_200000EF
#undef g_20000198
#undef g_200001A0
#undef g_200000FC
#undef DAT_200001ac
#undef DAT_2000019a
#undef DAT_200038a9
#undef DAT_200001a1
#undef DAT_2000019e
#undef DAT_200001b0
#undef DAT_200026b0
#undef DAT_20001e55
#undef DAT_20001e52
#undef DAT_20001e53
#undef DAT_20001e54
#undef DAT_2000163b
#undef g_200015D3
#undef g_20001605
#undef g_20000524
#undef g_20001637
#undef g_20000525
#define g_20000198 usr_appstate

/* Clear the USB application inhibit flag during deinitialization. */
void usb_user_deinitialize(void)

{
    g_20000198 = 0;
    return;
}

#undef g_20000198
#define g_20000052 g_usr_20000052
#define DAT_08013ffc g_usr_08013FFC
#define g_200000D5 g_usr_200000D5
#define DAT_e000ed0c (*(volatile uint *)0xE000ED0C)
#define g_20000606 g_usr_20000606
#define g_2000016C g_usr_2000016C

/* Persists capacity then clears the firmware marker word and requests reset.
   match: I retain the critical section, marker write and reset sequence. */
void firmware_clear_marker_and_reset(void)

{
    uint capacity_word;
    uint *flash;

    if (g_20000052 == '\x01') {
        if (g_200000D5 == '\x01') {
            afe_write_register(6, 0x10);
            capacity_word = __aeabi_f2uiz(g_2000016C);
            eeprom_write_word(6, capacity_word & 0xffff);
            afe_write_register(6, 0x10);
            os_task_delay((uint)(byte)g_20000606, 100, 10);
        }
        OS_ENTER_CRITICAL();
        FLASH_Unlock();
        FLASH_ClearFlag(0x35);
        flash = (uint *)&DAT_08013ffc;
        FLASH_ProgramWord(flash, 0);
        OS_EXIT_CRITICAL();
        if (*flash == 0) {
            board_set_pb5(1);
            __disable_fiq();
            DAT_e000ed0c = (DAT_e000ed0c & 0x700) | 0x05FA0004;
            __dsb(0xf);
            do {

            } while (true);
        }
    }
    return;
}

#undef g_20000052
#undef DAT_08013ffc
#undef g_200000D5
#undef DAT_e000ed0c
#undef g_20000606
#undef g_2000016C
#define g_20003D52 (*(ushort *)(g_usr_20003D52 + 0))
/* Finds the first peer index1..5 with a ready reply word. */
ushort firmware_find_first_ready_peer(void) {
    ushort i;
    for (i = 1; i < 6; i++) {
        if (*(__packed ushort *)((uchar *)&g_20003D52 + i * 0x91) == 1)
            break;
        if (i == 5)
            return 0;
    }
    return i;
}
#undef g_20003D52
#define g_20003D52 (*(ushort *)(g_usr_20003D52 + 0))
/* Finds the next ready peer or retains the current peer index. */
ushort firmware_find_next_ready_peer(ushort current_peer) {
    ushort i;
    for (i = current_peer + 1; i < 6; i++) {
        if (*(__packed ushort *)((uchar *)&g_20003D52 + i * 0x91) == 1)
            break;
        if (i == 5)
            return current_peer;
    }
    return i;
}
#undef g_20003D52
#define g_20001E4C (*(uchar *)(g_usr_20001E4C + 0))
#define g_2000247C g_usr_2000247C
#define g_20002AF4 g_usr_20002AF4
#define g_2000003C g_usr_2000003C
#define g_20000040 g_usr_20000040
#define g_20000044 g_usr_20000044
#define g_200000E3 g_usr_200000E3
#define g_200000E4 g_usr_200000E4
#define g_200000E7 g_usr_200000E7
#define DAT_200001a2 usr_relay_state
#define DAT_200001a4 usr_pack
#define DAT_20001638 (*(uchar *)(g_usr_20001637 + 1))
#define g_200000EF g_usr_200000EF
#define DAT_20001639 (*(uchar *)(g_usr_20001637 + 2))
#define DAT_2000163a (*(uchar *)(g_usr_20001637 + 3))
#define DAT_2000163b (*(uchar *)(g_usr_20001637 + 4))
#define DAT_2000163c (*(uchar *)(g_usr_20001637 + 5))
#define DAT_2000163d (*(uchar *)(g_usr_20001637 + 6))
#define DAT_2000163e (*(uchar *)(g_usr_20001637 + 7))
#define DAT_2000019c usr_end_count
#define DAT_200001a8 usr_relay_wait
#define DAT_200001b8 usr_relay_cur
#define DAT_200001b4 usr_relay_base
#define DAT_200001a6 usr_last_pack
#define DAT_2000019b usr_wait
#define DAT_200001c0 usr_relay_chunk
#define DAT_20002b00 (g_usr_20002AF4.obj.objsize)
#define DAT_20001e55 (*(uchar *)(g_usr_20001E4C + 9))
#define DAT_20001e52 (*(uchar *)(g_usr_20001E4C + 6))
#define DAT_20001e53 (*(uchar *)(g_usr_20001E4C + 7))
#define DAT_20001e54 (*(uchar *)(g_usr_20001E4C + 8))
#define DAT_200001bc usr_relay_end
#define DAT_200001c4 usr_relay_block
#define DAT_20001a3b (*(uchar *)(g_usr_20001637 + 1028))
#define DAT_20001a3c (*(uchar *)(g_usr_20001637 + 1029))
#define DAT_20000ebc (*(uchar *)((uchar *)&g_usr_20000CF7 + 453))
#define DAT_20000ebe (*(uchar *)((uchar *)&g_usr_20000CF7 + 455))
#define DAT_200001aa usr_relay_retries
#define g_200015D3 (*(uchar *)(g_usr_200015D3 + 0))
#define g_20001605 (*(uchar *)(g_usr_20001605 + 0))
#define g_20001637 (*(uchar *)(g_usr_20001637 + 0))
/* Advances start, data, end, acknowledgement and next-peer update states.
   I retain the start/data/end packets, acknowledgement states and next-peer retry order. */
void firmware_relay_poll(void) {
    ushort i = 0;
    ushort j = 0;
    ushort sum = 0;
    int res;
    uint br;
    switch (DAT_200001a2) {
    case 0:
        DAT_2000019b++;
        if (DAT_2000019b >= 20) {
            DAT_2000019b = 17;
            DAT_200001a2 = 1;
            crc32_init(&g_20000044);
        }
        if (DAT_2000019b <= 3) {
            DAT_200001a4 = firmware_find_first_ready_peer();
            if (DAT_200001a4 == 0) {
                strcpy((char *)&g_20001605, "0:ZH01.bin");
                strcpy((char *)&g_200015D3, "0:BMS_ARM.bin");
                if (f_open(&g_20002AF4, &g_20001605, 1) == 0) {
                    crc32_init(&g_2000003C);
                    for (i = 0; i < (DAT_20002b00 + 0x3ff) >> 10; i++) {
                        if (((DAT_20002b00 + 0x3ff) >> 10) - 1 == i) {
                            DAT_200001c0 = DAT_20002b00 & 0x3ff;
                            if (DAT_200001c0 == 0)
                                DAT_200001c0 = 0x400;
                        } else
                            DAT_200001c0 = 0x400;
                        f_read(&g_20002AF4, g_usr_20001E4C, DAT_200001c0, &br);
                        if (((DAT_20002b00 + 0x3ff) >> 10) - 1 != i)
                            crc_update(&g_2000003C, g_usr_20001E4C, DAT_200001c0);
                    }
                    {
                        uint v;
                        v = DAT_20001e55;
                        j = DAT_20001e52;
                        v = v + j * 0x1000000U;
                        v = v + (DAT_20001e53 * 0x10000 + DAT_20001e54 * 0x100);
                        g_20000040 = v;
                    }
                    g_2000003C = ~g_2000003C;
                    if (g_2000003C == g_20000040) {
                        f_unlink(&g_200015D3);
                        f_rename(&g_20001605, &g_200015D3);
                    }
                }
                f_close(&g_2000247C);
                f_unlink(&g_20001605);
                g_200000EF = 1;
                DAT_200001a2 = 2;
                DAT_200001a8 = 0;
            }
        }
        if (DAT_2000019b >= 18) {
            br = 0;
            DAT_200001a8 = 0;
            if (f_open(&g_20002AF4, "0:ZH01.bin", 1) == 0) {
                DAT_200001b8 = DAT_200001b4;
                DAT_200001bc = DAT_200001b4 + ((DAT_20002b00 + 1) >> 1);
                DAT_200001c4 = 0;
            }
        }
        if (DAT_2000019b == 5 || DAT_2000019b == 10 || DAT_2000019b == 15) {
            board_set_pa8(1);
            g_20001637 = 0x53;
            DAT_20001638 = 0x54;
            DAT_20001639 = 0x41;
            DAT_2000163a = DAT_200001a4;
            DAT_2000163b = 0xcc;
            DAT_2000163c = 0xbb;
            sub_08014F94(0, &g_20001637, 6);
        }
        break;
    case 1:
        for (j = 0; j < 0x400; j++)
            (g_usr_20001E4C)[j] = 0xff;
        g_20001637 = 0x42;
        DAT_20001638 = 0x4d;
        DAT_20001639 = 0x53;
        DAT_2000163a = DAT_200001a4;
        if (DAT_200001b8 + 0x200 >= DAT_200001bc) {
            DAT_200001c0 = (DAT_200001bc - DAT_200001b8) * 2;
            DAT_200001c4 = 0;
            g_200000E3 = 1;
            g_200000E7 = 4;
        } else {
            DAT_200001c0 = 0x400;
            g_200000E3 = 0;
            g_200000E7 = 3;
        }
        DAT_200001b8 += 0x200;
        if (DAT_200001c0 <= 2)
            DAT_200001c0 = 2;
        res = f_read(&g_20002AF4, g_usr_20001E4C, DAT_200001c0, &br);
        if (res != 0) {
            f_close(&g_20002AF4);
            DAT_200001b8 = 0;
            DAT_200001bc = 0;
            DAT_200001c0 = 0x400;
            DAT_200001c4 = 0;
            DAT_200001a2 = 2;
            return;
        }
        for (j = 0; j < 0x400; j++)
            (&DAT_2000163b)[j] = (g_usr_20001E4C)[j];
        crc_update(&g_20000044, g_usr_20001E4C, DAT_200001c0);
        for (j = 0; j < 0x404; j++)
            sum += (&g_20001637)[j];
        DAT_20001a3b = sum >> 8;
        DAT_20001a3c = sum;
        for (j = 0; j < 0x400; j++) {
            if ((g_usr_20001E4C)[j] != 0xff) {
                board_set_pa8(1);
                sub_08014F94(0, &g_20001637, 0x406);
                break;
            }
            DAT_200001c4++;
        }
        DAT_200001a2 = 0xa1;
        break;
    case 2:
        g_20000044 = ~g_20000044;
        g_20001637 = 0x45;
        DAT_20001638 = 0x4e;
        DAT_20001639 = 0x44;
        DAT_2000163a = DAT_200001a4;
        DAT_2000163b = g_20000044 >> 24;
        DAT_2000163c = g_20000044 >> 16;
        DAT_2000163d = g_20000044 >> 8;
        DAT_2000163e = g_20000044;
        board_set_pa8(1);
        sub_08014F94(0, &g_20001637, 10);
        g_200000E3 = 2;
        g_200000E7 = 0;
        if (DAT_2000019c++ >= 5) {
            DAT_200001a2 = 0xa2;
            DAT_2000019c = 0;
        }
        break;
    case 160:
        break;
    case 161:
        if (g_200000E4 == 1) {
            g_200000E4 = 0;
            if (g_200000E3 == 0)
                DAT_200001a2 = 1;
            else if (g_200000E3 == 1) {
                g_200000E3 = 0;
                DAT_200001a2 = 0xa2;
            } else if (g_200000E3 == 2) {
                g_200000E3 = 0;
                DAT_200001a2 = 0xa2;
                g_200000E7 = 0;
            }
            DAT_20000ebc = 0;
            DAT_20000ebe = 0;
        }
        break;
    case 162:
        if (DAT_200001a8++ >= 50) {
            DAT_200001b8 = 0;
            DAT_200001b4 = 0;
            DAT_200001a6 = DAT_200001a4;
            DAT_200001a4 = firmware_find_next_ready_peer(DAT_200001a4);
            if (DAT_200001a4 != DAT_200001a6) {
                crc32_init(&g_20000044);
                DAT_200001a8 = 0;
                DAT_200001a2 = 0;
            } else {
                DAT_200001a2 = 2;
                DAT_2000019c = 0;
                DAT_200001a8 = 50;
                if (DAT_200001aa++ >= 20) {
                    DAT_200001aa = 20;
                    strcpy((char *)&g_20001605, "0:ZH01.bin");
                    strcpy((char *)&g_200015D3, "0:BMS_ARM.bin");
                    if (f_open(&g_20002AF4, &g_20001605, 1) == 0) {
                        crc32_init(&g_2000003C);
                        for (i = 0; i < (DAT_20002b00 + 0x3ff) >> 10; i++) {
                            if (((DAT_20002b00 + 0x3ff) >> 10) - 1 == i) {
                                DAT_200001c0 = DAT_20002b00 & 0x3ff;
                                if (DAT_200001c0 == 0)
                                    DAT_200001c0 = 0x400;
                            } else
                                DAT_200001c0 = 0x400;
                            f_read(&g_20002AF4, g_usr_20001E4C, DAT_200001c0, &br);
                            if (((DAT_20002b00 + 0x3ff) >> 10) - 1 != i)
                                crc_update(&g_2000003C, g_usr_20001E4C, DAT_200001c0);
                        }
                        g_20000040 = DAT_20001e55 + DAT_20001e52 * 0x1000000U +
                                     (DAT_20001e53 * 0x10000 + DAT_20001e54 * 0x100);
                        g_2000003C = ~g_2000003C;
                        if (g_2000003C == g_20000040) {
                            f_unlink(&g_200015D3);
                            f_rename(&g_20001605, &g_200015D3);
                        }
                    }
                    f_close(&g_2000247C);
                    f_unlink(&g_20001605);
                    DAT_2000019b = 0;
                    g_200000EF = 1;
                }
            }
        }
        break;
    }
}
#undef g_20001E4C
#undef g_2000247C
#undef g_20002AF4
#undef g_2000003C
#undef g_20000040
#undef g_20000044
#undef g_200000E3
#undef g_200000E4
#undef g_200000E7
#undef DAT_200001a2
#undef DAT_200001a4
#undef DAT_20001638
#undef g_200000EF
#undef DAT_20001639
#undef DAT_2000163a
#undef DAT_2000163b
#undef DAT_2000163c
#undef DAT_2000163d
#undef DAT_2000163e
#undef DAT_2000019c
#undef DAT_200001a8
#undef DAT_200001b8
#undef DAT_200001b4
#undef DAT_200001a6
#undef DAT_2000019b
#undef DAT_200001c0
#undef DAT_20002b00
#undef DAT_20001e55
#undef DAT_20001e52
#undef DAT_20001e53
#undef DAT_20001e54
#undef DAT_200001bc
#undef DAT_200001c4
#undef DAT_20001a3b
#undef DAT_20001a3c
#undef DAT_20000ebc
#undef DAT_20000ebe
#undef DAT_200001aa
#undef g_200015D3
#undef g_20001605
#undef g_20001637

typedef void (*usr_fn)(void);
typedef struct {
    usr_fn Init;
    usr_fn DeInit;
    usr_fn DeviceAttached;
    usr_fn ResetDevice;
    usr_fn DeviceDisconnected;
    usr_fn OverCurrentDetected;
    usr_fn DeviceSpeedDetected;
    usr_fn DeviceDescAvailable;
    usr_fn DeviceAddressAssigned;
    usr_fn ConfigurationDescAvailable;
    usr_fn ManufacturerString;
    usr_fn ProductString;
    usr_fn SerialNumString;
    usr_fn EnumerationDone;
    usr_fn UserInput;
    int (*UserApplication)(void);
    usr_fn DeviceNotSupported;
    usr_fn UnrecoveredError;
} USR_cb_t;
USR_cb_t USR_cb = {(usr_fn)usb_user_initialize,
                   (usr_fn)usb_user_deinitialize,
                   (usr_fn)usb_user_device_attached,
                   (usr_fn)usb_user_reset_device,
                   (usr_fn)usb_user_device_disconnected,
                   (usr_fn)usb_user_overcurrent,
                   (usr_fn)usb_user_speed_detected,
                   (usr_fn)usb_user_device_descriptor,
                   (usr_fn)usb_user_address_assigned,
                   (usr_fn)usb_user_configuration,
                   (usr_fn)usb_user_manufacturer_string,
                   (usr_fn)usb_user_product_string,
                   (usr_fn)usb_user_serial_number,
                   (usr_fn)usb_user_enumeration_done,
                   (usr_fn)usb_user_input,
                   usb_user_application,
                   (usr_fn)usb_user_device_not_supported,
                   (usr_fn)usb_user_unrecovered_error};
