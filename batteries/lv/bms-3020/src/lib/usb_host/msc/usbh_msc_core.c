/* Source: ST USB Host/Device V2.2.0, Libraries/STM32_USB_HOST_Library/Class/MSC/src/usbh_msc_core.c.
   Mirror commit: df4528fb01fcb467e225b1ec6824a66b013139c9.
   Upstream SHA256: a9f3cd1264d74879b341ec9efb6d81f4e32025b2c38f5e29304216b2dd5c634e. */
/* opts: --cpu Cortex-M3 -O1 -DSTM32F10X_CL -DUSE_USB_OTG_FS -DUSE_EMBEDDED_PHY -I{root}/src/lib/usb_vendor/include -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc */
/* at: HCD_IsDeviceConnected 0x0802C56C */
/* at: MSCErrorCount 0x20000650 */
/* at: MSC_Machine 0x200077A8 */
/* at: USBH_Alloc_Channel 0x0802DBC2 */
/* at: USBH_ClrFeature 0x0802E178 */
/* at: USBH_CtlReq 0x0802DC2A */
/* at: USBH_Free_Channel 0x0802DBE6 */
/* at: USBH_MSC_BOTXferParam 0x20007798 */
/* at: USBH_MSC_ClassRequest 0x0802E618 */
/* at: USBH_MSC_ErrorHandle 0x0802E620 */
/* at: USBH_MSC_GETMaxLUN 0x0802E64C */
/* at: USBH_MSC_Handle 0x0802E668 */
/* at: USBH_MSC_HandleBOTXfer 0x0802E268 */
/* at: USBH_MSC_Init 0x0802E1B0 */
/* at: USBH_MSC_InterfaceDeInit 0x0802E5E2 */
/* at: USBH_MSC_InterfaceInit 0x0802E548 */
/* at: USBH_MSC_ModeSense6 0x0802EA40 */
/* at: USBH_MSC_ReadCapacity10 0x0802E9AC */
/* at: USBH_MSC_RequestSense 0x0802EACE */
/* at: USBH_MSC_TestUnitReady 0x0802E938 */
/* at: USBH_MSC_cb 0x20000654 */
/* at: USBH_Open_Channel 0x0802DB20 */
/* at: USB_OTG_HC_Halt 0x0802C424 */
/* at: maxLunExceed 0x20000651 */
/**
  ******************************************************************************
  * @file    usbh_msc_core.c
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    09-November-2015
  * @brief   This file implements the MSC class driver functions
  *          ===================================================================      
  *                                MSC Class  Description
  *          =================================================================== 
  *           This module manages the MSC class V1.0 following the "Universal 
  *           Serial Bus Mass Storage Class (MSC) Bulk-Only Transport (BOT) Version 1.0
  *           Sep. 31, 1999".
  *           This driver implements the following aspects of the specification:
  *             - Bulk-Only Transport protocol
  *             - Subclass : SCSI transparent command set (ref. SCSI Primary Commands - 3 (SPC-3))
  *      
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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

#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"
#include "usbh_core.h"


/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_MSC_CLASS
  * @{
  */
  
/** @defgroup USBH_MSC_CORE 
  * @brief    This file includes the mass storage related functions
  * @{
  */ 


/** @defgroup USBH_MSC_CORE_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_MSC_CORE_Private_Defines
  * @{
  */ 
#define USBH_MSC_ERROR_RETRY_LIMIT 10
/**
  * @}
  */ 

/** @defgroup USBH_MSC_CORE_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_MSC_CORE_Private_Variables
  * @{
  */ 
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN MSC_Machine_TypeDef         MSC_Machine __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* MSC_Setup is unused in this source and absent after section elimination. */
uint8_t MSCErrorCount = 0;


/**
  * @}
  */ 


/** @defgroup USBH_MSC_CORE_Private_FunctionPrototypes
  * @{
  */ 

USBH_Status USBH_MSC_InterfaceInit  (USB_OTG_CORE_HANDLE *pdev , 
                                            void *phost);

void USBH_MSC_InterfaceDeInit  (USB_OTG_CORE_HANDLE *pdev , 
                                       void *phost);

USBH_Status USBH_MSC_Handle(USB_OTG_CORE_HANDLE *pdev ,
                                   
                            void *phost);

USBH_Status USBH_MSC_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                         void *phost);

static USBH_Status USBH_MSC_GETMaxLUN(USB_OTG_CORE_HANDLE *pdev,
                               USBH_HOST *phost);




void USBH_MSC_ErrorHandle(uint8_t status);

/**
  * @}
  */ 


/** @defgroup USBH_MSC_CORE_Exported_Variables
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBH_MSC_CORE_Private_Functions
  * @{
  */ 


/**
  * @brief  USBH_MSC_InterfaceInit 
  *         Interface initialization for MSC class.
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status : Status of class request handled.
  */
USBH_Status USBH_MSC_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev, 
                                        void *phost)
{	 
  USBH_HOST *pphost = phost;
  
  if((pphost->device_prop.Itf_Desc[0].bInterfaceClass == MSC_CLASS) && \
     (pphost->device_prop.Itf_Desc[0].bInterfaceProtocol == MSC_PROTOCOL))
  {
    if(pphost->device_prop.Ep_Desc[0][0].bEndpointAddress & 0x80)
    {
      MSC_Machine.MSBulkInEp = (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress);
      MSC_Machine.MSBulkInEpSize  = pphost->device_prop.Ep_Desc[0][0].wMaxPacketSize;
    }
    else
    {
      MSC_Machine.MSBulkOutEp = (pphost->device_prop.Ep_Desc[0][0].bEndpointAddress);
      MSC_Machine.MSBulkOutEpSize  = pphost->device_prop.Ep_Desc[0] [0].wMaxPacketSize;      
    }
    
    if(pphost->device_prop.Ep_Desc[0][1].bEndpointAddress & 0x80)
    {
      MSC_Machine.MSBulkInEp = (pphost->device_prop.Ep_Desc[0][1].bEndpointAddress);
      MSC_Machine.MSBulkInEpSize  = pphost->device_prop.Ep_Desc[0][1].wMaxPacketSize;      
    }
    else
    {
      MSC_Machine.MSBulkOutEp = (pphost->device_prop.Ep_Desc[0][1].bEndpointAddress);
      MSC_Machine.MSBulkOutEpSize  = pphost->device_prop.Ep_Desc[0][1].wMaxPacketSize;      
    }
    
    MSC_Machine.hc_num_out = USBH_Alloc_Channel(pdev, 
                                                MSC_Machine.MSBulkOutEp);
    MSC_Machine.hc_num_in = USBH_Alloc_Channel(pdev,
                                                MSC_Machine.MSBulkInEp);  
    
    /* Open the new channels */
    USBH_Open_Channel  (pdev,
                        MSC_Machine.hc_num_out,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        MSC_Machine.MSBulkOutEpSize);  
    
    USBH_Open_Channel  (pdev,
                        MSC_Machine.hc_num_in,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        MSC_Machine.MSBulkInEpSize);    
    
  }
  
  else
  {
    if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_HUB)
    {
      LCD_ErrLog("Hub is not supported.\n");
    }
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_CDCC)
    {
      LCD_ErrLog("Communications and CDC Control device is not supported.\n");
    }
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_HID)
    {
      LCD_ErrLog("HID device is not supported.\n");
    }
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_PRINTER)
    {
      LCD_ErrLog("Printer device is not supported.\n");
    }
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_SMARTCARD)
    {
      LCD_ErrLog("Smart Card device is not supported.\n");
    }
    
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_VIDEO)
    {
      LCD_ErrLog("Video device  is not supported.\n");
    }
    
    
    else if (pphost->device_prop.Itf_Desc[0].bInterfaceClass == USB_AVD)
    {
      LCD_ErrLog("Audio/Video Devices is not supported.\n");
    }
    
    else
    {
      LCD_ErrLog ("The attached device is not supported. \n");
    }
    
    pphost->usr_cb->DeviceNotSupported();
  }
  
  return USBH_OK ;
 
}


/**
  * @brief  USBH_MSC_InterfaceDeInit 
  *         De-Initialize interface by freeing host channels allocated to interface
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval None
  */
void USBH_MSC_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
                                void *phost)
{	
  if ( MSC_Machine.hc_num_out)
  {
    USB_OTG_HC_Halt(pdev, MSC_Machine.hc_num_out);
    USBH_Free_Channel  (pdev, MSC_Machine.hc_num_out);
    MSC_Machine.hc_num_out = 0;     /* Reset the Channel as Free */
  }
   
  if ( MSC_Machine.hc_num_in)
  {
    USB_OTG_HC_Halt(pdev, MSC_Machine.hc_num_in);
    USBH_Free_Channel  (pdev, MSC_Machine.hc_num_in);
    MSC_Machine.hc_num_in = 0;     /* Reset the Channel as Free */
  } 
}

/**
  * @brief  USBH_MSC_ClassRequest 
  *         This function will only initialize the MSC state machine
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status : Status of class request handled.
  */

USBH_Status USBH_MSC_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                        void *phost)
{   
  
  USBH_Status status = USBH_OK ;
  USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOT_INIT_STATE;
  
  return status; 
}


/**
  * @brief  USBH_MSC_Handle 
  *         MSC state machine handler 
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status
  */

USBH_Status USBH_MSC_Handle(USB_OTG_CORE_HANDLE *pdev ,
                                   void   *phost)
{
  USBH_HOST *pphost = phost;
    
  USBH_Status status = USBH_BUSY;
  uint8_t mscStatus = USBH_MSC_BUSY;
  uint8_t appliStatus = 0;
  
 
  static uint8_t maxLunExceed = FALSE;
    
  if(HCD_IsDeviceConnected(pdev))
  {   
    switch(USBH_MSC_BOTXferParam.MSCState)
    {
    case USBH_MSC_BOT_INIT_STATE:
      USBH_MSC_Init(pdev);
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_GET_MAX_LUN;  
      break;
      
      
    case USBH_MSC_GET_MAX_LUN:
      /* Issue GetMaxLUN request */
      status = USBH_MSC_GETMaxLUN(pdev, phost);
      
      if(status == USBH_OK )
      {
        MSC_Machine.maxLun = *(MSC_Machine.buff) ;
        
        /* If device has more that one logical unit then it is not supported */
        if((MSC_Machine.maxLun > 0) && (maxLunExceed == FALSE))
        {
          maxLunExceed = TRUE;
          pphost->usr_cb->DeviceNotSupported();
          
          break;
        }
        
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_TEST_UNIT_READY;
      }
      
      if(status == USBH_NOT_SUPPORTED )
      {
               /* If the Command has failed, then we need to move to Next State, after
        STALL condition is cleared by Control-Transfer */
        USBH_MSC_BOTXferParam.MSCStateBkp = USBH_MSC_TEST_UNIT_READY;        
        /* a Clear Feature should be issued here */
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_CTRL_ERROR_STATE;
      }    
      break;
      
    case USBH_MSC_CTRL_ERROR_STATE:
      /* Issue Clearfeature request */
      status = USBH_ClrFeature(pdev,
                               phost,
                               0x00,
                               pphost->Control.hc_num_out);
      if(status == USBH_OK )
      {
        /* If GetMaxLun Request not support, assume Single LUN configuration */
        MSC_Machine.maxLun = 0;  
        
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateBkp;     
      }
      break;  
      
    case USBH_MSC_TEST_UNIT_READY:
         /* Issue SCSI command TestUnitReady */ 
         mscStatus = USBH_MSC_TestUnitReady(pdev);

         if(mscStatus == USBH_MSC_OK )
         {
           USBH_MSC_BOTXferParam.MSCState = USBH_MSC_READ_CAPACITY10;
MSCErrorCount = 0;
        status = USBH_OK;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      } 
      break;
      
    case USBH_MSC_READ_CAPACITY10:
      /* Issue READ_CAPACITY10 SCSI command */
      mscStatus = USBH_MSC_ReadCapacity10(pdev);
      if(mscStatus == USBH_MSC_OK )
      {
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_MODE_SENSE6;
        MSCErrorCount = 0;
        status = USBH_OK;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;

    case USBH_MSC_MODE_SENSE6:
      /* Issue ModeSense6 SCSI command for detecting if device is write-protected */
      mscStatus = USBH_MSC_ModeSense6(pdev);
      if(mscStatus == USBH_MSC_OK )
      {
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_DEFAULT_APPLI_STATE;
        MSCErrorCount = 0;
        status = USBH_OK;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }
      break;
       
    case USBH_MSC_BOT_USB_TRANSFERS:
      /* Process the BOT state machine */
      USBH_MSC_HandleBOTXfer(pdev,phost);
      break;
    
    case USBH_MSC_DEFAULT_APPLI_STATE:
      /* Process Application callback for MSC */
      appliStatus = pphost->usr_cb->UserApplication();
      if(appliStatus == 0)
      {
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_DEFAULT_APPLI_STATE;
      }
      else if (appliStatus == 1) 
      {
        /* De-init requested from application layer */
        status =  USBH_APPLY_DEINIT;
      }
      break;
      
    case USBH_MSC_UNRECOVERED_STATE:
      
      status = USBH_UNRECOVERED_ERROR;
      
      break;
    
    case USBH_MSC_REQUEST_SENSE:
      /* Issue RequestSense SCSI command for retrieving error code */
      mscStatus = USBH_MSC_RequestSense(pdev);
      if(mscStatus == USBH_MSC_OK )
      {
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_BOTXferParam.MSCStateBkp;
        status = USBH_OK;
      }
      else
      {
        USBH_MSC_ErrorHandle(mscStatus);
      }  
      break;
      
    default:
      break; 
      
    }
  }
   return status;
}


/**
  * @brief  USBH_MSC_GETMaxLUN
  *         This request is used to reset the mass storage device and its 
  *         associated interface. This class-specific request shall ready the 
  *         device for the next CBW from the host.
  * @param  pdev: Selected device
  * @retval USBH_Status : USB ctl xfer status
  */
static USBH_Status USBH_MSC_GETMaxLUN(USB_OTG_CORE_HANDLE *pdev , USBH_HOST *phost)
{
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_TYPE_CLASS | \
                              USB_REQ_RECIPIENT_INTERFACE;
  
  phost->Control.setup.b.bRequest = USB_REQ_GET_MAX_LUN;
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 1;           
  
  return USBH_CtlReq(pdev, phost, MSC_Machine.buff , 1 ); 
}

/**
  * @brief  USBH_MSC_ErrorHandle 
  *         The function is for handling errors occurring during the MSC
  *         state machine   
  * @param  status
  * @retval None
  */

void USBH_MSC_ErrorHandle(uint8_t status)
{  
    if(status == USBH_MSC_FAIL)
    { 
      MSCErrorCount++;
      if(MSCErrorCount < USBH_MSC_ERROR_RETRY_LIMIT)
      { /* Try MSC level error recovery, Issue the request Sense to get 
        Drive error reason  */
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_REQUEST_SENSE;
        USBH_MSC_BOTXferParam.CmdStateMachine = CMD_SEND_STATE;
      }
      else
      {
        /* Error trials exceeded the limit, go to unrecovered state */
        USBH_MSC_BOTXferParam.MSCState = USBH_MSC_UNRECOVERED_STATE;
      }
    } 
    else if(status == USBH_MSC_PHASE_ERROR)
    {
      /* Phase error, Go to Unrecovered state */
      USBH_MSC_BOTXferParam.MSCState = USBH_MSC_UNRECOVERED_STATE;
    }
    else if(status == USBH_MSC_BUSY)
    {
      /*No change in state*/
    }
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
