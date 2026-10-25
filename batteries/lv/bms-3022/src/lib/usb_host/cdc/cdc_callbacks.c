/* at: CDC_cb 0x20000694 */
/* at: CDC_InterfaceInit 0x0802F148, CDC_InterfaceDeInit 0x0802F240, CDC_ClassRequest 0x0802F28C, CDC_Handle 0x0802F41E */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* CDC class callback table, kept in a data-only source unit. */
#include "bms_cdc_port.h"
USBH_Class_cb_TypeDef CDC_cb = {
    CDC_InterfaceInit,
    CDC_InterfaceDeInit,
    CDC_ClassRequest,
    (uchar (*)(void *,void *))CDC_Handle
};
