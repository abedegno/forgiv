/* at: CDC_cb 0x2000067C */
/* at: CDC_InterfaceInit 0x0802ED04, CDC_InterfaceDeInit 0x0802EDFC, CDC_ClassRequest 0x0802EE48, CDC_Handle 0x0802EFDA */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* CDC class callback table, kept in a data-only source unit. */
#include "bms_cdc_port.h"
USBH_Class_cb_TypeDef CDC_cb = {
    CDC_InterfaceInit,
    CDC_InterfaceDeInit,
    CDC_ClassRequest,
    (uchar (*)(void *,void *))CDC_Handle
};
