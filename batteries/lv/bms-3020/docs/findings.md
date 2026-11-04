# Findings from the ASCII and serial pass

While naming and commenting `src/app/comm/ascii_protocol.c` and `src/app/comm/serial.c`, I read
every routine of the UART4 link between paralleled packs. This page lists what looks wrong there.
I re-read each item in the source and checked it against the compiled code before listing it.
3022 has the same code, so every item holds for 3022 too.

Confidence labels are the same as on the inverter pages:

- **confirmed**: the behaviour follows from the code alone;
- **likely**: the code certainly does this, and it is probably not what was meant;
- **possible**: it depends on something outside the code, or on intent I cannot see.

All four items are on UART4, the pack-to-pack link. None is on the Modbus link to the inverter
(USART3). What a finding does in a running battery is inference unless I say so. I have run none
of this on hardware. Nothing here is fixed in the sources: the trees stay byte-exact.

## The Modbus requests on UART4

`uart4_protocol_task` answers Modbus-style requests on UART4 for any address below 16, with
function 3 (read) or 6 (write). Function 4 is accepted too, but nothing fills its reply, so the
task sends whatever earlier replies left in the reply buffer.

1. **A read that starts at register 60 or above uses an unset reply index (confirmed).**
   - `uart4_modbus_read_peer_registers` rejects a count of 0, a count over 60, and a range that
     crosses a multiple of 60. A start register of 60 or more with a small count passes those
     checks.
   - Only the branch for a start below 60 copies the registers and sets `reply_index`. Otherwise
     the CRC is computed over `reply_index` bytes and stored at `modbus_reply + reply_index`, with
     `reply_index` never set.
   - In the compiled code `reply_index` is `r4`, which on this path still holds the caller's
     pointer to `uart4_frame` (0x200035EA in 3020, 0x20003602 in 3022). The CRC routine cuts the
     length to 16 bits after its first step, so it reads 13,802 bytes of RAM (13,826 in 3022).
     The two CRC bytes go to 0x40006D15 in 3020 and 0x40006D45 in 3022. That is in the reserved
     part of the backup-register block, where I expect the stores to have no effect.
   - The reply the task sends has its length from the request. Its data bytes are whatever
     earlier replies left in `modbus_reply`, and its last two bytes are not a CRC.

   The A316 inverter firmware has the same kind of fault in its Modbus slave
   ([A316 findings](../../../../inverters/gen3-lv/a316/docs/findings.md), item 3).
2. **A write reply never carries its CRC (confirmed).**
   - `uart4_modbus_write_peer_setting` computes the CRC over the first 16 bytes of
     `modbus_reply` and stores it at bytes 16 and 17.
   - The task sends only 8 bytes for a write. They are the address and the function code (6, or
     0x86 for a rejected write), followed by six bytes left from earlier replies.
   - Effect: a sender that checks the reply CRC sees every write reply as corrupt. The write
     itself still takes effect.
3. **Peer addresses 7 to 15 reach past the peer arrays (likely).**
   - The task accepts addresses up to 15, and the write handler caps the address at 15. The peer
     records `protocol_pack` and the pending settings `protocol_pending` have 7 entries each
     (1,015 bytes of 145 and 798 bytes of 114).
   - A read for peer 7 to 15 returns whatever RAM follows `protocol_pack`, starting with the
     pending settings.
   - A write for peer 7 to 15 sets bits in a flag word and stores one 16-bit value beyond
     `protocol_pending`. In 3020, peers 7 and 8 land in `protocol_out` and peer 9 in
     `protocol_reply`, the buffers of the register protocol. Peers 10 to 15 land in a 12 KB block
     at 0x200045AC whose role I haven't identified. In 3022 every address is 0x18 higher.
   - Effect: only something that addresses a pack above 6 reaches this. How many packs a stack
     can hold is not in this unit.

## The ASCII protocol

4. **The set-date command does not set the clock (confirmed; probably meant).** The command
   (`ASCII_CMD_SET_DATE`, 0xB2, in `ascii_dispatch_command`) converts six bytes to BCD in
   `date_now` and replies with return code 9, "rejected". Every reader of the date first calls
   `runtime_publish_date`, which copies the running clock over `date_now`. So the command has no
   lasting effect, and its reply says so.
