/* at: USBH_ParseDevDesc 0x08034E2C, USBH_GetDescriptor 0x08034E84, USBH_Get_DevDesc 0x08034EC8, USBH_ParseEPDesc 0x08034F00, USBH_ParseInterfaceDesc 0x08034F20, USBH_GetNextDesc 0x08034F46, USBH_ParseCfgDesc 0x08034F58, USBH_Get_CfgDesc 0x08035084, USBH_ParseStringDesc 0x080350EC, USBH_Get_StringDesc 0x08035122, USBH_SetAddress 0x0803515E, USBH_SetCfg 0x08035184, USBH_SetInterface 0x080351AA, USBH_SetFeature 0x080351D6, USBH_ClearFeature 0x08035202, USBH_ClrFeature 0x0803522E */
/* at: prev_ep_size 0x2000050E, prev_itf 0x20000510, USBH_CfgDesc 0x20007D48 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* USB host standard requests, usbh_stdreq.c: descriptors, address, configuration, features,
   and the descriptor parsers.
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
   Changes from ST: none found. USBH_SetFeature and USBH_ClearFeature (device requests 3 and 1)
   have no callers; their names are my guess, the rest are ST's.
   match: the functions are in an order where each is defined before its first use, not the
   order of ST's V2.1.0 file, and the parse functions must be static, as in ST: with
   USBH_ParseEPDesc and USBH_ParseInterfaceDesc global, armcc gives USBH_ParseCfgDesc a smaller
   frame than the image has (docs/matching.md, round 5). This file declares USBH_CtlReq as
   returning uchar (OWN_ define): it was compiled against that.
   name: original (map/filenames.tsv). Six old files were one object: only USBH_ParseCfgDesc has
   a literal pool, which gives the whole .text the 4-byte alignment the image shows at
   0x08034E2A. */
#include "../../../include/ghidra_types.h"
#include "../../../include/stdperiph_types.h"
#include "../../../include/usbh_types.h"
#include "../../../include/usb_otg_types.h"
#define OWN_USBH_CtlReq   /* match: declared below as uchar(void *,USBH_HOST *,uchar *,ushort): this file compiled against that, not the header's int(void *,USBH_HOST *,uchar *,ushort) */
#include "../../../include/usb.h"
uchar USBH_CtlReq(void *, USBH_HOST *, uchar *, ushort);
static void USBH_ParseStringDesc(uchar *psrc, uchar *pdest, ushort length);

/* Copy the device descriptor fields out of the receive buffer (8 or 18 bytes). */
static void USBH_ParseDevDesc(undefined1 * param_1, undefined1 * param_2, int param_3)
{

  *param_1 = *param_2;
  param_1[1] = param_2[1];
  *(ushort *)(param_1 + 2) = (ushort)(byte)param_2[2] + (ushort)(byte)param_2[3] * 0x100;
  param_1[4] = param_2[4];
  param_1[5] = param_2[5];
  param_1[6] = param_2[6];
  param_1[7] = param_2[7];
  if (8 < param_3) {
    *(ushort *)(param_1 + 8) = (ushort)(byte)param_2[8] + (ushort)(byte)param_2[9] * 0x100;
    *(ushort *)(param_1 + 10) = (ushort)(byte)param_2[10] + (ushort)(byte)param_2[0xb] * 0x100;
    *(ushort *)(param_1 + 0xc) = (ushort)(byte)param_2[0xc] + (ushort)(byte)param_2[0xd] * 0x100;
    param_1[0xe] = param_2[0xe];
    param_1[0xf] = param_2[0xf];
    param_1[0x10] = param_2[0x10];
    param_1[0x11] = param_2[0x11];
  }
  return;
}

/* GET_DESCRIPTOR control request (standard or class, value and index in value_idx). */
uchar USBH_GetDescriptor(void *pdev, USBH_HOST *phost, uchar req_type, ushort value_idx, uchar *buff, ushort length)
{
  phost->Control.setup.b.bmRequestType = 0x80 | req_type;
  phost->Control.setup.b.bRequest = 6;
  phost->Control.setup.b.wValue.w = value_idx;

  if ((value_idx & 0xff00) == 0x300)
  {
    phost->Control.setup.b.wIndex.w = 0x0409;
  }
  else
  {
    phost->Control.setup.b.wIndex.w = 0;
  }
  phost->Control.setup.b.wLength.w = length;
  return USBH_CtlReq(pdev, phost, buff, length);
}

/* Read the device descriptor (length bytes) and parse it. */
uchar USBH_Get_DevDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar length)
{
  uchar status;

  if ((status = USBH_GetDescriptor(pdev,
                             phost,
                             0x00 | 0x00,
                             0x100,
                             pdev->host.Rx_Buffer,
                             length)) == 0)
  {
    USBH_ParseDevDesc((undefined1 *)&phost->device_prop.Dev_Desc, pdev->host.Rx_Buffer, length);
  }
  return status;
}

/* Copy one endpoint descriptor. */
static void USBH_ParseEPDesc(undefined1 * param_1, undefined1 * param_2)
{

  *param_1 = *param_2;
  param_1[1] = param_2[1];
  param_1[2] = param_2[2];
  param_1[3] = param_2[3];
  *(ushort *)(param_1 + 4) = (ushort)(byte)param_2[4] + (ushort)(byte)param_2[5] * 0x100;
  param_1[6] = param_2[6];
  return;
}

/* Copy one interface descriptor. */
static void USBH_ParseInterfaceDesc(undefined1 * param_1, undefined1 * param_2)
{

  *param_1 = *param_2;
  param_1[1] = param_2[1];
  param_1[2] = param_2[2];
  param_1[3] = param_2[3];
  param_1[4] = param_2[4];
  param_1[5] = param_2[5];
  param_1[6] = param_2[6];
  param_1[7] = param_2[7];
  param_1[8] = param_2[8];
  return;
}

/* Step to the next descriptor in a configuration block; *ptr counts the bytes passed. */
USBH_DescHeader_t *USBH_GetNextDesc(uchar *pbuf, ushort *ptr)
{
  USBH_DescHeader_t *pnext;

  *ptr += ((USBH_DescHeader_t *)pbuf)->bLength;
  pnext = (USBH_DescHeader_t *)((uchar *)pbuf + ((USBH_DescHeader_t *)pbuf)->bLength);
  return pnext;
}


/* Parse a configuration block: the configuration, up to two interfaces and their
   endpoints (prev_itf and prev_ep_size skip repeats). */
static uchar USBH_ParseCfgDesc(USBH_CfgDesc_TypeDef *cfg_desc,
                   USBH_InterfaceDesc_TypeDef *itf_desc,
                   USBH_EpDesc_TypeDef ep_desc[][3],
                   uchar *buf,
                   ushort length)
{
  USBH_InterfaceDesc_TypeDef *pif;
  USBH_InterfaceDesc_TypeDef temp_pif;
  USBH_EpDesc_TypeDef *pep;
  USBH_DescHeader_t *pdesc = (USBH_DescHeader_t *)buf;
  ushort ptr;
  sbyte if_ix = 0;
  sbyte ep_ix = 0;
  uchar status;

  pdesc = (USBH_DescHeader_t *)buf;

  cfg_desc->bLength = *(uchar *)(buf + 0);
  cfg_desc->bDescriptorType = *(uchar *)(buf + 1);
  cfg_desc->wTotalLength = (((ushort)(*(uchar *)(buf + 2))) + (((ushort)(*(((uchar *)(buf + 2)) + 1))) << 8));
  cfg_desc->bNumInterfaces = *(uchar *)(buf + 4);
  cfg_desc->bConfigurationValue = *(uchar *)(buf + 5);
  cfg_desc->iConfiguration = *(uchar *)(buf + 6);
  cfg_desc->bmAttributes = *(uchar *)(buf + 7);
  cfg_desc->bMaxPower = *(uchar *)(buf + 8);

  if (length > 9)
  {
    ptr = 9;

    if (cfg_desc->bNumInterfaces <= 2)
    {
      pif = (USBH_InterfaceDesc_TypeDef *)0;

      while (ptr < cfg_desc->wTotalLength)
      {
        pdesc = USBH_GetNextDesc((uchar *)pdesc, &ptr);
        if (pdesc->bDescriptorType == 4)
        {
          if_ix = *(((uchar *)pdesc) + 2);
          pif = &itf_desc[if_ix];

          if ((*((uchar *)pdesc + 3)) < 3)
          {
            USBH_ParseInterfaceDesc((undefined1 *)&temp_pif, (uchar *)pdesc);
            ep_ix = 0;

            if (temp_pif.bNumEndpoints <= 3)
            {
              while (ep_ix < temp_pif.bNumEndpoints)
              {
                pdesc = USBH_GetNextDesc((void *)pdesc, &ptr);
                if (pdesc->bDescriptorType == 5)
                {
                  pep = &ep_desc[if_ix][ep_ix];

                  if ((*(uchar *)prev_itf) != if_ix)
                  {
                    (*(uchar *)prev_itf) = if_ix;
                    USBH_ParseInterfaceDesc((undefined1 *)pif, (uchar *)&temp_pif);
                  }
                  else
                  {
                    if ((*(ushort *)prev_ep_size) > (((ushort)(*(uchar *)((uchar *)pdesc + 4))) + (((ushort)(*(((uchar *)((uchar *)pdesc + 4)) + 1))) << 8)))
                    {
                      break;
                    }
                    else
                    {
                      USBH_ParseInterfaceDesc((undefined1 *)pif, (uchar *)&temp_pif);
                    }
                  }
                  USBH_ParseEPDesc((undefined1 *)pep, (uchar *)pdesc);
                  (*(ushort *)prev_ep_size) = (((ushort)(*(uchar *)((uchar *)pdesc + 4))) + (((ushort)(*(((uchar *)((uchar *)pdesc + 4)) + 1))) << 8));
                  ep_ix++;
                }
              }
            }
            else
            {
              status = 3;
              return status;
            }
          }
        }
      }
    }
    else
    {
      status = 3;
      return status;
    }
    (*(ushort *)prev_ep_size) = 0;
    (*(uchar *)prev_itf) = 0;
  }
  return 0;
}

/* Read the configuration descriptor (length bytes), copy it to USBH_CfgDesc, parse it. */
uchar USBH_Get_CfgDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, ushort length)
{
  uchar ret;
  ushort index = 0;
  uchar status;

  if ((status = USBH_GetDescriptor(pdev,
                             phost,
                             0x00 | 0x00,
                             0x200,
                             pdev->host.Rx_Buffer,
                             length)) == 0)
  {
    for ( ; index < length; index++)
    {
      USBH_CfgDesc[index] = pdev->host.Rx_Buffer[index];
    }

    ret = USBH_ParseCfgDesc(&phost->device_prop.Cfg_Desc,
                       phost->device_prop.Itf_Desc,
                       phost->device_prop.Ep_Desc,
                       pdev->host.Rx_Buffer,
                       length);
    if (ret != 0)
    {
      status = ret;
    }
  }
  return status;
}

/* Turn a UTF-16 string descriptor into 8-bit characters. */
static void USBH_ParseStringDesc(uchar *psrc, uchar *pdest, ushort length)
{
  ushort strlength;
  ushort idx;

  if (psrc[1] == 3)
  {
    strlength = (((psrc[0] - 2) <= length) ? (psrc[0] - 2) : length);
    psrc += 2;

    for (idx = 0; idx < strlength; idx += 2)
    {
      *pdest = psrc[idx];
      pdest++;
    }
    *pdest = 0;
  }
}

/* Read string descriptor string_index (language 0x0409) and convert it. */
uchar USBH_Get_StringDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar string_index, uchar *buff, ushort length)
{
  uchar status;

  if ((status = USBH_GetDescriptor(pdev,
                             phost,
                             0x00 | 0x00,
                             0x300 | string_index,
                             pdev->host.Rx_Buffer,
                             length)) == 0)
  {
    USBH_ParseStringDesc(pdev->host.Rx_Buffer, buff, length);
  }
  return status;
}

/* SET_ADDRESS request. */
uchar USBH_SetAddress(void *pdev, USBH_HOST *phost, uchar DeviceAddress)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x00 | 0x00;
  phost->Control.setup.b.bRequest = 5;
  phost->Control.setup.b.wValue.w = (ushort)DeviceAddress;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}

/* SET_CONFIGURATION request. */
uchar USBH_SetCfg(void *pdev, USBH_HOST *phost, ushort cfg_idx)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x00 | 0x00;
  phost->Control.setup.b.bRequest = 9;
  phost->Control.setup.b.wValue.w = cfg_idx;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}

/* SET_INTERFACE request (alternate setting altSetting of interface ep_num). */
uchar USBH_SetInterface(void *pdev, USBH_HOST *phost, uchar ep_num, uchar altSetting)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x01 | 0x00;
  phost->Control.setup.b.bRequest = 11;
  phost->Control.setup.b.wValue.w = altSetting;
  phost->Control.setup.b.wIndex.w = ep_num;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}

/* SET_FEATURE device request with wValue and wIndex. No callers; name mine. */
uchar USBH_SetFeature(void *pdev, USBH_HOST *phost, ushort wValue, ushort wIndex)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x00 | 0x00;
  phost->Control.setup.b.bRequest = 3;
  phost->Control.setup.b.wValue.w = wValue;
  phost->Control.setup.b.wIndex.w = wIndex;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}

/* CLEAR_FEATURE device request with wValue and wIndex. No callers; name mine. */
uchar USBH_ClearFeature(void *pdev, USBH_HOST *phost, ushort wValue, ushort wIndex)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x00 | 0x00;
  phost->Control.setup.b.bRequest = 1;
  phost->Control.setup.b.wValue.w = wValue;
  phost->Control.setup.b.wIndex.w = wIndex;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0, 0);
}

/* CLEAR_FEATURE(ENDPOINT_HALT) on endpoint ep_num and reset the channel's toggle.
   Used by the MSC abort and the CDC error path. */
uchar USBH_ClrFeature(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar ep_num, uchar hc_num)
{
  phost->Control.setup.b.bmRequestType = 0x00 | 0x02 | 0x00;
  phost->Control.setup.b.bRequest = 1;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = ep_num;
  phost->Control.setup.b.wLength.w = 0;

  if ((ep_num & 0x80) == 0x80)
  {
    pdev->host.hc[hc_num].toggle_in = 0;
  }
  else
  {
    pdev->host.hc[hc_num].toggle_out = 0;
  }

  return USBH_CtlReq(pdev, phost, 0, 0);
}

