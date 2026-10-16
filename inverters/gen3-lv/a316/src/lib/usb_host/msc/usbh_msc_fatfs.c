/* at: disk_initialize 0x08035ADC, disk_status 0x08035AFA, disk_read 0x08035B08, disk_write 0x08035BA4, disk_ioctl 0x08035C1E, get_fattime 0x08035C74 */
/* at: Stat 0x20000549, USB_OTG_Core 0x20003474, USB_Host 0x20003A24, USBH_MSC_Param 0x20007FD4 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* FatFs disk layer on the USB stick, usbh_msc_fatfs.c: disk_initialize, disk_status,
   disk_read, disk_write, disk_ioctl and get_fattime for drive 0.
   Library: the diskio glue of ST's USB host MSC example for FatFs, with GivEnergy changes.
   Job in this firmware: the STM32F105's USB OTG FS port, as a host only (USE_USB_OTG_FS,
   USE_HOST_MODE, no DMA). task_usb (src/app/usb/usb_task.c) runs USBH_Process. Two classes are
   linked: CDC for the communications dongle, which carries Modbus RTU (modbus_slave_process
   receives through CDC_ReceiveData and answers with CDC_SendData), and MSC for a USB stick with
   firmware update files (FatFs, usbh_msc_fatfs.c). The host starts with the CDC class; when a
   device's first interface is class 8, USBH_USR_Configuration_DescAvailable sets
   usb_class_code and task_usb starts the host again with the MSC class.
   Vendor changes (GivEnergy):
   - disk_read gives up after 0xFFFFF tries while the stick answers busy, and when the count has
     reached 0xFFFF0 it resets the chip (the inline NVIC_SystemReset: AIRCR with SYSRESETREQ, then
     a loop). ST's example loops for as long as the stick is busy.
   - disk_write gives up after 0x7FFFFFF tries. It is never called: the firmware writes no files.
   - get_fattime returns 0, so a file would get no date (FatFs asks for it only when writing).
   GET_BLOCK_SIZE stores 512 but returns RES_ERROR (the result stays 1); FatFs asks for it only in
   f_mkfs, which is not built.
   name: original (map/filenames.tsv). */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/ff.h"
#include "../../../include/usb.h"
#include "../../../include/stdperiph.h"


/* Clear STA_NOINIT when a device is connected; return the status. */
uchar disk_initialize(int param_1)
{

  
  if (HCD_IsDeviceConnected(&USB_OTG_Core[0]) != 0) {
    Stat = Stat & -2;
  }
  return Stat;
}


/* Status of drive 0 (STA_NOINIT until disk_initialize); other drives STA_NOINIT. */
int disk_status(int param_1)
{

  
  if (param_1 != 0) {
    return 1;
  }
  return Stat;
}


/* Read param_4 sectors at sector param_3 into param_2: READ (10) and the BOT machine in
   a loop until done. GivEnergy: bounded retries, and a chip reset when the stick stays busy. */
int disk_read(int param_1, int param_2, int param_3, int param_4)
{
  int v4;
  int v7;
  
  v4 = 0;
  v7 = 0;
  if (param_1 != 0 || param_4 == 0) {
    return 4;
  }
  if ((Stat & 1) != 0) {
    return 3;
  }
  v4 = 0;
  if (HCD_IsDeviceConnected(&USB_OTG_Core[0]) != 0) {
    do {
      if ((uint)v4 < 0xfffffff) {
        v4++;
      }
      v7 = USBH_MSC_Read10(&USB_OTG_Core[0], param_2, param_3, param_4 << 9);
      USBH_MSC_HandleBOTXfer(&USB_OTG_Core[0], &USB_Host[0]);
      if (HCD_IsDeviceConnected(&USB_OTG_Core[0]) == 0) {
        return 1;
      }
    } while (v7 == 3 && (uint)v4 < 0xfffff);
  }
  if ((uint)v4 >= 0xffff0) {
    __disable_fiq();
    __nop();
    *(volatile uint *)SCB_AIRCR_ADDR = (*(volatile uint *)SCB_AIRCR_ADDR & 0x700 | 0x5fa0000) + 4;
    __dsb(0xf);
    __nop();
    while (1) {
    }
  }
  if (v7 == 0) {
    return 0;
  }
  return 1;
}


/* Write param_4 sectors: WRITE (10) in a loop, refused if write protected. GivEnergy:
   bounded retries. No caller writes. */
int disk_write(int param_1, int param_2, int param_3, int param_4)
{
  int v6;
  int v7;
  
  v7 = 0;
  v6 = 0;
  if (param_1 != 0 || param_4 == 0) {
    return 4;
  }
  if ((Stat & 1) != 0) {
    return 3;
  }
  if ((Stat & 4) != 0) {
    return 2;
  }
  if (HCD_IsDeviceConnected(&USB_OTG_Core[0]) != 0) {
    do {
      v7++;
      v6 = USBH_MSC_Write10(&USB_OTG_Core[0], param_2, param_3, param_4 << 9);
      USBH_MSC_HandleBOTXfer(&USB_OTG_Core[0], &USB_Host[0]);
      if (HCD_IsDeviceConnected(&USB_OTG_Core[0]) == 0) {
        return 1;
      }
    } while (v6 == 3 && (uint)v7 < 0x7ffffff);
  }
  if (v6 == 0) {
    return 0;
  }
  return 1;
}

extern uchar USBH_MSC_Param[];

/* CTRL_SYNC (nothing to do), sector count from READ CAPACITY, sector size 512, block
   size 512. */
int disk_ioctl(int param_1, int param_2, int param_3)
{
  int v4 = 0;
  
  if (param_1 != 0) {
    return 4;
  }
  v4 = 1;
  if ((Stat & 1) != 0) {
    return 3;
  }
  switch (param_2) {
  case 0:
    v4 = 0;
    break;
  case 1:
    *(int *)param_3 = (*(int *)(((uchar *)&USBH_MSC_Param) + 0));
    v4 = 0;
    break;
  case 2:
    *(ushort *)param_3 = 0x200;
    v4 = 0;
    break;
  case 3:
    *(int *)param_3 = 0x200;
    break;
  default:
    v4 = 4;
  }
  return v4;
}

/* FatFs time stamp: 0 (no clock used). */
uint get_fattime()
{

  return 0;
}

