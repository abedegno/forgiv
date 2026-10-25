/* at: HCD_Init 0x080339EE, HCD_GetCurrentSpeed 0x08033A4E, HCD_ResetPort 0x08033A62 */
/* at: HCD_IsDeviceConnected 0x08033A70 */
/* at: HCD_IsPortEnabled 0x08033A78 */
/* at: HCD_GetCurrentFrame 0x08033A80, HCD_GetURB_State 0x08033A8A */
/* at: HCD_GetXferCnt 0x08033A94 */
/* at: HCD_GetHCState 0x08033AA0, HCD_HC_Init 0x08033AAA, HCD_SubmitRequest 0x08033ABA */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host controller layer, usb_hcd.c: the thin API the host library uses over usb_core.c.
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
   name: original (map/filenames.tsv). HCD_Init, HCD_GetCurrentSpeed and HCD_ResetPort are the
   second half of the old f08032550.c (from 0x080339EE, where usb_core.c ends); six pieces
   joined, and the joined file compiles to the same bytes. */
#define OWN_USB_OTG_HC_Init   /* match: declared below as void(int,uint): this file compiled against that, not the header's int(USB_OTG_CORE_HANDLE *,uchar) */
#define OWN_USB_OTG_HC_StartXfer   /* match: declared below as int(int,int): this file compiled against that, not the header's int(USB_OTG_CORE_HANDLE *,uchar) */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/usb_otg_types.h"
#include "../../include/usb.h"

/* Clear the channel counters and states, select and initialise the core, force host
   mode and set the host up (USB_OTG_CoreInitHost); enable the global interrupt. */
uint HCD_Init(USB_OTG_CORE_HANDLE *pdev, int coreID)
{
  uchar i = 0;
  pdev->host.ConnSts = 0;

  for (i = 0; i < 15; i++)
  {
    pdev->host.ErrCnt[i] = 0;
    pdev->host.XferCnt[i] = 0;
    pdev->host.HC_Status[i] = 0;
  }
  pdev->host.hc[0].max_packet = 8;

  USB_OTG_SelectCore(pdev, coreID);
  USB_OTG_DisableGlobalInt(pdev);
  USB_OTG_CoreInit(pdev);

  USB_OTG_SetCurrentMode(pdev, 1);
  USB_OTG_CoreInitHost(pdev);
  USB_OTG_EnableGlobalInt(pdev);

  return 0;
}

/* Speed of the device on the root port (HPRT0.PRTSPD). */
int HCD_GetCurrentSpeed(int param_1)
{
  volatile int local_0;

  local_0 = *(int *)(*(int *)(param_1 + 0xcc));
  return (uint)local_0 >> 0x11 & 3;
}

/* Reset the root port (USB_OTG_ResetPort); returns 0. */
uint HCD_ResetPort(int param_1)
{

  USB_OTG_ResetPort(param_1);
  return 0;
}

/* Non-zero while a device is connected (host.ConnSts). */
uint HCD_IsDeviceConnected(int param_1)
{

  return *(uint *)(param_1 + 0x310);
}

/* Non-zero while the root port is enabled (host.PortEnabled). */
uint HCD_IsPortEnabled(int param_1)
{

  return *(uint *)(param_1 + 0x314);
}

/* Current frame number (HFNUM). */
uint HCD_GetCurrentFrame(int param_1)
{

  return *(volatile uint *)(*(volatile int *)(param_1 + 0x14) + 8) & 0xffff;
}

/* Request state of a channel: idle, done, not ready, error, stall. */
uchar HCD_GetURB_State(int param_1, int param_2)
{

  return *(uchar *)(param_1 + 0x39f + param_2);
}

/* Bytes moved by a channel's last transfer. */
uint HCD_GetXferCnt(int param_1, int param_2)
{

  return *(uint *)(param_1 + 0x354 + param_2 * 4);
}

void USB_OTG_HC_Init(int, uint);
int USB_OTG_HC_StartXfer(int, int);

/* Channel state (HC_IDLE, HC_XFRC, HC_NAK ...). */
uchar HCD_GetHCState(int param_1, int param_2)
{

  return *(uchar *)(param_1 + 0x390 + param_2);
}

/* Set up a host channel (USB_OTG_HC_Init). */
void HCD_HC_Init(int param_1, uint param_2)
{

  USB_OTG_HC_Init(param_1,param_2);
  return;
}

/* Start the transfer prepared in host.hc[hc_num] (USB_OTG_HC_StartXfer). */
int HCD_SubmitRequest(int param_1, int param_2)
{


  *(uchar *)(param_1 + 0x39f + param_2) = 0;
  *(int *)(param_1 + 0x39f + 0x11 + (param_2 << 5) + 0x14) = 0;
  return USB_OTG_HC_StartXfer(param_1, param_2);
}
