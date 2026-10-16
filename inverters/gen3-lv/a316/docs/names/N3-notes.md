# N3 notes (0x0803240C to 0x080377A8)

My range is nearly all library code. I found no battery logic in it: no BMS protocol, no
charge or discharge limits, no SOC handling. What it does give is the two USB paths the
inverter uses, and the application code around them shows how they are used.

## FatFs (0x0803240C)

Only `f_rename` falls in my range; the rest of FatFs R0.12c is below it. Nothing in the image
calls `f_rename`, so it was linked in but is not used.

## STM32 USB OTG core and host driver (0x0803251C to 0x0803401A)

ST's USB OTG library: `usb_core.c` (host half), `usb_hcd.c` and `usb_hcd_int.c`. The function
order and bodies match ST's sources, so the names are ST's. `HCD_IsPortEnabled` and the
`USBH_PortEnabled`/`USBH_PortDisabled` callbacks only exist from release V2.2.0, so I think
the library is V2.2.0. The interrupt entry point is `USBH_OTG_ISR_Handler`, called from
`OTG_FS_IRQHandler` (0x08014C92). The callbacks go through `USBH_HCD_INT_fops` (0x200004F8).

## USB host core (0x080340C8 to 0x0803522E)

ST's `usbh_core.c`, `usbh_hcs.c`, `usbh_ioreq.c` and `usbh_stdreq.c`. GivEnergy changed
two things:

- `USBH_Process`, in the class state, calls a new function I named `usbh_line_state_stuck`
  (0x080341AE). It reads the D+/D- line state from HPRT0 and returns 1 once the state has not
  changed for 1000 calls. The result goes into `usb_line_stuck` (0x20000500). The application
  (0x0802A060) then clears the flag and calls `CDC_ChangeStateToIssueSetConfig`, which sends
  the line coding to the dongle again. My inference is that this is a watchdog for a dongle
  that has stopped talking.
- Two unused functions, `usb_host_port_resume` and `usb_host_port_suspend` (0x0803481E,
  0x0803489A), are written in ST's style but I could not match them to a named ST function.

## USB mass storage and diskio (0x08035288 to 0x0803606A)

ST's `usbh_msc_bot.c`, `usbh_msc_core.c`, `usbh_msc_scsi.c` and the FatFs glue
`usbh_msc_fatfs.c` (`disk_initialize`, `disk_status`, `disk_read`, `disk_write`,
`disk_ioctl`). The sector size is 512. `get_fattime` returns 0, so files the inverter writes
have no date. The USB task (0x0802AEF4) starts the host with `USBH_MSC_cb` when the device
class flag 0x2000030A is 8. My inference is that this flag holds the bInterfaceClass of the
device: 8 is mass storage and 2 is CDC. With a stick mounted, 0x0802A060 opens `0:SN.txt`,
reads 10 bytes and, if they differ from the stored serial number, copies them in with
`strcpy` and sets several "changed" flags. So a USB stick can set the inverter's serial
number.

## USB CDC host, the dongle link (0x08036188 to 0x0803673A)

This is ST's CDC host example (`usbh_cdc_core.c`, `usbh_cdc_funct.c`), with 500-byte
`TxBuf`/`RxBuf`. 0x0802AEE0 sets the line coding to 57600 baud, 8N1. GivEnergy's changes:

- `CDC_ReceiveData` hands a received block to `UserCb.Receive` only when `cdc_rx_busy`
  (0x2000030B) is 0. It then sets that flag and counts the block in `cdc_rx_count`. The
  receive callback is 0x08022C50 (set by 0x08029E70). It is the frame handler that checks a
  CRC and clears `dongle_link_timer`.
- `CDC_SendData` drops the frame if the transmitter is still busy. After 4 busy calls in a
  row it sets `dongle_link_timer` (0x20000310) to 5000. On the next tick 0x08029712 then
  turns off the OTG FS clock, which restarts USB. If it gets stuck again after that, it sets
  bit 0 of 0x2000019C, an application flag set from many places that I did not identify.
- In normal use the 1 s tick (0x08029712, inferred period) counts `dongle_link_timer` up. At
  120 it sends a 0x12E-byte frame to the dongle. At 1440 (24 minutes if the tick is 1 s) with no good frame it
  restarts USB.

So the cloud/dongle path is USB CDC serial at 57600 baud. Any Modbus traffic the dongle
carries is handled in 0x08022C50, which is outside my range.

## Keil C library (0x0803675C on)

Every row I named has a Keil name, checked against MDK516's `c_w.l` and `fz_ws.l` symbol
tables. The rows are `strcpy`, `__aeabi_memclr`, `strcmp`, the three `heapauxi.o` stubs,
`__aeabi_memclr4`, `__user_setup_stackheap`, `exit`, `__user_libspace`, `_sys_exit`,
`_fp_init`, `__aeabi_f2iz`, `__aeabi_f2uiz`, `__aeabi_fmul`, `__fpl_fnaninf` and
`__fpl_fretinf`. Two of the `heapauxi.o` names contain `$`, so a C file cannot declare them.
I skipped five rows because they are not separate functions but parts of a library function:
0x0803695C and 0x080369A0 (the saturating tails of f2iz and f2uiz) and 0x08036A80, 0x08036A84
and 0x08036A96 (inner paths of `_fmul`, which is 0x102 bytes long and ends at 0x08036AAA).
The single-precision float code is called only from 0x0802C182 to 0x0802C7FC. That would be
the place to look for float arithmetic, possibly battery or power calculations (not checked).
