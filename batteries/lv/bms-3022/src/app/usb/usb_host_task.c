/* USB host task; the core and host objects share this original zero-init unit. */
/* at: g_20000050 0x20000050, g_20000053 0x20000053, g_200001C8 0x200001D0, g_20000212 0x2000021A, g_20000654 0x2000066C, g_2000067C 0x20000694, g_20002D1C 0x20002D34, g_200032CC 0x200032E4, g_2000748C 0x200074A4 */
/* at: usb_host_task 0x080169DC */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow USB host scheduling and class selection here.
   usb_host_task owns the host/core zero-initialized buffers and dispatches the host callbacks.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
extern uchar g_20000050[];
typedef uchar T_g_20000050_v;
#define g_20000050_v (*(T_g_20000050_v *)(g_20000050 + 0))
extern uchar g_20000053[];
typedef uchar T_g_20000053_v;
#define g_20000053_v (*(T_g_20000053_v *)(g_20000053 + 0))
extern uchar g_200001C8[];
typedef uchar T_g_200001C8_v;
#define g_200001C8_v (*(T_g_200001C8_v *)(g_200001C8 + 0))
extern uchar g_20000212[];
typedef uchar T_g_20000212_v;
#define g_20000212_v (*(T_g_20000212_v *)(g_20000212 + 0))
extern uchar g_20000654[];
typedef uchar T_g_20000654_v;
#define g_20000654_v (*(T_g_20000654_v *)(g_20000654 + 0))
extern uchar g_2000067C[];
typedef uchar T_g_2000067C_v;
#define g_2000067C_v (*(T_g_2000067C_v *)(g_2000067C + 0))
uchar g_20002D1C[0x5B0];
typedef uchar T_g_20002D1C_v;
#define g_20002D1C_v (*(T_g_20002D1C_v *)(g_20002D1C + 0))
uchar g_200032CC[0x94];
typedef uchar T_g_200032CC_v;
#define g_200032CC_v (*(T_g_200032CC_v *)(g_200032CC + 0))
extern uchar g_2000748C[];
typedef ushort T_DAT_200074e6;
#define DAT_200074e6 (*(T_DAT_200074e6 *)(g_2000748C + 90))
typedef ushort T_DAT_200074e8;
#define DAT_200074e8 (*(T_DAT_200074e8 *)(g_2000748C + 92))
void crc_table_initialize(uint);
uint os_flags_pend(ushort);
void USBH_Init(void *, int, void *, void *, void *);
void USBH_Process(void *, void *);

void usb_host_task(void) {

    crc_table_initialize(0x28411561);
    do {
        os_flags_pend(0);
        if (g_20000050_v == 0x11) {
            g_200032CC_v = 3;
            g_20000050_v = 0x12;
        }
        if (g_20000050_v >= 0x14) {
            g_20000050_v = 0;
            g_20000212_v = '\0';
        }
        if (g_20000212_v != g_20000053_v) {
            g_20000212_v = g_20000053_v;
            if (g_20000053_v == '\x02') {
                USBH_Init(&g_20002D1C_v, 1, &g_200032CC_v, &g_2000067C_v, &g_200001C8_v);
            } else if (g_20000053_v == '\b') {
                USBH_Init(&g_20002D1C_v, 1, &g_200032CC_v, &g_20000654_v, &g_200001C8_v);
            }
        } else {
            USBH_Process(&g_20002D1C_v, &g_200032CC_v);
        }
        DAT_200074e8 = 0;
        DAT_200074e6 = 1;
    } while (true);
}
