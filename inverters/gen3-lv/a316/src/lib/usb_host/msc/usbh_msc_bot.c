/* at: USBH_MSC_Init 0x08035288, USBH_MSC_BOT_Abort 0x080352B4, USBH_MSC_DecodeCSW 0x0803530A, USBH_MSC_HandleBOTXfer 0x08035376 */
/* at: BOTStallErrorCount 0x20000514, BOTErrorCount 0x20000518, remainingDataLength 0x2000051C, datapointer 0x20000520, datapointer_prev 0x20000524, error_direction 0x20000528, MSCErrorCount 0x20000534, USBH_MSC_CBWData 0x20007F48, USBH_MSC_CSWData 0x20007F68, USBH_MSC_BOTXferParam 0x20007F78, MSC_Machine 0x20007F88 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host mass storage Bulk-Only Transport, usbh_msc_bot.c: command block, data and status
   stages of each SCSI command.
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
   match: this file declares USBH_BulkSendData and USBH_BulkReceiveData as returning uchar
   (OWN_ defines): it was compiled against those declarations.
   name: original (map/filenames.tsv). */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_msc_types.h"
#define OWN_USBH_BulkReceiveData   /* match: declared below as uchar(void *,uchar *,ushort,uchar): this file compiled against that, not the header's int(int,int,int,int) */
#define OWN_USBH_BulkSendData   /* match: declared below as uchar(void *,uchar *,ushort,uchar): this file compiled against that, not the header's int(int,int,int,int) */
#include "../../../include/usb.h"
uchar USBH_BulkSendData(void *, uchar *, ushort, uchar);
uchar USBH_BulkReceiveData(void *, uchar *, ushort, uchar);


/* Prepare the command block wrapper (signature USBC, tag 0x20304050, LUN 0) and clear the
   error counts. */
void USBH_MSC_Init(void *pdev)
{
  if (HCD_IsDeviceConnected(pdev))
  {
    USBH_MSC_CBWData.field.dCBWSignature = 0x43425355;
    USBH_MSC_CBWData.field.dCBWTag = 0x20304050;
    USBH_MSC_CBWData.field.bCBWLUN = 0;
    USBH_MSC_BOTXferParam.CmdStateMachine = 1;
  }

  BOTStallErrorCount = 0;
  MSCErrorCount[0] = 0;
}


/* Clear a stalled bulk endpoint (direction 0 IN, 1 OUT); after more than four stalls
   report the device as unrecoverable. */
uchar USBH_MSC_BOT_Abort(void *pdev, void *phost, uchar direction)
{
  uchar status;

  status = 1;

  switch (direction)
  {
  case 0:
    status = USBH_ClrFeature(pdev,
                          phost,
                          MSC_Machine.MSBulkInEp,
                          MSC_Machine.hc_num_in);
    break;

  case 1:
    status = USBH_ClrFeature(pdev,
                          phost,
                          MSC_Machine.MSBulkOutEp,
                          MSC_Machine.hc_num_out);
    break;

  default:
    break;
  }

  BOTStallErrorCount++;
  if (BOTStallErrorCount > 4)
  {
    status = 4;
  }

  return status;
}

/* Check the 13-byte command status wrapper (signature USBS, tag) and return its status:
   0 passed, 1 failed, 2 phase error. */
uchar USBH_MSC_DecodeCSW(void *pdev, void *phost)
{
  uchar status;
  uint dataXferCount = 0;
  status = 1;

  if (HCD_IsDeviceConnected(pdev))
  {
    dataXferCount = HCD_GetXferCnt(pdev, MSC_Machine.hc_num_in);

    if (dataXferCount != 13)
    {
      status = 2;
    }
    else
    {
      if (USBH_MSC_CSWData.field.dCSWSignature == 0x53425355)
      {
        if (USBH_MSC_CSWData.field.dCSWTag == USBH_MSC_CBWData.field.dCBWTag)
        {
          if (USBH_MSC_CSWData.field.bCSWStatus == 0)
          {
            status = 0;
          }
          else if (USBH_MSC_CSWData.field.bCSWStatus == 1)
          {
            status = 1;
          }
          else if (USBH_MSC_CSWData.field.bCSWStatus == 2)
          {
            status = 2;
          }
        }
      }
      else
      {
        status = 2;
      }
    }
  }

  USBH_MSC_BOTXferParam.BOTXferStatus = status;
  return status;
}


/* Bulk-Only Transport state machine: send the CBW, move the data in or out a packet at
   a time, read the CSW, and recover from stalls. */
void USBH_MSC_HandleBOTXfer(void *pdev, void *phost)
{
  uchar xferDirection, index;
  uchar status;
  uchar URB_Status = 0;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.BOTState)
    {
    case 1:
      USBH_BulkSendData(pdev,
                   &USBH_MSC_CBWData.CBWArray[0],
                   31,
                   MSC_Machine.hc_num_out);

      USBH_MSC_BOTXferParam.BOTStateBkp = 1;
      USBH_MSC_BOTXferParam.BOTState = 2;
      BOTErrorCount = 0;
      break;

    case 2:
      URB_Status = HCD_GetURB_State(pdev, MSC_Machine.hc_num_out);

      if (URB_Status == 1)
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = 2;

        xferDirection = (USBH_MSC_CBWData.field.bmCBWFlags & 0x80);

        if (USBH_MSC_CBWData.field.dCBWDataTransferLength != 0)
        {
          remainingDataLength = USBH_MSC_CBWData.field.dCBWDataTransferLength;
          datapointer = USBH_MSC_BOTXferParam.pRxTxBuff;
          datapointer_prev = datapointer;

          if (xferDirection == 0x80)
          {
            USBH_MSC_BOTXferParam.BOTState = 3;
          }
          else
          {
            USBH_MSC_BOTXferParam.BOTState = 4;
          }
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTState = 5;
        }
      }
      else if (URB_Status == 2)
      {
        USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
      }
      else if (URB_Status == 4)
      {
        error_direction = 1;
        USBH_MSC_BOTXferParam.BOTState = 8;
      }
      else if (URB_Status == 3)
      {
        BOTErrorCount++;
        if (BOTErrorCount < 3)
        {
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTXferStatus = 2;
        }
      }
      break;

    case 3:
      URB_Status = HCD_GetURB_State(pdev, MSC_Machine.hc_num_in);
      if ((URB_Status == 1) || (USBH_MSC_BOTXferParam.BOTStateBkp != 3))
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = 3;

        if (MSC_Machine.MSBulkInEpSize < remainingDataLength)
        {
          USBH_BulkReceiveData(pdev,
                       datapointer,
                       MSC_Machine.MSBulkInEpSize,
                       MSC_Machine.hc_num_in);

          remainingDataLength -= MSC_Machine.MSBulkInEpSize;
          datapointer = datapointer + MSC_Machine.MSBulkInEpSize;
        }
        else if (remainingDataLength == 0)
        {
          USBH_MSC_BOTXferParam.BOTState = 5;
        }
        else
        {
          USBH_BulkReceiveData(pdev,
                       datapointer,
                       remainingDataLength,
                       MSC_Machine.hc_num_in);

          remainingDataLength = 0;
        }
      }
      else if (URB_Status == 4)
      {
        error_direction = 0;
        USBH_MSC_BOTXferParam.BOTState = 7;
        USBH_MSC_BOTXferParam.BOTStateBkp = 5;
      }
      else if (URB_Status == 3)
      {
        BOTErrorCount++;
        if (BOTErrorCount < 3)
        {
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTXferStatus = 2;
        }
      }
      break;

    case 4:
      URB_Status = HCD_GetURB_State(pdev, MSC_Machine.hc_num_out);
      if (URB_Status == 1)
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = 4;
        if (MSC_Machine.MSBulkOutEpSize < remainingDataLength)
        {
          USBH_BulkSendData(pdev,
                       datapointer,
                       MSC_Machine.MSBulkOutEpSize,
                       MSC_Machine.hc_num_out);
          datapointer_prev = datapointer;
          datapointer = datapointer + MSC_Machine.MSBulkOutEpSize;

          remainingDataLength = remainingDataLength - MSC_Machine.MSBulkOutEpSize;
          BOTErrorCount = 0;
        }
        else if (remainingDataLength == 0)
        {
          USBH_MSC_BOTXferParam.BOTState = 5;
        }
        else
        {
          USBH_BulkSendData(pdev,
                       datapointer,
                       remainingDataLength,
                       MSC_Machine.hc_num_out);

          remainingDataLength = 0;
        }
      }
      else if (URB_Status == 2)
      {
        if (datapointer != datapointer_prev)
        {
          USBH_BulkSendData(pdev,
                       (datapointer - MSC_Machine.MSBulkOutEpSize),
                       MSC_Machine.MSBulkOutEpSize,
                       MSC_Machine.hc_num_out);
        }
        else
        {
          USBH_BulkSendData(pdev,
                       datapointer,
                       MSC_Machine.MSBulkOutEpSize,
                       MSC_Machine.hc_num_out);
        }
      }
      else if (URB_Status == 4)
      {
        error_direction = 1;
        USBH_MSC_BOTXferParam.BOTState = 8;
        USBH_MSC_BOTXferParam.BOTStateBkp = 5;
      }
      else if (URB_Status == 3)
      {
        BOTErrorCount++;
        if (BOTErrorCount < 3)
        {
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTXferStatus = 2;
        }
      }
      break;

    case 5:
      USBH_MSC_BOTXferParam.BOTStateBkp = 5;

      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_MSC_CSWData.CSWArray;
      USBH_MSC_BOTXferParam.DataLength = 63;

      for (index = 12; index != 0; index--)
      {
        USBH_MSC_CSWData.CSWArray[index] = 0;
      }

      USBH_MSC_CSWData.CSWArray[0] = 0;

      USBH_BulkReceiveData(pdev,
                   USBH_MSC_BOTXferParam.pRxTxBuff,
                   63,
                   MSC_Machine.hc_num_in);
      USBH_MSC_BOTXferParam.BOTState = 6;
      BOTErrorCount = 0;
      break;

    case 6:
      URB_Status = HCD_GetURB_State(pdev, MSC_Machine.hc_num_in);
      if (URB_Status == 1)
      {
        BOTStallErrorCount = 0;
        USBH_MSC_BOTXferParam.BOTStateBkp = 5;

        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateCurrent;

        USBH_MSC_BOTXferParam.BOTXferStatus = USBH_MSC_DecodeCSW(pdev, phost);
      }
      else if (URB_Status == 4)
      {
        error_direction = 0;
        USBH_MSC_BOTXferParam.BOTState = 7;
      }
      else if (URB_Status == 3)
      {
        BOTErrorCount++;
        if (BOTErrorCount < 3)
        {
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTXferStatus = 2;
        }
      }
      break;

    case 7:
      status = USBH_MSC_BOT_Abort(pdev, phost, 0);
      if (status == 0)
      {
        if (error_direction == 2)
        {
          USBH_MSC_BOTXferParam.BOTState = 8;
        }
        else
        {
          USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
        }
      }
      else if (status == 4)
      {
        USBH_MSC_BOTXferParam.BOTXferStatus = 2;
      }
      break;

    case 8:
      status = USBH_MSC_BOT_Abort(pdev, phost, 1);
      if (status == 0)
      {
        USBH_MSC_BOTXferParam.BOTState = USBH_MSC_BOTXferParam.BOTStateBkp;
      }
      else if (status == 4)
      {
        USBH_MSC_BOTXferParam.BOTXferStatus = 2;
      }
      break;

    default:
      break;
    }
  }
}

