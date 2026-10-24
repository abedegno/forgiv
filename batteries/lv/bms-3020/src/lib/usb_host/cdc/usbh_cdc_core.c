/* at: CDC_InitTxRxParam 0x0802ECE0, CDC_InterfaceInit 0x0802ED04, CDC_InterfaceDeInit 0x0802EDFC, CDC_ClassRequest 0x0802EE48, CDC_ReceiveData 0x0802EEA8, CDC_ProcessReception 0x0802EED2, CDC_ProcessTransmission 0x0802EF4C, CDC_Handle 0x0802EFDA, CDC_SendData 0x0802EFFE, CDC_StartReception 0x0802F040, CDC_StopReception 0x0802F048, CDC_Machine 0x20007A04, CDC_TxParam 0x20007A28, CDC_RxParam 0x20007A40, TxBuf 0x20007A58, RxBuf 0x20007C4C, RX_Enabled 0x20000668, cdc_tx_busy_count 0x20000669, cdc_tx_stalled 0x2000066A, CDC_ReqState 0x2000066B, cdc_rx_count 0x2000066C, cdc_tx_len 0x20000670, UserCb 0x20000674, usb_cdc_timer 0x20000058, CDC_GETLineCoding 0x0802F078, CDC_SETLineCoding 0x0802F096, CDC_SETControlLineState 0x0802F0B6, USBH_ClrFeature 0x0802E178, USBH_Alloc_Channel 0x0802DBC2, USBH_Open_Channel 0x0802DB20, USBH_Free_Channel 0x0802DBE6, USB_OTG_HC_Halt 0x0802C424, HCD_GetURB_State 0x0802C580, USBH_BulkReceiveData 0x0802DD30, USBH_BulkSendData 0x0802DCFC */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* USB host CDC class, adapted from the matched A316 source. */
#include "bms_cdc_port.h"

CDC_Machine_TypeDef CDC_Machine;
CDC_Xfer_TypeDef CDC_TxParam;
CDC_Xfer_TypeDef CDC_RxParam;
uchar TxBuf[500];
uchar RxBuf[500];
uchar RX_Enabled = 0;
uchar cdc_tx_busy_count = 0;
uchar cdc_tx_stalled = 0;
uchar CDC_ReqState = 0;
ushort cdc_rx_count = 0;
uint cdc_tx_len = 0;
uchar UserCb[8] = {0};

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


/* Pass a received block to UserCb.Receive, count it and reset the fill pointer. */
void CDC_ReceiveData(int param_1)
{
  int v5;
  
  if ((uint)*(int *)(param_1 + 8 + 4) < *(int *)(param_1 + 8)) {
    v5 = *(int *)(param_1 + 8);
    *(uchar *)v5 = 0;
    ((code *)(*(int *)(UserCb + 4)))(*(int *)(param_1 + 0xc));
    cdc_rx_count = cdc_rx_count + 1;
    *(int *)(param_1 + 8) = *(int *)(param_1 + 0xc);
    *(ushort *)(param_1 + 0x14) = 0;
  }
}


/* Receive machine: while reception is enabled, start a bulk IN read and, when it is
   done, add it to the buffer and pass it on (CDC_ReceiveData). */
void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{

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

#define len cdc_tx_len

/* Transmit machine: send the pending data in packets of the endpoint size and resend
   on NAK. */
void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  uchar URB_StatusTx = 0;

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


/* Queue data to send. Count busy retries and mark the link timer after four. */
void CDC_SendData(int param_1, ushort param_2)
{
  uchar tx_state = CDC_TxParam.CDCState;
  if (tx_state == 0) {
    if (cdc_tx_busy_count > 0) {
      cdc_tx_busy_count = cdc_tx_busy_count - 1;
    }
    else {
      cdc_tx_stalled = 0;
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
      if (cdc_tx_stalled == 0) {
        cdc_tx_stalled = 1;
        cdc_tx_busy_count = 1;
        usb_cdc_timer = (uchar)0x1388;
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

