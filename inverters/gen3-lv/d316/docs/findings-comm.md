# Findings: ARM link, boot loader, RTOS and drivers

Scope: src/app/comm, src/boot, src/rtos, src/drivers, src/ti. Each item was re-read in the source
before it was listed. Confidence is how sure I am that the code does what I say; the effect is my
estimate. Nothing here is fixed: the tree stays byte-exact.

## Likely bugs

### 1. sci_rx clears task 5's pending events on every ARM byte (high confidence)
`src/drivers/sci.c`, sci_rx, normal receive path: `arm_rx_block &= 8; if (arm_rx_block != 0) return;`.
arm_rx_block is placed at 0xDAAC (map/data.tsv). os_tcb starts at 0xDA80 with 8-word blocks (sp 2,
period 1, count 1, events 2, mask 2), so 0xDAAC = 0xDA80 + 5 x 8 + 4 is os_tcb[5].events: the event
word of task 5, task5_arm_link. Every ARM byte outside the BMS relay clears every pending task 5
event except bit 3. Nothing posts bit 3 to task 5, so the "drop the byte" test never fires, but the
clear always happens. The 45th byte posts EV5_FRAME after the clear, so that event lasts only until
the next byte.

Effect (inference): a 45-byte frame at 9600 baud takes about 47 ms of the ARM's 60 ms cycle, so tick
events posted during a frame are lost; the 0x44 frames go out less often than every 240 ms, and the
idle and fail counters see fewer wake-ups. If the next frame starts before task 5 runs, EV5_FRAME is
lost; the task can still wake (os_ready is set), but os_wait returns 0, task5_arm_link counts an idle
wake-up, and after 6 of those it empties the ring, possibly in the middle of a frame.

### 2. The boot loader's receive buffer can overflow by 4 words (high confidence, low effect)
boot_flash_loader.c accepts len < 140 and ends a frame at n = len + 4, but boot_rx_frame holds 140
words: len 136..139 writes up to boot_rx_frame[143] (0xD78C..0xD78F). Nothing in the map uses those
words and nothing else runs during the loader. The A316 sends at most 130 data bytes.

### 3. 'D' checks neither the frame length nor where the block ends (high confidence, low effect)
The 64 words always come from boot_rx_frame[5..132], whatever len was, so a short 'D' frame
programs stale bytes from an earlier frame. The limit test checks only the block's first word
(`addr + off < 0x3F4000`); an 'S' address that is not a multiple of 64 lets the last block run up
to 63 words into sector A. Neither happens with the A316, which sends 130-byte 'D' frames.

### 4. Run command 4 counts frames in total, not in a row (high confidence, low effect)
arm_run_command clears run_cmd4_cnt only when it fires, so any six frames carrying command 4 post
EV0_UPDATE, even with other commands in between.

### 5. arm_page2 calls os_post_isr from task code (medium confidence, low effect)
When byte 39 (aging_test_enable) goes from 0 to 1, arm_page2 calls os_post_isr(TASK_STATE,
EV0_STOP) from task 5 with interrupts enabled. os_post_isr updates os_tcb[0].events and os_ready by
read-modify-write without DINT, and timer2_isr posts to task 0 every 20 ms the same way, so an
interrupt in between can lose the stop or the tick. os_post is the task-level form.

### 6. 16-bit products before the clamp in arm_page7 (high confidence, low effect)
`(unsigned int)(t * 10)` (chg_max) and `(unsigned int)(t * 100)` (batt_max_charge_current) are
16-bit products: a word above 6553 or 655 wraps before the clamp (7000 becomes 4464, then is
clamped to 10000 instead of 60000).

### 7. adc_offset_measure reads results before the conversion ends (medium confidence, low effect)
After forcing the SOCs it waits only for ADCBSY to go to 1 and then reads the five results, which
are probably the previous conversion's; the first of the 128 samples is whatever the registers
held. This barely moves the mean.

## Dead code and dead tests

- RTOS functions with no callers: os_sched_isr, os_int_ctx_sw, os_post_flag, os_take_events,
  os_sleep, os_cpu_sr_save, os_cpu_sr_restore. TCB 7 is never used. The RTOS is cooperative: no
  interrupt switches tasks, and task 0 doubles as the idle loop.
- TI DMA functions with no callers: DMAInitialize and the 30 functions for channels 2..6.
- TI default handlers: all are in the image; nine are replaced in the vector table and EMPTY_ISR is
  in no vector.
- sci_send works only for port 0: sci_tx(1) sends from bms_tx_buf, not from sci_tx_state[1].
- `SciaRegs.SCIHBAUD != 2` in task5_arm_link is always true.
- The bit-3 "drop the byte" test in sci_rx never fires (finding 1).
- The loader's boot_rx_active tests at n 1 and 2 never fail, so its "send 'F' first" rule is dead;
  the A316 never sends 'F' and relies on that.
- Writes of 0 to GPxSET/GPxCLEAR bits do nothing (InitGpio: GPIO10, 11, 12, 21, 27, 52, 53; the
  boot loader: GPIO27).
- epwm_trip_force and epwm_trip_clear write ePWM2 and ePWM6, which have no clock; PIE 4.2 (eCAP2)
  is enabled with TI's default handler, but eCAP2 has no clock.
- arm_frame_parse sets qu_q_max = 438 on each of its 42 passes.
- Receive errors are handled twice, in the interrupt and again at the top of sci_rx.

## Mismatches with the ARM

- Fields the ARM reads but this DSP always sends as 0: page 3 bytes 13-22 (dsp_fault_data2,
  dsp_p3_w03/w05/w07, dsp_fault_data1), 25 (fault_code), 30 (batt_state_dsp) and 31; page 2
  bytes 24-27; page 1 byte 33.
- 0x44 bytes 7 and 8 are the low bytes of bms_alarm and batt_warning; the ARM reads them as one
  16-bit warning word, so neither high byte reaches it.
- The ARM sends a PV start voltage of 1550 while HR175 is set; the DSP clamps it to 1500.
- What the ARM sends as "grid country" bytes (page 2 byte 23, page 3 byte 32) the DSP stores as
  dci_trip_time and dci_trip_time_it.

## Open questions

- Stack on the cold update path: boot_main runs on whatever SP the boot ROM leaves. If that is near
  0x0400, MemCopy writes the update routine (0x0400..0x0621) over the live stack. The warm path,
  from the application with SP in os_stack, is fine. I have not checked where the boot ROM leaves SP.
- GPIO27: the 'E' command raises it and loops; power_limits.c raises it after a relay timeout and in
  some faults. The watchdog is off everywhere, so GPIO27 probably asks an outside part for a reset;
  the board would settle it.
- End of the BMS relay: only arm_link_frame clears bms_relay_active, and it cannot run during the
  relay. The A316's last relay frame ("BMS", 1, "END", CRC-32) is shorter than the 1034 bytes the
  DSP counts, so GPIO57 stays high afterwards. Is the reset after 15000 idle passes the intended end?
- XtalOscSel: the application's copy turns on the crystal and switches to it with no wait; the boot
  loader's inline copy waits about 1 ms.
- HR2: the DSP reads page 0 byte 8 (HR2 high byte) as the grid code and byte 9 (low byte) as the
  model, which fits a packed word; not yet checked against HR2 on the ARM.
- Calibration gains: the page 5 and page 6 gains need the A316's cal_gains layout to say which ARM
  gain is which DSP word.
- Image integrity: the update is guarded only by per-frame 16-bit byte sums (two swapped bytes
  still pass) and the 0xAA word at 0x3F3FFC. Whether the ARM checks DSPStore.bin is a question for
  the A316 side.
