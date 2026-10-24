# ST USB library

I use the STM32 USB Host/Device library V2.2.0 for the STM32F1 USB host.
The package is STSW-STM32046. I obtained these sources from the public mirror
https://github.com/jys0305/STM32_USB-Host-Device_Lib_V2.2.0 at commit
`df4528fb01fcb467e225b1ec6824a66b013139c9`. Each copied C file records its
upstream SHA256. The original copyright notices remain in the files; the public
ST license is in LICENSE.txt. This code targets an ST microcontroller.

The configuration selects the FS core, embedded PHY, host mode, three endpoints
per interface, two interfaces, a 1024-byte host buffer and a 64-byte MSC packet.
LCD error logging is disabled. I compile these units at -O1 without split sections.
The USB host interrupt unit remains at -O0.

In usb_core.c I read the local interrupt mask through a volatile view for its
second use, and read the local port state through a volatile view after the
100 ms delay. These retain the two stack values present in the original bytes.
The other USB core source is unchanged.

The unreferenced template MSC_Setup and USBH_DataOutBuffer declarations are
omitted. I compile the MSC callback table in its own C file so its ordinary .data
section can be placed independently from the other initialized data. The four
callback functions are exported; the complete .text section is unchanged. The callback values
and all code and data bytes are checked by the linker comparison.

In usbh_core.c I export USBH_HandleEnum rather than keeping it static, which
retains its original emission order. The resume handler also reads its local
port value through a volatile view after the delay. I compile the host interrupt
callback table and its pointer in separate C files; both remain ordinary .data
sections in the original RAM order. The unused FS suspend flag is omitted.
