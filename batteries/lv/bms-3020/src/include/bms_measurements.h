/* I share scalar readings whose caller views have the same width and sign.
   Names follow their producers and protection/serialization consumers.
   The temperature array keeps its separate signed-word and byte views. */
#ifndef BMS_MEASUREMENTS_H
#define BMS_MEASUREMENTS_H
#include "ghidra_types.h"
extern float measurement_current;
extern uchar measurement_cell_count;
extern ushort measurement_maximum_cell_voltage;
extern ushort measurement_minimum_cell_voltage;
extern ushort measurement_pack_voltage;
extern ushort measurement_maximum_temperature;
extern ushort measurement_minimum_temperature;
void measurement_compensate_cell_voltages(void);
void measurement_update_voltage_summary(void);
void measurement_update_temperature_summary(void);
void measurement_detect_cell_voltage_steps(void);
void measurement_update_voltage_and_temperature(void);
void measurement_update_current(void);
#endif
