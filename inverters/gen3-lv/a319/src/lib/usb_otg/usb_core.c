/* at: USB_OTG_EnableCommonInt 0x08032D9C */
/* at: USB_OTG_CoreReset 0x08032DD0, USB_OTG_WritePacket 0x08032E3C, USB_OTG_ReadPacket 0x08032E7C, USB_OTG_SelectCore 0x08032EA4, USB_OTG_CoreInit 0x08032F70, USB_OTG_EnableGlobalInt 0x08033096, USB_OTG_DisableGlobalInt 0x080330B8, USB_OTG_FlushTxFifo 0x080330DA, USB_OTG_FlushRxFifo 0x0803312A, USB_OTG_SetCurrentMode 0x08033170, USB_OTG_GetMode 0x080331BE, USB_OTG_IsDeviceMode 0x080331D4, USB_OTG_IsHostMode 0x080331EA, USB_OTG_ReadCoreItr 0x080331FE, USB_OTG_ReadOtgItr 0x0803320E, USB_OTG_EnableHostInt 0x08033216, USB_OTG_ReadHPRT0 0x0803329A, USB_OTG_DriveVbus 0x080332CA, USB_OTG_ResetPort 0x08033326, USB_OTG_InitFSLSPClkSel 0x08033366, USB_OTG_CoreInitHost 0x0803337E, USB_OTG_IsEvenFrame 0x08033454, USB_OTG_ReadHostAllChannels_intr 0x08033464, USB_OTG_HC_Init 0x0803346C, USB_OTG_HC_StartXfer 0x080336A4, USB_OTG_HC_Halt 0x0803389A, USB_OTG_HC_DoPing 0x08033934, USB_OTG_StopHost 0x0803398E */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB OTG low level driver, usb_core.c, host half: core reset and set-up, FIFOs, the root
   port and the host channels.
   Library: ST's STM32 USB OTG host library (STM32_USB-Host-Device_Lib), V2.2.0 or close to
   it: the host states (HOST_WAIT_PRT_ENABLED = 1 ... HOST_ERROR_STATE = 12) and the port-enabled
   callbacks are V2.2.0's; docs/matching.md (round 5) compared usbh_stdreq.c with V2.1.0, which
   has the same functions. I have no copy of the release here, so "unchanged" below means that
   the code has ST's shape and the map's analysis found nothing of GivEnergy's in it.
   Job in this firmware: the STM32F105's USB OTG FS port, as a host only (USE_USB_OTG_FS,
   USE_HOST_MODE, no DMA). task_usb (src/app/usb/usb_task.c) runs USBH_Process. Two classes are
   linked: CDC for the communications dongle, which carries Modbus RTU (modbus_slave_process
   receives through CDC_ReceiveData and answers with CDC_SendData), and MSC for a USB stick with
   firmware update files (FatFs, usbh_msc_fatfs.c). The host starts with the CDC class; when a
   device's first interface is class 8, USBH_USR_Configuration_DescAvailable sets
   usb_class_code and task_usb starts the host again with the MSC class.
   Configuration seen in the code: the FS core at 0x50000000 with 8 host channels and the
   embedded PHY, no DMA, no SOF output, VBUS sensing off (VBUS_SENSING_ENABLED not defined;
   usb_bsp.c drives VBUS), FIFOs of 128 words for receive and 96 each for non-periodic and
   periodic transmit (usb_conf.h). Device mode functions are not built.
   Changes from ST: none found.
   match: the USB_OTG_*_TypeDef union locals and the register structs (src/include/
   usb_otg_types.h) are what make armcc order the register writes as the image does; the
   functions still written with offsets from pdev compile the same (docs/matching.md, round 4).
   name: original (map/filenames.tsv). The old file f08032550.c held the end of usb_core.c and
   the start of usb_hcd.c; it was split at 0x080339EE, where usb_hcd.c starts (2 mod 4, no
   literal pool there), and its first half joined with f0803251C.c. */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/usb.h"
#include "../../include/usb_otg_types.h"

/* Clear the OTG interrupt flags and unmask the wake-up and suspend interrupts. */
void USB_OTG_EnableCommonInt(int param_1)
{
  volatile int local_0;

  local_0 = 0;
  *(int *)(*(int *)(param_1 + 0xc) + 4) = -1;
  *(int *)(*(int *)(param_1 + 0xc) + 0x14) = 0xbfffffff;
  local_0 = (local_0 & ~0x80000000) + 0x80000000;
  local_0 = (local_0 & ~0x800) + 0x800;
  *(int *)(*(int *)(param_1 + 0xc) + 0x18) = local_0;
}

/* Wait for the AHB master to go idle, then soft-reset the core (CSRST), each wait
   bounded at 200000 polls. */
int USB_OTG_CoreReset(int param_1)
{
  int v5;
  int v6;
  volatile int local_0;

  v6 = 0;
  v5 = 0;
  local_0 = 0;
  do {
    USB_OTG_BSP_uDelay(3);
    local_0 = *(int *)(*(int *)(param_1 + 0xc) + 0x10);
    if ((uint)(v5 = v5 + 1) > 0x30d40) {
      return 0;
    }
  } while ((uint)local_0 >> 0x1f == 0);
  v5 = 0;
  local_0 = (local_0 & ~1) + 1;
  *(int *)(*(int *)(param_1 + 0xc) + 0x10) = local_0;
  do {
    local_0 = *(int *)(*(int *)(param_1 + 0xc) + 0x10);
    if ((uint)(v5 = v5 + 1) > 0x30d40) {
      break;
    }
  } while ((local_0 & 1) != 0);
  USB_OTG_BSP_uDelay(3);
  return v6;
}

/* Copy len bytes into a channel's FIFO as 32-bit words (when not using DMA). */
int USB_OTG_WritePacket(uchar *pdev, uchar *src, uchar ch_ep_num, ushort len)
{
  int status = 0;
  if (pdev[3] == 0)
  {
    uint count32b = 0, i = 0;
    volatile uint *fifo;

    count32b = (len + 3) / 4;
    fifo = ((volatile uint **)(pdev + 0xd0))[ch_ep_num];
    for (i = 0; i < count32b; src += 4, i++)
    {
      *fifo = *((__packed uint *)src);
    }
  }
  return status;
}


/* Copy len bytes out of the receive FIFO as 32-bit words; returns the next free byte
   of the destination. */
int USB_OTG_ReadPacket(int param_1, int param_2, int param_3)
{
  int v4;
  int v5;
  int v6;

  v4 = 0;
  v5 = (param_3 + 3) / 4;
  v6 = *(int *)(param_1 + 0xd0);
  for (v4 = 0; (uint)v4 < v5; ) {
    *(int *)param_2 = *(int *)v6;
    param_2 = param_2 + 4;
    v4++;
  }
  return param_2;
}


/* Fill pdev->cfg and the register pointers for the FS core (param_2 1) or HS core. */
int USB_OTG_SelectCore(int param_1, int param_2)
{
  int v0;
  int v1;
  int v3;
  int v5;

  v3 = 0;
  v0 = 0;
  *(volatile uchar *)(param_1 + 3) = 0;
  *(volatile uchar *)(param_1 + 2) = 1;
  *(volatile ushort *)(param_1 + 4) = 0x40;
  if (param_2 == 1) {
    v3 = 0x50000000;
    *(volatile uchar *)(param_1 + 0xb) = 1;
    *(uchar *)param_1 = 8;
    *(volatile uchar *)(param_1 + 1) = 4;
    *(volatile ushort *)(param_1 + 6) = (0x50000000) >> 0x16;
    *(volatile uchar *)(param_1 + 8) = 2;
  }
  else {
    if (param_2 == 0) {
      v3 = 0x40040000;
      *(volatile uchar *)(param_1 + 0xb) = 0;
      *(uchar *)param_1 = 0xc;
      *(volatile uchar *)(param_1 + 1) = 6;
      *(volatile ushort *)(param_1 + 6) = 0x500;
    }
  }
  *(volatile int *)(param_1 + 0xc) = v3;
  *(volatile int *)(param_1 + 0x10) = v3 + 0x800;
  v1 = 0;
  while ((uint)*(volatile uchar *)(param_1 + 1) > v1) {
    *(volatile int *)(param_1 + 0x18 + v1 * 4) = v3 + 0x900 + (v1 << 5);
    *(volatile int *)(param_1 + 0x54 + v1 * 4) = v3 + 0xb00 + (v1 << 5);
    v1++;
  }
  *(volatile int *)(param_1 + 0x14) = (v3 + 0x400);
  *(volatile int *)(param_1 + 0xcc) = (v3 + 0x400) + 0x40;
  v1 = 0;
  while ((uint)*(uchar *)param_1 > v1) {
    *(volatile int *)(param_1 + 0x90 + v1 * 4) = v3 + 0x500 + (v1 << 5);
    v1++;
  }
  v1 = 0;
  while ((uint)*(uchar *)param_1 > v1) {
    *(volatile int *)(param_1 + 0xd0 + v1 * 4) = v3 + 0x1000 + (v1 << 0xc);
    v1++;
  }
  *(volatile int *)(param_1 + 0x10c) = v3 + 0xe00;
  return v0;
}


/* Set up the core: PHY selection, power-down off, VBUS sensing, core reset. */
int USB_OTG_CoreInit(int param_1)
{
  int v5;
  volatile int local_8;
  volatile int local_4;
  volatile int local_0;

  v5 = 0;
  local_8 = 0;
  local_4 = 0;
  local_0 = 0;
  if (*(uchar *)(param_1 + 8) == 1) {
    local_4 = *(int *)(*(int *)(param_1 + 0xc) + 0x38);
    local_4 = local_4 & -65537;
    if (*(uchar *)(param_1 + 9) != 0) {
      local_4 = (local_4 & -1048577) + 0x100000;
    }
    *(int *)(*(int *)(param_1 + 0xc) + 0x38) = local_4;
    local_8 = 0;
    local_8 = *(int *)(*(int *)(param_1 + 0xc) + 0xc);
    local_8 = local_8 & -65;
    local_8 = local_8 & -4194305;
    local_8 = local_8 & -131073;
    local_8 = local_8 & -524289;
    *(int *)(*(int *)(param_1 + 0xc) + 0xc) = local_8;
    USB_OTG_CoreReset(param_1);
    if (*(uchar *)(param_1 + 3) == 1) {
      local_0 = (local_0 & -31) + 0xa;
      local_0 = (local_0 & -33) + 0x20;
      *(int *)(*(int *)(param_1 + 0xc) + 8) = local_0;
    }
  }
  else {
    local_8 = *(int *)(*(int *)(param_1 + 0xc) + 0xc);
    local_8 = (local_8 & -65) + 0x40;
    *(int *)(*(int *)(param_1 + 0xc) + 0xc) = local_8;
    USB_OTG_CoreReset(param_1);
    local_4 = 0;
    local_4 = (local_4 & -65537) + 0x10000;
    local_4 = (local_4 & -262145) + 0x40000;
    local_4 = (local_4 & -524289) + 0x80000;
    local_4 = (local_4 & -2097153) + 0x200000;
    if (*(uchar *)(param_1 + 9) != 0) {
      local_4 = (local_4 & -1048577) + 0x100000;
    }
    *(int *)(*(int *)(param_1 + 0xc) + 0x38) = local_4;
    USB_OTG_BSP_mDelay(0x14);
  }
  if (*(uchar *)(param_1 + 3) == 1) {
    local_0 = *(int *)(*(int *)(param_1 + 0xc) + 8);
    local_0 = (local_0 & -31) + 0xa;
    local_0 = (local_0 & -33) + 0x20;
    *(int *)(*(int *)(param_1 + 0xc) + 8) = local_0;
  }
  return v5;
}


/* Set the global interrupt mask bit in GAHBCFG. */
int USB_OTG_EnableGlobalInt(int param_1)
{
  int v0;
  volatile int local_0;

  v0 = 0;
  local_0 = 0;
  local_0 = (local_0 & -2) + 1;
  *(int *)(*(int *)(param_1 + 0xc) + 8) = *(int *)(*(int *)(param_1 + 0xc) + 8) | local_0;
  return v0;
}


/* Clear the global interrupt mask bit in GAHBCFG. */
int USB_OTG_DisableGlobalInt(int param_1)
{
  int v0;
  volatile int local_0;

  v0 = 0;
  local_0 = 0;
  local_0 = (local_0 & -2) + 1;
  *(int *)(*(int *)(param_1 + 0xc) + 8) = *(int *)(*(int *)(param_1 + 0xc) + 8) & ~local_0;
  return v0;
}


/* Flush transmit FIFO param_2 (0x10 for all). */
int USB_OTG_FlushTxFifo(int param_1, int param_2)
{
  int v6;
  int v7;
  volatile int local_0;

  v6 = 0;
  v7 = 0;
  local_0 = 0;
  local_0 = (local_0 & -33) + 0x20;
  local_0 = local_0 & 0xfffff83f | (param_2 & 0x1f) << 6;
  *(int *)(*(int *)(param_1 + 0xc) + 0x10) = local_0;
  do {
    local_0 = *(int *)(*(int *)(param_1 + 0xc) + 0x10);
    if ((uint)(v7 = v7 + 1) > 0x30d40) {
      break;
    }
  } while (((uint)local_0 >> 5 & 1) != 0);
  USB_OTG_BSP_uDelay(3);
  return v6;
}


/* Flush the receive FIFO. */
int USB_OTG_FlushRxFifo(int param_1)
{
  int v5;
  int v6;
  volatile int local_0;

  v5 = 0;
  v6 = 0;
  local_0 = 0;
  local_0 = (local_0 & -17) + 0x10;
  *(int *)(*(int *)(param_1 + 0xc) + 0x10) = local_0;
  do {
    local_0 = *(int *)(*(int *)(param_1 + 0xc) + 0x10);
    if ((uint)(v6 = v6 + 1) > 0x30d40) {
      break;
    }
  } while (((uint)local_0 >> 4 & 1) != 0);
  USB_OTG_BSP_uDelay(3);
  return v5;
}


/* Force host (1) or device (0) mode in GUSBCFG and wait 50 ms. */
int USB_OTG_SetCurrentMode(int param_1, int param_2)
{
  int v6;
  volatile int local_0;

  v6 = 0;
  local_0 = *(int *)(*(int *)(param_1 + 0xc) + 0xc);
  local_0 = local_0 & -536870913;
  local_0 = local_0 & -1073741825;
  if (param_2 == 1) {
    local_0 = (local_0 & -536870913) + 0x20000000;
  }
  else {
    if (param_2 == 0) {
      local_0 = (local_0 & -1073741825) + 0x40000000;
    }
  }
  *(int *)(*(int *)(param_1 + 0xc) + 0xc) = local_0;
  USB_OTG_BSP_mDelay(0x32);
  return v6;
}

/* Current mode from GINTSTS: 1 host, 0 device. */
uint USB_OTG_GetMode(int param_1)
{

  return *(volatile uint *)(*(volatile int *)(param_1 + 0xc) + 0x14) & 1;
}


/* Non-zero in device mode. */
uchar USB_OTG_IsDeviceMode(int param_1)
{
  uint uVar1;

  return (USB_OTG_GetMode(param_1)) != 1;
}


/* Non-zero in host mode. */
uchar USB_OTG_IsHostMode(int param_1)
{
  uint uVar1;

  return (USB_OTG_GetMode(param_1)) == 1;
}


/* Pending and unmasked core interrupts (GINTSTS & GINTMSK). */
int USB_OTG_ReadCoreItr(int param_1)
{
  int v0;

  v0 = 0;
  v0 = *(int *)(*(int *)(param_1 + 0xc) + 0x14);
  v0 = v0 & *(int *)(*(int *)(param_1 + 0xc) + 0x18);
  return v0;
}

/* The OTG interrupt register GOTGINT. */
uint USB_OTG_ReadOtgItr(int param_1)
{

  return *(uint *)(*(int *)(param_1 + 0xc) + 4);
}


/* Unmask the host interrupts (receive FIFO, incomplete periodic, channels, port) and
   mask disconnect and SOF, as ST does. */
int USB_OTG_EnableHostInt(int param_1)
{
  int v1;
  int v5;
  volatile int local_0;

  v5 = 0;
  local_0 = 0;
  *(uint *)(*(uint *)(param_1 + 0xc) + 0x18) = 0;
  *(uint *)(*(uint *)(param_1 + 0xc) + 0x14) = -1;
  USB_OTG_EnableCommonInt(param_1);
  if (*(sbyte *)(param_1 + 3) == 0) {
    local_0 = (local_0 & -17) + 0x10;
  }
  local_0 = (local_0 & -2097153) + 0x200000;
  local_0 = (local_0 & -33554433) + 0x2000000;
  local_0 = (local_0 & -16777217) + 0x1000000;
  *(uint *)(*(uint *)(param_1 + 0xc) + 0x18) = *(uint *)(*(uint *)(param_1 + 0xc) + 0x18) & ~(local_0) | (local_0);
  local_0 = 0;
  local_0 = (local_0 & -536870913) + 0x20000000;
  local_0 = (local_0 & -9) + 8;
  *(uint *)(*(uint *)(param_1 + 0xc) + 0x18) = *(uint *)(*(uint *)(param_1 + 0xc) + 0x18) & ~local_0;
  return v5;
}

/* HPRT0 with its write-1-to-clear bits masked off, safe to write back. */
int USB_OTG_ReadHPRT0(int param_1)
{
  volatile int local_0;

  local_0 = *(int *)(*(int *)(param_1 + 0xcc));
  local_0 = local_0 & -5;
  local_0 = local_0 & -3;
  local_0 = local_0 & -9;
  local_0 = local_0 & -33;
  return local_0;
}

/* Switch VBUS through the board (USB_OTG_BSP_DriveVBUS) and the port power bit; 200 ms. */
void USB_OTG_DriveVbus(USB_OTG_CORE_HANDLE *pdev, uchar state)
{
  USB_OTG_HPRT0_TypeDef hprt0;

  hprt0.d32 = 0;
  USB_OTG_BSP_DriveVBUS(pdev, state);
  hprt0.d32 = USB_OTG_ReadHPRT0(pdev);
  if ((hprt0.b.prtpwr == 0) && (state == 1))
  {
    hprt0.b.prtpwr = 1;
    USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
  }
  if ((hprt0.b.prtpwr == 1) && (state == 0))
  {
    hprt0.b.prtpwr = 0;
    USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
  }
  USB_OTG_BSP_mDelay(200);
}

/* Reset the root port: PRTRST for 100 ms, then release and wait 20 ms. */
uint USB_OTG_ResetPort(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HPRT0_TypeDef hprt0;

  hprt0.d32 = USB_OTG_ReadHPRT0(pdev);
  hprt0.b.prtrst = 1;
  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
  USB_OTG_BSP_mDelay(100);
  hprt0.b.prtrst = 0;
  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
  USB_OTG_BSP_mDelay(20);
  return 1;
}

/* Set the FS/LS PHY clock select in HCFG (48 or 6 MHz). */
void USB_OTG_InitFSLSPClkSel(USB_OTG_CORE_HANDLE *pdev, uchar freq)
{
  USB_OTG_HCFG_TypeDef hcfg;

  hcfg.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HCFG);
  hcfg.b.fslspclksel = freq;
  USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HCFG, hcfg.d32);
}

/* Host set-up: FIFO sizes, flushes, all channel interrupts cleared, VBUS on, host
   interrupts enabled. */
int USB_OTG_CoreInitHost(USB_OTG_CORE_HANDLE *pdev)
{
  int status = 0;
  USB_OTG_FSIZ_TypeDef nptxfifosize;
  USB_OTG_FSIZ_TypeDef ptxfifosize;
  USB_OTG_HCFG_TypeDef hcfg;
  uint i = 0;

  nptxfifosize.d32 = 0;
  ptxfifosize.d32 = 0;
  hcfg.d32 = 0;

  USB_OTG_BSP_ConfigVBUS(pdev);

  USB_OTG_WRITE_REG32(pdev->regs.PCGCCTL, 0);

  if (pdev->cfg.phy_itface == 1)
  {
    USB_OTG_InitFSLSPClkSel(pdev, 0);
  }
  else
  {
    USB_OTG_InitFSLSPClkSel(pdev, 1);
  }
  USB_OTG_ResetPort(pdev);

  hcfg.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HCFG);
  hcfg.b.fslssupp = 0;
  USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HCFG, hcfg.d32);

  if (pdev->cfg.coreID == 1)
  {
    USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GRXFSIZ, 128);
    nptxfifosize.b.startaddr = 128;
    nptxfifosize.b.depth = 96;
    USB_OTG_WRITE_REG32(&pdev->regs.GREGS->DIEPTXF0_HNPTXFSIZ, nptxfifosize.d32);

    ptxfifosize.b.startaddr = 128 + 96;
    ptxfifosize.b.depth = 96;
    USB_OTG_WRITE_REG32(&pdev->regs.GREGS->HPTXFSIZ, ptxfifosize.d32);
  }

  USB_OTG_FlushTxFifo(pdev, 0x10);
  USB_OTG_FlushRxFifo(pdev);

  for (i = 0; i < pdev->cfg.host_channels; i++)
  {
    USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[i]->HCINT, 0xFFFFFFFF);
    USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[i]->HCINTMSK, 0);
  }
  USB_OTG_DriveVbus(pdev, 1);

  USB_OTG_EnableHostInt(pdev);
  return status;
}

/* Non-zero in an even (micro)frame. */
uint USB_OTG_IsEvenFrame(int param_1)
{

  return *(volatile uint *)(*(volatile int *)(param_1 + 0x14) + 8) & 1 ^ 1;
}

/* The host channel interrupt summary HAINT. */
uint USB_OTG_ReadHostAllChannels_intr(int param_1)
{

  return *(uint *)(*(int *)(param_1 + 0x14) + 0x14);
}

/* Set up host channel hc_num: interrupt mask by endpoint type, HCCHAR (address,
   endpoint, direction, type, packet size). */
int USB_OTG_HC_Init(USB_OTG_CORE_HANDLE *pdev, uchar hc_num)
{
  int status = 0;
  uint intr_enable = 0;
  USB_OTG_HCINTMSK_TypeDef hcintmsk;
  USB_OTG_GINTMSK_TypeDef gintmsk;
  USB_OTG_HCCHAR_TypeDef hcchar;
  USB_OTG_HCINTn_TypeDef hcint;

  gintmsk.d32 = 0;
  hcintmsk.d32 = 0;
  hcchar.d32 = 0;

  hcint.d32 = 0xFFFFFFFF;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCINT, hcint.d32);

  hcintmsk.d32 = 0;

  if (pdev->cfg.dma_enable == 1)
  {
    hcintmsk.b.ahberr = 1;
  }

  switch (pdev->host.hc[hc_num].ep_type)
  {
  case 0:
  case 2:
    hcintmsk.b.xfercompl = 1;
    hcintmsk.b.stall = 1;
    hcintmsk.b.xacterr = 1;
    hcintmsk.b.datatglerr = 1;
    hcintmsk.b.nak = 1;
    if (pdev->host.hc[hc_num].ep_is_in)
    {
      hcintmsk.b.bblerr = 1;
    }
    else
    {
      hcintmsk.b.nyet = 1;
      if (pdev->host.hc[hc_num].do_ping)
      {
        hcintmsk.b.ack = 1;
      }
    }
    break;
  case 3:
    hcintmsk.b.xfercompl = 1;
    hcintmsk.b.nak = 1;
    hcintmsk.b.stall = 1;
    hcintmsk.b.xacterr = 1;
    hcintmsk.b.datatglerr = 1;
    hcintmsk.b.frmovrun = 1;

    if (pdev->host.hc[hc_num].ep_is_in)
    {
      hcintmsk.b.bblerr = 1;
    }

    break;
  case 1:
    hcintmsk.b.xfercompl = 1;
    hcintmsk.b.frmovrun = 1;
    hcintmsk.b.ack = 1;

    if (pdev->host.hc[hc_num].ep_is_in)
    {
      hcintmsk.b.xacterr = 1;
      hcintmsk.b.bblerr = 1;
    }
    break;
  }

  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCINTMSK, hcintmsk.d32);

  intr_enable = (1 << hc_num);
  USB_OTG_MODIFY_REG32(&pdev->regs.HREGS->HAINTMSK, 0, intr_enable);

  gintmsk.b.hcintr = 1;
  USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, 0, gintmsk.d32);

  hcchar.d32 = 0;
  hcchar.b.devaddr = pdev->host.hc[hc_num].dev_addr;
  hcchar.b.epnum   = pdev->host.hc[hc_num].ep_num;
  hcchar.b.epdir   = pdev->host.hc[hc_num].ep_is_in;
  hcchar.b.lspddev = (pdev->host.hc[hc_num].speed == 2);
  hcchar.b.eptype  = pdev->host.hc[hc_num].ep_type;
  hcchar.b.mps     = pdev->host.hc[hc_num].max_packet;
  if (pdev->host.hc[hc_num].ep_type == 3)
  {
    hcchar.b.oddfrm  = 1;
  }
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);
  return status;
}

/* Start a transfer on a channel: packet count, PID, odd frame for periodic, enable;
   writes the first OUT data into the FIFO when there is room. */
int USB_OTG_HC_StartXfer(USB_OTG_CORE_HANDLE *pdev, uchar hc_num)
{
  int status = 0;
  USB_OTG_HCCHAR_TypeDef hcchar;
  USB_OTG_HCTSIZn_TypeDef hctsiz;
  USB_OTG_HNPTXSTS_TypeDef hnptxsts;
  USB_OTG_HPTXSTS_TypeDef hptxsts;
  USB_OTG_GINTMSK_TypeDef intmsk;
  ushort len_words = 0;

  ushort num_packets;
  ushort max_hc_pkt_count;

  max_hc_pkt_count = 256;
  hctsiz.d32 = 0;
  hcchar.d32 = 0;
  intmsk.d32 = 0;

  if (pdev->host.hc[hc_num].xfer_len > 0)
  {
    num_packets = (pdev->host.hc[hc_num].xfer_len +
      pdev->host.hc[hc_num].max_packet - 1) / pdev->host.hc[hc_num].max_packet;

    if (num_packets > max_hc_pkt_count)
    {
      num_packets = max_hc_pkt_count;
      pdev->host.hc[hc_num].xfer_len = num_packets *
        pdev->host.hc[hc_num].max_packet;
    }
  }
  else
  {
    num_packets = 1;
  }
  if (pdev->host.hc[hc_num].ep_is_in)
  {
    pdev->host.hc[hc_num].xfer_len = num_packets *
      pdev->host.hc[hc_num].max_packet;
  }
  hctsiz.b.xfersize = pdev->host.hc[hc_num].xfer_len;
  hctsiz.b.pktcnt = num_packets;
  hctsiz.b.pid = pdev->host.hc[hc_num].data_pid;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCTSIZ, hctsiz.d32);

  if (pdev->cfg.dma_enable == 1)
  {
    USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCDMA, (unsigned int)pdev->host.hc[hc_num].xfer_buff);
  }

  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR);
  hcchar.b.oddfrm = USB_OTG_IsEvenFrame(pdev);

  hcchar.b.chen = 1;
  hcchar.b.chdis = 0;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);

  if (pdev->cfg.dma_enable == 0)
  {
    if ((pdev->host.hc[hc_num].ep_is_in == 0) &&
        (pdev->host.hc[hc_num].xfer_len > 0))
    {
      switch (pdev->host.hc[hc_num].ep_type)
      {
      case 0:
      case 2:

        hnptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->HNPTXSTS);
        len_words = (pdev->host.hc[hc_num].xfer_len + 3) / 4;

        if (len_words > hnptxsts.b.nptxfspcavail)
        {
          intmsk.b.nptxfempty = 1;
          USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, 0, intmsk.d32);
        }

        break;
      case 3:
      case 1:
        hptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HPTXSTS);
        len_words = (pdev->host.hc[hc_num].xfer_len + 3) / 4;
        if (len_words > hptxsts.b.ptxfspcavail)
        {
          intmsk.b.ptxfempty = 1;
          USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, 0, intmsk.d32);
        }
        break;

      default:
        break;
      }

      USB_OTG_WritePacket(pdev,
                   pdev->host.hc[hc_num].xfer_buff,
                   hc_num, pdev->host.hc[hc_num].xfer_len);
    }
  }
  return status;
}

/* Halt a channel: CHDIS with CHEN; when its request queue has no room, first write
   CHDIS without CHEN. */
int USB_OTG_HC_Halt(USB_OTG_CORE_HANDLE *pdev, uchar hc_num)
{
  int status = 0;
  USB_OTG_HNPTXSTS_TypeDef nptxsts;
  USB_OTG_HPTXSTS_TypeDef hptxsts;
  USB_OTG_HCCHAR_TypeDef hcchar;

  nptxsts.d32 = 0;
  hptxsts.d32 = 0;
  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR);
  hcchar.b.chdis = 1;

  if (hcchar.b.eptype == 0 || hcchar.b.eptype == 2)
  {
    nptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->HNPTXSTS);
    if (nptxsts.b.nptxqspcavail == 0)
    {
      hcchar.b.chen = 0;
      USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);
    }
  }
  else
  {
    hptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HPTXSTS);
    if (hptxsts.b.ptxqspcavail == 0)
    {
      hcchar.b.chen = 0;
      USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);
    }
  }
  hcchar.b.chen = 1;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);
  return status;
}

/* Start a PING on a high-speed OUT channel (not used by the FS core). */
int USB_OTG_HC_DoPing(USB_OTG_CORE_HANDLE *pdev, uchar hc_num)
{
  int status = 0;
  USB_OTG_HCCHAR_TypeDef hcchar;
  USB_OTG_HCTSIZn_TypeDef hctsiz;

  hctsiz.d32 = 0;
  hctsiz.b.dopng = 1;
  hctsiz.b.pktcnt = 1;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCTSIZ, hctsiz.d32);

  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR);
  hcchar.b.chen = 1;
  hcchar.b.chdis = 0;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[hc_num]->HCCHAR, hcchar.d32);
  return status;
}

/* Stop all channels, clear their interrupts and flush the FIFOs. No callers. */
void USB_OTG_StopHost(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HCCHAR_TypeDef hcchar;
  uint i;

  USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HAINTMSK, 0);
  USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HAINT, 0xFFFFFFFF);

  for (i = 0; i < pdev->cfg.host_channels; i++)
  {
    hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[i]->HCCHAR);
    hcchar.b.chen = 0;
    hcchar.b.chdis = 1;
    hcchar.b.epdir = 0;
    USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[i]->HCCHAR, hcchar.d32);
  }

  USB_OTG_FlushRxFifo(pdev);
  USB_OTG_FlushTxFifo(pdev, 0x10);
}
