# Libraries in the A316 image

About a quarter of the ARM image (some 37 KB of 145 KB) is third-party code: ST's peripheral drivers and CMSIS file,
ST's USB OTG host library with a CDC class, FatFs, a handful of Keil C library members, and the
startup file. This page lists what is there, which version, how I know, how it is configured,
and what the vendor changed. The scheduler is the vendor's own code and has its own page,
[rtos.md](rtos.md).

Every claim here comes from the matched sources and the map (`map/filenames.tsv`,
`map/names.tsv`) and from the round notes in [../matching.md](../matching.md). The gate proves
each file byte for byte, so "unchanged" below means the code compiles to the image's bytes from
the library source, or, where I had no copy of the library, that the code has the library's
shape and nothing in it is GivEnergy's.

## What is in the image

| library | version | files here | evidence |
|---|---|---|---|
| ST STM32F10x Standard Peripheral Library | V3.5.0 | `src/stdperiph/` (12 driver files), `src/startup/startup_stm32f10x_cl.s` | the drivers are V3.5.0's own sources from `lib/stdperiph35`, included unchanged and proved whole; the startup file is V3.5.0's with two sizes changed |
| CMSIS device file for STM32F10x | from StdPeriph V3.5.0 | `src/cmsis/system_stm32f10x.c` | SetSysClockTo72, SetSysClock, SystemInit, SystemCoreClockUpdate and AHBPrescTable, written in ST's shape |
| ST USB OTG host library (STM32_USB-Host-Device_Lib) | V2.2.0 or close | `src/lib/usb_otg/` (usb_core, usb_hcd, usb_hcd_int), `src/lib/usb_host/core/` (usbh_core, usbh_hcs, usbh_ioreq, usbh_stdreq), `src/lib/usb_host/msc/` (usbh_msc_core, usbh_msc_bot, usbh_msc_scsi, usbh_msc_fatfs) | ST's function names, structures and order; the host state numbers (HOST_WAIT_PRT_ENABLED = 1 to HOST_ERROR_STATE = 12) and the port-enabled callbacks fit V2.2.0; usbh_stdreq.c has V2.1.0's functions (round 5) |
| CDC host class for that library | unknown | `src/lib/usb_host/cdc/` (usbh_cdc_core, usbh_cdc_funct) | the class's own names (CDC_Machine, CDC_TxParam, CDC_ReqState, CDC_cb); not a class of ST's release as far as I know |
| FatFs (ChaN) | R0.12c | `src/lib/fatfs/ff_p01.c`, `ff_p09.c` (one ff.c) | ChaN's names in ff.c order; R0.12c and not an earlier R0.12 because `create_chain` tests `free_clst <= n_fatent - 2` and `f_lseek` moves `fptr` before following the chain |
| ARM/Keil C library (armcc 5) | MDK 5.16 to 5.21 library | linked from `c_w.l` and `fz_ws.l` by the gate; two members also as C in `src/clib/` | `__main` to `__rt_exit` at 0x08014150 and the members from 0x0803675C link to the image's bytes from MDK516's library (docs/gate.md) |
| Startup file | StdPeriph V3.5.0 | `src/startup/startup_stm32f10x_cl.s` | V3.5.0's `startup/arm/startup_stm32f10x_cl.s`, assembled with armasm |

The ST template's user files are in the application tree and the other agent's pages cover
them: `src/app/stm32f10x_it.c` (interrupt handlers), `src/app/usb/usb_bsp.c` and
`src/app/usb/usbh_usr.c` (the USB example's board and user layers, much extended by GivEnergy).

Everything was built with armcc 5.06 at `-O0` and without `--split_sections`. So each source
file is one section, and armlink keeps all of a file if any of it is used. That is why the
image holds many library functions that nothing calls: most of the StdPeriph drivers, FatFs's
write side, `USB_OTG_StopHost`, the isochronous requests, `CDC_StopReception` and others.

## StdPeriph drivers and CMSIS

The twelve drivers in `src/stdperiph/` are `misc`, `adc`, `bkp`, `dma`, `exti`, `flash`,
`gpio`, `pwr`, `rcc`, `rtc`, `tim` and `usart`. Each wrapper includes ST's file from
`lib/stdperiph35` with `-DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER`. What uses them:

| driver | used by |
|---|---|
| misc | `nvic_init`: vector table offset 0x14000, priority group, IRQ priorities |
| adc, dma | `board.c` (ADC sampling with DMA), `ADC1_2_IRQHandler` |
| bkp, pwr, rtc | `rtc.c` (calendar in the RTC counter; backup register 4 holds 0xA5A5 once set), `modbus_slave.c`, `schedule_p3.c` |
| exti | `EXTI1_IRQHandler` (USB over-current) |
| flash | `usbh_usr.c`, when it takes `ARMStore.bin` from a stick |
| gpio | `board.c`, `panel_led.c`, `i2c.c`, `eeprom.c`, `stm32f10x_it.c` |
| rcc | clock enables, LSE and RTC clock, OTG FS clock |
| tim | TIM3 (ADC trigger and buzzer) and TIM4 in `board.c`, TIM2 in `usb_bsp.c` |
| usart | USART1 (auxiliary), USART2 (RS485 meter), USART3 (RS485 Modbus slave), UART4 (DSP link) |

Configuration found:

- **8 MHz crystal.** `HSE_VALUE` is 8000000 where V3.5.0's `stm32f10x.h` sets 25000000 for
  the connectivity line. `RCC_GetClocksFreq` loads 8000000 and its literal pool has no
  25000000. The wrapper passes `-DHSE_VALUE=8000000`; an edited line in `stm32f10x.h` gives the
  same bytes, so I cannot tell which the vendor did.
- **72 MHz from the 8 MHz crystal.** `SetSysClockTo72` writes `RCC_CFGR2` = 0x10814: PREDIV2
  /2, PLL2 x10 (40 MHz), PREDIV1 from PLL2 /5 (8 MHz), then the main PLL x9. ST's value for a
  25 MHz crystal is 0x10644. This is a vendor edit of the CMSIS file (or of its defines), made
  for the crystal. `SystemCoreClockUpdate` also has 8000000 for the HSE.
- **Vector table.** `SystemInit` sets VTOR to 0x08000000, as shipped (`VECT_TAB_OFFSET` 0).
  The application lives at 0x08014000, above what is probably a boot loader, and `nvic_init`
  moves VTOR to 0x08014000 later.
- **Function order.** In `system_stm32f10x.c` the two static clock functions come first, with
  prototypes, where ST's file has them last. The image has this order.
- **Startup sizes.** `Stack_Size` is 0x800 (shipped 0x400) and `Heap_Size` 0x1500 (shipped
  0x200), the values set in Keil's configuration wizard. The heap is at 0x20008898 and the
  main stack after it, so `__initial_sp` is 0x2000A598. Tasks run on their own stacks
  ([rtos.md](rtos.md)); the main stack is for start-up and interrupts.
- **SystemInit runs twice.** `Reset_Handler` calls it before `__main`, as ST's file does, and
  `main` calls it again first thing.

Vendor changes in the StdPeriph sources: none. Round 1 took `adc`, `rcc` and `tim` for edited
files because they scored 1 or 2; all three turned out to be V3.5.0 unchanged (a literal pool
placed inside `TIM_OC4Init`, the `HSE_VALUE` above, a scoring artefact in `adc`).

## USB OTG host library

The STM32F105's OTG FS port runs as a host only. `task_usb` (`src/app/usb/usb_task.c`) calls
`USBH_Process` on each pass. The host starts with the CDC class; when a device's first
interface is class 8, `USBH_USR_Configuration_DescAvailable` sets `usb_class_code` to 8 and
`task_usb` starts the host again with the MSC class.

- **CDC** is for the communications dongle. It carries Modbus RTU: `CDC_ReceiveData` hands a
  frame to `UserCb.Receive`, which is `modbus_slave_process`, and the reply goes out with
  `CDC_SendData`. The 1 s housekeeping (`sys_tick_1s`) also sends a
  0x12E-byte frame with `CDC_SendData` when the dongle has been quiet for a while.
- **MSC** is for a USB stick with update files, read through FatFs.

Configuration found (the library's `usb_conf.h` values, seen in the code): the FS core at
0x50000000, 8 host channels, the embedded PHY, no DMA, no SOF output, VBUS sensing off
(`VBUS_SENSING_ENABLED` not defined; `usb_bsp.c` drives VBUS), FIFOs of 128 words receive and
96 each for non-periodic and periodic transmit, control timeouts of 5000 frames with a data
stage and 50 without, configuration descriptors up to 0x400 bytes, a CDC receive buffer of 500
bytes, line coding 57600 8N1 (set by `usb_cdc_line_coding_init`).

Vendor changes:

1. **Line state watchdog** (`usbh_core.c`). `usbh_read_line_state` and `usbh_line_state_stuck`
   are not ST's. `USBH_Process` calls `usbh_line_state_stuck` in the class state before the
   class machine and puts the result in `usb_line_stuck`. It returns 1 when HPRT0's D+/D- line
   state has not changed for 1000 passes. `USBH_USR_MSC_Application` then clears the flag and
   sends SET_LINE_CODING again (`CDC_ChangeStateToIssueSetConfig`).
2. **Port suspend and resume** (`usbh_core.c`). `usb_host_port_resume` and
   `usb_host_port_suspend` sit at the end of the file. They are ST-style code (PCGCCTL clock
   gating, HPRT0 suspend and resume) that I do not know from ST's `usbh_core.c`. Nothing calls
   them. Their names are mine.
3. **CDC receive** (`CDC_ReceiveData`). The block is handed to `UserCb.Receive` only when
   `modbus_src` is 0; the function then sets it to 1, so the Modbus code knows the frame came
   from USB (and, I think, so that a second frame is not taken before the first is answered). It also counts blocks in
   `cdc_rx_count`. `CDC_ProcessReception` clears `RxBuf[1]` after each block, which I also
   take for GivEnergy's.
4. **CDC transmit** (`CDC_SendData`). When the transmitter is still busy the data is still
   dropped, as in the class, but the call is counted (`cdc_tx_busy_count`). After four it sets the dongle link
   timer `usb_cdc_timer` to 5000, past the limit at which `sys_tick_1s` power-cycles the USB
   port. If the transmitter is still stuck after that, it sets bit 0 of `cfg_changed_flags`.
5. **Disk read** (`usbh_msc_fatfs.c`). `disk_read` gives up after 0xFFFFF tries while the stick
   answers busy, and when a read has taken 0xFFFF0 tries or more it resets the chip (the inline
   `NVIC_SystemReset`). ST's example loops for as long as the stick is busy. `disk_write` is
   bounded at 0x7FFFFFF tries; nothing calls it.
6. **No time stamps.** `get_fattime` returns 0.

Everything else in the USB files follows ST: `usb_core.c` (host half), `usb_hcd.c`,
`usb_hcd_int.c`, `usbh_hcs.c`, `usbh_ioreq.c`, `usbh_stdreq.c`, and the MSC class
(`usbh_msc_core.c`, `usbh_msc_bot.c`, `usbh_msc_scsi.c`). `USBH_SetFeature` and
`USBH_ClearFeature` (device requests 3 and 1, no callers) have names I gave them.

Shapes kept for matching, noted in the files: several files declare a library function with
the narrower return type they were compiled against (`OWN_` defines before the includes);
`usbh_stdreq.c`'s parse functions must be `static`; a few `volatile` locals hold dead stores
the image has.

## FatFs R0.12c

`ff.c` is two files, `ff_p01.c` and `ff_p09.c`. Joined, armcc knows `ld_word` and `st_word`
when it compiles `ld_clust`, `st_clust` and `get_fileinfo` and saves fewer registers in them
than the image does, so the vendor's compile did not see those definitions first
([../readability.md](../readability.md), step 4).

Configuration found (`src/include/fatfs_types.h`): `_FS_READONLY 0`, `_FS_MINIMIZE 0`,
`_USE_LFN 0` (8.3 names), `_FS_RPATH 0`, `_FS_EXFAT 0`, `_FS_LOCK 0`, `_USE_FASTSEEK 0`,
`_FS_TINY 0` (each FIL has its own 512-byte buffer), `_MAX_SS 512`, one volume, code page 437
(`ExCvt`), no `f_mkfs`, `f_chmod`, `f_utime`, `f_findfirst` or string functions. The
configuration tests ChaN keeps in C (`if (_FS_RPATH && ...)`) leave a `nop` in the image.

Use: `usbh_usr.c` mounts `"0:"`, checks free space with `f_getfree`, reads `DSPStore.bin`,
`ARMStore.bin`, `SN.txt` and `MSN.txt`, and deletes the two update files with `f_unlink`.
It writes nothing. `f_write`, `f_lseek`, `f_mkdir`, `f_rename`, `f_truncate`, `f_stat` and the
directory functions are in the image with no callers.

Vendor changes in ff.c: none found.

## Keil C library

The gate links the C library from MDK516's `c_w.l` (and `fz_ws.l` for `_fp_init`): `__main`,
scatter loading, the stack and heap set-up, `__rt_exit`, `strcpy`, `strcmp`, `__aeabi_memclr`
and the float helpers `__aeabi_f2iz`, `__aeabi_f2uiz` and `__aeabi_fmul`
(`exhume.toml`, `[gate]`). Two members are also proved as C at the same addresses:

- `src/clib/keil_heapauxi.c`: `heapauxi.o`'s three `bx lr` functions,
  `__use_two_region_memory` and two more that are probably the two-region heap hooks
  (`__rt_heap_escrow`, `__rt_heap_expand`; still `sub_` names).
- `src/clib/keil_fpinit.c`: `fpinit.o`'s `_fp_init`.

The startup file IMPORTs `__use_two_region_memory`, so the library uses two memory regions:
the heap and the stack it gets from `__user_initial_stackheap`.
