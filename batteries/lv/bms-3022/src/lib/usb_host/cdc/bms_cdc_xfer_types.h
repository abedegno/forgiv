/* usbh_cdc_types.h: the USB host CDC class (the STM32 host library's CDC example,
   usbh_cdc_core.h) as A316 lays it out: the class machine at 0x200083FC, the transmit and
   receive parameters after it. The dongle link runs on it (src/lib/usb_host/cdc).
   name: original (ST's type names). */
#ifndef USBH_CDC_TYPES_H
#define USBH_CDC_TYPES_H

typedef struct {
    unsigned char hc_num_in; /* 0x00 */
    unsigned char hc_num_out;
    unsigned char notificationEp;
    unsigned char state;
    unsigned char buff[8];
    unsigned short length; /* 0x0c */
    unsigned char ep_addr; /* 0x0e */
} CDC_CommInterface_Typedef;

typedef struct {
    unsigned char hc_num_in; /* 0x00 */
    unsigned char hc_num_out;
    unsigned char cdcOutEp;
    unsigned char cdcInEp;
    unsigned char state;
    unsigned char buff[8];
    unsigned short length; /* 0x0e */
    unsigned char ep_addr; /* 0x10 */
} CDC_DataInterface_Typedef;

typedef struct {
    CDC_CommInterface_Typedef CDC_CommItf; /* 0x200083FC */
    CDC_DataInterface_Typedef CDC_DataItf; /* 0x2000840C */
} CDC_Machine_TypeDef;

typedef struct {
    volatile unsigned char CDCState; /* 0x00 */
    unsigned char *pRxTxBuff;        /* 0x04 */
    unsigned char *pFillBuff;        /* 0x08 */
    unsigned char *pEmptyBuff;       /* 0x0c */
    unsigned int BufferLen;          /* 0x10 */
    unsigned short DataLength;       /* 0x14 */
} CDC_Xfer_TypeDef;

extern CDC_Machine_TypeDef CDC_Machine;
extern CDC_Xfer_TypeDef CDC_TxParam;
extern CDC_Xfer_TypeDef CDC_RxParam;
extern unsigned char CDC_ReqState;

#endif
