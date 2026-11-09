P4.22 live integration: the normal Linux/macOS/Windows builds now include this
loader. Release packages contain this SDK, all three example sources, the build
script and their own `p421-target.txt` under `modding/` (macOS: Contents/Resources).
No compiled examples or enabled selections are packaged.

Default install folders:
- Linux: `$XDG_CONFIG_HOME/saturn-recomp/mods`, otherwise `~/.config/saturn-recomp/mods`.
- macOS: `~/Library/Application Support/Die Hard Arcade/mods`.
- Windows: `%APPDATA%/Die Hard Arcade/mods`.

`--mods-dir DIR` overrides this explicitly; custom settings paths do not relocate
mods. Use the Mods tab to open the exact folder. Build, for example:

```sh
./build_mod.sh examples/frame_hud /path/to/mods/frame-hud p421-target.txt
```

The script runs on Linux, macOS and an MSYS2/MinGW shell. Build on the app's
platform/architecture. Enabling/disabling mods invalidates exchanged connection
codes; generate and exchange fresh codes after choosing the mod set. The complete
native file set must match for netplay. Frame HUD text appears bottom-left above
the MODDED badge, clear of the game's health bar.

The `qualified-base.patch` and `.json` files in the source checkout pin the
reviewed 216-adapter deployment used by `port/p422_prepare.py`. They are build
inputs, not mod-installation files, and are omitted from release packages.

# Readable C mods, prototype ABI 1

A mod is a flat folder containing `manifest.ini` and a native library. ZIPs use
the same files at the archive root (stored or deflated). Install a folder in the
Mods tab's data directory, or drop its ZIP onto the launcher. New installs are
disabled. Tick a mod before Start; selections persist in `mods/enabled.txt`.
`--mods-dir DIR --mods none` forces stock; `--mods id1,id2` overrides saved flags.
`--mods-list`, `--mods-check` and `--mods-install ZIP` support headless tooling.
Mid-session changes are locked. No disabled mod code is executed.

Copy an example and edit its ordinary C. From the repository root:

```sh
port/mods/build_mod.sh port/mods/examples/identity work/p421/data/mods/identity work/p421/build/p421-target.txt
work/p421/build/saturn_stv --cart work/p421/snapshot/diehard_main.bin --cartrom work/p421/snapshot/cart.bin --mods-dir work/p421/data/mods --mods identity --vblanks 2000 --shots 1900 --out work/p421/mod-play
```

The build script replaces `@TARGET@` and `@LIBRARY@` in `manifest.in`. The output
`manifest.ini` must have all eight fields, each once:

```ini
id=identity
name=Identical fixed-point multiply
version=1.0.0
author=Your name
description=What this mod does.
target=the exact 64-digit fingerprint from p421-target.txt
library=mod.so
hooks=replace:fixed_mul16
```

The target covers compiler/ABI, runtime, generated program, readable C and
convergence contracts. Rebuild mods after rebuilding the port from changed
sources. Never substitute `*` for the target. Exact manifest bytes and library
bytes have a SHA-256 digest; changing either changes the mod set. Sorted IDs
make discovery/selection order irrelevant. Two replacements for one entry,
even using different aliases, are refused; overlapping declarative patches are
refused. Callback writes are explicit native code and cannot be statically
checked for conflicts.

`sdk/dh_mod.h` is the versioned C interface. Export `dh_mod_query`, returning a
static `dh_mod` and static `dh_hook[]`; the hook declarations must exactly match
the manifest. Callback arguments and results are fixed-width words and guest
addresses. `sdk/dh_hooks.h` generates semantic function constants and actor
layout offsets from the frozen symbol map/readable headers. `sdk/hookable.json`
contains all 216 qualification identities, aliases, hashes, argument metadata,
adapter classes and contract references. Names and cart/runtime hex addresses
are accepted. An address does not grant access to an unqualified function.

| Hook | Manifest spelling | Execution contract |
|---|---|---|
| Replacement | `replace:fixed_mul16` | Runs inside the original qualified convergence adapter. Args are immutable. The multiply contract permits a changed return only; its original MACH/MACL/r1 recipe survives. Its memory writes are refused. Other nonpoll replacements must preserve results and ordered effects; direct adapters audit full context/effects and transactional adapters retain their residue checks. |
| Pre | `pre:NAME` | Before the readable C body, observing invocation inputs. Guest writes are refused. |
| Post | `post:NAME` | After the complete adapter commits. Observes C result and committed guest memory; may patch guest RAM without modifying the adapter's register result. |
| Frame | `frame` | Once per VBlank-IN before composition/input handoff, including hidden rollback fields; may read/write guest RAM. |
| Startup data | `data:start` | A `DH_DATA_START` hook writes its address/width/value once before netplay/guest execution. |
| Frame data | `data:frame` | A `DH_DATA_FRAME` hook writes its address/width/value at the same frame boundary. |

For example, `dh_hook {DH_DATA_START,0,0,0x06072660+43,1,0}` followed by a
`DH_DATA_START` write of `28` to `0x06072660+44` selects the original scene-28
branch table, like the documented P4.13 startup selector. Its manifest lists
`hooks=data:start,data:start`. Do not patch running scene pointers to simulate
this. Declarative data hooks are additionally exercised by the loader tests.

The 21 direct + 187 transactional entries accept hooks. Eight qualified
resumable C functions remain installed in the game, but are catalogue-only for
this native ABI: arbitrary callbacks cannot replace their serialized poll
continuations. MMIO/device/BIOS entry floors and unqualified interior labels
are rejected. ABI 1 does not expose terminal-call forwarding or synthetic
register-output channels; entries needing those cannot be fully reimplemented
with this header. Use the catalogue metadata and the original generated adapter
as the contract, rather than assuming every entry is an ordinary C ABI.

The memory API returns success/failure. Widths are 1, 2 or 4. Values are
big-endian guest integers, not host byte order. Reads cover the two 1 MiB work
RAM banks and immutable cartridge `02000000..033fffff`; writes cover work RAM
only. Whole spans must fit without overflow. Hardware, VRAM, ROM writes and
unsupported addresses are refused. Cache aliases normalize to the same bytes.
Replacement stores remain inside the convergence transaction and cannot change
private spills, caller-owned spans, store ordering or register contracts.

Use `api->state[0..255]` for mutable gameplay state. These bytes are serialized
with the machine for saves, rewind and rollback. Use guest frame/input/RNG for
determinism; never use wall clock, external files, host randomness or mutable
C statics for gameplay. Overlay text is one bounded line per mod, regenerated
after a restore; it is cosmetic host state. Callbacks cannot suspend, retain
API/call pointers, throw across the ABI or call runtime internals. This ABI is
not a general coroutine compiler.

All enabled native mods mark the UI/title **MODDED**, including the identity
example. Native libraries are trusted code with the application's host access;
the bounded guest API is not an OS sandbox. An arbitrary module cannot be
certified cosmetic from its manifest. Save states contain the full sorted
ID/version/SHA-256 list and reject a changed set before restoring machine parts.
Netplay includes the complete mod-set SHA-256 in HELLO and the existing source
fingerprint; mismatches stop before gameplay. Matching deterministic
mods may play together. Different platform libraries/compiler targets have
different hashes; cross-platform native-mod matching is not claimed.

Examples:

- `identity`: identical signed fixed-point multiplication through the full original register recipe.
- `infinite_health`: RAM health restoration after qualified `player_refresh_ready_mask` and once per field, using joined-state, task, actor, slot and bounds checks. Counters in SDK state demonstrate both hook paths; built-in cheats stay off.
- `frame_hud`: field/scene overlay and a callback counter in serialized SDK state.

Linux uses the host `cc` and `dlopen`; no new shared dependency beyond libc/dl.
macOS uses `CC=clang`, `-dynamiclib` and `mod.dylib`; match arm64/x86_64 to the
app. Windows uses MSYS2/MinGW `CC=clang` or `gcc`, `-shared`, `mod.dll`; with MSVC,
`cl /LD /I port\mods\sdk mod.c /Fe:mod.dll` exports the same C query function.
Generate the manifest with that host build's fingerprint and library filename.
Do not ship foreign object formats or a host pointer in a guest field. The
loader uses UTF-8 filesystem conversion and `LoadLibraryExW` on Windows.

The complete design, screenshots, measured tests and PM patch instructions are
in `notes/port/p421_mods.md`.
