/* opts: --cpu Cortex-M3 -O1 -DSTM32F10X_CL -DUSE_USB_OTG_FS -DUSE_EMBEDDED_PHY -I{root}/src/lib/usb_vendor/include -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc */
/* The interrupt callback pointer is an independent initialized data section. */
/* at: USBH_HCD_INT_fops 0x20000648 */
/* at: USBH_HCD_INT_cb 0x20000634 */
#include "usb_hcd_int.h"
extern USBH_HCD_INT_cb_TypeDef USBH_HCD_INT_cb;
USBH_HCD_INT_cb_TypeDef  *USBH_HCD_INT_fops = &USBH_HCD_INT_cb;
