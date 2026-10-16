/* at: SetSysClockTo72 0x08014CA4, SetSysClock 0x08014DAC, SystemInit 0x08014DB4, SystemCoreClockUpdate 0x08014E10 */
/* at: SystemCoreClock 0x20000034, AHBPrescTable 0x20000038 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* CMSIS system file: the clock tree at reset and SystemCoreClock.
   Library: CMSIS Cortex-M3 device support from ST's STM32F10x StdPeriph Library V3.5.0,
   system_stm32f10x.c, for the connectivity line (STM32F10X_CL, the STM32F105/107).
   Job in this firmware: main calls SystemInit first. It sets 72 MHz from the HSE crystal. The 1 ms
   tick (OS_CPU_SysTickInit) uses SystemCoreClock; the StdPeriph baud rate code reads the clocks
   through RCC_GetClocksFreq, built with the same 8 MHz HSE_VALUE (stm32f10x_rcc.c).
   Configuration and vendor changes:
   - The board has an 8 MHz crystal (HSE_VALUE 8000000, 0x7A1200 in SystemCoreClockUpdate), where
     V3.5.0 assumes 25 MHz for the connectivity line. SetSysClockTo72 writes RCC_CFGR2 0x10814:
     PREDIV2 /2, PLL2 x10 (40 MHz), PREDIV1 from PLL2 /5 (8 MHz), then PLL x9 = 72 MHz. ST's
     value for a 25 MHz crystal is 0x10644 (/5, x8, /5). Flash latency 2, APB1 /2.
   - SYSCLK_FREQ_72MHz is the only clock choice compiled in (SetSysClock calls SetSysClockTo72).
   - VTOR is set to 0x08000000 (VECT_TAB_OFFSET 0, as shipped). The application runs at
     0x08014000 behind a boot loader and nvic_init sets the offset 0x14000 later.
   - The registers are written as plain addresses here (RCC_BASE + n), not ST's RCC-> fields;
     the bytes are the same.
   match: the order of the functions differs from ST's file: the two static clock functions
   come first, with prototypes above them. ST's file has SystemInit and SystemCoreClockUpdate
   first and the static ones at the end. This order gives the image's layout.
   name: original (map/filenames.tsv): the four CMSIS names and AHBPrescTable; third file of the
   ST template's USER group after main.c and stm32f10x_it.c. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/sys.h"
#include "../include/stdperiph.h"
static void SetSysClockTo72();
static void SetSysClock();

/* Start the HSE (timeout 0x500 polls), set flash latency 2 and the bus dividers, run
   PLL2 and then the main PLL from the 8 MHz crystal to 72 MHz, and switch SYSCLK to the PLL.
   If the HSE does not start, the clock stays on the 8 MHz HSI. */
static void SetSysClockTo72()
{
  volatile int local_4;
  volatile int local_0;
  
  local_4 = 0;
  local_0 = 0;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE | 0x10000;
  do {
    local_0 = *(volatile uint *)RCC_BASE & 0x20000;
    local_4 = local_4 + 1;
  } while (local_0 == 0 && local_4 != 0x500);
  if ((*(volatile uint *)RCC_BASE & 0x20000) != 0) {
    local_0 = 1;
  }
  else {
    local_0 = 0;
  }
  if (local_0 == 1) {
    *(volatile uint *)FLASH_R_BASE = *(volatile uint *)FLASH_R_BASE | 0x10;
    *(volatile uint *)FLASH_R_BASE = *(volatile uint *)FLASH_R_BASE & ~3;
    *(volatile uint *)FLASH_R_BASE = *(volatile uint *)FLASH_R_BASE | 2;
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4);
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4);
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4) | 0x400;
    *(volatile uint *)(RCC_BASE + 0x2c) = *(volatile uint *)(RCC_BASE + 0x2c) & 0xfffef000;
    *(volatile uint *)(RCC_BASE + 0x2c) = *(volatile uint *)(RCC_BASE + 0x2c) | 0x10814;
    *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE | 0x4000000;
    do {
    } while ((*(volatile uint *)RCC_BASE & 0x8000000) == 0);
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4) & ~0x3f0000;
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4) | 0x1d0000;
    *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE | 0x1000000;
    do {
    } while ((*(volatile uint *)RCC_BASE & 0x2000000) == 0);
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4) & ~3;
    *(volatile uint *)(RCC_BASE + 4) = *(volatile uint *)(RCC_BASE + 4) | 2;
    do {
    } while ((*(volatile uint *)(RCC_BASE + 4) & 0xc) != 8);
  }
}

/* Clock choice: only SYSCLK_FREQ_72MHz is built. */
static void SetSysClock()
{

  
  SetSysClockTo72();
}

/* Reset the RCC to its default state (HSI on, PLLs off, interrupts cleared), set the
   clock with SetSysClock, and point the vector table at 0x08000000. */
void SystemInit()
{

  
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE | 1;
  *(volatile uint *)0x40021004 = *(volatile uint *)0x40021004 & 0xf0ff0000;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & 0xfef6ffff;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & -262145;
  *(volatile uint *)0x40021004 = *(volatile uint *)0x40021004 & -8323073;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & -335544321;
  *(volatile uint *)0x40021008 = 0xff0000;
  *(volatile uint *)0x4002102c = 0;
  SetSysClock();
  *(volatile uint *)SCB_VTOR_ADDR = 0x8000000;
}


/* Recompute SystemCoreClock from the RCC registers (HSI, HSE or PLL via PREDIV1/PLL2) and
   the AHB prescaler. No callers. */
void SystemCoreClockUpdate()
{
  uint tmp = 0, pllmull = 0, pllsource = 0;
  uint prediv1source = 0, prediv1factor = 0, prediv2factor = 0, pll2mull = 0;

  tmp = *(volatile uint *)(RCC_BASE + 4) & 0xc;
  switch (tmp) {
  case 0x00:
    SystemCoreClock = 0x7a1200;
    break;
  case 0x04:
    SystemCoreClock = 0x7a1200;
    break;
  case 0x08:
    pllmull = *(volatile uint *)(RCC_BASE + 4) & 0x3c0000;
    pllsource = *(volatile uint *)(RCC_BASE + 4) & 0x10000;
    pllmull = pllmull >> 18;
    if (pllmull != 0x0D)
      pllmull += 2;
    else
      pllmull = 13 / 2;
    if (pllsource == 0x00)
      SystemCoreClock = (0x7a1200 >> 1) * pllmull;
    else {
      prediv1source = *(volatile uint *)(RCC_BASE + 0x2c) & 0x10000;
      prediv1factor = (*(volatile uint *)(RCC_BASE + 0x2c) & 0xf) + 1;
      if (prediv1source == 0)
        SystemCoreClock = (0x7a1200 / prediv1factor) * pllmull;
      else {
        prediv2factor = ((*(volatile uint *)(RCC_BASE + 0x2c) & 0xf0) >> 4) + 1;
        pll2mull = ((*(volatile uint *)(RCC_BASE + 0x2c) & 0xf00) >> 8) + 2;
        SystemCoreClock = (((0x7a1200 / prediv2factor) * pll2mull) / prediv1factor) * pllmull;
      }
    }
    break;
  default:
    SystemCoreClock = 0x7a1200;
    break;
  }
  tmp = ((uchar *)(AHBPrescTable + 0))[((*(volatile uint *)(RCC_BASE + 4) & 0xf0) >> 4)];
  SystemCoreClock >>= tmp;
}

