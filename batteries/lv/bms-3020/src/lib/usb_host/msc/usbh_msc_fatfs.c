/* at: disk_initialize 0x0802E7A8, disk_status 0x0802E7C0, disk_read 0x0802E7CE, disk_write 0x0802E85E, disk_ioctl 0x0802E8D4, get_fattime 0x0802E918, Stat 0x20000664, USB_OTG_Core 0x20002D1C, USB_Host 0x200032CC, USBH_MSC_Param 0x200077F4, HCD_IsDeviceConnected 0x0802C56C, USBH_MSC_Read10 0x0802EBFC, USBH_MSC_Write10 0x0802EB60, USBH_MSC_HandleBOTXfer 0x0802E268 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* ST USB MSC FatFs disk glue, adapted from the matched A316 source. */
#include "bms_msc_disk.h"

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
    } while (v7 == 3 && (uint)v4 < 0x1ffff);
  }
  if ((uint)v4 >= 0x1fff0) {
    __disable_fiq();
    *(volatile uint *)SCB_AIRCR_ADDR = (*(volatile uint *)SCB_AIRCR_ADDR & 0x700) | 0x5fa0004;
    __dsb(0xf);
    while (1) {
    }
  }
  if (v7 == 0) {
    return 0;
  }
  return 1;
}


/* Write param_4 sectors: WRITE (10) in a loop, refused if write protected. GivEnergy:
   bounded retries. */
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

