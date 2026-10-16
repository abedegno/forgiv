/* at: USBH_MSC_InterfaceInit 0x080357A4, USBH_MSC_InterfaceDeInit 0x080358A0, USBH_MSC_ClassRequest 0x080358E8, USBH_MSC_ErrorHandle 0x080358F6, USBH_MSC_GETMaxLUN 0x08035930, USBH_MSC_Handle 0x08035958 */
/* at: MSCErrorCount 0x20000534, maxLunExceed 0x20000548, USBH_MSC_BOTXferParam 0x20007F78, MSC_Machine 0x20007F88 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host mass storage class, usbh_msc_core.c: the class callbacks (USBH_MSC_cb) and the MSC
   state machine.
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
   The class finds the stick's bulk endpoints, asks for the number of LUNs (only LUN 0 is
   used), then TEST UNIT READY, READ CAPACITY and MODE SENSE, and hands over to the user
   application (USBH_USR_MSC_Application in usbh_usr.c), which mounts FatFs.
   Changes from ST: none found.
   match: this file declares USBH_CtlReq as returning uchar (OWN_ define): it was compiled
   against that.
   name: original (map/filenames.tsv). */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_types.h"
#include "../../../include/usbh_msc_types.h"
#define OWN_USBH_CtlReq   /* match: declared below as uchar(void *,USBH_HOST *,uchar *,ushort): this file compiled against that, not the header's int(void *,USBH_HOST *,uchar *,ushort) */
#include "../../../include/usb.h"
uchar USBH_CtlReq(void *, USBH_HOST *, uchar *, ushort);

#define MSC_hc_num_in (*(uchar *)(((uchar *)&MSC_Machine) + 0))
#define MSC_hc_num_out (*(uchar *)(((uchar *)&MSC_Machine) + 1))
#define MSC_MSBulkOutEp (*(uchar *)(((uchar *)&MSC_Machine) + 2))
#define MSC_MSBulkInEp (*(uchar *)(((uchar *)&MSC_Machine) + 3))
#define MSC_MSBulkInEpSize (*(ushort *)(((uchar *)&MSC_Machine) + 4))
#define MSC_MSBulkOutEpSize (*(ushort *)(((uchar *)&MSC_Machine) + 6))

/* Class init: for a mass storage interface (class 8, protocol 0x50 bulk-only) take the
   bulk IN and OUT endpoints, allocate and open their channels; anything else is reported
   as not supported. */
uchar USBH_MSC_InterfaceInit(void *pdev, void *phost)
{
  USBH_HOST *pphost = phost;

  if ((pphost->device_prop.Itf_Desc[0].bInterfaceClass == 0x08) &&
      (pphost->device_prop.Itf_Desc[0].bInterfaceProtocol == 0x50))
  {
    if (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress & 0x80)
    {
      MSC_MSBulkInEp = (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress);
      MSC_MSBulkInEpSize = pphost->device_prop.Ep_Desc[0][0].wMaxPacketSize;
    }
    else
    {
      MSC_MSBulkOutEp = (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress);
      MSC_MSBulkOutEpSize = pphost->device_prop.Ep_Desc[0][0].wMaxPacketSize;
    }

    if (pphost->device_prop.Ep_Desc[0][1].bEndpointAddress & 0x80)
    {
      MSC_MSBulkInEp = (pphost->device_prop.Ep_Desc[0][1].bEndpointAddress);
      MSC_MSBulkInEpSize = pphost->device_prop.Ep_Desc[0][1].wMaxPacketSize;
    }
    else
    {
      MSC_MSBulkOutEp = (pphost->device_prop.Ep_Desc[0][1].bEndpointAddress);
      MSC_MSBulkOutEpSize = pphost->device_prop.Ep_Desc[0][1].wMaxPacketSize;
    }

    MSC_hc_num_out = USBH_Alloc_Channel(pdev, MSC_MSBulkOutEp);
    MSC_hc_num_in = USBH_Alloc_Channel(pdev, MSC_MSBulkInEp);

    USBH_Open_Channel(pdev,
                 MSC_hc_num_out,
                 pphost->device_prop.address,
                 pphost->device_prop.speed,
                 2,
                 MSC_MSBulkOutEpSize);

    USBH_Open_Channel(pdev,
                 MSC_hc_num_in,
                 pphost->device_prop.address,
                 pphost->device_prop.speed,
                 2,
                 MSC_MSBulkInEpSize);
  }
  else
  {
    if ((pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x09) &&
        (pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x02) &&
        (pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x03) &&
        (pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x07) &&
        (pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x0B) &&
        (pphost->device_prop.Itf_Desc[0].bInterfaceClass != 0x0E))
    {
      (void)0;
    }
    pphost->usr_cb->DeviceNotSupported();
  }

  return 0;
}
#undef MSC_hc_num_in
#undef MSC_hc_num_out
#undef MSC_MSBulkOutEp
#undef MSC_MSBulkInEp
#undef MSC_MSBulkInEpSize
#undef MSC_MSBulkOutEpSize


/* Class de-init: halt and free the bulk channels. */
void USBH_MSC_InterfaceDeInit(int param_1, uint param_2)
{

  if ((*(uchar *)(((uchar *)&MSC_Machine) + 1)) != 0) {
    USB_OTG_HC_Halt(param_1,(uint)(*(uchar *)(((uchar *)&MSC_Machine) + 1)));
    USBH_Free_Channel(param_1,(uint)(*(uchar *)(((uchar *)&MSC_Machine) + 1)));
    (*(uchar *)(((uchar *)&MSC_Machine) + 1)) = 0;
  }
  if ((*(uchar *)(((uchar *)&MSC_Machine) + 0)) != 0) {
    USB_OTG_HC_Halt(param_1,(uint)(*(uchar *)(((uchar *)&MSC_Machine) + 0)));
    USBH_Free_Channel(param_1,(uint)(*(uchar *)(((uchar *)&MSC_Machine) + 0)));
    (*(uchar *)(((uchar *)&MSC_Machine) + 0)) = 0;
  }
  return;
}


/* Class requests: nothing to send; start the MSC machine at its first state. */
int USBH_MSC_ClassRequest(int param_1, int p_x1)
{
  int v0;
  
  v0 = 0;
  (*(uchar *)(((uchar *)&USBH_MSC_BOTXferParam) + 0)) = 0;
  return v0;
}


/* After a failed command: up to ten tries through REQUEST SENSE, then unrecoverable. */
void USBH_MSC_ErrorHandle(uchar status)
{
  if (status == 1)
  {
    (*(uchar *)MSCErrorCount)++;
    if ((*(uchar *)MSCErrorCount) < 10)
    {
      USBH_MSC_BOTXferParam.MSCState = 6;
      USBH_MSC_BOTXferParam.CmdStateMachine = 1;
    }
    else
    {
      USBH_MSC_BOTXferParam.MSCState = 10;
    }
  }
  else if (status == 2)
  {
    USBH_MSC_BOTXferParam.MSCState = 10;
  }
  else if (status == 3)
  {
    /* No change in state */
  }
}

/* GET MAX LUN class request (0xFE), one byte into MSC_Machine.buff. */
uchar USBH_MSC_GETMaxLUN(void *pdev, USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = 0x80 | 0x20 | 0x01;

  phost->Control.setup.b.bRequest = 0xFE;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 1;

  return USBH_CtlReq(pdev, phost, MSC_Machine.buff, 1);
}

#define MSC_buff (((uchar *)&MSC_Machine) + 8)
#define MSC_maxLun (*(uchar *)(((uchar *)&MSC_Machine) + 0x48))

/* MSC state machine, run by USBH_Process in the class state: init, GET MAX LUN, test
   unit ready, read capacity, mode sense, then the user application; errors go through
   REQUEST SENSE. */
uchar USBH_MSC_Handle(void *pdev, void *phost)
{
  USBH_HOST *pphost = phost;

  uchar status = 1;
  uchar mscStatus = 3;
  uchar appliStatus = 0;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.MSCState)
    {
    case 0:
      USBH_MSC_Init(pdev);
      USBH_MSC_BOTXferParam.MSCState = 2;
      break;

    case 2:
      status = USBH_MSC_GETMaxLUN(pdev, phost);

      if (status == 0)
      {
        MSC_maxLun = *(MSC_buff);

        if ((MSC_maxLun > 0) && ((*(uchar *)maxLunExceed) == 0))
        {
          (*(uchar *)maxLunExceed) = 1;
          pphost->usr_cb->DeviceNotSupported();

          break;
        }

        USBH_MSC_BOTXferParam.MSCState = 3;
      }

      if (status == 3)
      {
        USBH_MSC_BOTXferParam.MSCStateBkp = 3;
        USBH_MSC_BOTXferParam.MSCState = 9;
      }
      break;

    case 9:
      status = USBH_ClrFeature(pdev,
                            phost,
                            0x00,
                            pphost->Control.hc_num_out);
      if (status == 0)
      {
        MSC_maxLun = 0;
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateBkp;
      }
      break;

    case 3:
      mscStatus = USBH_MSC_TestUnitReady(pdev);

      if (mscStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = 4;
        (*(uchar *)MSCErrorCount) = 0;
        status = 0;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;

    case 4:
      mscStatus = USBH_MSC_ReadCapacity10(pdev);
      if (mscStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = 5;
        (*(uchar *)MSCErrorCount) = 0;
        status = 0;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;

    case 5:
      mscStatus = USBH_MSC_ModeSense6(pdev);
      if (mscStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = 8;
        (*(uchar *)MSCErrorCount) = 0;
        status = 0;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;

    case 7:
      USBH_MSC_HandleBOTXfer(pdev, phost);
      break;

    case 8:
      appliStatus = pphost->usr_cb->UserApplication();
      if (appliStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = 8;
      }
      else if (appliStatus == 1)
      {
        status = 6;
      }
      break;

    case 10:
      status = 4;
      break;

    case 6:
      mscStatus = USBH_MSC_RequestSense(pdev);
      if (mscStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateBkp;
        status = 0;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;

    default:
      break;
    }
  }
  return status;
}
#undef MSC_buff
#undef MSC_maxLun

