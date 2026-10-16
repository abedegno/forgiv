# Adding a firmware

The repository holds one firmware per directory, each with its own sources, map, tools, docs and
gate. Another inverter, a battery, or another version of the Gen3 LV firmwares goes in a
directory of its own. This page is the checklist; [method.md](method.md) explains the method
and [conventions.md](conventions.md) the notes and maps a tree carries.

## Where it goes

`inverters/<model>/<firmware>/` or `batteries/<model>/<firmware>/`, named in lower case after the
version (`a316`, `d316`). A model directory has a README that says what each firmware runs on and
how the chips talk ([inverters/gen3-lv/README.md](../../inverters/gen3-lv/README.md) is the
example).

A tree builds without the others. Shared code between trees is fine when it is copied in, not
imported across trees.

## The stages

The method and most tools are [Exhume](https://github.com/abedegno/Exhume)'s, and its Claude Code
skills walk through each stage:

1. **Fingerprint** the toolchain: the exact compiler release, options and libraries, proved by
   compiling known code to the image's bytes
   ([fingerprint-toolchain](https://github.com/abedegno/Exhume/tree/master/skills/fingerprint-toolchain)).
2. **Map** the image: every function and data object, with addresses and sizes
   ([map-binary](https://github.com/abedegno/Exhume/tree/master/skills/map-binary)).
3. **Match** each source file
   ([match-file](https://github.com/abedegno/Exhume/tree/master/skills/match-file),
   [match-asm](https://github.com/abedegno/Exhume/tree/master/skills/match-asm),
   [verify-and-merge](https://github.com/abedegno/Exhume/tree/master/skills/verify-and-merge)).
   With several agents: [orchestrate](https://github.com/abedegno/Exhume/tree/master/skills/orchestrate).
4. **Gate**: link the whole image and compare it with the vendor's file, with no blobs of
   original bytes left
   ([link-and-diff](https://github.com/abedegno/Exhume/tree/master/skills/link-and-diff)). The
   tree's `tools/gate.py` must print IDENTICAL.
5. **Readability**: names, headers, constants, structs, file names and comments, every step
   proved by the gate
   ([readability-pass](https://github.com/abedegno/Exhume/tree/master/skills/readability-pass)).

## The checklist

- **The image.** Add the package to [firmware/manifest.toml](../../firmware/manifest.toml): its
  URL in a public archive pinned to a commit, a fallback URL, each member's name, size, sha256
  and the `dest` path the tree's gate reads, and any derived file (such as an Intel HEX form)
  with its own sha256. `tools/fetch_firmware.py` then fetches and checks it. No image goes into
  the repository.
- **The toolchain.** Describe it in [toolchains.md](toolchains.md) if it is a new one: where to
  download each piece, its size and sha256 where the vendor publishes a fixed file, and how to
  install it. Tool paths go through environment variables or a `toolchain.local.toml`, never
  into the tree.
- **The README.** The image layout, a status table like the top README's (bytes from proved
  sources, from libraries, from the linker, original bytes), how to run the gate and what it
  needs. Add a row to the status table in the [top README](../../README.md).
- **Third-party code.** Add it to [NOTICE.md](../../NOTICE.md).
- **The docs.** A tree's own `docs/` with its notes and findings; and, when the firmware changes
  how the system works, a line in the cross-tree pages here ([docs/README.md](../README.md)).
- **The checks.** `python3 tools/repocheck.py` passes (no binaries, links resolve, ASCII, no home
  paths, the manifest well formed), and the gate passes.

A firmware may come in before it is complete, if its README says so plainly and its gate passes
for what is there, with the blobs of original bytes counted and reported. The new-firmware issue
template asks for the package's source and sha256 first, so that it can be checked before work
starts.

The rules for every contribution (one topic per pull request, the gate's last lines in it, no
binaries, no flashing claims) are in [CONTRIBUTING.md](../../CONTRIBUTING.md).
