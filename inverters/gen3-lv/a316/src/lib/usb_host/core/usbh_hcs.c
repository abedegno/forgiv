/* at: USBH_Open_Channel 0x080348FC, USBH_Modify_Channel 0x08034996, USBH_GetFreeChannel 0x080349FA, USBH_Alloc_Channel 0x08034A22 */
/* at: USBH_Free_Channel 0x08034A48, USBH_DeAllocate_AllChannel 0x08034A66 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host channels, usbh_hcs.c: allocating, opening and freeing the controller's eight host
   channels.
   Library: ST's STM32 USB OTG host library (STM32_USB-Host-Device_Lib), V2.2.0 or close to
   it: the host states (HOST_WAIT_PRT_ENABLED = 1 ... HOST_ERROR_STATE = 12) and the port-enabled
   callbacks are V2.2.0's; docs/matching.md (round 5) compared usbh_stdreq.c with V2.1.0, which
   has the same functions. I have no copy of the release here, so "unchanged" below means that
   the code has ST's shape and the map's analysis found nothing of GivEnergy's in it.
   Job in this firmware: the STM32F105's USB OTG FS port, as a host only (USE_USB_OTG_FS,
   USE_HOST_MODE, no DMA). task_usb (src/app/usb/usb_task.c) runs USBH_Process. Two classes are
   linked: CDC for the communications dongle, which carries Modbus RTU (modbus_slave_process
   receives through CDC_ReceiveData and answers with CDC_SendData), and MSC for a USB stick with
   firmware update files (FatFs, usbh_msc_fatfs.c). The host starts with the CDC class; when a
   device's first interface is class 8, USBH_USR_Configuration_DescAvailable sets
   usb_class_code and task_usb starts the host again with the MSC class.
   Changes from ST: none found.
   match: this file declares USB_OTG_HC_Init with no parameters (the OWN_ define keeps the
   header's prototype out): it was compiled against that declaration, and the full prototype
   changes the call. USBH_GetFreeChannel is static, as in ST: a global callee defined earlier
   would let armcc save fewer registers in USBH_Alloc_Channel.
   name: original (map/filenames.tsv). The old file f08034A48.c held the end of this file and
   the start of usbh_ioreq.c; it was split at 0x08034A82 (2 mod 4, no pool there). */
#define OWN_USB_OTG_HC_Init   /* match: declared below as int(): this file compiled against that, not the header's int(USB_OTG_CORE_HANDLE *,uchar) */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usb_otg_types.h"
#include "../../../include/usb.h"
#include "../../../include/usbh_types.h"

int USB_OTG_HC_Init();
static ushort USBH_GetFreeChannel(USB_OTG_CORE_HANDLE *pdev);

/* Fill host.hc[hc_num] (endpoint, direction, address, type, packet size, speed, toggles
   cleared) and set the channel up. */
uchar USBH_Open_Channel(USB_OTG_CORE_HANDLE *pdev, uchar hc_num, uchar dev_address, uchar speed, uchar ep_type, ushort mps)
{
  pdev->host.hc[hc_num].ep_num = (uchar) pdev->host.channel[hc_num] & 0x7F;
  pdev->host.hc[hc_num].ep_is_in = (pdev->host.channel[hc_num] & 0x80) == 0x80;
  pdev->host.hc[hc_num].dev_addr = dev_address;
  pdev->host.hc[hc_num].ep_type = ep_type;
  pdev->host.hc[hc_num].max_packet = mps;
  pdev->host.hc[hc_num].speed = speed;
  pdev->host.hc[hc_num].toggle_in = 0;
  pdev->host.hc[hc_num].toggle_out = 0;
  if (speed == 0)
  {
    pdev->host.hc[hc_num].do_ping = 1;
  }

  USB_OTG_HC_Init(pdev, hc_num);

  return 0;
}

/* Change the address, speed or packet size of an open channel (0 keeps the old value)
   and set it up again. Used during enumeration. */
int USBH_Modify_Channel(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6)
{

  
  if (param_3 != 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_2 << 5)) = param_3;
  }
  if (*(ushort *)(param_1 + 0x3b0 + (param_2 << 5) + 6) != param_6 && param_6 != 0) {
    *(ushort *)(param_1 + 0x3b0 + (param_2 << 5) + 6) = param_6;
  }
  if (*(uchar *)(param_1 + 0x3b0 + (param_2 << 5) + 3) != param_4 && param_4 != 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_2 << 5) + 3) = param_4;
  }
  USB_OTG_HC_Init(param_1, param_2);
  return 0;
}

/* First channel without the in-use bit (0x8000), or 0xFFFF. */
static ushort USBH_GetFreeChannel(USB_OTG_CORE_HANDLE *pdev)
{
  uchar idx = 0;

  for (idx = 0; idx < 8; idx++)
  {
    if ((pdev->host.channel[idx] & 0x8000) == 0)
    {
      return idx;
    }
  }
  return 0xFFFF;
}

/* Take a free channel for endpoint ep_addr and mark it in use. */
uchar USBH_Alloc_Channel(USB_OTG_CORE_HANDLE *pdev, uchar ep_addr)
{
  ushort hc_num;

  hc_num = USBH_GetFreeChannel(pdev);

  if (hc_num != 0xFFFF)
  {
    pdev->host.channel[hc_num] = 0x8000 | ep_addr;
  }
  return hc_num;
}

/* Clear a channel's in-use bit. */
uint USBH_Free_Channel(int param_1, int param_2)
{

  if (param_2 < 8) {
    *(ushort *)(param_1 + 0x590 + param_2 * 2) =
         (ushort)(((uint)*(ushort *)(param_1 + 0x590 + param_2 * 2) << 0x11) >> 0x11);
  }
  return 0;
}


/* Free channels 2 to 7 (0 and 1 stay with control transfers). */
uint USBH_DeAllocate_AllChannel(int param_1)
{
  int uVar1;
  
  for (uVar1 = 2; uVar1 < 8; uVar1 = uVar1 + 1 & 0xff) {
    *(undefined2 *)(param_1 + 0x590 + uVar1 * 2) = 0;
  }
  return 0;
}
