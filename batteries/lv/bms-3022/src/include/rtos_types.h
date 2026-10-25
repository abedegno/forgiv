/* rtos_types.h: the application's small task scheduler (names mine, from their use): eleven task
   control blocks of 20 bytes at 0x20007C84, a 16-bit ready mask, the current and highest ready
   task. Used by tasks.h and src/rtos.
   name: descriptive. */
#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

typedef struct {
    unsigned int *StkPtr; /* 0x00, saved by the PendSV handler */
    unsigned char Reserved_[6];
    unsigned short period;    /* 0x0a, ticks between wake-ups (0: none) */
    unsigned short count;     /* 0x0c */
    unsigned short FlagsRdy;  /* 0x0e, events posted */
    unsigned short FlagsWait; /* 0x10, events waited for */
    unsigned char Reserved2_[2];
} OS_TCB;

#endif
