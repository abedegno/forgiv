/* opts: --cpu Cortex-M3 -O1 -DSTM32F10X_CL -DUSE_USB_OTG_FS -DUSE_EMBEDDED_PHY -I{root}/src/lib/usb_vendor/include -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc */
/* The callback table is compiled separately to keep its ordinary .data section independently placed. */
/* at: USBH_MSC_cb 0x2000066C */
/* at: USBH_MSC_InterfaceInit 0x0802E98C */
/* at: USBH_MSC_InterfaceDeInit 0x0802EA26 */
/* at: USBH_MSC_ClassRequest 0x0802EA5C */
/* at: USBH_MSC_Handle 0x0802EAAC */
#include "usbh_core.h"
USBH_Status USBH_MSC_InterfaceInit(USB_OTG_CORE_HANDLE *, void *);
void USBH_MSC_InterfaceDeInit(USB_OTG_CORE_HANDLE *, void *);
USBH_Status USBH_MSC_ClassRequest(USB_OTG_CORE_HANDLE *, void *);
USBH_Status USBH_MSC_Handle(USB_OTG_CORE_HANDLE *, void *);
USBH_Class_cb_TypeDef  USBH_MSC_cb =
{
  USBH_MSC_InterfaceInit,
  USBH_MSC_InterfaceDeInit,
  USBH_MSC_ClassRequest,
  USBH_MSC_Handle,
};
