# The boot loader and the DSP update

Files: `src/boot/boot_main.asm`, `boot_update.c`, `boot_csm.c`, `boot_flash_loader.c`,
`app_header.c`, `src/ti/CodeStartBranch.asm`, `src/ti/F2806x_CSMPasswords.asm`,
`src/ti/F2806x_MemCopy.c`, `src/include/boot.h`, `src/include/Flash2806x_API_Library.h`.

## Flash layout

The image is 0x3E8000..0x3F7FFF (D316_DSPStore.bin covers all of it).

| Address | What |
|---|---|
| 0x3E8000 | version = 0x013C (316), app_header.c |
| 0x3E8004 | app_entry: start (os_main.c), the application's entry |
| ... 0x3F3FFB | the application |
| 0x3F3FFC | app_valid = 0x00AA, the last words of sector B |
| 0x3F4000 .. 0x3F7FFF | sector A: the boot loader, never erased or written by an update |
| 0x3F5000 | boot_update |
| 0x3F5004 .. 0x3F5225 | boot_flash_loader, loaded here and run at 0x0400 |
| 0x3F5226 | boot_main |
| 0x3F528C, 0x3F52BB, 0x3F52C4 | CsmUnlock, MemCopy, boot_copy_run |
| 0x3F7F80 .. 0x3F7FF5 | csm_rsvd, zeros |
| 0x3F7FF6 | codestart: LB _boot_main |
| 0x3F7FF8 .. 0x3F7FFF | the CSM password |

## Start-up

The boot ROM jumps to codestart, which branches to boot_main. boot_main makes GPIO ports A and B
inputs without pull-ups and reads 0x3F3FFC:

- 0xAA: watchdog off, RAM blocks M0, M1 and L0..L8 cleared, ST0 = 0, ST1 = 0x0A0B, then
  LB 0x3E8004. start turns the watchdog off again and runs c_int00, which calls main.
- anything else: interrupts and watchdog off, then LB boot_update, and the DSP waits for an
  update on SCI-A.

That word is the only check. There is no checksum, CRC or signature of the application.

### Why boot_main is assembly

It was tried in C with asm statements (docs/hard.md, "boot_main"). It cannot match:

- The GPIO writes come before the stack frame is allocated (ADDB SP,#4 follows EDIS). A TI C
  compiler always allocates the frame first.
- Nothing follows the final LB to the application, no SUBB SP and no LRETR. Every TI C compiler
  emits the epilogue of a function whose last statement is an asm statement.
- RPT || MOV block clears, POP ST1 / POP ST0 and LB to absolute addresses are boot ROM style
  code that C cannot express.

The middle part (a volatile local read back, an SBF/SB/SB if/else) looks like compiler output that
was pasted in and edited; it is kept as the image has it.

## The CSM unlock

The code security module guards the flash, the OTP and L0..L3 RAM while the 128-bit password at
0x3F7FF8 is not all 0xFFFF. TI's file has all 0xFFFF; this build has 0xFFFF x 4, 0xF5F4, 0xF0F3,
0xF5F2, 0xF2F1, so the chip is secure after every reset. Code that runs outside the secure area
cannot touch it, and the update routine runs in M1 RAM at 0x0400. So boot_copy_run first calls
CsmUnlock (TI's code): it writes the eight password words to KEY0..KEY7 and reads the eight
password locations, which is what the CSM needs to open. SECURE then reads 0 until the next reset.
boot_copy_run ignores CsmUnlock's result. The password is in the clear in DSPStore.bin.

## How an update starts

This is the path the ARM's DSPStore.bin update uses (A316 docs/subsystems/usb.md):

1. The ARM finds DSPStore.bin on a USB stick and stops normal work.
2. It sends run command 4 in byte 1 of its 0x43 frames. On the 6th such frame arm_run_command
   posts EV0_UPDATE, and the state machine goes to OP_UPDATE.
3. task5_arm_link sends one more 0x44 frame and then stops sending for good, which leaves SCI-A
   to the boot loader.
4. state4_update stops the converters, waits 100 ticks, then posts EV0_BOOT to itself and calls
   boot_update with interrupts off.
5. boot_update calls boot_copy_run: CsmUnlock, MemCopy of 0x3F5004..0x3F5225 to 0x0400, then
   boot_flash_loader at 0x0400. It never returns.

With no valid application, boot_main reaches step 5 directly.

## The update protocol (boot_flash_loader)

boot_flash_loader sets the clocks (crystal, PLL x 9 / 2), the PIE (all off), the flash wait states
(TI's InitFlash with OTPWAIT 5), SCI-A (9600 8N1, polled) and GPIO27 itself, so that it does not
call into the flash it is about to erase.

Frame from the ARM:

| Byte | Content |
|---|---|
| 0 | 0x0A |
| 1 | len, the number of data bytes (below 140) |
| 2 | command |
| 3 .. len + 2 | data |
| len + 3, len + 4 | 16-bit sum of bytes 0 .. len + 2, high byte first |

Reply from the DSP: 0x0A, 0, code, then the 16-bit sum of those three bytes, high byte first.
The code is 'O' (0x4F) for done, 'C' (0x43) for a bad sum, or the flash API's nonzero status.

| Command | Data | What it does | Reply |
|---|---|---|---|
| 'W' | none | Flash_Erase(0xFE): sectors B..H. Sector A, with the boot loader, stays. | 'O' or the status |
| 'S' | 4 bytes | sets the flash word address, high byte first | 'O' |
| 'D' | 2 + 128 bytes | block number n (high byte first) and 64 words (each high byte first); Flash_Program writes them at address + 64 x n when that is below 0x3F4000 | 'O', or the status; 'O' and no write at 0x3F4000 or above |
| 'F' | none | nothing (a hello) | 'O' |
| 'E' | none | after the reply: a pause, GPIO27 high, then a loop until something outside resets the chip | 'O' |
| other | | nothing | 0 |

The A316 sends 'W', then 'S', then the 128-byte 'D' frames, then 'E'. It restarts from 'W' after
2000 calls without an answer.

### The 0x3F4000 limit and the 0xAA rule

The 'D' check keeps every block start below 0x3F4000, so an update cannot write sector A: the
boot loader, codestart and the password survive any update. 'W' erases sector B, and with it
app_valid at 0x3F3FFC, so from the erase until the block that holds 0x3F3FFC is written, a reset
lands in the boot loader and waits for the ARM again. The application runs again only after
that block is written with 0xAA.

### What is checked

I looked for a signature or an image check and found none. The checks are:

- each frame's 16-bit byte sum (a reply of 'C' when it is wrong);
- the 0x3F4000 limit on each 'D' block's first word;
- the 0xAA word, read by boot_main at the next start.

There is no CRC of the image, no length check of a 'D' frame (a short one programs stale data
from the receive buffer) and no check that the ARM sent the blocks in any order. The code also
has a rule that a session must begin with an 'F' frame of length 0, but it never takes effect:
boot_rx_active is set to 1 as each byte arrives, before the tests read it. The A316 never sends
'F', so it relies on that.

### The end of an update

'E' does not restart the DSP itself. The watchdog is off on every path, so the loader raises
GPIO27 and loops. I take GPIO27 to be a request to an outside part (the application raises it in
power_limits.c for its own fault cases too), but I have not traced the board.

## Open points

See docs/findings-comm.md: the stack during the cold update path, the 'D' length, the receive
buffer overflow for len 136..139.
