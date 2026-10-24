/* at: SetSysClockTo72 0x080148F8, SetSysClock 0x080149BC, SystemInit 0x080149BE, SystemCoreClockUpdate 0x08014A0A */
/* at: SystemCoreClock 0x2000000C, AHBPrescTable 0x20000010 */
/* opts: --cpu Cortex-M3 -O1 */
/* CMSIS clock setup for STM32F105/107, adapted from ST StdPeriph 3.5.0 and
   the matched A316 source. The 8 MHz HSE uses PLL2 /2 x10, then PREDIV1 /5
   and PLL x9 for 72 MHz. This unit defines its clock and prescaler globals;
   SystemCoreClockUpdate addresses the prescaler table from SystemCoreClock.
   VTOR initially selects 0x08000000, as in the shipped application. */
typedef unsigned int uint;
typedef unsigned char uchar;
#define RCC_BASE 0x40021000
#define FLASH_R_BASE 0x40022000
#define SCB_VTOR_ADDR 0xE000ED08
uint SystemCoreClock = 72000000;
uchar AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
static void SetSysClockTo72(void);
static void SetSysClock(void);

/* Start the HSE (timeout 0x500 polls), set flash latency 2 and the bus dividers, run
   PLL2 and then the main PLL from the 8 MHz crystal to 72 MHz, and switch SYSCLK to the PLL.
   If the HSE does not start, the clock stays on the 8 MHz HSI. */
static void SetSysClockTo72(void)
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
static void SetSysClock(void)
{

  
  SetSysClockTo72();
}

/* Reset the RCC to its default state (HSI on, PLLs off, interrupts cleared), set the
   clock with SetSysClock, and point the vector table at 0x08000000. */
void SystemInit(void)
{

  
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE | 1;
  *(volatile uint *)0x40021004 = *(volatile uint *)0x40021004 & 0xf0ff0000;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & 0xfef6ffff;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & ~0x40000u;
  *(volatile uint *)0x40021004 = *(volatile uint *)0x40021004 & ~0x7f0000u;
  *(volatile uint *)RCC_BASE = *(volatile uint *)RCC_BASE & ~0x14000000u;
  *(volatile uint *)0x40021008 = 0xff0000;
  *(volatile uint *)0x4002102c = 0;
  SetSysClock();
  *(volatile uint *)SCB_VTOR_ADDR = 0x8000000;
}


/* Recompute SystemCoreClock from the RCC registers (HSI, HSE or PLL via PREDIV1/PLL2) and
   the AHB prescaler. No callers. */
void SystemCoreClockUpdate(void)
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

