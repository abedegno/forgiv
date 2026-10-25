/* at: USBH_MSC_TestUnitReady 0x08036514, USBH_MSC_ReadCapacity10 0x080365B6, USBH_MSC_ModeSense6 0x08036680, USBH_MSC_RequestSense 0x08036740, USBH_MSC_Write10 0x08036810, USBH_MSC_Read10 0x080368EA */
/* at: msc_scsi_status 0x20000556, USBH_MSC_CBWData 0x2000818C, USBH_MSC_CSWData 0x200081AC, USBH_MSC_BOTXferParam 0x200081BC, USBH_MSC_Param 0x20008218, USBH_DataInBuffer 0x20008228 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host mass storage SCSI commands, usbh_msc_scsi.c. Each function runs over several calls:
   the first fills the command block and starts the transfer, later calls return the result
   when USBH_MSC_HandleBOTXfer has finished (3 busy, 0 done, 1 failed, 2 phase error).
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
   match: in USBH_MSC_Read10 the status is a static variable in RAM (msc_scsi_status), where
   USBH_MSC_Write10 has a local; the #define keeps the local name.
   name: original (map/filenames.tsv). */
#include "../../../include/firmware_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_msc_types.h"
#include "../../../include/usb.h"

/* TEST UNIT READY (opcode 0x00). */
uchar USBH_MSC_TestUnitReady(void *pdev)
{
  uchar index;
  uchar status = 3;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = 0;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x00;
      USBH_MSC_CBWData.field.bCBWLength = 6;
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_MSC_CSWData.CSWArray;
      USBH_MSC_BOTXferParam.DataLength = 63;
      USBH_MSC_BOTXferParam.MSCStateCurrent = 3;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x00;
      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;
      break;

    case 2:
      if (USBH_MSC_BOTXferParam.BOTXferStatus == 0)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 1)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}

/* READ CAPACITY (10) (0x25): block count and block size into USBH_MSC_Param. */
uchar USBH_MSC_ReadCapacity10(void *pdev)
{
  uchar index;
  uchar status = 3;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = 8;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x80;
      USBH_MSC_CBWData.field.bCBWLength = 10;
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateCurrent = 4;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x25;
      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;
      break;

    case 2:
      if (USBH_MSC_BOTXferParam.BOTXferStatus == 0)
      {
        (((uchar *)&USBH_MSC_Param.MSCapacity)[3]) = USBH_DataInBuffer[0];
        (((uchar *)&USBH_MSC_Param.MSCapacity)[2]) = USBH_DataInBuffer[1];
        (((uchar *)&USBH_MSC_Param.MSCapacity)[1]) = USBH_DataInBuffer[2];
        (((uchar *)&USBH_MSC_Param.MSCapacity)[0]) = USBH_DataInBuffer[3];

        (((uchar *)&USBH_MSC_Param.MSPageLength)[1]) = USBH_DataInBuffer[6];
        (((uchar *)&USBH_MSC_Param.MSPageLength)[0]) = USBH_DataInBuffer[7];

        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 1)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}

/* MODE SENSE (6) (0x1A): keeps the write-protect bit. */
uchar USBH_MSC_ModeSense6(void *pdev)
{
  uchar index;
  uchar status = 3;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = 63;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x80;
      USBH_MSC_CBWData.field.bCBWLength = 10;
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateCurrent = 5;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x1a;
      USBH_MSC_CBWData.field.CBWCB[2] = 0x00 | 0x3f;
      USBH_MSC_CBWData.field.CBWCB[4] = 63;
      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;
      break;

    case 2:
      if (USBH_MSC_BOTXferParam.BOTXferStatus == 0)
      {
        if (USBH_DataInBuffer[2] & 0x80)
        {
          USBH_MSC_Param.MSWriteProtect = 1;
        }
        else
        {
          USBH_MSC_Param.MSWriteProtect = 0;
        }

        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 1)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}

/* REQUEST SENSE (0x03) after a failed command. */
uchar USBH_MSC_RequestSense(void *pdev)
{
  uchar index;
  uchar status = 3;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = 63;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x80;
      USBH_MSC_CBWData.field.bCBWLength = 10;
      USBH_MSC_BOTXferParam.pRxTxBuff = USBH_DataInBuffer;
      USBH_MSC_BOTXferParam.MSCStateBkp = USBH_MSC_BOTXferParam.MSCStateCurrent;
      USBH_MSC_BOTXferParam.MSCStateCurrent = 6;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x03;
      USBH_MSC_CBWData.field.CBWCB[1] = 0x00;
      USBH_MSC_CBWData.field.CBWCB[4] = 63;
      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;
      break;

    case 2:
      if (USBH_MSC_BOTXferParam.BOTXferStatus == 0)
      {
        (((uchar *)&USBH_MSC_Param.MSSenseKey)[3]) = USBH_DataInBuffer[0];
        (((uchar *)&USBH_MSC_Param.MSSenseKey)[2]) = USBH_DataInBuffer[1];
        (((uchar *)&USBH_MSC_Param.MSSenseKey)[1]) = USBH_DataInBuffer[2];
        (((uchar *)&USBH_MSC_Param.MSSenseKey)[0]) = USBH_DataInBuffer[3];

        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 1)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}

/* WRITE (10) (0x2A): nbOfbytes from dataBuffer at block address. */
uchar USBH_MSC_Write10(void *pdev, uchar *dataBuffer, uint address, uint nbOfbytes)
{
  uchar index;
  uchar status = 3;
  ushort nbOfPages;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = nbOfbytes;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x00;
      USBH_MSC_CBWData.field.bCBWLength = 10;
      USBH_MSC_BOTXferParam.pRxTxBuff = dataBuffer;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x2a;

      USBH_MSC_CBWData.field.CBWCB[2] = (((uchar *)&address)[3]);
      USBH_MSC_CBWData.field.CBWCB[3] = (((uchar *)&address)[2]);
      USBH_MSC_CBWData.field.CBWCB[4] = (((uchar *)&address)[1]);
      USBH_MSC_CBWData.field.CBWCB[5] = (((uchar *)&address)[0]);

      nbOfPages = nbOfbytes / 512;

      USBH_MSC_CBWData.field.CBWCB[7] = (((uchar *)&nbOfPages)[1]);
      USBH_MSC_CBWData.field.CBWCB[8] = (((uchar *)&nbOfPages)[0]);

      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;

      break;

    case 2:
      if (USBH_MSC_BOTXferParam.BOTXferStatus == 0)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 1)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}

#define status (*(uchar *)msc_scsi_status)

/* READ (10) (0x28): nbOfbytes into dataBuffer from block address. */
uchar USBH_MSC_Read10(void *pdev, uchar *dataBuffer, uint address, uint nbOfbytes)
{
  uchar index;
  ushort nbOfPages;

  status = 3;

  if (HCD_IsDeviceConnected(pdev))
  {
    switch (USBH_MSC_BOTXferParam.CmdStateMachine)
    {
    case 1:
      USBH_MSC_CBWData.field.dCBWDataTransferLength = nbOfbytes;
      USBH_MSC_CBWData.field.bmCBWFlags = 0x80;
      USBH_MSC_CBWData.field.bCBWLength = 10;
      USBH_MSC_BOTXferParam.pRxTxBuff = dataBuffer;

      for (index = 15; index != 0; index--)
      {
        USBH_MSC_CBWData.field.CBWCB[index] = 0x00;
      }

      USBH_MSC_CBWData.field.CBWCB[0] = 0x28;

      USBH_MSC_CBWData.field.CBWCB[2] = (((uchar *)&address)[3]);
      USBH_MSC_CBWData.field.CBWCB[3] = (((uchar *)&address)[2]);
      USBH_MSC_CBWData.field.CBWCB[4] = (((uchar *)&address)[1]);
      USBH_MSC_CBWData.field.CBWCB[5] = (((uchar *)&address)[0]);

      nbOfPages = nbOfbytes / 512;

      USBH_MSC_CBWData.field.CBWCB[7] = (((uchar *)&nbOfPages)[1]);
      USBH_MSC_CBWData.field.CBWCB[8] = (((uchar *)&nbOfPages)[0]);

      USBH_MSC_BOTXferParam.BOTState = 1;
      USBH_MSC_BOTXferParam.MSCState = 7;
      USBH_MSC_BOTXferParam.BOTXferStatus = 3;
      USBH_MSC_BOTXferParam.CmdStateMachine = 2;

      status = 3;

      break;

    case 2:
      if ((USBH_MSC_BOTXferParam.BOTXferStatus == 0) && (HCD_IsDeviceConnected(pdev)))
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 0;
      }
      else if ((USBH_MSC_BOTXferParam.BOTXferStatus == 1) && (HCD_IsDeviceConnected(pdev)))
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
      }
      else if (USBH_MSC_BOTXferParam.BOTXferStatus == 2)
      {
        USBH_MSC_BOTXferParam.CmdStateMachine = 1;
        status = 2;
      }
      break;

    default:
      break;
    }
  }
  return status;
}
#undef status
