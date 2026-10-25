/* opts: --cpu Cortex-M3 -O1 -DSTM32F10X_CL -DUSE_USB_OTG_FS -DUSE_EMBEDDED_PHY -I{root}/src/lib/usb_vendor/include -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc */
/* This independent C table retains the original ordinary .data layout. */
/* at: USBH_HCD_INT_cb 0x20000634 */
/* at: USBH_HCD_INT_fops 0x20000648 */
/* at: USBH_SOF 0x0802D8A8 */
/* at: USBH_Connected 0x0802D864 */
/* at: USBH_Disconnected 0x0802D882 */
/* at: USBH_PortEnabled 0x0802D86E */
/* at: USBH_PortDisabled 0x0802D878 */
#include "usbh_core.h"
#include "usb_hcd_int.h"
uint8_t USBH_SOF(USB_OTG_CORE_HANDLE *);
uint8_t USBH_Connected(USB_OTG_CORE_HANDLE *);
uint8_t USBH_Disconnected(USB_OTG_CORE_HANDLE *);
uint8_t USBH_PortEnabled(USB_OTG_CORE_HANDLE *);
uint8_t USBH_PortDisabled(USB_OTG_CORE_HANDLE *);
USBH_HCD_INT_cb_TypeDef USBH_HCD_INT_cb =
{
  USBH_SOF,
  USBH_Connected,
  USBH_Disconnected,
  USBH_PortEnabled,
  USBH_PortDisabled
};


