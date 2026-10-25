/* usbh_types.h: STM32 USB host library (usbh_core.h) objects as A316 lays them out. Only the
   members the hand sources use are named; offsets checked against the image.
   name: original (ST's type names). */
#ifndef USBH_TYPES_H
#define USBH_TYPES_H

typedef union {
    unsigned short w;
    struct {
        unsigned char msb;
        unsigned char lsb;
    } bw;
} uint16_t_uint8_t;

typedef union {
    unsigned char d8[8];
    struct {
        unsigned char bmRequestType;
        unsigned char bRequest;
        uint16_t_uint8_t wValue;
        uint16_t_uint8_t wIndex;
        uint16_t_uint8_t wLength;
    } b;
} USB_Setup_TypeDef;

typedef struct {
    unsigned char hc_num_in; /* 0x04 in USBH_HOST */
    unsigned char hc_num_out;
    unsigned char ep0size;    /* 0x06 */
    unsigned char *buff;      /* 0x08 */
    unsigned short length;    /* 0x0c */
    unsigned char errorcount; /* 0x0e */
    unsigned short timer;     /* 0x10 */
    unsigned char status;     /* 0x12, enum CTRL_STATUS */
    USB_Setup_TypeDef setup;  /* 0x14 */
    unsigned char state;      /* 0x1c, enum CTRL_State */
} USBH_Ctrl_TypeDef;

typedef struct {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} USBH_DevDesc_TypeDef;

typedef struct {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short wTotalLength;
    unsigned char bNumInterfaces;
    unsigned char bConfigurationValue;
    unsigned char iConfiguration;
    unsigned char bmAttributes;
    unsigned char bMaxPower;
} USBH_CfgDesc_TypeDef;

typedef struct {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bInterfaceNumber;
    unsigned char bAlternateSetting;
    unsigned char bNumEndpoints;
    unsigned char bInterfaceClass;
    unsigned char bInterfaceSubClass;
    unsigned char bInterfaceProtocol;
    unsigned char iInterface;
} USBH_InterfaceDesc_TypeDef;

typedef struct {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bEndpointAddress;
    unsigned char bmAttributes;
    unsigned short wMaxPacketSize;
    unsigned char bInterval;
} USBH_EpDesc_TypeDef;

typedef struct {
    unsigned char bLength;
    unsigned char bDescriptorType;
} USBH_DescHeader_t;

typedef struct {
    unsigned char address;                  /* 0x20 in USBH_HOST */
    unsigned char speed;                    /* 0x21 */
    USBH_DevDesc_TypeDef Dev_Desc;          /* 0x22 */
    USBH_CfgDesc_TypeDef Cfg_Desc;          /* 0x34 */
    USBH_InterfaceDesc_TypeDef Itf_Desc[2]; /* 0x3e */
    USBH_EpDesc_TypeDef Ep_Desc[2][3];      /* 0x50 */
    unsigned char Reserved_[0x8c - 0x80];   /* HID_Desc */
} USBH_Device_TypeDef;

typedef struct {
    unsigned char (*Init)(void *pdev, void *phost); /* 0x00, USBH_Status (an enum: one byte) */
    void (*DeInit)(void *pdev, void *phost);
    unsigned char (*Requests)(void *pdev, void *phost);
    unsigned char (*Machine)(void *pdev, void *phost); /* 0x0c */
} USBH_Class_cb_TypeDef;

typedef struct {
    void (*Init)(void); /* 0x00 */
    void (*DeInit)(void);
    void (*DeviceAttached)(void);
    void (*ResetDevice)(void); /* 0x0c */
    void (*DeviceDisconnected)(void);
    void (*OverCurrentDetected)(void);
    void (*DeviceSpeedDetected)(unsigned char DeviceSpeed); /* 0x18 */
    void (*DeviceDescAvailable)(void *);
    void (*DeviceAddressAssigned)(void); /* 0x20 */
    void (*ConfigurationDescAvailable)(void *, void *, void *);
    void (*ManufacturerString)(void *);
    void (*ProductString)(void *); /* 0x2c */
    void (*SerialNumString)(void *);
    void (*EnumerationDone)(void); /* 0x34 */
    int (*UserInput)(void);        /* 0x38, USBH_USR_Status */
    int (*UserApplication)(void);
    void (*DeviceNotSupported)(void); /* 0x40 */
    void (*UnrecoveredError)(void);
} USBH_Usr_cb_TypeDef;

typedef struct {
    unsigned char gState; /* 0x00, enum HOST_State */
    unsigned char gStateBkp;
    unsigned char EnumState;
    unsigned char RequestState;
    USBH_Ctrl_TypeDef Control;       /* 0x04 */
    USBH_Device_TypeDef device_prop; /* 0x20 */
    USBH_Class_cb_TypeDef *class_cb; /* 0x8c */
    USBH_Usr_cb_TypeDef *usr_cb;     /* 0x90 */
} USBH_HOST;

#endif
