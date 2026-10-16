# CI: the gates on GitHub's runners

The gates need the vendors' toolchains, and none of them may be published in this repository
([toolchains.md](toolchains.md)). So CI runs them from an encrypted bundle that only this
repository's own workflows can open. This follows
[Exhume's encrypted-bundle pattern](https://github.com/abedegno/Exhume/blob/master/docs/ci-bundle.md).

## What runs

Two workflows:

| Workflow | When | What it runs |
|---|---|---|
| `check.yml` | every push and pull request, forks included | `tools/repocheck.py` ([building.md](building.md#checks-without-the-toolchains)) |
| `gate.yml` | pushes to `main`, pull requests from branches of this repository, and by hand | the gates and mod builds of both trees, each with a fail test |

`gate.yml` has two jobs that run side by side:

| Job | Steps |
|---|---|
| `a316` | `tools/gate.py` (MDK-Lite, in pieces), `tools/gate.py --whole` (the licensed MDK, one link), `tools/modbuild.py` (the licensed MDK; the unchanged tree must give the original `ARMStore.bin` byte for byte), then three fail tests |
| `d316` | `tools/gate.py`, `tools/modbuild.py` (the unchanged tree must give the original image word for word), then two fail tests |

Each fail test (`tools/ci/failtest.py`) copies the tree's `src/` to `build/failtest/src`,
changes one constant in the copy, runs the tool on it and requires it to fail:

| Tool | The change | Why it must fail |
|---|---|---|
| A316 `gate.py` | `schedule_p3.c`: `grid_out_day >= 0x7fbc` to `0x7fbd` | the image differs |
| A316 `gate.py --whole` | `meter.c`: `hr_regs[21] = 0x13c` to `0x13d` | the image differs |
| A316 `modbuild.py` | `main_p1.c`: `OSTaskCreate(0x802735b,` to `0x802735d,` | a flash address written as a number (check 5) |
| D316 `gate.py` | `rcmu.c`: `rcmu.c.limit = 0.3f` to `0.25f` | the image differs |
| D316 `modbuild.py` | `app_header.c`: `app_valid = 0x00AA` to `0x00AB` | a fixed word changed (check 2) |

The gates' own fail tests run last in each job, because they leave `build/gate` holding the
changed source, and the mod build reads `build/gate`.

A pull request from a fork gets `check.yml` only: GitHub gives it no secrets, and the jobs of
`gate.yml` skip themselves for it rather than fail.

## The bundle

The private repository `abedegno/forgiv-ci-assets` holds the bundle and nothing else: an
[age](https://age-encryption.org)-encrypted, zstd-compressed tar, split into parts under 95 MB
(GitHub refuses files over 100 MB), named `forgiv-ci-assets.tar.zst.age.00`, `.01` and so on. Its
Actions are disabled, so nothing in it ever runs. Inside:

| Path | What it is |
|---|---|
| `mdk/MDK521`, `mdk/MDK516` | the two MDK-Lite homes the A316 gate uses: `TOOLS.INI`, `ARM/ARMCC`, `ARM/sw` |
| `keil/TOOLS.INI` | the licensed MDK's `TOOLS.INI`, which holds the licence |
| `keil/licence.reg` | two registry keys of the licensed MDK's Wine prefix (below) |
| `stdperiph35` | ST's StdPeriph V3.5.0: `Libraries/` and `Project/STM32F10x_StdPeriph_Template` |
| `c2000/ti-cgt-c2000_16.9.6.LTS` | TI's Windows CGT 16.9.6, `bin/` and `include/` |
| `c2000/rts2800_fpu32.lib` | the runtime library of TI CGT 15.12.3 |
| `MANIFEST.txt`, `SHA256SUMS` | what each is and where it came from; a hash of every file |

The rest comes from public sources: TI's Linux build of CGT 18.12.8 (the D316 native object
tools) from TI's server, checked against its SHA-256 and cached by
`tools/ci/install-c2000-native.sh`; the firmware images from `tools/fetch_firmware.py`; Exhume
checked out at the commit `gate.yml` pins. The bundle is about 30 MB, one part.

### The licensed MDK

Keil confirmed that the licence may be used on the CI runner. The licence is in the bundle only.

A licensed armlink only works in the Wine prefix MDK was installed in: outside it, it stops with
`L9555E` and a registry read error. I found the smallest set that makes a new prefix work by
starting from a prefix made by `wineboot` and adding the keys of the licensed prefix's
`system.reg` that it lacks. `user.reg` and `userdef.reg` are not needed, and neither key alone is
enough. The set is:

- two keys under `HKLM\Software\Classes`, appended to the new prefix's `system.reg`. Their names
  are not published: `tools/ci/make-bundle.sh` takes them from `LICENCE_KEYS` or from the
  gitignored `tools/ci/licence-keys.local`;
- the licensed `TOOLS.INI` at `drive_c/Keil_v5`;
- `drive_c/Keil_v5/ARM/ARMCC` and `ARM/sw`. The licensed MDK's `ARMCC` is MDK 5.21's plus the C++
  libraries, which nothing here links, so the CI prefix links to the bundle's `MDK521` copies
  rather than carry a second one. `ARM/sw` holds the tool variant mappings; without it armasm
  fails its licence check.

`tools/ci/setup-toolchains.sh` builds that prefix on the runner. With it, `gate.py --whole`
passes on macOS (Wine 11) and on Linux (Wine 8). The A316 tools drop every line of tool output
that names the licence (`tools/a316env.py`), so it never reaches the log.

## The secrets and keys

| Where | What |
|---|---|
| this repository's secret `FORGIV_ASSETS_DEPLOY_KEY` | the private half of a read-only deploy key of `forgiv-ci-assets`. It can read that one repository and nothing else |
| this repository's secret `FORGIV_ASSETS_AGE_KEY` | the age secret key. Without it the bundle is noise, so a leaked deploy key alone exposes nothing |
| my macOS Keychain, service `forgiv-ci-assets-age` | the same age secret key |
| `~/.forgiv-ci-age.pub` | its public recipient, which `make-bundle.sh` encrypts to |

## The safeguards

- **Nothing decrypted is cached**, nor anything built from it. The only cache holds TI's
  public Linux tools, keyed on the script that pins them.
- **Nothing is uploaded.** The workflow has no artifacts.
- **Nothing lists or prints the bundle.** `tools/ci/ci-assets.sh` reports a count of files that
  passed their check.
- **The last step of each job deletes it**, whatever happened before (`if: always()`,
  `tools/ci/cleanup.sh`): it stops the Wine servers and removes the decrypted tree, the clone,
  the key files, the Wine prefixes, the `toolchain.local.toml` files and the StdPeriph link the
  set-up wrote, and both trees' `build/`.
- **No `pull_request_target`.** The jobs with secrets run on pushes to `main`, by hand, and on
  pull requests whose branch is in this repository, whose authors can already push here.
- **Least privilege.** `permissions: contents: read`; `persist-credentials: false` on every
  checkout; the deploy key reads one repository; the clone trusts only GitHub's published host
  key.
- **One run per branch at a time** (a concurrency group); a new push cancels the old run.

## Linux on the runner

The jobs run on `ubuntu-latest` in a `debian:bookworm` container, whose Wine 8.0 (with its
32-bit half, `wine32:i386`) runs the 32-bit Windows tools: armcc 5.06 and TI CGT 16.9.6.
`tools/ci/install-linux.sh` installs it. I tested the whole sequence in that container under
Docker on an Apple silicon Mac, where it passed. Ubuntu 24.04's own Wine 9.0 and WineHQ's 11.0
could not be tested there: under Docker's x86 emulation (QEMU user mode for 32-bit programs),
Ubuntu's Wine fails to load any program that must sit at 0x400000
(`c0000018`, its 32-bit loader is position independent) and WineHQ's aborts. They may well work
on a real x86 runner, but Debian's is the one I have seen work.

What differs from macOS, and how the tools allow for it:

- The D316 tools copy the master prefix for each job prefix with `cp -Rc` (an APFS clone) on
  macOS, and with `cp -a --reflink=auto` elsewhere.
- Wine's paths: the tools find `wine` on `PATH` when Wine Devel is not installed.
- The file system is case-sensitive. Nothing in the trees depended on case.

Times in the emulated container: the A316 gate 90 s, `--whole` 85 s, the mod build 50 s; the D316
gate 335 s, its mod build 85 s. A real runner should be faster.

## Making and replacing the bundle

On the Mac that holds the toolchains:

```sh
sh tools/ci/make-bundle.sh                # writes ~/forgiv-ci-assets-staging/forgiv-ci-assets.tar.zst.age.NN
```

The defaults of its inputs are at the top of the script; each is an environment variable. It
checks that the licensed MDK's `ARMCC` is still MDK 5.21's, makes the tree in a private
temporary directory (removed on exit), and prints only counts and sizes. Then, in a clone of
`abedegno/forgiv-ci-assets`, remove the old parts, copy in the new ones and commit them. To check
a bundle before pushing it:

```sh
FORGIV_ASSETS_AGE_KEY="$(security find-generic-password -s forgiv-ci-assets-age -w)" \
  sh tools/ci/ci-assets.sh ~/forgiv-ci-assets-staging /tmp/forgiv-assets && rm -rf /tmp/forgiv-assets
```

## Rotation

- **A new bundle** (a toolchain changed, or the licence renewed): make it as above and commit
  the parts in place of the old ones.
- **A new age key**: `age-keygen -o new.key`; store its `AGE-SECRET-KEY-1...` line with
  `security add-generic-password -U -s forgiv-ci-assets-age -a "$USER" -w "$(grep AGE-SECRET-KEY new.key)"`;
  write the recipient with `age-keygen -y new.key > ~/.forgiv-ci-age.pub`; make and push the
  bundle again; set the secret with
  `security find-generic-password -s forgiv-ci-assets-age -w | gh secret set FORGIV_ASSETS_AGE_KEY -R abedegno/forgiv`;
  delete `new.key`. Until the new bundle is pushed, `gate.yml` fails.
- **A new deploy key**: `ssh-keygen -t ed25519 -N '' -C 'forgiv CI (read-only)' -f deploy`;
  `gh repo deploy-key add deploy.pub -R abedegno/forgiv-ci-assets -t 'forgiv CI (read-only)'`
  (read-only unless `-w` is given); `gh secret set FORGIV_ASSETS_DEPLOY_KEY -R abedegno/forgiv < deploy`;
  remove the old key (`gh repo deploy-key list`, `gh repo deploy-key delete ID`); delete
  `deploy` and `deploy.pub`.

Rotate both keys and make a new bundle on any suspicion of a leak, and when a collaborator with
access to the secrets leaves.

## The scripts

| Script | Where it runs | Use |
|---|---|---|
| `tools/ci/make-bundle.sh` | the owner's Mac | make the encrypted bundle parts |
| `tools/ci/install-linux.sh` | CI | Wine (with 32-bit), age, zstd, git, ssh, Python |
| `tools/ci/install-c2000-native.sh` | CI | TI CGT 18.12.8 for Linux, checked and cached |
| `.github/actions/forgiv-assets` | CI | clone the storage repository and run `ci-assets.sh` |
| `tools/ci/ci-assets.sh` | CI | join, decrypt, unpack and check the bundle |
| `tools/ci/setup-toolchains.sh` | CI | the licensed prefix, the D316 master prefix, the StdPeriph link and each tree's `toolchain.local.toml` |
| `tools/ci/failtest.py` | CI, or anywhere | one fail test |
| `tools/ci/cleanup.sh` | CI | the last step of each job |
