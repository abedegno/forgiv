/* F2806x_Gpio.h: TI's GPIO registers: the control registers GpioCtrlRegs at 0x6F80 (mux,
   direction, pull-up, qualification) and the data registers GpioDataRegs at 0x6FC0 (DAT, SET,
   CLEAR, TOGGLE). Writing 0 to a SET or CLEAR bit does nothing.
   Users: drivers/device_init.c (InitGpio), drivers/sci.c (GPIO57), boot_flash_loader.c and the
   application's output pins.
   name: original, TI's file name. */
#ifndef F2806X_GPIO_H
#define F2806X_GPIO_H

#include "F2806x_Device.h"

/* GPIO A: qualification and mux, two bits per pin */
struct GPA1_BITS {
    Uint16 GPIO0 : 2;
    Uint16 GPIO1 : 2;
    Uint16 GPIO2 : 2;
    Uint16 GPIO3 : 2;
    Uint16 GPIO4 : 2;
    Uint16 GPIO5 : 2;
    Uint16 GPIO6 : 2;
    Uint16 GPIO7 : 2;
    Uint16 GPIO8 : 2;
    Uint16 GPIO9 : 2;
    Uint16 GPIO10 : 2;
    Uint16 GPIO11 : 2;
    Uint16 GPIO12 : 2;
    Uint16 GPIO13 : 2;
    Uint16 GPIO14 : 2;
    Uint16 GPIO15 : 2;
};
union GPA1_REG {
    Uint32 all;
    struct GPA1_BITS bit;
};

struct GPA2_BITS {
    Uint16 GPIO16 : 2;
    Uint16 GPIO17 : 2;
    Uint16 GPIO18 : 2;
    Uint16 GPIO19 : 2;
    Uint16 GPIO20 : 2;
    Uint16 GPIO21 : 2;
    Uint16 GPIO22 : 2;
    Uint16 GPIO23 : 2;
    Uint16 GPIO24 : 2;
    Uint16 GPIO25 : 2;
    Uint16 GPIO26 : 2;
    Uint16 GPIO27 : 2;
    Uint16 GPIO28 : 2;
    Uint16 GPIO29 : 2;
    Uint16 GPIO30 : 2;
    Uint16 GPIO31 : 2;
};
union GPA2_REG {
    Uint32 all;
    struct GPA2_BITS bit;
};

/* GPIO B: qualification and mux, two bits per pin */
struct GPB1_BITS {
    Uint16 GPIO32 : 2;
    Uint16 GPIO33 : 2;
    Uint16 GPIO34 : 2;
    Uint16 GPIO35 : 2;
    Uint16 GPIO36 : 2;
    Uint16 GPIO37 : 2;
    Uint16 GPIO38 : 2;
    Uint16 GPIO39 : 2;
    Uint16 GPIO40 : 2;
    Uint16 GPIO41 : 2;
    Uint16 GPIO42 : 2;
    Uint16 GPIO43 : 2;
    Uint16 GPIO44 : 2;
    Uint16 rsvd1 : 6;
};
union GPB1_REG {
    Uint32 all;
    struct GPB1_BITS bit;
};

struct GPB2_BITS {
    Uint16 rsvd1 : 4;
    Uint16 GPIO50 : 2;
    Uint16 GPIO51 : 2;
    Uint16 GPIO52 : 2;
    Uint16 GPIO53 : 2;
    Uint16 GPIO54 : 2;
    Uint16 GPIO55 : 2;
    Uint16 GPIO56 : 2;
    Uint16 GPIO57 : 2;
    Uint16 GPIO58 : 2;
    Uint16 rsvd2 : 10;
};
union GPB2_REG {
    Uint32 all;
    struct GPB2_BITS bit;
};

/* one bit per pin: data, set, clear, toggle, direction, pull-up disable */
struct GPADAT_BITS {
    Uint16 GPIO0 : 1;
    Uint16 GPIO1 : 1;
    Uint16 GPIO2 : 1;
    Uint16 GPIO3 : 1;
    Uint16 GPIO4 : 1;
    Uint16 GPIO5 : 1;
    Uint16 GPIO6 : 1;
    Uint16 GPIO7 : 1;
    Uint16 GPIO8 : 1;
    Uint16 GPIO9 : 1;
    Uint16 GPIO10 : 1;
    Uint16 GPIO11 : 1;
    Uint16 GPIO12 : 1;
    Uint16 GPIO13 : 1;
    Uint16 GPIO14 : 1;
    Uint16 GPIO15 : 1;
    Uint16 GPIO16 : 1;
    Uint16 GPIO17 : 1;
    Uint16 GPIO18 : 1;
    Uint16 GPIO19 : 1;
    Uint16 GPIO20 : 1;
    Uint16 GPIO21 : 1;
    Uint16 GPIO22 : 1;
    Uint16 GPIO23 : 1;
    Uint16 GPIO24 : 1;
    Uint16 GPIO25 : 1;
    Uint16 GPIO26 : 1;
    Uint16 GPIO27 : 1;
    Uint16 GPIO28 : 1;
    Uint16 GPIO29 : 1;
    Uint16 GPIO30 : 1;
    Uint16 GPIO31 : 1;
};
union GPADAT_REG {
    Uint32 all;
    struct GPADAT_BITS bit;
};

struct GPBDAT_BITS {
    Uint16 GPIO32 : 1;
    Uint16 GPIO33 : 1;
    Uint16 GPIO34 : 1;
    Uint16 GPIO35 : 1;
    Uint16 GPIO36 : 1;
    Uint16 GPIO37 : 1;
    Uint16 GPIO38 : 1;
    Uint16 GPIO39 : 1;
    Uint16 GPIO40 : 1;
    Uint16 GPIO41 : 1;
    Uint16 GPIO42 : 1;
    Uint16 GPIO43 : 1;
    Uint16 GPIO44 : 1;
    Uint16 rsvd1 : 3;
    Uint16 rsvd2 : 2;
    Uint16 GPIO50 : 1;
    Uint16 GPIO51 : 1;
    Uint16 GPIO52 : 1;
    Uint16 GPIO53 : 1;
    Uint16 GPIO54 : 1;
    Uint16 GPIO55 : 1;
    Uint16 GPIO56 : 1;
    Uint16 GPIO57 : 1;
    Uint16 GPIO58 : 1;
    Uint16 rsvd3 : 5;
};
union GPBDAT_REG {
    Uint32 all;
    struct GPBDAT_BITS bit;
};

struct AIODAT_BITS {
    Uint16 rsvd1 : 2;
    Uint16 AIO2 : 1;
    Uint16 rsvd2 : 1;
    Uint16 AIO4 : 1;
    Uint16 rsvd3 : 1;
    Uint16 AIO6 : 1;
    Uint16 rsvd4 : 3;
    Uint16 AIO10 : 1;
    Uint16 rsvd5 : 1;
    Uint16 AIO12 : 1;
    Uint16 rsvd6 : 1;
    Uint16 AIO14 : 1;
    Uint16 rsvd7 : 1;
    Uint16 rsvd8 : 16;
};
union AIODAT_REG {
    Uint32 all;
    struct AIODAT_BITS bit;
};

struct GPIO_CTRL_REGS {
    Uint32 GPACTRL;          /* 0x00 */
    union GPA1_REG GPAQSEL1; /* 0x02 */
    union GPA2_REG GPAQSEL2; /* 0x04 */
    union GPA1_REG GPAMUX1;  /* 0x06 */
    union GPA2_REG GPAMUX2;  /* 0x08 */
    union GPADAT_REG GPADIR; /* 0x0A */
    union GPADAT_REG GPAPUD; /* 0x0C */
    Uint32 rsvd1;            /* 0x0E */
    Uint32 GPBCTRL;          /* 0x10 */
    union GPB1_REG GPBQSEL1; /* 0x12 */
    union GPB2_REG GPBQSEL2; /* 0x14 */
    union GPB1_REG GPBMUX1;  /* 0x16 */
    union GPB2_REG GPBMUX2;  /* 0x18 */
    union GPBDAT_REG GPBDIR; /* 0x1A */
    union GPBDAT_REG GPBPUD; /* 0x1C */
};

struct GPIO_DATA_REGS {
    union GPADAT_REG GPADAT;    /* 0x00 */
    union GPADAT_REG GPASET;    /* 0x02 */
    union GPADAT_REG GPACLEAR;  /* 0x04 */
    union GPADAT_REG GPATOGGLE; /* 0x06 */
    union GPBDAT_REG GPBDAT;    /* 0x08 */
    union GPBDAT_REG GPBSET;    /* 0x0A */
    union GPBDAT_REG GPBCLEAR;  /* 0x0C */
    union GPBDAT_REG GPBTOGGLE; /* 0x0E */
    Uint16 rsvd1[8];            /* 0x10 */
    union AIODAT_REG AIODAT;    /* 0x18 */
    union AIODAT_REG AIOSET;    /* 0x1A */
    union AIODAT_REG AIOCLEAR;  /* 0x1C */
    union AIODAT_REG AIOTOGGLE; /* 0x1E */
};

extern volatile struct GPIO_CTRL_REGS GpioCtrlRegs;
extern volatile struct GPIO_DATA_REGS GpioDataRegs;

#endif
