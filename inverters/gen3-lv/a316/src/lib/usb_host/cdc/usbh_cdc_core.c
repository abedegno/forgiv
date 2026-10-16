/* at: CDC_InitTxRxParam 0x08036188, CDC_InterfaceInit 0x080361AA, CDC_InterfaceDeInit 0x080362EE, CDC_ClassRequest 0x08036356, CDC_ReceiveData 0x080363E8, CDC_ProcessReception 0x08036422, CDC_ProcessTransmission 0x080364D2, CDC_Handle 0x080365CE, CDC_SendData 0x080365F8, CDC_StartReception 0x08036662, CDC_StopReception 0x0803666A */
/* at: cfg_changed_flags 0x2000019C, modbus_src 0x2000030B, usb_cdc_timer 0x20000310, CDC_ReqState 0x2000054C, UserCb 0x20000550, RX_Enabled 0x20000558, cdc_rx_count 0x2000056C, cdc_tx_len 0x20000570, cdc_tx_busy_count 0x20000574, cdc_tx_stalled 0x20000575, CDC_Machine 0x200083E4, CDC_TxParam 0x20008408, CDC_RxParam 0x20008420, TxBuf 0x20008438, RxBuf 0x2000862C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host CDC class, usbh_cdc_core.c: the class callbacks (CDC_cb) and the bulk transmit and
   receive machines for a CDC-ACM serial device.
   Library: the CDC host class written for ST's USB OTG host library (usbh_cdc_core.c,
   usbh_cdc_funct.c, with CDC_Machine, CDC_TxParam and CDC_RxParam). It is not one of the classes
   in ST's library release as far as I know; I have no reference copy, so the changes below are those that do not
   fit the class's own logic, found by the map's analysis.
   Job in this firmware: the STM32F105's USB OTG FS port, as a host only (USE_USB_OTG_FS,
   USE_HOST_MODE, no DMA). task_usb (src/app/usb/usb_task.c) runs USBH_Process. Two classes are
   linked: CDC for the communications dongle, which carries Modbus RTU (modbus_slave_process
   receives through CDC_ReceiveData and answers with CDC_SendData), and MSC for a USB stick with
   firmware update files (FatFs, usbh_msc_fatfs.c). The host starts with the CDC class; when a
   device's first interface is class 8, USBH_USR_Configuration_DescAvailable sets
   usb_class_code and task_usb starts the host again with the MSC class.
   Configuration: a 500-byte receive buffer (RxBuf) and the transmit buffer TxBuf; the line
   coding is 57600 8N1 (usb_cdc_line_coding_init in usb_task.c).
   Vendor changes (GivEnergy):
   - CDC_ReceiveData hands a received block to UserCb.Receive (modbus_slave_process) only when
     modbus_src is 0, sets it to 1 so the Modbus code knows the frame came from USB, and counts
     blocks in cdc_rx_count. The upstream class hands on every block.
   - CDC_SendData counts calls made while the transmitter is still busy (cdc_tx_busy_count).
     After four it sets usb_cdc_timer (the dongle link timer) to 5000, past the limit at which
     sys_tick_1s power-cycles the USB port; if the transmitter is still stuck after that, it
     sets bit 0 of cfg_changed_flags.
   - CDC_ProcessReception clears RxBuf[1] after each block. I take it for GivEnergy's as well.
   match: the volatile len and unused locals are dead stores the image has (docs/matching.md,
   round 4). CDC_ProcessTransmission's len is a static in RAM (cdc_tx_len); the #define gives it
   the local name. This file declares USBH_BulkSendData as returning uchar (OWN_ define).
   name: original (map/filenames.tsv). */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_types.h"
#include "../../../include/usbh_cdc_types.h"
#include "../../../include/usb_otg_types.h"
#define OWN_USBH_BulkSendData   /* match: declared below as uchar(void *,uchar *,ushort,uchar): this file compiled against that, not the header's int(int,int,int,int) */
#include "../../../include/modbus.h"
#include "../../../include/settings.h"
#include "../../../include/usb.h"
uchar USBH_BulkSendData(void *, uchar *, ushort, uchar);

typedef uchar * T8036188_DAT_2000840c;
typedef uchar * T8036188_DAT_20008428;
typedef uchar * T8036188_DAT_2000842c;

/* Reset the transmit and receive state and point them at TxBuf and RxBuf (500 bytes). */
void CDC_InitTxRxParam()
{

  (*(uchar *)(((uchar *)&CDC_TxParam) + 0)) = 0;
  (*(ushort *)(((uchar *)&CDC_TxParam) + 20)) = 0;
  (*(T8036188_DAT_2000840c *)(((uchar *)&CDC_TxParam) + 4)) = &TxBuf[0];
  (*(uchar *)(((uchar *)&CDC_RxParam) + 0)) = 0;
  (*(ushort *)(((uchar *)&CDC_RxParam) + 20)) = 0;
  (*(T8036188_DAT_20008428 *)(((uchar *)&CDC_RxParam) + 8)) = &RxBuf[0];
  (*(T8036188_DAT_2000842c *)(((uchar *)&CDC_RxParam) + 12)) = &RxBuf[0];
  (*(uint *)(((uchar *)&CDC_RxParam) + 16)) = 500;
  return;
}

/* Class init: interface 0 must be CDC ACM (2/2/1): open its notification endpoint;
   interface 1 must be CDC data (0x0A): open its bulk IN and OUT endpoints. */
uchar CDC_InterfaceInit(void *pdev, void *phost)
{
  USBH_HOST *pphost = phost;
  uchar status = 0;

  if ((pphost->device_prop.Itf_Desc[0].bInterfaceClass == 0x02) &&
      (pphost->device_prop.Itf_Desc[0].bInterfaceSubClass == 0x02) &&
      (pphost->device_prop.Itf_Desc[0].bInterfaceProtocol == 0x01))
  {
    CDC_Machine.CDC_CommItf.ep_addr = pphost->device_prop.Ep_Desc[0][0].bEndpointAddress;
    CDC_Machine.CDC_CommItf.length = pphost->device_prop.Ep_Desc[0][0].wMaxPacketSize;

    if (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress & 0x80)
    {
      CDC_Machine.CDC_CommItf.notificationEp =
        (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress);
    }
    CDC_Machine.CDC_CommItf.hc_num_in = USBH_Alloc_Channel(pdev,
                                                     CDC_Machine.CDC_CommItf.notificationEp);

    USBH_Open_Channel(pdev,
                 CDC_Machine.CDC_CommItf.hc_num_in,
                 pphost->device_prop.address,
                 pphost->device_prop.speed,
                 3,
                 CDC_Machine.CDC_CommItf.length);
  }
  else
  {
    pphost->usr_cb->DeviceNotSupported();
  }

  if ((pphost->device_prop.Itf_Desc[1].bInterfaceClass == 0x0A) &&
      (pphost->device_prop.Itf_Desc[1].bInterfaceSubClass == 0x00) &&
      (pphost->device_prop.Itf_Desc[1].bInterfaceProtocol == 0x00))
  {
    CDC_Machine.CDC_DataItf.ep_addr = pphost->device_prop.Ep_Desc[1][0].bEndpointAddress;
    CDC_Machine.CDC_DataItf.length = pphost->device_prop.Ep_Desc[1][0].wMaxPacketSize;

    if (pphost->device_prop.Ep_Desc[1][0].bEndpointAddress & 0x80)
    {
      CDC_Machine.CDC_DataItf.cdcInEp = (pphost->device_prop.Ep_Desc[1][0].bEndpointAddress);
    }
    else
    {
      CDC_Machine.CDC_DataItf.cdcOutEp = (pphost->device_prop.Ep_Desc[1][0].bEndpointAddress);
    }

    if (pphost->device_prop.Ep_Desc[1][1].bEndpointAddress & 0x80)
    {
      CDC_Machine.CDC_DataItf.cdcInEp = (pphost->device_prop.Ep_Desc[1][1].bEndpointAddress);
    }
    else
    {
      CDC_Machine.CDC_DataItf.cdcOutEp = (pphost->device_prop.Ep_Desc[1][1].bEndpointAddress);
    }

    CDC_Machine.CDC_DataItf.hc_num_out = USBH_Alloc_Channel(pdev,
                                                      CDC_Machine.CDC_DataItf.cdcOutEp);
    CDC_Machine.CDC_DataItf.hc_num_in = USBH_Alloc_Channel(pdev,
                                                     CDC_Machine.CDC_DataItf.cdcInEp);

    USBH_Open_Channel(pdev,
                 CDC_Machine.CDC_DataItf.hc_num_out,
                 pphost->device_prop.address,
                 pphost->device_prop.speed,
                 2,
                 CDC_Machine.CDC_DataItf.length);
    USBH_Open_Channel(pdev,
                 CDC_Machine.CDC_DataItf.hc_num_in,
                 pphost->device_prop.address,
                 pphost->device_prop.speed,
                 2,
                 CDC_Machine.CDC_DataItf.length);

    CDC_InitTxRxParam();

    CDC_ReqState = 1;
  }
  else
  {
    pphost->usr_cb->DeviceNotSupported();
  }
  return status;
}

/* Class de-init: halt and free the three channels. */
void CDC_InterfaceDeInit(void *pdev, void *phost)
{
  if (CDC_Machine.CDC_CommItf.hc_num_in)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.CDC_CommItf.hc_num_in);
    USBH_Free_Channel(pdev, CDC_Machine.CDC_CommItf.hc_num_in);
    CDC_Machine.CDC_CommItf.hc_num_in = 0;
  }

  if (CDC_Machine.CDC_DataItf.hc_num_out)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.CDC_DataItf.hc_num_out);
    USBH_Free_Channel(pdev, CDC_Machine.CDC_DataItf.hc_num_out);
    CDC_Machine.CDC_DataItf.hc_num_out = 0;
  }

  if (CDC_Machine.CDC_DataItf.hc_num_in)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.CDC_DataItf.hc_num_in);
    USBH_Free_Channel(pdev, CDC_Machine.CDC_DataItf.hc_num_in);
    CDC_Machine.CDC_DataItf.hc_num_in = 0;
  }
}

/* Class requests, one per call: GET_LINE_CODING then SET_CONTROL_LINE_STATE;
   SET_LINE_CODING when CDC_ChangeStateToIssueSetConfig asks for it; clear a stall. */
uchar CDC_ClassRequest(void *pdev, void *phost)
{
  USBH_HOST *pphost = phost;
  uchar status = 1;
  uchar ClassReqStatus = 1;

  switch (CDC_ReqState)
  {
  case 1:
    ClassReqStatus = CDC_GETLineCoding(pdev, phost);
    if (ClassReqStatus == 0)
    {
      CDC_ReqState = 2;
    }
    break;
  case 0:
    ClassReqStatus = CDC_SETLineCoding(pdev, phost);
    if (ClassReqStatus == 0)
    {
      CDC_ReqState = 1;
    }
    if (ClassReqStatus == 3)
    {
      CDC_ReqState = 3;
    }
    break;
  case 2:
    ClassReqStatus = CDC_SETControlLineState(pdev, phost);
    if (ClassReqStatus == 0)
    {
      CDC_ReqState = 2;
      CDC_RxParam.CDCState = 0;
      status = 0;
    }
    break;
  case 3:
    ClassReqStatus = USBH_ClrFeature(pdev, phost, 0x00, pphost->Control.hc_num_out);
    if (ClassReqStatus == 0)
    {
      CDC_ReqState = 1;
    }
    break;
  }
  return status;
}


/* GivEnergy change: pass a received block to UserCb.Receive only when modbus_src is 0
   (then set it to 1) and count it; reset the fill pointer. */
void CDC_ReceiveData(int param_1)
{
  int v5;
  
  if ((uint)*(int *)(param_1 + 8 + 4) < *(int *)(param_1 + 8)) {
    v5 = *(int *)(param_1 + 8);
    *(uchar *)v5 = 0;
    if (modbus_src == 0) {
      modbus_src = 1;
      ((code *)(*(int *)(UserCb + 4)))(*(int *)(param_1 + 0xc));
      cdc_rx_count = cdc_rx_count + 1;
    }
    *(int *)(param_1 + 8) = *(int *)(param_1 + 0xc);
    *(ushort *)(param_1 + 0x14) = 0;
  }
}


/* Receive machine: while reception is enabled, start a bulk IN read and, when it is
   done, add it to the buffer and pass it on (CDC_ReceiveData). */
void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  volatile uint len = 0;

  if (RX_Enabled == 1)
  {
    uchar URB_StatusRx = HCD_GetURB_State(pdev, CDC_Machine.CDC_DataItf.hc_num_in);

    switch (CDC_RxParam.CDCState)
    {
    case 0:
      if (CDC_RxParam.DataLength < (CDC_RxParam.BufferLen - CDC_Machine.CDC_DataItf.length))
      {
        USBH_BulkReceiveData(pdev,
                     CDC_RxParam.pFillBuff,
                     CDC_Machine.CDC_DataItf.length,
                     CDC_Machine.CDC_DataItf.hc_num_in);
        CDC_RxParam.CDCState = 5;
      }
      break;

    case 5:
      if (URB_StatusRx == 1)
      {
        CDC_RxParam.DataLength += pdev->host.hc[CDC_Machine.CDC_DataItf.hc_num_in].xfer_count;
        CDC_RxParam.pFillBuff += pdev->host.hc[CDC_Machine.CDC_DataItf.hc_num_in].xfer_count;

        CDC_ReceiveData(&CDC_RxParam);

        CDC_RxParam.CDCState = 0;
        ((uchar *)RxBuf)[1] = 0;
        pdev->host.hc[CDC_Machine.CDC_DataItf.hc_num_in].xfer_count = 0;
      }
      break;
    }
  }
}

#define len (*(uint *)cdc_tx_len)

/* Transmit machine: send the pending data in packets of the endpoint size and resend
   on NAK. */
void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  uchar URB_StatusTx = 0;
  volatile uint unused = 0;

  URB_StatusTx = HCD_GetURB_State(pdev, CDC_Machine.CDC_DataItf.hc_num_out);

  switch (CDC_TxParam.CDCState)
  {
  case 0:
    break;

  case 2:
    if ((URB_StatusTx == 1) || (URB_StatusTx == 0))
    {
      if (CDC_TxParam.DataLength > CDC_Machine.CDC_DataItf.length)
      {
        len = CDC_Machine.CDC_DataItf.length;
        USBH_BulkSendData(pdev,
                     CDC_TxParam.pRxTxBuff,
                     len,
                     CDC_Machine.CDC_DataItf.hc_num_out);
      }
      else
      {
        len = CDC_TxParam.DataLength;
        USBH_BulkSendData(pdev,
                     CDC_TxParam.pRxTxBuff,
                     len,
                     CDC_Machine.CDC_DataItf.hc_num_out);
      }
      CDC_TxParam.CDCState = 3;
    }
    break;

  case 3:
    if (URB_StatusTx == 1)
    {
      CDC_TxParam.pRxTxBuff += len;
      CDC_TxParam.DataLength -= len;

      if (CDC_TxParam.DataLength == 0)
      {
        CDC_TxParam.CDCState = 0;
      }
      else
      {
        CDC_TxParam.CDCState = 2;
      }
    }
    else if (URB_StatusTx == 2)
    {
      USBH_BulkSendData(pdev,
                   (CDC_TxParam.pRxTxBuff),
                   len,
                   CDC_Machine.CDC_DataItf.hc_num_out);
    }
    break;
  }
}
#undef len


/* Class machine: the user callback, then transmission and reception. */
int CDC_Handle(int param_1, int param_2)
{
  int v5;
  int v7;
  
  v7 = 0;
  v5 = param_2;
  ((code *)*(int *)(*(int *)(v5 + 0x90) + 0x3c))();
  CDC_ProcessTransmission(param_1, v5);
  CDC_ProcessReception(param_1, v5);
  return v7;
}


/* Queue param_2 bytes at param_1 to send. GivEnergy change: when the transmitter is
   still busy, count it and after four force a USB restart (usb_cdc_timer = 5000). */
void CDC_SendData(int param_1, ushort param_2)
{

  
  if ((*(uchar *)(((uchar *)&CDC_TxParam) + 0)) == 0) {
    if (cdc_tx_busy_count > 0) {
      cdc_tx_busy_count = cdc_tx_busy_count - 1;
    }
    else {
      cdc_tx_stalled[0] = 0;
    }
    (*(int *)(((uchar *)&CDC_TxParam) + 4)) = param_1;
    (*(ushort *)(((uchar *)&CDC_TxParam) + 20)) = param_2;
    (*(uchar *)(((uchar *)&CDC_TxParam) + 0)) = 2;
  }
  else {
    if (cdc_tx_busy_count < 4) {
      cdc_tx_busy_count = cdc_tx_busy_count + 1;
    }
    else {
      if (cdc_tx_stalled[0] == 0) {
        cdc_tx_stalled[0] = 1;
        cdc_tx_busy_count = 1;
        usb_cdc_timer = 0x1388;
      }
      else {
        cfg_changed_flags = cfg_changed_flags | 1;
      }
    }
  }
}


/* Allow reception (RX_Enabled = 1). */
void CDC_StartReception(int p_x1)
{

  RX_Enabled = 1;
  return;
}


/* Stop reception and free the data IN channel. No callers. */
void CDC_StopReception(int param_1)
{

  RX_Enabled = 0;
  USB_OTG_HC_Halt(param_1,(uint)(*(uchar *)(((uchar *)&CDC_Machine) + 16)));
  USBH_Free_Channel(param_1,(uint)(*(uchar *)(((uchar *)&CDC_Machine) + 16)));
  return;
}

