# How byte-exact matching works here

The goal for each firmware is source that the original toolchain turns back into the vendor's
image, byte for byte, with nothing copied from the image. Then the source is a proved description
of the firmware: whatever it says the code does, the shipped code does.

The method and most of the tools are [Exhume](https://github.com/abedegno/Exhume)'s
(its docs/method.md and its armcc5 and ti-c2000 profiles). In short:

1. **Fingerprint the toolchain.** Find the exact compiler release, options and libraries that
   built the image, by compiling known code (vendor libraries, the C library) with each candidate
   and comparing. A316: armcc 5.06 from MDK 5.21 at `-O0`, MDK 5.16's library, ST's StdPeriph
   V3.5.0. D316: TI CGT 16.9.6 at `-O1` with the FPU, the runtime library of 15.12.3. Each
   tree's README and toolchain configuration say how each was settled.
2. **Map the image.** Every function and data object, with its address and size
   (`map/functions.tsv`, `map/data.tsv`), so that every byte of the image belongs to something.
3. **Match.** Write C (or assembly where the original was assembly) for each function, compile it,
   link it at the function's real address with every other symbol at its map address, and compare
   the finished bytes with the image. A difference is a different source shape, not noise: the
   compiler is deterministic, so a mismatch means the source is not yet equivalent to what the
   vendor compiled. Drafts came from Ghidra and from generated C; the matching was done function by
   function, partly by agents running Exhume's skills.
4. **Gate.** `tools/gate.py` in each tree proves the whole tree at once: every source matches,
   the whole image links from the sources and the toolchain's own libraries, and the result is
   identical to the vendor's file. It must say IDENTICAL after every change.
5. **Make it readable without changing a byte.** Names for functions, variables and fields,
   shared headers, structs, named constants, original file names where the evidence gives them,
   subsystem directories, and comments with the evidence for each name. Each step is proved by
   the gate. Each tree's `docs/readability.md` records how
   ([A316](../../inverters/gen3-lv/a316/docs/readability.md),
   [D316](../../inverters/gen3-lv/d316/docs/readability.md)).

What is particular to each tree is in its own docs:
[gate.md](../../inverters/gen3-lv/a316/docs/gate.md) and
[matching.md](../../inverters/gen3-lv/a316/docs/matching.md) for A316 (the image is linked in
pieces under MDK-Lite's 32 KB link limit, and one file too large for it is proved by placing
armcc's object at its address), [notes.md](../../inverters/gen3-lv/d316/docs/notes.md) and
[hard.md](../../inverters/gen3-lv/d316/docs/hard.md) for D316 (the whole
image is linked in one `lnk2000` run).

How to run the gates is in [building.md](building.md); the notes the sources carry (`at:`,
`match:`, `name:`) and the maps are described in [conventions.md](conventions.md).
