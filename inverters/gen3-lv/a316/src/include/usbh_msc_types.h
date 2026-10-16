/* usbh_msc_types.h: STM32 USB host library, mass storage class (usbh_msc_bot.h,
   usbh_msc_scsi.h): the objects A316's MSC code uses, at the addresses the image gives them
   (src/lib/usb_host/msc).
   name: original (ST's type names). */
#ifndef USBH_MSC_TYPES_H
#define USBH_MSC_TYPES_H

typedef union {
    struct {
        unsigned int dCBWSignature;
        unsigned int dCBWTag;
        unsigned int dCBWDataTransferLength; /* 0x08 */
        unsigned char bmCBWFlags;            /* 0x0c */
        unsigned char bCBWLUN;
        unsigned char bCBWLength; /* 0x0e */
        unsigned char CBWCB[16];  /* 0x0f */
    } field;
    unsigned char CBWArray[31];
} HostCBWPkt_TypeDef;

typedef union {
    struct {
        unsigned int dCSWSignature;
        unsigned int dCSWTag;
        unsigned int dCSWDataResidue;
        unsigned char bCSWStatus;
    } field;
    unsigned char CSWArray[13];
} HostCSWPkt_TypeDef;

typedef struct {
    unsigned char MSCState; /* 0x00 */
    unsigned char MSCStateBkp;
    unsigned char MSCStateCurrent;
    unsigned char CmdStateMachine; /* 0x03 */
    unsigned char BOTState;
    unsigned char BOTStateBkp;
    unsigned char *pRxTxBuff;  /* 0x08 */
    unsigned short DataLength; /* 0x0c */
    unsigned char BOTXferErrorCount;
    unsigned char BOTXferStatus; /* 0x0f */
} USBH_BOTXfer_TypeDef;

typedef struct {
    unsigned int MSCapacity; /* 0x00 */
    unsigned int MSSenseKey;
    unsigned short MSPageLength; /* 0x08 */
    unsigned char MSBulkOutEp;
    unsigned char MSBulkInEp;
    unsigned char MSWriteProtect; /* 0x0c */
} MassStorageParameter_TypeDef;

typedef struct {
    unsigned char hc_num_in; /* 0x00 */
    unsigned char hc_num_out;
    unsigned char MSBulkOutEp;
    unsigned char MSBulkInEp;
    unsigned short MSBulkInEpSize; /* 0x04 */
    unsigned short MSBulkOutEpSize;
    unsigned char buff[0x40]; /* 0x08 */
    unsigned char maxLun;     /* 0x48 */
} MSC_Machine_TypeDef;

extern MSC_Machine_TypeDef MSC_Machine;
extern HostCBWPkt_TypeDef USBH_MSC_CBWData;
extern HostCSWPkt_TypeDef USBH_MSC_CSWData;
extern USBH_BOTXfer_TypeDef USBH_MSC_BOTXferParam;
extern MassStorageParameter_TypeDef USBH_MSC_Param;
extern unsigned char USBH_DataInBuffer[];

#endif
