#ifndef BMS_MSC_DISK_H
#define BMS_MSC_DISK_H
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
#define SCB_AIRCR_ADDR 0xE000ED0C
extern volatile uchar Stat;
extern uchar USB_OTG_Core[], USB_Host[], USBH_MSC_Param[];
uint HCD_IsDeviceConnected(int);
void USBH_MSC_HandleBOTXfer(void *,void *);
uchar USBH_MSC_Read10(void *,uchar *,uint,uint);
uchar USBH_MSC_Write10(void *,uchar *,uint,uint);
#endif
