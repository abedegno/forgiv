/* at: USB_OTG_USBH_handle_IncompletePeriodicXfer_ISR 0x0803325C, USB_OTG_USBH_handle_Disconnect_ISR 0x080332A2, USB_OTG_USBH_handle_port_ISR 0x080332CA, USB_OTG_USBH_handle_hc_n_Out_ISR 0x08033400 */
/* at: USBH_HCD_INT_fops 0x200004F8 */
/* at: USB_OTG_USBH_handle_hc_n_In_ISR 0x080337C0, USB_OTG_USBH_handle_hc_ISR 0x08033BDE */
/* at: USB_OTG_USBH_handle_ptxfempty_ISR 0x08033C34, USB_OTG_USBH_handle_nptxfempty_ISR 0x08033D62 */
/* at: USB_OTG_USBH_handle_rx_qlvl_ISR 0x08033E90 */
/* at: USB_OTG_USBH_handle_sof_ISR 0x08033FF4, USBH_OTG_ISR_Handler 0x0803401A */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host interrupt handling, usb_hcd_int.c: the OTG FS interrupt in host mode.
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
   OTG_FS_IRQHandler (stm32f10x_it.c) calls USBH_OTG_ISR_Handler. The connect, disconnect, port
   and SOF events go to usbh_core.c through USBH_HCD_INT_fops.
   Changes from ST: none found.
   match: channelnum in the receive FIFO handler is a volatile local, as in ST's source; the
   image keeps it on the stack.
   name: original (map/filenames.tsv). Five old parts joined; the joined file compiles to the
   same bytes. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/usb_otg_types.h"
#include "../../include/usb.h"

/* Incomplete periodic transfer: halt channel 0 and clear the interrupt. */
uint USB_OTG_USBH_handle_IncompletePeriodicXfer_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_GINTSTS_TypeDef gintsts;
  USB_OTG_HCCHAR_TypeDef hcchar;

  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[0]->HCCHAR);
  hcchar.b.chen = 1;
  hcchar.b.chdis = 1;
  USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[0]->HCCHAR, hcchar.d32);

  gintsts.d32 = 0;
  gintsts.b.incomplisoout = 1;
  USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GINTSTS, gintsts.d32);

  return 1;
}

/* Device disconnected: tell usbh_core.c (DevDisconnected) and clear the interrupt. */
uint USB_OTG_USBH_handle_Disconnect_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_GINTSTS_TypeDef gintsts;

  gintsts.d32 = 0;

  (*(uchar (**)(USB_OTG_CORE_HANDLE *))(*(uchar **)USBH_HCD_INT_fops + 8))(pdev);

  gintsts.b.disconnect = 1;
  USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GINTSTS, gintsts.d32);

  return 1;
}

/* Root port interrupt: connect detect, port enable or disable (setting the FS/LS PHY
   clock and HFIR for the device speed), over-current. */
uint USB_OTG_USBH_handle_port_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HPRT0_TypeDef hprt0;
  USB_OTG_HPRT0_TypeDef hprt0_dup;
  USB_OTG_HCFG_TypeDef hcfg;
  USB_OTG_GINTMSK_TypeDef intmsk;
  uint retval = 0;

  intmsk.d32 = 0;
  hcfg.d32 = 0;
  hprt0.d32 = 0;
  hprt0_dup.d32 = 0;

  hprt0.d32 = USB_OTG_READ_REG32(pdev->regs.HPRT0);
  hprt0_dup.d32 = USB_OTG_READ_REG32(pdev->regs.HPRT0);

  hprt0_dup.b.prtena = 0;
  hprt0_dup.b.prtconndet = 0;
  hprt0_dup.b.prtenchng = 0;
  hprt0_dup.b.prtovrcurrchng = 0;

  if (hprt0.b.prtconndet)
  {
    hprt0_dup.b.prtconndet = 1;
    (*(uchar (**)(USB_OTG_CORE_HANDLE *))(*(uchar **)USBH_HCD_INT_fops + 4))(pdev);
    retval |= 1;
  }

  if (hprt0.b.prtenchng)
  {
    hprt0_dup.b.prtenchng = 1;

    if (hprt0.b.prtena == 1)
    {
      if ((hprt0.b.prtspd == 2) ||
          (hprt0.b.prtspd == 1))
      {
        hcfg.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HCFG);

        if (hprt0.b.prtspd == 2)
        {
          USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HFIR, 6000);
          if (hcfg.b.fslspclksel != 2)
          {
            if (pdev->cfg.phy_itface == 2)
            {
              USB_OTG_InitFSLSPClkSel(pdev, 2);
            }
            else
            {
              USB_OTG_WRITE_REG32(&pdev->regs.HREGS->HFIR, 48000);
              if (hcfg.b.fslspclksel != 1)
              {
                USB_OTG_InitFSLSPClkSel(pdev, 1);
              }
            }
          }
        }
      }

      (*(uchar (**)(USB_OTG_CORE_HANDLE *))(*(uchar **)USBH_HCD_INT_fops + 0xc))(pdev);

      intmsk.d32 = 0;
      intmsk.b.disconnect = 1;
      USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, intmsk.d32, intmsk.d32);
    }
    else
    {
      (*(uchar (**)(USB_OTG_CORE_HANDLE *))(*(uchar **)USBH_HCD_INT_fops + 0x10))(pdev);
    }
  }

  if (hprt0.b.prtovrcurrchng)
  {
    hprt0_dup.b.prtovrcurrchng = 1;
    retval |= 1;
  }

  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0_dup.d32);

  return retval;
}

/* OUT channel interrupt: ACK, NAK, STALL, NYET, transfer complete, errors and halt;
   sets the channel and request states. */
uint USB_OTG_USBH_handle_hc_n_Out_ISR(USB_OTG_CORE_HANDLE *pdev, uint num)
{
  USB_OTG_HCINTn_TypeDef hcint;
  USB_OTG_HCINTMSK_TypeDef hcintmsk;
  USB_OTG_HC_REGS *hcreg;
  USB_OTG_HCCHAR_TypeDef hcchar;

  hcreg = pdev->regs.HC_REGS[num];
  hcint.d32 = USB_OTG_READ_REG32(&hcreg->HCINT);
  hcintmsk.d32 = USB_OTG_READ_REG32(&hcreg->HCINTMSK);
  hcint.d32 = hcint.d32 & hcintmsk.d32;

  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[num]->HCCHAR);

  if (hcint.b.ahberr)
  {
    CLEAR_HC_INT(hcreg, ahberr);
    UNMASK_HOST_INT_CHH(num);
  }
  else if (hcint.b.ack)
  {
    CLEAR_HC_INT(hcreg, ack);
  }
  else if (hcint.b.frmovrun)
  {
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, frmovrun);
  }
  else if (hcint.b.xfercompl)
  {
    pdev->host.ErrCnt[num] = 0;
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, xfercompl);
    pdev->host.HC_Status[num] = 1;
  }
  else if (hcint.b.stall)
  {
    CLEAR_HC_INT(hcreg, stall);
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    pdev->host.HC_Status[num] = 5;
  }
  else if (hcint.b.nak)
  {
    pdev->host.ErrCnt[num] = 0;
    UNMASK_HOST_INT_CHH(num);
    if (pdev->cfg.dma_enable == 0)
    {
      USB_OTG_HC_Halt(pdev, num);
    }
    CLEAR_HC_INT(hcreg, nak);
    pdev->host.HC_Status[num] = 3;
  }
  else if (hcint.b.xacterr)
  {
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    pdev->host.HC_Status[num] = 6;
    CLEAR_HC_INT(hcreg, xacterr);
  }
  else if (hcint.b.nyet)
  {
    pdev->host.ErrCnt[num] = 0;
    UNMASK_HOST_INT_CHH(num);
    if (pdev->cfg.dma_enable == 0)
    {
      USB_OTG_HC_Halt(pdev, num);
    }
    CLEAR_HC_INT(hcreg, nyet);
    pdev->host.HC_Status[num] = 4;
  }
  else if (hcint.b.datatglerr)
  {
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, nak);
    pdev->host.HC_Status[num] = 8;

    CLEAR_HC_INT(hcreg, datatglerr);
  }
  else if (hcint.b.chhltd)
  {
    MASK_HOST_INT_CHH(num);

    if (pdev->host.HC_Status[num] == 1)
    {
      pdev->host.URB_State[num] = 1;

      if (hcchar.b.eptype == 2)
      {
        pdev->host.hc[num].toggle_out ^= 1;
      }
    }
    else if (pdev->host.HC_Status[num] == 3)
    {
      pdev->host.URB_State[num] = 2;
    }
    else if (pdev->host.HC_Status[num] == 4)
    {
      if (pdev->host.hc[num].do_ping == 1)
      {
        USB_OTG_HC_DoPing(pdev, num);
      }
      pdev->host.URB_State[num] = 2;
    }
    else if (pdev->host.HC_Status[num] == 5)
    {
      pdev->host.URB_State[num] = 4;
    }
    else if (pdev->host.HC_Status[num] == 6)
    {
      pdev->host.URB_State[num] = 3;
    }
    CLEAR_HC_INT(hcreg, chhltd);
  }

  return 1;
}

/* IN channel interrupt: as for OUT, plus the data toggles and re-arming of interrupt
   endpoints. */
uint USB_OTG_USBH_handle_hc_n_In_ISR(USB_OTG_CORE_HANDLE *pdev, uint num)
{
  USB_OTG_HCINTn_TypeDef hcint;
  USB_OTG_HCINTMSK_TypeDef hcintmsk;
  USB_OTG_HCCHAR_TypeDef hcchar;
  USB_OTG_HCTSIZn_TypeDef hctsiz;
  USB_OTG_HC_REGS *hcreg;

  hcreg = pdev->regs.HC_REGS[num];
  hcint.d32 = USB_OTG_READ_REG32(&hcreg->HCINT);
  hcintmsk.d32 = USB_OTG_READ_REG32(&hcreg->HCINTMSK);
  hcint.d32 = hcint.d32 & hcintmsk.d32;
  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[num]->HCCHAR);
  hcintmsk.d32 = 0;

  if (hcint.b.ahberr)
  {
    CLEAR_HC_INT(hcreg, ahberr);
    UNMASK_HOST_INT_CHH(num);
  }
  else if (hcint.b.ack)
  {
    CLEAR_HC_INT(hcreg, ack);
  }
  else if (hcint.b.stall)
  {
    UNMASK_HOST_INT_CHH(num);
    pdev->host.HC_Status[num] = 5;
    CLEAR_HC_INT(hcreg, nak);
    CLEAR_HC_INT(hcreg, stall);
    hcint.b.nak = 0;
    USB_OTG_HC_Halt(pdev, num);
  }
  else if (hcint.b.datatglerr)
  {
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, nak);
    pdev->host.HC_Status[num] = 8;
    CLEAR_HC_INT(hcreg, datatglerr);
  }

  if (hcint.b.frmovrun)
  {
    UNMASK_HOST_INT_CHH(num);
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, frmovrun);
  }
  else if (hcint.b.xfercompl)
  {
    if (pdev->cfg.dma_enable == 1)
    {
      hctsiz.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[num]->HCTSIZ);
      pdev->host.XferCnt[num] = pdev->host.hc[num].xfer_len - hctsiz.b.xfersize;
    }

    pdev->host.HC_Status[num] = 1;
    pdev->host.ErrCnt[num] = 0;
    CLEAR_HC_INT(hcreg, xfercompl);

    if ((hcchar.b.eptype == 0) ||
        (hcchar.b.eptype == 2))
    {
      UNMASK_HOST_INT_CHH(num);
      USB_OTG_HC_Halt(pdev, num);
      CLEAR_HC_INT(hcreg, nak);
      pdev->host.hc[num].toggle_in ^= 1;
    }
    else if (hcchar.b.eptype == 3)
    {
      hcchar.b.oddfrm = 1;
      USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[num]->HCCHAR, hcchar.d32);
      pdev->host.URB_State[num] = 1;
    }
  }
  else if (hcint.b.chhltd)
  {
    MASK_HOST_INT_CHH(num);

    if (pdev->host.HC_Status[num] == 1)
    {
      pdev->host.URB_State[num] = 1;
    }
    else if (pdev->host.HC_Status[num] == 5)
    {
      pdev->host.URB_State[num] = 4;
    }
    else if ((pdev->host.HC_Status[num] == 6) ||
             (pdev->host.HC_Status[num] == 8))
    {
      pdev->host.ErrCnt[num] = 0;
      pdev->host.URB_State[num] = 3;
    }
    else if (hcchar.b.eptype == 3)
    {
      pdev->host.hc[num].toggle_in ^= 1;
    }

    CLEAR_HC_INT(hcreg, chhltd);
  }
  else if (hcint.b.xacterr)
  {
    UNMASK_HOST_INT_CHH(num);
    pdev->host.HC_Status[num] = 6;
    USB_OTG_HC_Halt(pdev, num);
    CLEAR_HC_INT(hcreg, xacterr);
  }
  else if (hcint.b.nak)
  {
    if (hcchar.b.eptype == 3)
    {
      UNMASK_HOST_INT_CHH(num);
      if (pdev->cfg.dma_enable == 0)
      {
        USB_OTG_HC_Halt(pdev, num);
      }
    }

    pdev->host.HC_Status[num] = 3;
    CLEAR_HC_INT(hcreg, nak);

    if ((hcchar.b.eptype == 0) ||
        (hcchar.b.eptype == 2))
    {
      hcchar.b.chen = 1;
      hcchar.b.chdis = 0;
      USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[num]->HCCHAR, hcchar.d32);
    }
  }

  return 1;
}

/* Host channel interrupt: for each channel flagged in HAINT, call the IN or OUT handler. */
uint USB_OTG_USBH_handle_hc_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HAINT_TypeDef haint;
  USB_OTG_HCCHAR_TypeDef hcchar;
  uint i = 0;
  uint retval = 0;

  haint.d32 = USB_OTG_ReadHostAllChannels_intr(pdev);

  for (i = 0; i < pdev->cfg.host_channels; i++)
  {
    if (haint.b.chint & (1 << i))
    {
      hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[i]->HCCHAR);

      if (hcchar.b.epdir)
      {
        retval |= USB_OTG_USBH_handle_hc_n_In_ISR(pdev, i);
      }
      else
      {
        retval |= USB_OTG_USBH_handle_hc_n_Out_ISR(pdev, i);
      }
    }
  }

  return retval;
}

/* Periodic transmit FIFO has room: write the next words of the current OUT transfer. */
uint USB_OTG_USBH_handle_ptxfempty_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_GINTMSK_TypeDef intmsk;
  USB_OTG_HPTXSTS_TypeDef hptxsts;
  ushort len_words, len;

  hptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HPTXSTS);

  len_words = (pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len + 3) / 4;

  while ((hptxsts.b.ptxfspcavail > len_words) &&
         (pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len != 0))
  {
    len = hptxsts.b.ptxfspcavail * 4;

    if (len > pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len)
    {
      len = pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len;
      intmsk.d32 = 0;
      intmsk.b.ptxfempty = 1;
      USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, intmsk.d32, 0);
    }

    len_words = (pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len + 3) / 4;

    USB_OTG_WritePacket(pdev, pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_buff, hptxsts.b.ptxqtop.chnum, len);

    pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_buff += len;
    pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_len -= len;
    pdev->host.hc[hptxsts.b.ptxqtop.chnum].xfer_count += len;

    hptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.HREGS->HPTXSTS);
  }

  return 1;
}

/* Non-periodic transmit FIFO has room: as above for bulk and control. */
uint USB_OTG_USBH_handle_nptxfempty_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_GINTMSK_TypeDef intmsk;
  USB_OTG_HNPTXSTS_TypeDef hnptxsts;
  ushort len_words, len;

  hnptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->HNPTXSTS);

  len_words = (pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len + 3) / 4;

  while ((hnptxsts.b.nptxfspcavail > len_words) &&
         (pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len != 0))
  {
    len = hnptxsts.b.nptxfspcavail * 4;

    if (len > pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len)
    {
      len = pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len;
      intmsk.d32 = 0;
      intmsk.b.nptxfempty = 1;
      USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, intmsk.d32, 0);
    }

    len_words = (pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len + 3) / 4;

    USB_OTG_WritePacket(pdev, pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_buff, hnptxsts.b.nptxqtop.chnum, len);

    pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_buff += len;
    pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_len -= len;
    pdev->host.hc[hnptxsts.b.nptxqtop.chnum].xfer_count += len;

    hnptxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->HNPTXSTS);
  }

  return 1;
}

/* Receive FIFO not empty: pop the status, read an IN packet into the channel's buffer
   and re-enable the channel if more is expected. */
uint USB_OTG_USBH_handle_rx_qlvl_ISR(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_GRXFSTS_TypeDef grxsts;
  USB_OTG_GINTMSK_TypeDef intmsk;
  USB_OTG_HCTSIZn_TypeDef hctsiz;
  USB_OTG_HCCHAR_TypeDef hcchar;
  volatile uchar channelnum = 0;
  uint count;

  intmsk.d32 = 0;
  intmsk.b.rxstsqlvl = 1;
  USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, intmsk.d32, 0);

  grxsts.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->GRXSTSP);
  channelnum = grxsts.b.chnum;
  hcchar.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[channelnum]->HCCHAR);

  switch (grxsts.b.pktsts)
  {
  case 2:
    if ((grxsts.b.bcnt > 0) && (pdev->host.hc[channelnum].xfer_buff != (void *)0))
    {
      USB_OTG_ReadPacket(pdev, pdev->host.hc[channelnum].xfer_buff, grxsts.b.bcnt);
      pdev->host.hc[grxsts.b.chnum].xfer_buff += grxsts.b.bcnt;
      pdev->host.hc[grxsts.b.chnum].xfer_count += grxsts.b.bcnt;

      count = pdev->host.hc[channelnum].xfer_count;
      pdev->host.XferCnt[channelnum] = count;

      hctsiz.d32 = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[channelnum]->HCTSIZ);
      if (hctsiz.b.pktcnt > 0)
      {
        hcchar.b.chen = 1;
        hcchar.b.chdis = 0;
        USB_OTG_WRITE_REG32(&pdev->regs.HC_REGS[channelnum]->HCCHAR, hcchar.d32);
      }
    }
    break;

  case 3:
  case 5:
  case 7:
  default:
    break;
  }

  intmsk.b.rxstsqlvl = 1;
  USB_OTG_MODIFY_REG32(&pdev->regs.GREGS->GINTMSK, 0, intmsk.d32);
  return 1;
}

/* Start of frame: call usbh_core.c's SOF callback and clear the interrupt. */
int USB_OTG_USBH_handle_sof_ISR(int param_1)
{
  volatile int local_0;
  
  local_0 = 0;
  ((code *)*(int *)((*(int *)(USBH_HCD_INT_fops + 0))))(param_1);
  local_0 = (local_0 & -9) + 8;
  *(int *)(*(int *)(param_1 + 0xc) + 0x14) = local_0;
  return 1;
}


/* Host mode interrupt entry: dispatch each pending core interrupt to its handler. */
int USBH_OTG_ISR_Handler(int param_1)
{
  int v4;
  volatile int local_0;
  
  v4 = 0;
  local_0 = 0;
  if (USB_OTG_IsHostMode(param_1) != 0) {
    local_0 = USB_OTG_ReadCoreItr(param_1);
    if (local_0 == 0) {
      return 0;
    }
    if (((uint)local_0 >> 3 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_sof_ISR(param_1);
    }
    if (((uint)local_0 >> 4 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_rx_qlvl_ISR(param_1);
    }
    if (((uint)local_0 >> 5 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_nptxfempty_ISR(param_1);
    }
    if (((uint)local_0 >> 0x1a & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_ptxfempty_ISR(param_1);
    }
    if (((uint)local_0 >> 0x19 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_hc_ISR(param_1);
    }
    if (((uint)local_0 >> 0x18 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_port_ISR(param_1);
    }
    if (((uint)local_0 >> 0x1d & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_Disconnect_ISR(param_1);
    }
    if (((uint)local_0 >> 0x15 & 1) != 0) {
      v4 = v4 | USB_OTG_USBH_handle_IncompletePeriodicXfer_ISR(param_1);
    }
  }
  return v4;
}
