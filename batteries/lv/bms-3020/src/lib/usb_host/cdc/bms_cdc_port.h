#ifndef BMS_CDC_PORT_H
#define BMS_CDC_PORT_H
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef int code();
#include "bms_cdc_host_types.h"
#include "bms_cdc_otg_types.h"
#include "bms_cdc_xfer_types.h"
extern uchar UserCb[], RX_Enabled, cdc_tx_stalled, TxBuf[], RxBuf[], usb_cdc_timer;
extern uchar cdc_tx_busy_count;
extern ushort cdc_rx_count;
extern uint cdc_tx_len;
uchar HCD_GetURB_State(int,int);
uchar USBH_Alloc_Channel(USB_OTG_CORE_HANDLE *,uchar);
uchar USBH_Open_Channel(USB_OTG_CORE_HANDLE *,uchar,uchar,uchar,uchar,ushort);
uint USBH_Free_Channel(int,int);
int USB_OTG_HC_Halt(USB_OTG_CORE_HANDLE *,uchar);
uchar USBH_ClrFeature(USB_OTG_CORE_HANDLE *,USBH_HOST *,uchar,uchar);
int USBH_BulkReceiveData(int,int,int,int);
uchar USBH_BulkSendData(void *,uchar *,ushort,uchar);
uchar CDC_GETLineCoding(void *,USBH_HOST *);
uchar CDC_SETLineCoding(void *,USBH_HOST *);
uchar CDC_SETControlLineState(void *,USBH_HOST *);
uchar CDC_InterfaceInit(void *,void *);
void CDC_InterfaceDeInit(void *,void *);
uchar CDC_ClassRequest(void *,void *);
int CDC_Handle(int,int);
#endif
