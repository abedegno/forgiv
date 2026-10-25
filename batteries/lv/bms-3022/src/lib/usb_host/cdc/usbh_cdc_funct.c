/* Source: ST USB Host/Device V2.2.0, Libraries/STM32_USB_HOST_Library/Class/CDC/src/usbh_cdc_funct.c.
   Mirror commit: df4528fb01fcb467e225b1ec6824a66b013139c9.
   Upstream SHA256: 0b26e0cee19516acfe4f195768b1ec7c404ae268bbf665ee4453b68ec032c0db. */
/* opts: --cpu Cortex-M3 -O1 -DSTM32F10X_CL -DUSE_USB_OTG_FS -DUSE_EMBEDDED_PHY -I{root}/src/lib/usb_vendor/include -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc */
/* at: CDC_ChangeStateToIssueSetConfig 0x0802F516 */
/* at: CDC_Desc 0x20007E58 */
/* at: CDC_GETLineCoding 0x0802F4BC */
/* at: CDC_GetLineCode 0x200006A4 */
/* at: CDC_IssueGetConfig 0x0802F526 */
/* at: CDC_ReqState 0x20000683 */
/* at: CDC_SETControlLineState 0x0802F4FA */
/* at: CDC_SETLineCoding 0x0802F4DA */
/* at: CDC_SetLineCode 0x200006AC */
/* at: USBH_CtlReq 0x0802E06E */
/**
  ******************************************************************************
  * @file    usbh_cdc_funct.c
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    09-November-2015
    * @brief   This file is the CDC Layer Handlers for USB Host CDC class.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbh_cdc_funct.h"

/** @addtogroup USBH_LIB
* @{
*/

/** @addtogroup USBH_CLASS
* @{
*/

/** @addtogroup CDC_CLASS
* @{
*/

/** @defgroup CDC_CORE
  * @brief    This file includes HID Layer Handlers for USB Host HID class.
* @{
*/

/** @defgroup CDC_CORE_Private_TypesDefinitions
* @{
*/
/**
* @}
*/


/** @defgroup CDC_CORE_Private_Defines
* @{
*/
/**
* @}
*/


/** @defgroup CDC_CORE_Private_Macros
* @{
*/
/**
* @}
*/


/** @defgroup CDC_CORE_Private_Variables
* @{
*/
CDC_InterfaceDesc_Typedef         CDC_Desc;
CDC_LineCodingTypeDef             CDC_GetLineCode;
CDC_LineCodingTypeDef             CDC_SetLineCode;

extern CDC_Requests               CDC_ReqState;
/**
* @}
*/


/** @defgroup CDC_CORE_Private_FunctionPrototypes
* @{
*/

/**
* @}
*/


/** @defgroup CDC_CORE_Private_Functions
* @{
*/
/**
  * @brief  This request allows the host to find out the currently
  *         configured line coding.
  * @param  pdev: Selected device
  * @retval USBH_Status : USB ctl xfer status
  */
USBH_Status CDC_GETLineCoding(USB_OTG_CORE_HANDLE *pdev , USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_TYPE_CLASS | \
                              USB_REQ_RECIPIENT_INTERFACE;

  phost->Control.setup.b.bRequest = CDC_GET_LINE_CODING;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = CDC_Desc.CDC_UnionFuncDesc.bMasterInterface; /*At to be checked*/
  phost->Control.setup.b.wLength.w = LINE_CODING_STRUCTURE_SIZE;


  return USBH_CtlReq(pdev, phost, CDC_GetLineCode.Array, LINE_CODING_STRUCTURE_SIZE);
}


/**
  * @brief  This request allows the host to specify typical asynchronous
  *         line-character formatting properties
  *         This request applies to asynchronous byte stream data class interfaces
  *         and endpoints
  * @param  pdev: Selected device
  * @retval USBH_Status : USB ctl xfer status
  */
USBH_Status CDC_SETLineCoding(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{

  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_TYPE_CLASS | \
                              USB_REQ_RECIPIENT_INTERFACE;

  phost->Control.setup.b.bRequest = CDC_SET_LINE_CODING;
  phost->Control.setup.b.wValue.w = 0;

  phost->Control.setup.b.wIndex.w = CDC_Desc.CDC_UnionFuncDesc.bMasterInterface;

  phost->Control.setup.b.wLength.w = LINE_CODING_STRUCTURE_SIZE;

  return USBH_CtlReq(pdev, phost, CDC_SetLineCode.Array , LINE_CODING_STRUCTURE_SIZE );
}

/**
  * @brief  This request generates RS-232/V.24 style control signals.
  * @param  pdev: Selected device
  * @retval USBH_Status : USB ctl xfer status
  */
USBH_Status CDC_SETControlLineState(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_TYPE_CLASS | \
                              USB_REQ_RECIPIENT_INTERFACE;

  phost->Control.setup.b.bRequest = CDC_SET_CONTROL_LINE_STATE;
  /*Control Signal Bitmap Values for SetControlLineState*/
  phost->Control.setup.b.wValue.w = CDC_DEACTIVATE_CARRIER_SIGNAL_RTS | \
                          CDC_DEACTIVATE_SIGNAL_DTR;

  phost->Control.setup.b.wIndex.w = CDC_Desc.CDC_UnionFuncDesc.bMasterInterface;

  /*Length feild is zero*/
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0 , 0 );
}

/**
  * @brief  This function prepares the state before issuing the class specific commands
  * @param  None
  * @retval None
  */
void CDC_ChangeStateToIssueSetConfig(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  phost->gStateBkp = phost->gState  ;
  phost->gState = HOST_CLASS_REQUEST;
  CDC_ReqState = CDC_SET_LINE_CODING_RQUEST;
}

/**
  * @brief  This function prepares the state before issuing the class specific commands
  * @param  None
  * @retval None
  */
void CDC_IssueGetConfig(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
  phost->gStateBkp =  phost->gState ;
  phost->gState = HOST_CLASS_REQUEST;
  CDC_ReqState = CDC_GET_LINE_CODING_RQUEST;
}

/**
* @}
*/

/**
* @}
*/

/**
* @}
*/


/**
* @}
*/


/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
