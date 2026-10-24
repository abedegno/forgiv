/* ghidra_types.h: Ghidra's decompiler types, for drafts and the automatic matcher
   (the matching drafts), included by every source. Widths are Cortex-M3's: int and
   long are 32 bits.
   name: descriptive. */
#ifndef GHIDRA_TYPES_H
#define GHIDRA_TYPES_H
typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined3;
typedef unsigned int undefined4;
typedef unsigned long long undefined8;
typedef unsigned char byte;
typedef unsigned char uchar;
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned int uint3;
typedef int int3;
typedef unsigned int dword;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;
typedef unsigned long long qword;
typedef unsigned char bool;
typedef int code();
#define true 1
#define false 0
/* Ghidra's helpers */
#define CONCAT11(a, b) ((ushort)(((uint)(uchar)(a) << 8) | (uchar)(b)))
#define CONCAT22(a, b) ((uint)(((uint)(ushort)(a) << 16) | (ushort)(b)))
#define CONCAT44(a, b) ((ulonglong)(((ulonglong)(uint)(a) << 32) | (uint)(b)))
#define CONCAT31(a, b) ((uint)(((uint)(a) << 8) | (uchar)(b)))
#define CONCAT21(a, b) ((uint)(((uint)(ushort)(a) << 8) | (uchar)(b)))
#define CONCAT12(a, b) ((uint)(((uint)(uchar)(a) << 16) | (ushort)(b)))
#define CONCAT13(a, b) ((uint)(((uint)(uchar)(a) << 24) | ((uint)(b) & 0xffffff)))
#define SUB41(x, n) ((uchar)((uint)(x) >> ((n) * 8)))
#define SUB42(x, n) ((ushort)((uint)(x) >> ((n) * 8)))
#define ZEXT14(x) ((uint)(uchar)(x))
#define ZEXT24(x) ((uint)(ushort)(x))
#define SEXT14(x) ((int)(signed char)(x))
#define SEXT24(x) ((int)(short)(x))
#define DataSynchronizationBarrier(x) __dsb(0xf)
#define DataMemoryBarrier(x) __dmb(0xf)
#define InstructionSynchronizationBarrier(x) __isb(0xf)
#define WaitForInterrupt() __wfi()
#define WaitForEvent() __wfe()
#define LZCOUNT(x) __clz(x)
#define disableIRQinterrupts() __disable_irq()
#define enableIRQinterrupts() __enable_irq()
#define disableFIQinterrupts() __disable_fiq()
#define enableFIQinterrupts() __enable_fiq()
#endif
