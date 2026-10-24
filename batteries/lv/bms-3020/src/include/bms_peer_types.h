/* I share the packed peer record used by protocol and measurement code.
   I follow both reply directions and preserve four unassigned slots without a protocol meaning.
   match: the byte-packed stride and signed 32-bit current must remain unchanged. */
#ifndef BMS_PEER_TYPES_H
#define BMS_PEER_TYPES_H
#include "ghidra_types.h"
#include <stddef.h>
typedef __packed struct {
    ushort reply_seen;
    uchar identification[42];
    uchar cell_count;
    ushort cycle_count;
    int auxiliary_voltage;
    ushort pack_voltage;
    int pack_current;
    uint full_capacity, rated_capacity, remaining_capacity;
    uchar soc_primary, voltage_current_protection_flags, temperature_protection_flags,
        output_status_flags, current_direction_latch_flags, hardware_fault_flags,
        protection_inhibit_flags, capacity_latch_flags, voltage_current_limit_flags,
        temperature_limit_flags;
    ushort maximum_cell_voltage, minimum_cell_voltage, cell_voltages[16], maximum_temperature,
        minimum_temperature, firmware_version, temperature_0, temperature_1, temperature_2,
        mosfet_temperature, ambient_temperature;
    uchar usb_class, reserved_132, reserved_133, reserved_134, reserved_135, soc_secondary;
    ushort charge_current_limit, discharge_current_limit;
    uchar participation_state, voltage_hold_mode, output_phase_toggle, soc_voltage_hold;
} BmsPeerRecord;
typedef char BmsPeerSizeCheck[(sizeof(BmsPeerRecord) == 145) ? 1 : -1];
typedef char BmsPeerOffset_reply_seen[(offsetof(BmsPeerRecord, reply_seen) == 0) ? 1 : -1];
typedef char BmsPeerOffset_cell_count[(offsetof(BmsPeerRecord, cell_count) == 44) ? 1 : -1];
typedef char BmsPeerOffset_pack_voltage[(offsetof(BmsPeerRecord, pack_voltage) == 51) ? 1 : -1];
typedef char BmsPeerOffset_pack_current[(offsetof(BmsPeerRecord, pack_current) == 53) ? 1 : -1];
typedef char BmsPeerOffset_full_capacity[(offsetof(BmsPeerRecord, full_capacity) == 57) ? 1 : -1];
typedef char BmsPeerOffset_rated_capacity[(offsetof(BmsPeerRecord, rated_capacity) == 61) ? 1 : -1];
typedef char
    BmsPeerOffset_remaining_capacity[(offsetof(BmsPeerRecord, remaining_capacity) == 65) ? 1 : -1];
typedef char BmsPeerOffset_soc_primary[(offsetof(BmsPeerRecord, soc_primary) == 69) ? 1 : -1];
typedef char BmsPeerOffset_soc_secondary[(offsetof(BmsPeerRecord, soc_secondary) == 136) ? 1 : -1];
typedef char
    BmsPeerOffset_charge_current_limit[(offsetof(BmsPeerRecord, charge_current_limit) == 137) ? 1
                                                                                              : -1];
typedef char BmsPeerOffset_discharge_current_limit
    [(offsetof(BmsPeerRecord, discharge_current_limit) == 139) ? 1 : -1];
typedef char BmsPeerOffset_participation_state[(offsetof(BmsPeerRecord, participation_state) == 141)
                                                   ? 1
                                                   : -1];
typedef char
    BmsPeerOffset_voltage_hold_mode[(offsetof(BmsPeerRecord, voltage_hold_mode) == 142) ? 1 : -1];
typedef char
    BmsPeerOffset_soc_voltage_hold[(offsetof(BmsPeerRecord, soc_voltage_hold) == 144) ? 1 : -1];
/* match: protocol uses an unsigned rated capacity; measurement casts its two
   arithmetic reads to int to preserve its original signed view. */
#endif
