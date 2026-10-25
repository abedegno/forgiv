# Compiler context recovered for A319

The acceptance result is the complete unit and full image, with all relocations
resolved and no masked bytes. Individual-function scores were diagnostics only.

The ARM application retains A316's `Cortex-M3 -O0 --loose_implicit_cast` options,
with one `.text` per C unit. Most library, RTOS and driver units transfer after
rebinding their symbols. The runtime also needs `__aeabi_ui2f` in A319.

The largest context changes were:

- `board.c`, interrupt setup, four empty hooks and panel output code belong in one
  reconstruction unit. This reproduces the native shared pools and halfword starts.
- Meter records are packed 144-byte records. The old 46-byte type creates incorrect
  strides and offsets across the meter, Modbus and monitoring units.
- Native pool words sometimes address the first member of a pair of records rather
  than the whole array. Named views for records 3/4, 5/6 and 7/8 preserve these bases.
- The floating-point wire decoder naturally uses a four-byte reversed local array,
  a float pointer, an integer scale converted to float, and a signed integer result.
  Extra named product/multiplier temporaries produce different code.
- Extended-meter power factors truncate the signed scaled product before subtracting
  it from 10000. The record's power-factor fields are unsigned halfwords.
- The extended request builder uses a byte address, halfword function/register/count/
  CRC locals, and a volatile read of the live enable mask. Ordinary reads have a
  different load order.

The last parser mismatch was eleven bytes representing a register swap in its
legacy-meter direction test. A context audit checked unit boundaries, same-file
callees, ownership (temporarily defining the address variable did not help), packed
field views, signedness, declarations and linkage. MDK 5.21, 5.22, 5.24, 5.25 and
5.36 produced the same remaining difference.

Bounded diagnostic batches compiled 96 inline-accessor variants, 64 mask-type forms,
24 declaration/case-label forms, 48 copy-expression forms and 12 promotion forms.
Accessors and type/declaration changes did not produce an exact candidate. The
copy/promotion audit identified a minimal explicit eight-bit width mask:

```c
((cfg_settings.reverse_115_meter_dir & 0xFF) & (1 << meter_fc04_addr)) ==
    (1 << meter_fc04_addr)
```

This is ordinary C with the direction register's width stated explicitly. It emits
no additional operation but changes armcc's allocation. The chosen source then
passed unmasked placement of the entire 10,856-byte meter unit, the zero-blob gate,
and independent armlink verification and linking of the full image. No assembler
replacement or forced-register instruction was used.

The standard-library region table decodes to RW at `0x20000000` (size `0x594`) and
ZI at `0x20000594` (size `0xA244`). Both gate modes therefore give their synthetic ZI
section four-byte alignment; eight-byte alignment creates an unwanted four-byte
hole. This changes placement only; the original counting/comparison rules remain.
