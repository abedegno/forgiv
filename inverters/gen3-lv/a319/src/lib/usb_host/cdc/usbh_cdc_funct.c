/* at: CDC_GETLineCoding 0x08036F2C, CDC_SETLineCoding 0x08036F56, CDC_SETControlLineState 0x08036F80, CDC_ChangeStateToIssueSetConfig 0x08036FAA, CDC_IssueGetConfig 0x08036FBA */
/* at: CDC_ReqState 0x20000558, CDC_GetLineCode 0x20000584, CDC_SetLineCode 0x2000058C, CDC_Desc 0x20008A64 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host CDC class requests, usbh_cdc_funct.c: GET_LINE_CODING, SET_LINE_CODING,
   SET_CONTROL_LINE_STATE, and the helpers that queue them.
   Library: the CDC host class written for ST's USB OTG host library (see usbh_cdc_core.c).
   Job in this firmware: the STM32F105's USB OTG FS port, as a host only (USE_USB_OTG_FS,
   USE_HOST_MODE, no DMA). task_usb (src/app/usb/usb_task.c) runs USBH_Process. Two classes are
   linked: CDC for the communications dongle, which carries Modbus RTU (modbus_slave_process
   receives through CDC_ReceiveData and answers with CDC_SendData), and MSC for a USB stick with
   firmware update files (FatFs, usbh_msc_fatfs.c). The host starts with the CDC class; when a
   device's first interface is class 8, USBH_USR_Configuration_DescAvailable sets
   usb_class_code and task_usb starts the host again with the MSC class.
   Changes: none found. The requests go to the interface in CDC_Desc[18] (the union functional
   descriptor's master interface).
   match: this file declares USBH_CtlReq as returning uchar (OWN_ define): it was compiled
   against that.
   name: original (map/filenames.tsv). */
#include "../../../include/firmware_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_types.h"
#define OWN_USBH_CtlReq   /* match: declared below as uchar(void *,USBH_HOST *,uchar *,ushort): this file compiled against that, not the header's int(void *,USBH_HOST *,uchar *,ushort) */
#include "../../../include/usb.h"
uchar USBH_CtlReq(void *, USBH_HOST *, uchar *, ushort);

#define CDC_UnionFuncDesc_bMasterInterface CDC_Desc[18]

/* GET_LINE_CODING (0x21): seven bytes into CDC_GetLineCode. */
uchar CDC_GETLineCoding(void *pdev, USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = 0x80 | 0x20 | 0x01;

  phost->Control.setup.b.bRequest = 0x21;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = CDC_UnionFuncDesc_bMasterInterface;
  phost->Control.setup.b.wLength.w = 7;

  return USBH_CtlReq(pdev, phost, CDC_GetLineCode, 7);
}
#undef CDC_UnionFuncDesc_bMasterInterface

#define CDC_UnionFuncDesc_bMasterInterface CDC_Desc[18]

/* SET_LINE_CODING (0x20) from CDC_SetLineCode (57600 8N1). */
uchar CDC_SETLineCoding(void *pdev, USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x20 | 0x01;

  phost->Control.setup.b.bRequest = 0x20;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = CDC_UnionFuncDesc_bMasterInterface;
  phost->Control.setup.b.wLength.w = 7;

  return USBH_CtlReq(pdev, phost, CDC_SetLineCode, 7);
}
#undef CDC_UnionFuncDesc_bMasterInterface

#define CDC_UnionFuncDesc_bMasterInterface CDC_Desc[18]

/* SET_CONTROL_LINE_STATE (0x22) with DTR and RTS off. */
uchar CDC_SETControlLineState(void *pdev, USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x20 | 0x01;

  phost->Control.setup.b.bRequest = 0x22;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = CDC_UnionFuncDesc_bMasterInterface;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}
#undef CDC_UnionFuncDesc_bMasterInterface

extern uchar CDC_ReqState[];

/* Go back to the class request state (6) and send SET_LINE_CODING next. usbh_usr.c
   calls it once per connection and when usb_line_stuck is set. */
void CDC_ChangeStateToIssueSetConfig(uint param_1, uchar * param_2)
{

  param_2[1] = *param_2;
  *param_2 = 6;
  CDC_ReqState[0] = 0;
  return;
}

extern uchar CDC_ReqState[];

/* Go back to the class request state and send GET_LINE_CODING next. No callers. */
void CDC_IssueGetConfig(uint param_1, uchar * param_2)
{

  param_2[1] = *param_2;
  *param_2 = 6;
  CDC_ReqState[0] = 1;
  return;
}
