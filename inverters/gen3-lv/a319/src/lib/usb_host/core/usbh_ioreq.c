/* at: USBH_SubmitSetupRequest 0x08035302, USBH_CtlReq 0x0803531A, USBH_CtlSendSetup 0x0803537E */
/* at: USBH_CtlSendData 0x080353C0, USBH_CtlReceiveData 0x0803542E */
/* at: USBH_BulkSendData 0x08035474, USBH_BulkReceiveData 0x080354D2, USBH_InterruptReceiveData 0x08035532, USBH_InterruptSendData 0x080355AA */
/* at: USBH_IsocReceiveData 0x08035620, USBH_IsocSendData 0x08035666 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host I/O requests, usbh_ioreq.c: control, bulk, interrupt and isochronous transfers on
   a host channel.
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
   name: original (map/filenames.tsv). USBH_SubmitSetupRequest, USBH_CtlReq and
   USBH_CtlSendSetup are the second half of the old f08034A48.c (from 0x08035302, where
   usbh_hcs.c ends); four pieces joined, and the joined file compiles to the same bytes. */
#include "../../../include/firmware_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_types.h"
#include "../../../include/usb.h"

/* Start a control request: remember the state, go to HOST_CTRL_XFER (8) and set up
   the buffer for USBH_HandleControl. */
int USBH_SubmitSetupRequest(USBH_HOST *phost, uchar *buff, ushort length)
{
  phost->gStateBkp = phost->gState;
  phost->gState = 8;
  phost->Control.buff = buff;
  phost->Control.length = length;
  phost->Control.state = 1;
  return 0;
}

/* Run a control request over several calls: 1 while busy, 0 when done, 2 on error, 3 on
   a stall. */
int USBH_CtlReq(void *pdev, USBH_HOST *phost, uchar *buff, ushort length)
{
  int status;
  status = 1;
  switch (phost->RequestState)
  {
  case 1:
    USBH_SubmitSetupRequest(phost, buff, length);
    phost->RequestState = 2;
    status = 1;
    break;
  case 2:
    if (phost->Control.state == 13)
    {
      phost->RequestState = 1;
      phost->Control.state = 0;
      status = 0;
    }
    else if (phost->Control.state == 11)
    {
      phost->RequestState = 1;
      status = 2;
    }
    else if (phost->Control.state == 12)
    {
      phost->RequestState = 1;
      status = 3;
    }
    break;
  default:
    break;
  }
  return status;
}

/* Send the 8-byte SETUP packet on the control OUT channel. */
uchar USBH_CtlSendSetup(int param_1, int param_2, int param_3)
{


  *(uchar *)(param_1 + 0x3b0 + (param_3 << 5) + 2) = 0;
  *(uchar *)(param_1 + 0x3b0 + (param_3 << 5) + 8) = 3;
  *(int *)(param_1 + 0x3b0 + (param_3 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_3 << 5) + 0x10) = 8;
  return (uchar)HCD_SubmitRequest(param_1, param_3);
}

/* Send a control OUT data packet: PID from the channel's OUT toggle, which a zero-length
   status packet forces to DATA1. */
int USBH_CtlSendData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 0;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  if (param_3 == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x19) = 1;
  }
  if (*(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x19) == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 0;
  }
  else {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  }
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Receive a control IN data packet (DATA1). */
int USBH_CtlReceiveData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 1;
  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Start a bulk OUT transfer with the channel's current toggle. */
int USBH_BulkSendData(int param_1, int param_2, int param_3, int param_4)
{
  int v1;

  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = (0);
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  if (*(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x19) == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = (0);
  }
  else {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  }
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Start a bulk IN transfer with the channel's current toggle. */
int USBH_BulkReceiveData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 1;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  if (*(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 0;
  }
  else {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  }
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Start an interrupt IN transfer and flip the toggle. */
int USBH_InterruptReceiveData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 1;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  if (*(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 0;
  }
  else {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  }
  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) = *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) ^ 1;
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}


/* Start an interrupt OUT transfer and flip the toggle. */
int USBH_InterruptSendData(int param_1, int param_2, int param_3, int param_4)
{
  int v1;

  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = (0);
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  if (*(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) == 0) {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = (0);
  }
  else {
    *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 2;
  }
  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) = *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 0x18) ^ 1;
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Start an isochronous IN transfer (DATA0). No callers. */
int USBH_IsocReceiveData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 1;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 0;
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}

/* Start an isochronous OUT transfer (DATA0). No callers. */
int USBH_IsocSendData(int param_1, int param_2, int param_3, int param_4)
{


  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 2) = 0;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0xc) = param_2;
  *(int *)(param_1 + 0x3b0 + (param_4 << 5) + 0x10) = param_3;
  *(uchar *)(param_1 + 0x3b0 + (param_4 << 5) + 8) = 0;
  HCD_SubmitRequest(param_1, param_4);
  return 0;
}
