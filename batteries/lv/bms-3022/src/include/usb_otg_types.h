/* usb_otg_types.h: register unions of the STM32 USB OTG library (usb_regs.h) used by A316's USB
   host code, as locals: each is kept on the stack at -O0 and a bitfield store is a read, mask,
   insert, write. Only the unions the hand sources need; bit positions checked against the image.
   name: original (ST's union names). */
#ifndef USB_OTG_TYPES_H
#define USB_OTG_TYPES_H

typedef union {
    unsigned int d32;
    struct {
        unsigned int prtconnsts : 1;
        unsigned int prtconndet : 1;
        unsigned int prtena : 1;
        unsigned int prtenchng : 1;
        unsigned int prtovrcurract : 1;
        unsigned int prtovrcurrchng : 1;
        unsigned int prtres : 1;
        unsigned int prtsusp : 1;
        unsigned int prtrst : 1;
        unsigned int Reserved9 : 1;
        unsigned int prtlnsts : 2;
        unsigned int prtpwr : 1;
        unsigned int prttstctl : 4;
        unsigned int prtspd : 2;
        unsigned int Reserved19_31 : 13;
    } b;
} USB_OTG_HPRT0_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int fslspclksel : 2;
        unsigned int fslssupp : 1;
    } b;
} USB_OTG_HCFG_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int chnum : 4;
        unsigned int bcnt : 11;
        unsigned int dpid : 2;
        unsigned int pktsts : 4;
        unsigned int Reserved : 11;
    } b;
} USB_OTG_GRXFSTS_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int stoppclk : 1;
        unsigned int gatehclk : 1;
        unsigned int Reserved2_3 : 2;
        unsigned int phy_susp : 1;
        unsigned int Reserved5_31 : 27;
    } b;
} USB_OTG_PCGCCTL_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int chint : 16;
        unsigned int Reserved : 16;
    } b;
} USB_OTG_HAINT_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int startaddr : 16;
        unsigned int depth : 16;
    } b;
} USB_OTG_FSIZ_TypeDef;

typedef struct {
    volatile unsigned int GOTGCTL; /* 0x00 */
    volatile unsigned int GOTGINT;
    volatile unsigned int GAHBCFG;
    volatile unsigned int GUSBCFG; /* 0x0c */
    volatile unsigned int GRSTCTL; /* 0x10 */
    volatile unsigned int GINTSTS;
    volatile unsigned int GINTMSK; /* 0x18 */
    volatile unsigned int GRXSTSR;
    volatile unsigned int GRXSTSP; /* 0x20 */
    volatile unsigned int GRXFSIZ;
    volatile unsigned int DIEPTXF0_HNPTXFSIZ;
    volatile unsigned int HNPTXSTS; /* 0x2c */
    unsigned int Reserved30[2];
    volatile unsigned int GCCFG; /* 0x38 */
    volatile unsigned int CID;
    unsigned int Reserved40[48];
    volatile unsigned int HPTXFSIZ; /* 0x100 */
    volatile unsigned int DIEPTXF[15];
} USB_OTG_GREGS;

typedef struct {
    volatile unsigned int HCFG; /* 0x00 */
    volatile unsigned int HFIR;
    volatile unsigned int HFNUM;
    unsigned int Reserved40C;
    volatile unsigned int HPTXSTS; /* 0x10 */
    volatile unsigned int HAINT;
    volatile unsigned int HAINTMSK; /* 0x18 */
} USB_OTG_HREGS;

typedef struct {
    volatile unsigned int HCCHAR; /* 0x00 */
    volatile unsigned int HCSPLT;
    volatile unsigned int HCINT;
    volatile unsigned int HCINTMSK; /* 0x0c */
    volatile unsigned int HCTSIZ;   /* 0x10 */
    volatile unsigned int HCDMA;
    unsigned int Reserved[2];
} USB_OTG_HC_REGS;

typedef struct {
    USB_OTG_GREGS *GREGS; /* 0x0c in the handle */
    void *DREGS;
    USB_OTG_HREGS *HREGS; /* 0x14 */
    void *INEP_REGS[15];
    void *OUTEP_REGS[15];
    USB_OTG_HC_REGS *HC_REGS[15];     /* 0x90 */
    volatile unsigned int *HPRT0;     /* 0xcc */
    volatile unsigned int *DFIFO[15]; /* 0xd0 */
    volatile unsigned int *PCGCCTL;
} USB_OTG_CORE_REGS;

typedef union {
    unsigned int d32;
    struct {
        unsigned int xfercompl : 1;
        unsigned int chhltd : 1;
        unsigned int ahberr : 1;
        unsigned int stall : 1;
        unsigned int nak : 1;
        unsigned int ack : 1;
        unsigned int nyet : 1;
        unsigned int xacterr : 1;
        unsigned int bblerr : 1;
        unsigned int frmovrun : 1;
        unsigned int datatglerr : 1;
        unsigned int Reserved : 21;
    } b;
} USB_OTG_HCINTMSK_TypeDef;

typedef USB_OTG_HCINTMSK_TypeDef USB_OTG_HCINTn_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int Reserved0 : 1;
        unsigned int modemismatch : 1;
        unsigned int otgintr : 1;
        unsigned int sofintr : 1;
        unsigned int rxstsqlvl : 1;
        unsigned int nptxfempty : 1;
        unsigned int ginnakeff : 1;
        unsigned int goutnakeff : 1;
        unsigned int Reserved8_9 : 2;
        unsigned int erlysuspend : 1;
        unsigned int usbsuspend : 1;
        unsigned int usbreset : 1;
        unsigned int enumdone : 1;
        unsigned int isooutdrop : 1;
        unsigned int eopframe : 1;
        unsigned int Reserved16 : 1;
        unsigned int epmismatch : 1;
        unsigned int inepintr : 1;
        unsigned int outepintr : 1;
        unsigned int incomplisoin : 1;
        unsigned int incomplisoout : 1;
        unsigned int Reserved22_23 : 2;
        unsigned int portintr : 1;
        unsigned int hcintr : 1;
        unsigned int ptxfempty : 1;
        unsigned int Reserved27 : 1;
        unsigned int conidstschng : 1;
        unsigned int disconnect : 1;
        unsigned int sessreqintr : 1;
        unsigned int wkupintr : 1;
    } b;
} USB_OTG_GINTMSK_TypeDef;

typedef USB_OTG_GINTMSK_TypeDef USB_OTG_GINTSTS_TypeDef; /* the same bits */

typedef union {
    unsigned int d32;
    struct {
        unsigned int mps : 11;
        unsigned int epnum : 4;
        unsigned int epdir : 1;
        unsigned int Reserved : 1;
        unsigned int lspddev : 1;
        unsigned int eptype : 2;
        unsigned int multicnt : 2;
        unsigned int devaddr : 7;
        unsigned int oddfrm : 1;
        unsigned int chdis : 1;
        unsigned int chen : 1;
    } b;
} USB_OTG_HCCHAR_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int xfersize : 19;
        unsigned int pktcnt : 10;
        unsigned int pid : 2;
        unsigned int dopng : 1;
    } b;
} USB_OTG_HCTSIZn_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int nptxfspcavail : 16;
        unsigned int nptxqspcavail : 8;
        struct {
            unsigned int terminate : 1;
            unsigned int token : 2;
            unsigned int chnum : 4;
        } nptxqtop; /* a struct of its own: the union is 12 bytes */
        unsigned int Reserved : 1;
    } b;
} USB_OTG_HNPTXSTS_TypeDef;

typedef union {
    unsigned int d32;
    struct {
        unsigned int ptxfspcavail : 16;
        unsigned int ptxqspcavail : 8;
        struct {
            unsigned int terminate : 1;
            unsigned int token : 2;
            unsigned int chnum : 4;
            unsigned int odd_even : 1;
        } ptxqtop; /* 8 bytes */
    } b;
} USB_OTG_HPTXSTS_TypeDef;

typedef struct {
    unsigned char dev_addr; /* 0x00 */
    unsigned char ep_num;
    unsigned char ep_is_in;
    unsigned char speed;
    unsigned char do_ping; /* 0x04 */
    unsigned char ep_type;
    unsigned short max_packet; /* 0x06 */
    unsigned char data_pid;    /* 0x08 */
    unsigned char *xfer_buff;  /* 0x0c */
    unsigned int xfer_len;     /* 0x10 */
    unsigned int xfer_count;
    unsigned char toggle_in; /* 0x18 */
    unsigned char toggle_out;
    unsigned int dma_addr; /* 0x1c */
} USB_OTG_HC;

typedef struct {
    unsigned char Rx_Buffer[512];         /* 0x110 in the handle */
    volatile unsigned int ConnSts;        /* 0x310 */
    volatile unsigned int PortEnabled;    /* 0x314 */
    volatile unsigned int ErrCnt[15];     /* 0x318 */
    volatile unsigned int XferCnt[15];    /* 0x354 */
    volatile unsigned char HC_Status[15]; /* 0x390, enum HC_STATUS (one byte) */
    volatile unsigned char URB_State[15]; /* 0x39f, enum URB_STATE */
    USB_OTG_HC hc[15];                    /* 0x3b0 */
    unsigned short channel[15];           /* 0x590 */
    void *port_cb;
} HCD_DEV;

typedef struct {
    unsigned char host_channels;
    unsigned char dev_endpoints;
    unsigned char speed;
    unsigned char dma_enable; /* 0x03 */
    unsigned short mps;
    unsigned short TotalFifoSize;
    unsigned char phy_itface;
    unsigned char Sof_output;
    unsigned char low_power;
    unsigned char coreID;
} USB_OTG_CORE_CFGS;

typedef struct {
    USB_OTG_CORE_CFGS cfg;  /* 0x00 */
    USB_OTG_CORE_REGS regs; /* 0x0c */
    HCD_DEV host;           /* 0x110 */
} USB_OTG_CORE_HANDLE;

#define USB_OTG_READ_REG32(reg) (*(volatile unsigned int *)reg)
#define USB_OTG_WRITE_REG32(reg, value) (*(volatile unsigned int *)reg = value)
#define USB_OTG_MODIFY_REG32(reg, clear_mask, set_mask)                                            \
    USB_OTG_WRITE_REG32(reg, (((USB_OTG_READ_REG32(reg)) & ~clear_mask) | set_mask))

#define CLEAR_HC_INT(HC_REGS, intr)                                                                \
    {                                                                                              \
        USB_OTG_HCINTn_TypeDef hcint_clear;                                                        \
        hcint_clear.d32 = 0;                                                                       \
        hcint_clear.b.intr = 1;                                                                    \
        USB_OTG_WRITE_REG32(&((HC_REGS)->HCINT), hcint_clear.d32);                                 \
    }

#define MASK_HOST_INT_CHH(hc_num)                                                                  \
    {                                                                                              \
        USB_OTG_HCINTMSK_TypeDef INTMSK;                                                           \
        INTMSK.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc_num]->HCINTMSK);                    \
        INTMSK.b.chhltd = 0;                                                                       \
        USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCINTMSK, INTMSK.d32);                    \
    }

#define UNMASK_HOST_INT_CHH(hc_num)                                                                \
    {                                                                                              \
        USB_OTG_HCINTMSK_TypeDef INTMSK;                                                           \
        INTMSK.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc_num]->HCINTMSK);                    \
        INTMSK.b.chhltd = 1;                                                                       \
        USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCINTMSK, INTMSK.d32);                    \
    }

#endif
