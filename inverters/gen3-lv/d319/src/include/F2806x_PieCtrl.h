/* F2806x_PieCtrl.h: the part of TI's F2806x_PieCtrl.h this firmware uses: PieCtrlRegs at
   0x0CE0 (PIECTRL, PIEACK and each group's enable and flag registers) and TI's PIEACK_GROUPn
   masks. Users: drivers/device_init.c, drivers/sci_isr.c, the other interrupts and
   boot_flash_loader.c.
   name: original, TI's file name. */
#ifndef F2806X_PIECTRL_H
#define F2806X_PIECTRL_H

struct PIECTRL_BITS {
    unsigned int ENPIE : 1;
    unsigned int PIEVECT : 15;
};
union PIECTRL_REG {
    unsigned int all;
    struct PIECTRL_BITS bit;
};

struct PIEACK_BITS {
    unsigned int ACK1 : 1;
    unsigned int ACK2 : 1;
    unsigned int ACK3 : 1;
    unsigned int ACK4 : 1;
    unsigned int ACK5 : 1;
    unsigned int ACK6 : 1;
    unsigned int ACK7 : 1;
    unsigned int ACK8 : 1;
    unsigned int ACK9 : 1;
    unsigned int ACK10 : 1;
    unsigned int ACK11 : 1;
    unsigned int ACK12 : 1;
    unsigned int rsvd : 4;
};
union PIEACK_REG {
    unsigned int all;
    struct PIEACK_BITS bit;
};

struct PIEIER_BITS {
    unsigned int INTx1 : 1;
    unsigned int INTx2 : 1;
    unsigned int INTx3 : 1;
    unsigned int INTx4 : 1;
    unsigned int INTx5 : 1;
    unsigned int INTx6 : 1;
    unsigned int INTx7 : 1;
    unsigned int INTx8 : 1;
    unsigned int rsvd : 8;
};
union PIEIER_REG {
    unsigned int all;
    struct PIEIER_BITS bit;
};
union PIEIFR_REG {
    unsigned int all;
    struct PIEIER_BITS bit;
};

struct PIE_CTRL_REGS {
    union PIECTRL_REG PIECTRL;
    union PIEACK_REG PIEACK;
    union PIEIER_REG PIEIER1;
    union PIEIFR_REG PIEIFR1;
    union PIEIER_REG PIEIER2;
    union PIEIFR_REG PIEIFR2;
    union PIEIER_REG PIEIER3;
    union PIEIFR_REG PIEIFR3;
    union PIEIER_REG PIEIER4;
    union PIEIFR_REG PIEIFR4;
    union PIEIER_REG PIEIER5;
    union PIEIFR_REG PIEIFR5;
    union PIEIER_REG PIEIER6;
    union PIEIFR_REG PIEIFR6;
    union PIEIER_REG PIEIER7;
    union PIEIFR_REG PIEIFR7;
    union PIEIER_REG PIEIER8;
    union PIEIFR_REG PIEIFR8;
    union PIEIER_REG PIEIER9;
    union PIEIFR_REG PIEIFR9;
    union PIEIER_REG PIEIER10;
    union PIEIFR_REG PIEIFR10;
    union PIEIER_REG PIEIER11;
    union PIEIFR_REG PIEIFR11;
    union PIEIER_REG PIEIER12;
    union PIEIFR_REG PIEIFR12;
};

extern volatile struct PIE_CTRL_REGS PieCtrlRegs;

#define PIEACK_GROUP1 0x0001
#define PIEACK_GROUP2 0x0002
#define PIEACK_GROUP3 0x0004
#define PIEACK_GROUP4 0x0008
#define PIEACK_GROUP5 0x0010
#define PIEACK_GROUP6 0x0020
#define PIEACK_GROUP7 0x0040
#define PIEACK_GROUP8 0x0080
#define PIEACK_GROUP9 0x0100
#define PIEACK_GROUP10 0x0200
#define PIEACK_GROUP11 0x0400
#define PIEACK_GROUP12 0x0800

#endif
