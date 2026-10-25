/* at: sys_startup_delay 0x08014234, firmware_main 0x08014240, main 0x08014240 */
/* at: g_20007496 0x200074AE */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow the startup wait, board initialization and five RTOS task creations here.
   main starts the scheduler after setting the task delay/wait fields.
   firmware_main retains the descriptive entry alias. Defining main(void) also
   lets armcc emit the standard library's zero-byte no-argv selector.
   This unit owns no RAM objects; the scheduler storage is an external view.
   match: I retain literal task entry addresses, stack sizes and creation order. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
void OSInit(void);
void OS_CPU_SysTickInit(void);
void SystemInit(void);
void sys_irq_disable(void);
void board_initialize_pb5(void);
void board_initialize_peripherals(void);
void sub_0802A21A(void);
void sub_0802A23C(undefined4, undefined4, int, int);
int main(void);
uint firmware_main(void) __attribute__((alias("main")));
void sys_startup_delay();

/* Busy wait through 10,000,000 iterations.
   match: I keep the original loop and count; no calibrated duration is claimed. */
void sys_startup_delay() {
    uint startup_delay_count;

    startup_delay_count = 0;
    do {
        startup_delay_count = startup_delay_count + 1;
    } while (startup_delay_count < 10000000);
    return;
}

extern uchar g_20007496[];
#define DAT_20007498 (*(ushort *)(g_20007496 + 2))
#define DAT_200074aa (*(ushort *)(g_20007496 + 20))
#define DAT_200074ac (*(ushort *)(g_20007496 + 22))
#define DAT_200074be (*(ushort *)(g_20007496 + 40))
#define DAT_200074c0 (*(ushort *)(g_20007496 + 42))
#define DAT_200074d2 (*(ushort *)(g_20007496 + 60))
#define DAT_200074d4 (*(ushort *)(g_20007496 + 62))
#define DAT_200074e6 (*(ushort *)(g_20007496 + 80))
#define DAT_200074e8 (*(ushort *)(g_20007496 + 82))
#define g_20007496_v (*(ushort *)(g_20007496 + 0))

/* Initializes system/board and creates five RTOS tasks before scheduler start.
   match: Task entry values retain their Thumb bit and original integer ABI. */
int main(void) {

    SystemInit();
    board_initialize_pb5();
    sys_irq_disable();
    sys_startup_delay();
    board_initialize_peripherals();
    OSInit();
    sub_0802A23C(0x08023C09, 0, 900, 0);
    DAT_20007498 = 0;
    g_20007496_v = 2000;
    sub_0802A23C(0x08024B11, 0, 400, 1);
    DAT_200074ac = 0;
    DAT_200074aa = 200;
    sub_0802A23C(0x08025B95, 0, 400, 2);
    DAT_200074c0 = 0;
    DAT_200074be = 2000;
    sub_0802A23C(0x08015877, 0, 400, 3);
    DAT_200074d4 = 0;
    DAT_200074d2 = 0x14;
    sub_0802A23C(0x080169DD, 0, 800, 4);
    DAT_200074e8 = 0;
    DAT_200074e6 = 1000;
    OS_CPU_SysTickInit();
    sub_0802A21A();
    return 0;
}
#undef DAT_20007498
#undef DAT_200074aa
#undef DAT_200074ac
#undef DAT_200074be
#undef DAT_200074c0
#undef DAT_200074d2
#undef DAT_200074d4
#undef DAT_200074e6
#undef DAT_200074e8
#undef g_20007496_v
