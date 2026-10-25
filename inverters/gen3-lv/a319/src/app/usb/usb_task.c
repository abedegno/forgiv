// clang-format off
/* at: usb_cdc_line_coding_init 0x0802B3CC, task_usb 0x0802B3E0 */
/* at: eeprom_load_ok 0x200001A7, usb_reset_state 0x200002BC, usb_start_delay 0x20000306, usb_class_code 0x2000030A, usb_task_tick_count 0x2000030F, USR_cb 0x200003E0, usb_class_prev 0x20000466, USBH_MSC_cb 0x20000544, CDC_cb 0x20000568, CDC_SetLineCode 0x2000058C, USB_OTG_Core 0x20003480, USB_Host 0x20003A30, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* usb_task.c: task_usb, which runs ST's USB host stack, either as a mass storage host (a USB
   stick with firmware or a serial number file) or as a CDC host (the WiFi/LAN dongle).

   Entry points: task_usb (task 7, every tick). Data: usb_class_prev.
   Neighbours: rtc.c before, meter.c after; usbh_usr.c (the application callbacks), the host
   library in src/lib/usb_host, scheduler_periodic_update (the dongle watchdog that sets usb_reset_state).

   usb_class_code is the bInterfaceClass seen in the configuration descriptor (usbh_usr.c): 8
   mass storage, 2 CDC. When it changes, the host is started again with the matching class
   driver.

   name: descriptive (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/eeprom.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"
#include "../../include/usb.h"

/* The CDC line coding to set on the dongle: 57600 baud, 8 data bits, 1 stop bit, no parity. */
void usb_cdc_line_coding_init() {

    /* CDC_SetLineCode and UserCb are ST USB library objects, declared as bytes for its sources */
    *(uint *)CDC_SetLineCode = 0xe100;
    CDC_SetLineCode[6] = 8;
    CDC_SetLineCode[5] = 0;
    CDC_SetLineCode[4] = 0;
    return;
}

/* Task 7 (priority 7), every tick, 10 s after start (usb_start_delay counts 500 ms passes to 20)
   and once the settings are loaded: on a class change starts USBH_Init with the CDC or MSC class
   driver; otherwise runs USBH_Process. The dongle watchdog's restart: usb_reset_state 0x11 puts
   the host state machine back to 3 (inference: a re-enumeration); 0x14 ends the restart. */
void task_usb() {

    usb_cdc_line_coding_init();
    while (1) {
        if (usb_start_delay >= 0x14) {
            if (usb_reset_state == 0x11) {
                USB_Host[0] = 3;
                usb_reset_state = 0x12;
            }
            if (usb_reset_state >= 0x14) {
                usb_reset_state = 0;
                usb_class_prev = 0;
            }
            if (eeprom_load_ok >= 2) {
                if (usb_class_prev != usb_class_code) {
                    usb_class_prev = usb_class_code;
                    if (usb_class_code == 2) {
                        USBH_Init(&USB_OTG_Core[0], 1, &USB_Host[0], &CDC_cb[0], &USR_cb[0]);
                    } else {
                        if (usb_class_code == 8) {
                            USBH_Init(&USB_OTG_Core[0], 1, &USB_Host[0], &USBH_MSC_cb[0],
                                      &USR_cb[0]);
                        }
                    }
                } else {
                    usb_task_tick_count = 0;
                    USBH_Process(&USB_OTG_Core[0], &USB_Host[0]);
                }
            }
        }
        task_tbl[TASK_USB].count = 0;
        task_tbl[TASK_USB].period = 1;
        os_flags_pend(0);
    }
}
