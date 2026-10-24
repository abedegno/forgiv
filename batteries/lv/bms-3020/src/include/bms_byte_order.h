/* I share the byte assembly used by the protocol reply decoder.
   These macros are for plain indexed buffers, with side-effect-free arguments.
   match: I retain the original casts and sum grouping rather than changing to ORs. */
#ifndef BMS_BYTE_ORDER_H
#define BMS_BYTE_ORDER_H
#include "ghidra_types.h"
#define BMS_READ_BE16(bytes, offset)                                                               \
    ((ushort)(bytes)[(offset) + 1] + (ushort)(bytes)[offset] * 0x100)
#define BMS_READ_BE32(bytes, offset)                                                               \
    (((uint)(bytes)[(offset) + 3] + ((uint)(bytes)[offset] << 24)) +                               \
     (((uint)(bytes)[(offset) + 1] << 16) + ((uint)(bytes)[(offset) + 2] << 8)))
#endif
