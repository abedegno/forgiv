/* firmware_types.h: integer widths and compiler intrinsics shared by the ARM sources.
   uchar/ushort/uint are 8/16/32 bits; long long is 64 bits on armcc's Cortex-M3 ABI.
   Keep these widths when changing a declaration: signed loads and conversions affect bytes.
   name: descriptive; replaces the inherited decompiler helper header. */
#ifndef FIRMWARE_TYPES_H
#define FIRMWARE_TYPES_H
typedef unsigned char byte;
typedef unsigned char uchar;
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned short word;
typedef unsigned int uint;
typedef long long longlong;
typedef unsigned long long ulonglong;
typedef unsigned char bool;
typedef int code();
#define true 1
#define false 0
/* Interrupt intrinsics used by the startup helpers. */
#define disableIRQinterrupts() __disable_irq()
#define enableIRQinterrupts() __enable_irq()
#endif
