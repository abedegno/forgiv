/* at: USBH_Connected 0x08034948 */
/* at: USBH_PortEnabled 0x08034954 */
/* at: USBH_PortDisabled 0x08034960 */
/* at: USBH_Disconnected 0x0803496C */
/* at: USBH_SOF 0x08034994, USBH_DeInit 0x0803499A */
/* at: USBH_Init 0x080349D4, usbh_read_line_state 0x08034A1A, usbh_line_state_stuck 0x08034A2E, USBH_HandleControl 0x08034A78, USBH_ErrorHandle 0x08034C9E, USBH_HandleEnum 0x08034CCC */
/* at: usb_line_same_count 0x2000050E, usb_line_state 0x20000510, usb_line_state_prev 0x20000514, usbh_ctrl_timeout 0x20000518 */
/* at: USBH_Process 0x08034EC0, usb_host_port_resume 0x0803509E, usb_host_port_suspend 0x0803511A */
/* at: usb_line_stuck 0x2000050C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host core, usbh_core.c: the host state machine, enumeration, control transfers and
   the callbacks from the interrupt layer.
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
   Vendor changes (GivEnergy), all in this file:
   - usbh_read_line_state and usbh_line_state_stuck are not ST's. USBH_Process calls
     usbh_line_state_stuck in the HOST_CLASS state (7) before the class machine and stores the
     result in usb_line_stuck. It returns 1 when HPRT0's line state has not changed for 1000
     passes. USBH_USR_MSC_Application (usbh_usr.c) then clears the flag and sends the CDC line
     coding again (CDC_ChangeStateToIssueSetConfig): a watchdog for a dongle that stops talking.
   - usb_host_port_resume and usb_host_port_suspend, at the end of the file, are ST-style
     code (PCGCCTL clock gating, HPRT0 suspend and resume) that I do not know from ST's
     usbh_core.c. Nothing calls them. Names descriptive.
   The rest follows ST: control transfer timeouts 5000 frames with a data stage and 50 without,
   three tries, device address 1, configuration descriptors up to 0x400 bytes.
   match: USBH_HandleControl's timeout is a static in RAM (usbh_ctrl_timeout), as ST has it;
   the #define gives it ST's local name.
   name: original (map/filenames.tsv); seven old parts joined, and the joined file compiles to
   the same bytes. */
#include "../../../include/firmware_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usb.h"
#include "../../../include/usb_otg_types.h"
#include "../../../include/usbh_types.h"

/* Interrupt callback: a device has connected (host.ConnSts = 1). */
int USBH_Connected(int param_1)
{
  int v0;

  *(int *)(param_1 + 0x310) = 1;
  v0 = 0;
  return v0;
}

/* Interrupt callback: the root port is enabled (host.PortEnabled = 1). */
int USBH_PortEnabled(int param_1)
{
  int v0;

  *(int *)(param_1 + 0x314) = 1;
  v0 = 0;
  return v0;
}

/* Interrupt callback: the root port is disabled. */
int USBH_PortDisabled(int param_1)
{
  int v0;

  *(int *)(param_1 + 0x314) = 0;
  v0 = 0;
  return v0;
}

/* Interrupt callback: device gone. VBUS off, all core interrupts masked and cleared,
   ConnSts = 0. */
uint USBH_Disconnected(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_BSP_DriveVBUS(pdev, 0);
  USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GINTMSK, 0);
  USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GINTSTS, 0xFFFFFFFF);
  USB_OTG_DisableGlobalInt(pdev);
  pdev->host.ConnSts = 0;
  return 0;
}

/* Interrupt callback for start of frame: nothing to do, returns 0. match: the longlong
   return is the draft's; it gives the image's mov r1, r0. */
longlong USBH_SOF(uint param_1)
{

  return (ulonglong)param_1 << 0x20;
}

/* Reset the host state, enumeration and control state; free the two control channels. */
int USBH_DeInit(void *pdev, USBH_HOST *phost)
{
  phost->gState = 0;
  phost->gStateBkp = 0;
  phost->EnumState = 0;
  phost->RequestState = 1;
  phost->Control.state = 1;
  phost->Control.ep0size = 0x40;
  phost->device_prop.address = 0;
  phost->device_prop.speed = 1;
  USBH_Free_Channel(pdev, phost->Control.hc_num_in);
  USBH_Free_Channel(pdev, phost->Control.hc_num_out);
  return 0;
}

static uint usbh_read_line_state(USB_OTG_CORE_HANDLE *pdev);

/* Start the host: board set-up, VBUS off, state reset, keep the class and user
   callbacks, initialise the controller (HCD_Init), user Init, enable the OTG interrupt.
   task_usb calls it with CDC_cb or USBH_MSC_cb. */
void USBH_Init(byte * param_1, int param_2, uchar * param_3, uint param_4, uint param_5)
{

  USB_OTG_BSP_Init(param_1);
  USB_OTG_BSP_ConfigVBUS(0);
  USBH_DeInit((int)param_1,param_3);
  *(uint *)(param_3 + 0x8c) = param_4;
  *(uint *)(param_3 + 0x90) = param_5;
  HCD_Init(param_1,param_2);
  (*(code *)**(uint **)(param_3 + 0x90))();
  USB_OTG_BSP_EnableInterrupt(param_1);
  return;
}

/* GivEnergy: the D+/D- line state from HPRT0.PRTLNSTS. */
static uint usbh_read_line_state(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HPRT0_TypeDef hprt0;

  hprt0.d32 = USB_OTG_READ_REG32(pdev->regs.HPRT0);
  return hprt0.b.prtlnsts;
}


/* GivEnergy: 1 when the line state has been the same for 1000 calls in a row, else 0.
   Called once per USBH_Process pass in the class state. */
int usbh_line_state_stuck(int param_1)
{
  int v5;

  v5 = 0;
  usb_line_state = usbh_read_line_state(param_1);
  if (usb_line_state != usb_line_state_prev) {
    usb_line_same_count = 0;
  }
  else {
    usb_line_same_count = usb_line_same_count + 1;
    if (usb_line_same_count >= 0x3e8) {
      usb_line_same_count = 0;
      v5 = 1;
    }
  }
  usb_line_state_prev = usb_line_state;
  return v5;
}

#define timeout (*(ushort *)usbh_ctrl_timeout)

/* Control transfer state machine: setup, optional data in or out, status, with
   timeouts and up to three tries. Returns 0 while busy, 2 on failure, 3 on a stall. */
uchar USBH_HandleControl(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  uchar direction;
  uchar status = 0;
  uchar URB_Status = 0;

  phost->Control.status = 0;

  switch (phost->Control.state)
  {
  case 1:
    USBH_CtlSendSetup(pdev,
                 phost->Control.setup.d8,
                 phost->Control.hc_num_out);
    phost->Control.state = 2;
    break;

  case 2:
    URB_Status = HCD_GetURB_State(pdev, phost->Control.hc_num_out);
    if (URB_Status == 1)
    {
      direction = (phost->Control.setup.b.bmRequestType & 0x80);

      if (phost->Control.setup.b.wLength.w != 0)
      {
        timeout = 5000;
        if (direction == 0x80)
        {
          phost->Control.state = 3;
        }
        else
        {
          phost->Control.state = 5;
        }
      }
      else
      {
        timeout = 50;

        if (direction == 0x80)
        {
          phost->Control.state = 9;
        }
        else
        {
          phost->Control.state = 7;
        }
      }
      phost->Control.timer = HCD_GetCurrentFrame(pdev);
    }
    else if (URB_Status == 3)
    {
      phost->Control.state = 11;
      phost->Control.status = 5;
    }
    break;

  case 3:
    USBH_CtlReceiveData(pdev,
                 phost->Control.buff,
                 phost->Control.length,
                 phost->Control.hc_num_in);

    phost->Control.state = 4;
    break;

  case 4:
    URB_Status = HCD_GetURB_State(pdev, phost->Control.hc_num_in);

    if (URB_Status == 1)
    {
      phost->Control.state = 9;
    }

    if (URB_Status == 4)
    {
      phost->gState = phost->gStateBkp;
      phost->Control.state = 12;
    }
    else if (URB_Status == 3)
    {
      phost->Control.state = 11;
    }
    else if ((HCD_GetCurrentFrame(pdev) - phost->Control.timer) > timeout)
    {
      phost->Control.state = 11;
    }
    break;

  case 5:
    pdev->host.hc[phost->Control.hc_num_out].toggle_out = 1;

    USBH_CtlSendData(pdev,
                 phost->Control.buff,
                 phost->Control.length,
                 phost->Control.hc_num_out);

    phost->Control.state = 6;
    break;

  case 6:
    URB_Status = HCD_GetURB_State(pdev, phost->Control.hc_num_out);
    if (URB_Status == 1)
    {
      phost->Control.state = 7;
    }
    else if (URB_Status == 4)
    {
      phost->gState = phost->gStateBkp;
      phost->Control.state = 12;
    }
    else if (URB_Status == 2)
    {
      phost->Control.state = 5;
    }
    else if (URB_Status == 3)
    {
      phost->Control.state = 11;
    }
    break;

  case 7:
    USBH_CtlReceiveData(pdev,
                 0,
                 0,
                 phost->Control.hc_num_in);

    phost->Control.state = 8;

    break;

  case 8:
    URB_Status = HCD_GetURB_State(pdev, phost->Control.hc_num_in);

    if (URB_Status == 1)
    {
      phost->gState = phost->gStateBkp;
      phost->Control.state = 13;
    }
    else if (URB_Status == 3)
    {
      phost->Control.state = 11;
    }
    else if ((HCD_GetCurrentFrame(pdev)
      - phost->Control.timer) > timeout)
    {
      phost->Control.state = 11;
    }
    else if (URB_Status == 4)
    {
      phost->gState = phost->gStateBkp;
      phost->Control.state = 12;
      status = 3;
    }
    break;

  case 9:
    pdev->host.hc[phost->Control.hc_num_out].toggle_out ^= 1;
    USBH_CtlSendData(pdev,
                 0,
                 0,
                 phost->Control.hc_num_out);

    phost->Control.state = 10;
    break;

  case 10:
    URB_Status = HCD_GetURB_State(pdev, phost->Control.hc_num_out);
    if (URB_Status == 1)
    {
      phost->gState = phost->gStateBkp;
      phost->Control.state = 13;
    }
    else if (URB_Status == 2)
    {
      phost->Control.state = 9;
    }
    else if (URB_Status == 3)
    {
      phost->Control.state = 11;
    }
    break;

  case 11:
    if (++ phost->Control.errorcount <= 2)
    {
      phost->Control.state = 1;
    }
    else
    {
      phost->Control.status = 8;
      phost->gState = phost->gStateBkp;

      status = 2;
    }
    break;

  default:
    break;
  }
  return status;
}
#undef timeout

/* Class errors: unrecovered or speed errors go to the user callback and the error
   state (12); an application de-init request restarts the user layer. */
void USBH_ErrorHandle(uchar * param_1, int param_2)
{

  if ((param_2 == 5) || (param_2 == 4)) {
    (**(code **)(*(int *)(param_1 + 0x90) + 0x44))();
    *param_1 = 0xc;
  }
  else if (param_2 == 6) {
    *param_1 = 0xc;
    (*(code *)**(uint **)(param_1 + 0x90))();
  }
  return;
}

/* Enumeration: device descriptor (8 then 18 bytes), address 1, configuration descriptor
   (9 bytes, then whole), the three strings, set configuration. Returns 0 when done. */
uchar USBH_HandleEnum(void *pdev, USBH_HOST *phost)
{
  uchar Status = 1;
  uchar Local_Buffer[64];

  switch (phost->EnumState)
  {
  case 0:
    if (USBH_Get_DevDesc(pdev, phost, 8) == 0)
    {
      phost->Control.ep0size = phost->device_prop.Dev_Desc.bMaxPacketSize;

      phost->EnumState = 1;

      USBH_Modify_Channel(pdev,
                   phost->Control.hc_num_out,
                   0,
                   0,
                   0,
                   phost->Control.ep0size);

      USBH_Modify_Channel(pdev,
                   phost->Control.hc_num_in,
                   0,
                   0,
                   0,
                   phost->Control.ep0size);
    }
    break;

  case 1:
    if (USBH_Get_DevDesc(pdev, phost, 18) == 0)
    {
      phost->usr_cb->DeviceDescAvailable(&phost->device_prop.Dev_Desc);
      phost->EnumState = 2;
    }
    break;

  case 2:
    if (USBH_SetAddress(pdev, phost, 1) == 0)
    {
      USB_OTG_BSP_mDelay(2);
      phost->device_prop.address = 1;

      phost->usr_cb->DeviceAddressAssigned();
      phost->EnumState = 3;

      USBH_Modify_Channel(pdev,
                   phost->Control.hc_num_in,
                   phost->device_prop.address,
                   0,
                   0,
                   0);

      USBH_Modify_Channel(pdev,
                   phost->Control.hc_num_out,
                   phost->device_prop.address,
                   0,
                   0,
                   0);
    }
    break;

  case 3:
    if (USBH_Get_CfgDesc(pdev, phost, 9) == 0)
    {
      if (phost->device_prop.Cfg_Desc.wTotalLength <= 0x400)
      {
        phost->EnumState = 4;
      }
    }
    break;

  case 4:
    if (USBH_Get_CfgDesc(pdev, phost, phost->device_prop.Cfg_Desc.wTotalLength) == 0)
    {
      phost->usr_cb->ConfigurationDescAvailable(&phost->device_prop.Cfg_Desc,
                                                phost->device_prop.Itf_Desc,
                                                phost->device_prop.Ep_Desc[0]);

      phost->EnumState = 5;
    }
    break;

  case 5:
    if (phost->device_prop.Dev_Desc.iManufacturer != 0)
    {
      if (USBH_Get_StringDesc(pdev, phost, phost->device_prop.Dev_Desc.iManufacturer, Local_Buffer, 0xff) == 0)
      {
        phost->usr_cb->ManufacturerString(Local_Buffer);
        phost->EnumState = 6;
      }
    }
    else
    {
      phost->usr_cb->ManufacturerString("N/A");
      phost->EnumState = 6;
    }
    break;

  case 6:
    if (phost->device_prop.Dev_Desc.iProduct != 0)
    {
      if (USBH_Get_StringDesc(pdev, phost, phost->device_prop.Dev_Desc.iProduct, Local_Buffer, 0xff) == 0)
      {
        phost->usr_cb->ProductString(Local_Buffer);
        phost->EnumState = 7;
      }
    }
    else
    {
      phost->usr_cb->ProductString("N/A");
      phost->EnumState = 7;
    }
    break;

  case 7:
    if (phost->device_prop.Dev_Desc.iSerialNumber != 0)
    {
      if (USBH_Get_StringDesc(pdev, phost, phost->device_prop.Dev_Desc.iSerialNumber, Local_Buffer, 0xff) == 0)
      {
        phost->usr_cb->SerialNumString(Local_Buffer);
        phost->EnumState = 8;
      }
    }
    else
    {
      phost->usr_cb->SerialNumString("N/A");
      phost->EnumState = 8;
    }
    break;

  case 8:
    if (USBH_SetCfg(pdev, phost, phost->device_prop.Cfg_Desc.bConfigurationValue) == 0)
    {
      phost->EnumState = 9;
    }
    break;

  case 9:
    Status = 0;
    break;

  default:
    break;
  }
  return Status;
}

/* Host state machine, called by task_usb on every pass: connect, reset, speed,
   enumeration, user input, class requests, class machine (with GivEnergy's line state check),
   control transfers, disconnect. */
void USBH_Process(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  volatile uchar status = 2;

  if (((HCD_IsDeviceConnected(pdev) == 0) || (HCD_IsPortEnabled(pdev) == 0)) && (phost->gState != 0))
  {
    if (phost->gState != 3)
    {
      phost->gState = 3;
    }
  }

  switch (phost->gState)
  {
  case 0:
    if (HCD_IsDeviceConnected(pdev))
    {
      phost->gState = 1;
      USB_OTG_BSP_mDelay(100);
      HCD_ResetPort(pdev);
      phost->usr_cb->ResetDevice();
    }
    break;

  case 1:
    if (pdev->host.PortEnabled == 1)
    {
      phost->gState = 2;
      USB_OTG_BSP_mDelay(50);
    }
    break;

  case 2:
    phost->usr_cb->DeviceAttached();
    phost->Control.hc_num_out = USBH_Alloc_Channel(pdev, 0x00);
    phost->Control.hc_num_in = USBH_Alloc_Channel(pdev, 0x80);

    if (HCD_ResetPort(pdev) == 0)
    {
      phost->usr_cb->ResetDevice();

      phost->device_prop.speed = HCD_GetCurrentSpeed(pdev);

      phost->gState = 5;
      phost->usr_cb->DeviceSpeedDetected(phost->device_prop.speed);

      USBH_Open_Channel(pdev,
                   phost->Control.hc_num_in,
                   phost->device_prop.address,
                   phost->device_prop.speed,
                   0,
                   phost->Control.ep0size);

      USBH_Open_Channel(pdev,
                   phost->Control.hc_num_out,
                   phost->device_prop.address,
                   phost->device_prop.speed,
                   0,
                   phost->Control.ep0size);
    }
    break;

  case 5:
    if (USBH_HandleEnum(pdev, phost) == 0)
    {
      phost->usr_cb->EnumerationDone();
      phost->gState = 9;
    }
    break;

  case 9:
    if (phost->usr_cb->UserInput() == 1)
    {
      if ((phost->class_cb->Init(pdev, phost)) == 0)
      {
        phost->gState = 6;
      }
    }
    break;

  case 6:
    status = phost->class_cb->Requests(pdev, phost);

    if (status == 0)
    {
      phost->gState = 7;
    }
    else
    {
      USBH_ErrorHandle(phost, status);
    }
    break;

  case 7:
    ((uchar *)&usb_line_stuck)[0] = usbh_line_state_stuck(pdev);
    status = phost->class_cb->Machine(pdev, phost);
    USBH_ErrorHandle(phost, status);
    break;

  case 8:
    USBH_HandleControl(pdev, phost);
    break;

  case 12:
    USBH_DeInit(pdev, phost);
    phost->usr_cb->DeInit();
    phost->class_cb->DeInit(pdev, &phost->device_prop);
    break;

  case 3:
    phost->usr_cb->DeviceDisconnected();
    USBH_DeInit(pdev, phost);
    phost->usr_cb->DeInit();
    phost->class_cb->DeInit(pdev, &phost->device_prop);
    USBH_DeAllocate_AllChannel(pdev);
    phost->gState = 0;
    HCD_Init(pdev, 1);
    break;

  default:
    break;
  }
}

/* Resume the root port: ungate the PHY clocks, drive resume for 20 ms, release.
   No callers. */
void usb_host_port_resume(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HPRT0_TypeDef hprt0;
  USB_OTG_PCGCCTL_TypeDef pcgcctl;

  hprt0.d32 = 0;
  pcgcctl.d32 = 0;
  pcgcctl.b.stoppclk = 1;
  USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, pcgcctl.d32, 0);
  pcgcctl.b.gatehclk = 1;
  USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, pcgcctl.d32, 0);

  hprt0.d32 = 0;
  hprt0.d32 = USB_OTG_ReadHPRT0(pdev);
  hprt0.b.prtsusp = 0;
  hprt0.b.prtres = 1;
  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
  USB_OTG_BSP_mDelay(20);
  hprt0.b.prtres = 0;
  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);
}

/* Suspend the root port, then stop and gate the PHY clocks. No callers. */
void usb_host_port_suspend(USB_OTG_CORE_HANDLE *pdev)
{
  USB_OTG_HPRT0_TypeDef hprt0;
  USB_OTG_PCGCCTL_TypeDef pcgcctl;

  hprt0.d32 = 0;
  hprt0.d32 = USB_OTG_ReadHPRT0(pdev);
  hprt0.b.prtsusp = 1;
  USB_OTG_WRITE_REG32(pdev->regs.HPRT0, hprt0.d32);

  pcgcctl.d32 = 0;
  pcgcctl.b.stoppclk = 1;
  USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, 0, pcgcctl.d32);
  pcgcctl.b.gatehclk = 1;
  USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, 0, pcgcctl.d32);
}
