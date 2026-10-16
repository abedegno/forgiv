## What this changes

<!-- One topic per pull request. Which trees, and what changes in each. -->

## Gate output

<!-- For each tree touched, the last lines of a full `tools/gate.py` run (not --fast). -->

```
```

## Checklist

- [ ] gate output: IDENTICAL for every tree touched
- [ ] no binaries: no images, packages, objects, executables or toolchain files added (`python3 tools/repocheck.py` passes)
- [ ] no machine paths, user names or serial numbers
- [ ] new statements that go beyond the code are marked "inference"
- [ ] for a new firmware: manifest entry with sha256s, README, NOTICE.md and the top README's status table
