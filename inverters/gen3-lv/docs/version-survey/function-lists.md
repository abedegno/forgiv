# Named candidate lists

No-hit rows need investigation; they are not a count of source logic changes.
Source links identify the existing 316 definitions, not accepted newer ports.

Ambiguous and unique rows are retained in the full candidate JSON inventories.

## ARM 316 -> 318 no shape hit

| Name | Baseline address | Existing 316 source |
| --- | --- | --- |
| `dsp_parse_page` | 080153CC | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `dsp_build_page` | 08015D02 | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `task_dsp_tx` | 08016FDC | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `modbus_fc16_write_regs` | 08018816 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc06_write_reg` | 0801C160 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_battery_regs` | 0801FA04 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_bcu_regs` | 0802007C | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_meter_ir` | 080206A4 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc04_read_input` | 08020A32 | unassigned |
| `modbus_fc03_read_holding` | 08021464 | unassigned |
| `cfg_load_defaults` | 08024064 | [src/app/board.c](../../a316/src/app/board.c) |
| `cfg_load_all` | 08024368 | [src/app/board.c](../../a316/src/app/board.c) |
| `power_flow_update` | 08025B80 | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `task_monitor` | 08026426 | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `bms_poll` | 08026C40 | [src/app/control/measure.c](../../a316/src/app/control/measure.c) |
| `batt_night_recovery_charge` | 08029548 | [src/app/control/schedule_p3.c](../../a316/src/app/control/schedule_p3.c) |
| `meter_poll` | 0802B028 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `meter_parse_fc04` | 0802C0D6 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `modbus_hr_update` | 0802CE2E | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `modbus_ir_update` | 0802D0EA | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `USB_OTG_USBH_handle_hc_n_In_ISR` | 080337C0 | [src/lib/usb_otg/usb_hcd_int.c](../../a316/src/lib/usb_otg/usb_hcd_int.c) |
| `USBH_HandleControl` | 080341F8 | [src/lib/usb_host/core/usbh_core.c](../../a316/src/lib/usb_host/core/usbh_core.c) |

## ARM 316 -> 319 no shape hit

| Name | Baseline address | Existing 316 source |
| --- | --- | --- |
| `dsp_parse_page` | 080153CC | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `dsp_build_page` | 08015D02 | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `task_dsp_tx` | 08016FDC | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `task_dsp_rx` | 080171DA | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `modbus_fill_meter_regs` | 08018022 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc22_read_meter` | 08018386 | unassigned |
| `modbus_fc16_write_regs` | 08018816 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc06_write_reg` | 0801C160 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_battery_regs` | 0801FA04 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_bcu_regs` | 0802007C | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fill_meter_ir` | 080206A4 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc04_read_input` | 08020A32 | unassigned |
| `modbus_fc03_read_holding` | 08021464 | unassigned |
| `uart_init_all` | 08023530 | [src/app/board.c](../../a316/src/app/board.c) |
| `adc_dma_init` | 08023878 | [src/app/board.c](../../a316/src/app/board.c) |
| `grid_load_country_limits` | 08023B92 | [src/app/board.c](../../a316/src/app/board.c) |
| `cfg_load_defaults` | 08024064 | [src/app/board.c](../../a316/src/app/board.c) |
| `cfg_load_all` | 08024368 | [src/app/board.c](../../a316/src/app/board.c) |
| `tim4_init` | 0802495E | [src/app/board.c](../../a316/src/app/board.c) |
| `fault_log_push` | 08024FD4 | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `power_flow_update` | 08025B80 | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `task_monitor` | 08026426 | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `bms_poll` | 08026C40 | [src/app/control/measure.c](../../a316/src/app/control/measure.c) |
| `energy_accumulate` | 0802752C | [src/app/control/schedule_p3.c](../../a316/src/app/control/schedule_p3.c) |
| `sched_slot_soc_targets` | 08027A9E | unassigned |
| `batt_night_recovery_charge` | 08029548 | [src/app/control/schedule_p3.c](../../a316/src/app/control/schedule_p3.c) |
| `meter_poll` | 0802B028 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `meter_parse_fc04` | 0802C0D6 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `meter_parse_fc03` | 0802C94C | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `meter_rx_frame` | 0802CAF8 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `dev11_parse_fc03` | 0802CBC0 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `modbus_hr_update` | 0802CE2E | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `modbus_ir_update` | 0802D0EA | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `task_meter` | 0802D41E | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `USB_OTG_USBH_handle_hc_n_In_ISR` | 080337C0 | [src/lib/usb_otg/usb_hcd_int.c](../../a316/src/lib/usb_otg/usb_hcd_int.c) |
| `USBH_HandleControl` | 080341F8 | [src/lib/usb_host/core/usbh_core.c](../../a316/src/lib/usb_host/core/usbh_core.c) |

## ARM 318 -> 319 no shape hit

| Name | Baseline address | Existing 316 source |
| --- | --- | --- |
| `function_08015D24` | 08015D24 | unassigned |
| `function_08015F00` | 08015F00 | unassigned |
| `function_08016018` | 08016018 | unassigned |
| `function_0801700A` | 0801700A | unassigned |
| `task_dsp_rx` | 0801720A | [src/app/comm/dsp_link.c](../../a316/src/app/comm/dsp_link.c) |
| `modbus_fill_meter_regs` | 08018052 | [src/app/comm/modbus_slave.c](../../a316/src/app/comm/modbus_slave.c) |
| `modbus_fc22_read_meter` | 080183B6 | unassigned |
| `function_08018846` | 08018846 | unassigned |
| `function_0801C1C0` | 0801C1C0 | unassigned |
| `function_0801FA9C` | 0801FA9C | unassigned |
| `function_08020132` | 08020132 | unassigned |
| `function_08020762` | 08020762 | unassigned |
| `function_08020AF2` | 08020AF2 | unassigned |
| `function_080210E4` | 080210E4 | unassigned |
| `function_0802153E` | 0802153E | unassigned |
| `function_08022BB0` | 08022BB0 | unassigned |
| `uart_init_all` | 08023634 | [src/app/board.c](../../a316/src/app/board.c) |
| `adc_dma_init` | 0802397C | [src/app/board.c](../../a316/src/app/board.c) |
| `grid_load_country_limits` | 08023C96 | [src/app/board.c](../../a316/src/app/board.c) |
| `function_08024168` | 08024168 | unassigned |
| `function_08024472` | 08024472 | unassigned |
| `tim4_init` | 08024A64 | [src/app/board.c](../../a316/src/app/board.c) |
| `fault_log_push` | 080250DC | [src/app/control/monitor.c](../../a316/src/app/control/monitor.c) |
| `function_08025C88` | 08025C88 | unassigned |
| `function_08025D7E` | 08025D7E | unassigned |
| `function_080264BC` | 080264BC | unassigned |
| `function_08026CD0` | 08026CD0 | unassigned |
| `function_080274F4` | 080274F4 | unassigned |
| `energy_accumulate` | 080275C4 | [src/app/control/schedule_p3.c](../../a316/src/app/control/schedule_p3.c) |
| `sched_slot_soc_targets` | 08027B36 | unassigned |
| `function_0802A2A8` | 0802A2A8 | unassigned |
| `function_0802B0C0` | 0802B0C0 | unassigned |
| `function_0802BC6A` | 0802BC6A | unassigned |
| `function_0802BDEC` | 0802BDEC | unassigned |
| `function_0802C154` | 0802C154 | unassigned |
| `function_0802C16E` | 0802C16E | unassigned |
| `function_0802C680` | 0802C680 | unassigned |
| `meter_parse_fc03` | 0802C9E4 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `meter_rx_frame` | 0802CB90 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `dev11_parse_fc03` | 0802CC58 | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `function_0802CEC6` | 0802CEC6 | unassigned |
| `function_0802D182` | 0802D182 | unassigned |
| `task_meter` | 0802D4DC | [src/app/comm/meter.c](../../a316/src/app/comm/meter.c) |
| `function_08030584` | 08030584 | unassigned |

## DSP 318 no shape hit

| Name | Baseline address | Existing 316 source |
| --- | --- | --- |
| `arm_link_frame` | 003EAC73 | [src/app/comm/arm_link.c](../../d316/src/app/comm/arm_link.c) |
| `arm_page4` | 003EB83A | [src/app/comm/arm_link.c](../../d316/src/app/comm/arm_link.c) |
| `arm_page7` | 003EBB98 | unassigned |
| `batt_power_setpoint` | 003ECDE3 | [src/app/power/power_limits.c](../../d316/src/app/power/power_limits.c) |
| `status_led_and_reset` | 003ED31B | unassigned |
| `app_vars_init` | 003EF687 | [src/app/state/app_init.c](../../d316/src/app/state/app_init.c) |
| `bms_parse_hr` | 003F0895 | [src/app/battery/bms_link.c](../../d316/src/app/battery/bms_link.c) |
| `task6_bms_link` | 003F09E0 | unassigned |
| `bms_lost` | 003F0BC4 | [src/app/battery/bms_link.c](../../d316/src/app/battery/bms_link.c) |
| `c_int00` | 003F197D | unassigned |

## DSP 319 no shape hit

| Name | Baseline address | Existing 316 source |
| --- | --- | --- |
| `grid_import_current_limit` | 003E8D5F | [src/app/grid/grid_support.c](../../d316/src/app/grid/grid_support.c) |
| `arm_link_frame` | 003EAC73 | [src/app/comm/arm_link.c](../../d316/src/app/comm/arm_link.c) |
| `arm_page4` | 003EB83A | [src/app/comm/arm_link.c](../../d316/src/app/comm/arm_link.c) |
| `arm_page7` | 003EBB98 | unassigned |
| `batt_power_setpoint` | 003ECDE3 | [src/app/power/power_limits.c](../../d316/src/app/power/power_limits.c) |
| `status_led_and_reset` | 003ED31B | unassigned |
| `app_vars_init` | 003EF687 | [src/app/state/app_init.c](../../d316/src/app/state/app_init.c) |
| `bms_parse_hr` | 003F0895 | [src/app/battery/bms_link.c](../../d316/src/app/battery/bms_link.c) |
| `task6_bms_link` | 003F09E0 | unassigned |
| `bms_lost` | 003F0BC4 | [src/app/battery/bms_link.c](../../d316/src/app/battery/bms_link.c) |
| `c_int00` | 003F197D | unassigned |

## DSP 318 changed RAM operands

`active_power_rate_limit`, `grid_import_current_limit`, `soft_start_ramp`, `power_limits_update`, `task5_arm_link`, `arm_page0`, `arm_page1`, `arm_page2`, `arm_page5`, `task3_measure`, `pv_iso_check`, `pv_voltage_check`, `iso_fault_clear`, `batt_power_blocks`, `soc_power_limits`, `current_limit_tapers`, `soc_floor`, `batt_voltage_monitor`, `state0_idle`, `converters_stop`, `start_grid_pv`, `start_eps_pv`, `state_init`, `relay_test_step`, `soft_start`, `relay_test_abort`, `relays_off_when_disabled`, `select_state`, `eps_output_check`, `rating_apply`, `rcmu_selftest_step`, `bms_fc4_reply`, `sci_rx`, `sci_tx`, `iso_dcdc_pwm`, `converter_mode`, `fast_fault_check`

## DSP 319 changed RAM operands

`active_power_rate_limit`, `soft_start_ramp`, `power_limits_update`, `task5_arm_link`, `arm_page0`, `arm_page1`, `arm_page2`, `arm_page5`, `task3_measure`, `pv_iso_check`, `pv_voltage_check`, `iso_fault_clear`, `batt_power_blocks`, `soc_power_limits`, `current_limit_tapers`, `soc_floor`, `batt_voltage_monitor`, `state0_idle`, `converters_stop`, `start_grid_pv`, `start_eps_pv`, `state_init`, `relay_test_step`, `soft_start`, `relay_test_abort`, `relays_off_when_disabled`, `select_state`, `eps_output_check`, `rating_apply`, `rcmu_selftest_step`, `bms_fc4_reply`, `sci_rx`, `sci_tx`, `iso_dcdc_pwm`, `converter_mode`, `fast_fault_check`

## DSP 318 -> 319 changed RAM operands

None in this subset.
