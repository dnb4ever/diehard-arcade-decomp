# P4.21 readable C mod loader prototype

Design recorded before implementation, 2026-11-09. Work is isolated in a single
copy under `work/p421/`; live port, runtime and functional C inputs are owned by
other jobs. No downloads or network access are part of this work.

## Format and execution contract

A mod is a folder with `manifest.ini` and a native C shared library, optionally
transported as a ZIP. Required metadata: id, name, version, author, description,
target build fingerprint, library, and hooks. IDs determine stable sorted load
order. The SDK is a versioned C ABI: fixed-width integers, guest addresses rather
than host pointers, bounds-checked big-endian memory operations, function
arguments/results and a frame callback. Native modules are trusted local code,
not a sandbox; disabled mods must never be dlopened. Linux uses dlopen, macOS
uses dlopen for dylibs, Windows uses LoadLibrary for DLLs. Libraries must match
the host architecture. All validation happens before starting gameplay.

Hook kinds: replace a named/addressed qualified C entry through its convergence
adapter; pre/post observations around a qualified entry; declarative RAM/table
patches at startup or the deterministic frame boundary; and a per-frame callback
with a safe RAM API plus host overlay text. Cartridge writes require a private
shadow and invalidation contract, so the prototype will reject ROM writes.
Read-only cartridge access does not authorize device/MMIO access.

The qualified convergence catalogue has 216 entries (21 direct, eight resumable,
187 transactional). The generated SDK catalogue records semantic source name,
cart address, runtime address, adapter kind and its contract reference. Only
contracts actually supported by the loader may be replaced: transactional
entries retain ordered effects and original register residue; direct entries
retain their reviewed scratch/T/MAC recipes; resumable entries retain the
original poll IDs and serialized live state. General replacement of arbitrary
C control flow, MMIO, bus/device floors, BIOS calls, escaping stack objects and
unqualified interior aliases is forbidden. Unsupported replacement requests
must be rejected rather than silently bypassing the adapter. Hook names resolve
to callable source entries, not pool labels or duplicate drafts.

Mods load only when explicitly enabled before boot. Every active native mod is
conservatively marked MODDED, including an identity demonstration, since a
manifest cannot prove arbitrary native code is harmless. The sorted complete
mod set hashes metadata and library contents, joins the netplay fingerprint,
and is recorded in save-state parts. Different sets reject during the existing
handshake before gameplay. Matching sets may play together provided mods
use only deterministic guest inputs and serialized state. Native wall clocks,
randomness, I/O and mutable static gameplay state are outside that guarantee.
The SDK offers no host pointers to guest memory. Mid-session toggling is locked.

## User and modder workflow

The launcher gains a Mods tab with discovery, enable flags, metadata, status and
an Open mods folder action. The data directory contains `mods/`. CLI overrides
support reproducible headless tests. Installation accepts folders; ZIP drag/drop
will be implemented if it can reuse bounded in-process archive support safely.
Archive traversal, symlinks, duplicate IDs, conflicts and target/ABI mismatches
must fail before code runs. A host compiler builds each example against
`port/mods/sdk/dh_mod.h`; platform build notes accompany the script.

Planned examples: joined-player infinite health using validated actor pointers
and the documented actor health offset (same evidence as P4.13); a frame/scene
HUD using the existing host overlay; and an identical fixed-point multiplication
replacement through the qualified `fixmul_16_16` direct recipe.

## Acceptance plan

Build Linux in the snapshot. Compare disabled and identity runs to stock PNGs,
deterministic traces and WAVs, including the 1900-field MD5
`9bd5386ea7439621c19674381086e8fd` and no-coin attract. Exercise invalid manifests,
disabled library side effects, memory bounds, conflicts, enabled set hashes,
save-state rejection and two-process mismatched netplay over the file-backed
transport. Run runtime tests. Capture and inspect the gameplay and launcher
screenshots. Report measured results and remaining limits without inferring
macOS/Windows execution from portable source alone.

Implementation, measured verification, screenshots and PM integration steps
will be appended below as work completes.

## Delivered implementation

The pre-code design above is preserved. The final implementation is in
`work/p421/runtime`, `work/p421/generated`, and `work/p421/port`. Shared additions
are only `port/mods/`, new `port/p421_*` helpers, three new P4.21 patches, and this
note. **No existing live port/runtime/decomp source was edited by this job.**
P4.19/P4.20 changes appearing in the working tree remain their owners' work.

The single independent snapshot ran **18:16:12.593197–18:16:13.539906 UTC on
2026-11-09**, starting from HEAD `8852d7fb91c1fd9c13215e90bc6a341042db1e58`.
`work/p421/snapshot.json` pins **2,542 files**; the final audit finds **zero
changes**. This is a copy interval during concurrent work, not an atomic Git
revision. It includes the runtime/main state present at that interval, including
the P4.19 work already present then, and the independent qualified round-4
convergence output. All **216 current readable source hashes match that
catalogue's qualification hashes**. The copied stock-generated program is used
for a fresh stock build; the prototype links the copied 216-entry convergence
program. No generation, qualification or patching edits shared `project/` or
`tools/saturn-recomp/`.

Final Linux executable SHA-256:
`8631de5e779d0263fcc046edd6f5dbd8ebb4f12435084544f5abfb36a3af4951`.
Exact mod target:
`26fa88600ed1243b1d20763c7f9470ba8fdafd96896cdf292122b4f73a7b4205`.
Reconfiguring an independently reconstructed integration produces the **same
mod target**. Compiler/ABI/source changes intentionally require rebuilding mods.

The runtime adds discovery/validation, dlopen/LoadLibraryExW, stable hook order,
RAM/table patches, deterministic frame callbacks, bounded overlay text, complete
mod-set hashing and a `mods` snapshot part. The convergence bridge uses a small registered core interface so existing
standalone stub/probe targets remain loader-independent. It dispatches
the replacement inside its existing transaction/recipe and invokes post-hooks
**after committing the complete original adapter**. Pre-hooks observe invocation
inputs; their RAM writes are refused. Post-hooks may patch committed RAM;
replacement writes must obey their adapter's transaction. This gives data mods
a real function boundary without weakening the original register contract.

The Mods tab lists metadata, availability/errors and enable toggles, persists
selection in `enabled.txt`, opens the mods folder and accepts a ZIP dropped onto
the launcher. The default directory is beside the settings file when available,
otherwise `<platform data>/saturn-recomp/mods`. Thus existing app-specific
Application Support/AppData settings paths retain their own `mods` folder.
Enables/installations are locked after gameplay starts. Command line tools:
`--mods-dir`, `--mods none|ID,...`, `--mods-list`, `--mods-check`, and
`--mods-install ZIP`. Discovery reads metadata/library hashes but never dlopens
a disabled library. A test constructor proves this distinction.

## Writing, building and installing a C mod

The modder reference is [port/mods/README.md](../../port/mods/README.md), and the
public header is [dh_mod.h](../../port/mods/sdk/dh_mod.h). A complete example:

```c
#include "dh_mod.h"
static uint32_t fixed_mul16(const dh_api *api, const dh_call *c) {
    (void)api;
    uint64_t p = (uint64_t)((int64_t)(int32_t)c->args[0] *
                           (int32_t)c->args[1]);
    return (uint32_t)(p >> 16);
}
static const dh_hook hooks[] = {
    {DH_REPLACE, "fixed_mul16", fixed_mul16, 0, 0, 0}
};
static const dh_mod mod = {DH_MOD_ABI, sizeof(dh_mod), "identity", 1, hooks};
DH_EXPORT const dh_mod *dh_mod_query(void) { return &mod; }
```

Copy one of the examples, change the unique lower-case ID, metadata and hooks
in `manifest.in`, then build it with the exact target file from your app build:

```sh
port/mods/build_mod.sh port/mods/examples/identity \
  work/p421/data/mods/identity work/p421/build/p421-target.txt
work/p421/build/saturn_stv \
  --cart work/p421/snapshot/diehard_main.bin \
  --cartrom work/p421/snapshot/cart.bin \
  --mods-dir work/p421/data/mods --mods identity --vblanks 2000 \
  --shots 1900 --out work/p421/try-identity
```

The script uses the host compiler, C11 and no runtime imports. Linux builds
`mod.so`; macOS uses `-dynamiclib` and `mod.dylib`; an MSYS2/MinGW shell builds
`mod.dll`. `CC` selects a compiler. For MSVC use
`cl /LD /I port\mods\sdk mod.c /Fe:mod.dll`, then substitute the fingerprint
and filename into the manifest. Match the app's CPU architecture/compiler
runtime. No interpreter or compiler is required when playing.

The final manifest has exactly `id`, `name`, `version`, `author`, `description`,
`target`, `library`, and comma-separated `hooks`. Its hook multiset must match
the exported descriptor exactly. `target` is a full 64-hex fingerprint with no
wildcard. A mod hash is **SHA-256(manifest bytes || NUL || library bytes)**;
the set hash is SHA-256 of sorted `ID@VERSION:MOD_HASH\n` records. Changing
metadata or native bytes therefore changes compatibility. ID order in the CLI
or filesystem does not change the set. Conflicting replacements and overlapping
declarative writes are rejected. Arbitrary callback writes cannot be statically
checked for conflicts.

Put the resulting folder in the displayed mods directory, or make a flat ZIP
with `manifest.ini` and the library at its root and drop it onto the launcher.
Stored/deflate archives are bounded to 32 MiB input/uncompressed total and 64
entries. The installer verifies CRCs, directory/local bounds, names and target;
it rejects traversal, absolute paths, symlinks, duplicate/case-colliding names,
unsupported flags, truncation and existing destinations. Files stage under the
mods directory and rename into place only after full validation. Installations
start disabled. Updating requires removing the old folder while the game is
closed; automatic update/uninstall is outside this prototype.

## Hookable set and exact contracts

[hookable.json](../../port/mods/sdk/hookable.json) is the complete generated
216-entry catalogue with names, cart/runtime addresses, aliases, source/body
hashes, argument metadata, adapter kind and capability flags. Its generated
[dh_hooks.h](../../port/mods/sdk/dh_hooks.h) supplies readable function constants
and actor field offsets (`slot_id +34`, `health +56`, position fields and actor
size `18c`). Guest pointers remain 32-bit addresses; these constants do not
invite casting guest memory to a native pointer-bearing structure.

| Cohort | Installed game code | Native hook contract |
|---|---:|---|
| Reviewed direct recipes | 21 | Pre/post and replacement dispatch. Original scratch/T/MAC recipes remain authoritative. All replacements except the reviewed multiply result contract force full original context/effect auditing. |
| Transactional residue | 187 | Pre/post and replacement dispatch. Result, ordered public stores, private stack, caller spans and supported tail targets/arguments retain original checks. Changing a transaction's effect shape is refused. |
| Resumable C | 8 | Installed unchanged; catalogue-only in native ABI 1. Replacement/pre/post requests are rejected rather than losing poll/continuation contracts. |

The eight catalogue-only functions are `fixed_sqrt16` (`02213164`),
`actor_allocate_effect_slot` (`02228106`), `frame_measure_pulse_timing`
(`02223c74`), the two radix entries (`02214e2c/2e`),
`matrix_translate_current` (`02213ca4`), `matrix_transform_translation`
(`02213b5c`), and `copy_longs` (`0221326c`). Their original checkpoint IDs,
budget charges, serialized live state and MAC restrictions stay intact.

For the identity example the exact public interface is cart `022131c8`, runtime
`060171c8`, current readable name `fixed_mul16`; historical `fixmul_16_16` is
also accepted. Arguments are r4/r5, result is r0. The original signed 32×32
product supplies complete MACH/MACL and r1=MACH; r0 receives bits 16..47;
PC becomes entry PR, continuation entry clears, and other registers, flags,
stack, timing budget and RAM stay unchanged. ABI 1 permits changing this result
but refuses memory writes from this pure replacement. The identity example
returns the identical result and runs with full original-body auditing in tests.

Other recipe details remain in the retained generated adapter and the existing
[round-4 recipes](../../port/converge/round4_recipes.json),
[contracts](../../port/converge/round4_contracts.json), and
[compute contracts](../../port/converge/round4_contracts_compute.json). The
header intentionally omits terminal-call forwarding, pseudo register-output
channels and general continuation APIs; a replacement requiring those cannot
be fully expressed through ABI 1. The broader catalogue is qualification and
contract evidence, not blanket permission for arbitrary C algorithms.

Device/bus floors stay excluded: collision readers `02244874/d0/492c`, hardware
ISRs, DIVU/DMAC/SCU, cache-purge/projection and upload/device wait paths,
unqualified interior aliases and unsupported nonterminal/inlined helpers.
Names/addresses resolving outside the installed nonpoll set reject at load.
A readable source name does not establish its machine ABI or device safety.

The memory API accepts 1/2/4-byte complete spans, canonicalizes cache aliases,
and returns big-endian guest integer values. Reads cover the two work RAM banks
and immutable cartridge `02000000..033fffff`; writes cover RAM only. MMIO, VRAM,
ROM writes, unsupported widths and overflow/cross-bank spans fail. Replacement
memory is routed through the original transaction. SDK mutable state is
256 bytes per mod and participates in saves/rewind/rollback. Overlay strings
are bounded cosmetic host data and regenerate after a restore. Callbacks may
not suspend, retain callback pointers, throw across C, or use host clocks,
randomness/I/O/static variables to determine gameplay.

## Session safety and determinism

Every enabled native library conservatively marks the session **MODDED** in
the existing host HUD/title. This includes the proven identity example:
metadata cannot certify arbitrary native code as cosmetic. Disabled mods have
no loaded code. Native libraries are trusted local code with process privileges;
the safe guest API is not an operating-system sandbox.

Save identity mixes the set digest into the existing build/ROM identity. The
new `mods` machine part also records the complete sorted ID/version/hash text
and each mod's 256 SDK bytes. Wrong sets or changed library hashes reject
before the machine parts load. P4.13 input-file build metadata uses the same
identity; its embedded states include the mods part. Old save/input files from
other exact builds are intentionally incompatible.

Netplay fingerprints the convergence C/recipes as well as runtime/generated
code, then mixes the mod-set hash into its compatibility fingerprint and carries
all **64 SHA-256 hex characters** in the extended HELLO (124 bytes including
CRC). Host checks it during the initial compatibility handshake. Both peers reject a mismatch with
`mod set/hash mismatch`; no game field executes. Equal sets with deterministic
callbacks may play. Native libraries differ between OS/architectures, and exact
target fingerprints can differ with compiler/ABI; cross-platform native mod
interoperability is not claimed. Old pre-P4.21 handshakes are incompatible.

`mods_frame` runs once at VBlank-IN, before composition and later panel handoff,
including hidden rollback fields. SDK state restores with the machine. Function
hooks have no new guest poll, timing budget shortcut or escaping C stack.
Netplay state stays outside the machine as before. The implementation cannot
prove a third-party native library obeys these determinism rules.

## Examples and inspected screenshots

The three source examples are under [port/mods/examples](../../port/mods/examples).
All screenshots below were captured through CPU-only SDL dummy/software output
and **opened and visually inspected**, including the final build after adding
committed post-hooks. The game screenshots in the regression tests exclude
host overlays; a MODDED badge intentionally makes an enabled identity session's
host UI differ while its guest pixels/trace/PCM remain identical.

**Infinite health:** [C source](../../port/mods/examples/infinite_health/mod.c)
patches health only for joined players, checking task/actor bounds and slot IDs.
It executes after qualified `player_refresh_ready_mask` and at each frame
boundary. The built-in training flags remain off. At saved field 1500, SDK
counters prove **572 actual qualified post-hook calls and 1500 frame calls**.
A separate read-only native observer samples **1412 joined-player fields**:
stock HP ranges 78–114; enabled HP is **114 in every sample**. At field 1900,
stock is 78 and mod is 114. Raw game images differ in **492 pixels**, all inside
the health bar rectangle `[240,18)..[336,24)` of the 704×240 image. The inspected
window captures show the expected longer green bar and MODDED badge.

![Stock health at field 1900](../../port/mods/evidence/stock-1900.png)

![Infinite health after a qualified function and frame callback](../../port/mods/evidence/health-1900.png)

**Frame HUD:** [C source](../../port/mods/examples/frame_hud/mod.c) draws the
existing overlay API's field, scene and a counter stored in SDK state. At field
1900 it reads `FRAME 1900 | SCENE 3 | HUD TICKS 1900`. Restoring a field-1500
save and advancing to 1800 displays `HUD TICKS 1800`, with identical guest PNG.

![Readable C frame and scene HUD](../../port/mods/evidence/hud-1900.png)

![SDK counter restored with the saved session](../../port/mods/evidence/restored-hud.png)

**Identity replacement:** [C source](../../port/mods/examples/identity/mod.c)
is the pure fixed-point multiply shown above. It dynamically replaces the
qualified C entry and preserves the complete original register contract.
20,000 random/edge vectors across both CPUs compare every SH2Context byte and
both complete RAM banks. Standard guest screenshots, traces and WAVs match
stock with the library loaded and original-body auditing enabled.

**Launcher/install:** actual SDL mouse events enable the first mod and persist
`frame-hud` to the selection file. An Open-folder click emits a correctly encoded
local file URI, including a space and `ü` (the test intercepts launching the OS
file manager). A real `SDL_EVENT_DROP_FILE` installs a deflated test ZIP disabled.
The resulting UI was inspected:

![Mods tab after enable toggle and a dropped ZIP](../../port/mods/evidence/launcher-drop.png)

## Verification results and reproduction

Portable machine-readable evidence is in [port/mods/evidence](../../port/mods/evidence).
Full logs/runs remain under `work/p421/`; earlier exploratory runs are preserved.
An integration check also found loader-only references in standalone probe
links; the registered core interface fixes this, and all six original probes
were rebuilt and rerun successfully. Mutation checks prove changing the
convergence bridge changes both mod target and netplay code; compiler flags
and host-only UI edits change the exact mod target while preserving the portable
netplay code. The reconstructed stage restores exactly (`fingerprint-proof.json`).

The first matching-set test had a harness assertion comparing CLI strings
rather than sets; both game processes had succeeded. The final test canonicalizes
selection order and passes. No runtime failure was hidden by that correction.

| Verification | Final result |
|---|---|
| Linux SDL build | `work/p421/build/saturn_stv`, successful; `build-delivery.log` |
| Runtime suite | **169 tests, 0 failures**; `tests-delivery.log` |
| Standalone convergence probes | All **six** original ABI/poll/pulse/stack/compute probes build and pass with the stub runtime; `convergence-probes.json` |
| Disabled mods vs P4.11 reference | **56/56 PNGs**, four full and guest traces, four complete WAVs identical |
| Loaded identity vs same reference | **56/56 PNGs**, four full and guest traces, four complete WAVs identical; direct original-body audit enabled |
| Smoke stock/off/identity | All three shot-1900 MD5s **9bd5386ea7439621c19674381086e8fd**; traces and complete WAVs equal |
| Dynamic identity boundary proof | **20,000** complete-context vectors, both CPUs, signed/wrap edges and unchanged RAM; `abi-delivery.log` |
| Loader/SDK/ZIP negatives | **32 cases pass**, including disabled constructors, exact SHA, wrong target/ABI, unknown/poll/device entries, alias conflicts, overlapping patches, bounds/endian/cache alias/ROM-read checks, pre/replace/post order, ZIP traversal/symlink/CRC/truncation |
| Actual SDL UI | Checkbox persists, encoded folder URI, dropped deflate ZIP installs disabled; `ui-delivery.log` |
| Behaviour | **1412/1412** health samples held at 114; 572 qualified post-hook calls proven at VB1500; inspected health/HUD PNGs |
| Saves | Full set recorded, SDK counter=1500 saved, cold restore reaches identical VB1800 PNG/counter; wrong set and mutated DLL bytes rejected |
| Netplay mismatch | Different sets and same-ID mutated native bytes each rejected by both processes, exit 2, before gameplay |
| Matching mod sets, 2100 fields | **36 confirmed checksums**, final PNG/WAV/complete machine payload identical; P1/P2 joined; selection order differs deliberately |
| Matching mods, 30 ms latency + 1% loss + 10 ms jitter | **21 confirmed checksums** through 1200 fields, identical PNG/WAV/machine payload, 6/6 rollbacks and maximum depth 8; no desync |
| Windows portability | Existing LLVM-MinGW compiles loader object, launcher object and example DLLs; no new platform dependency |
| Patch reconstruction | **17 changed/new files** reconstructed byte-for-byte with **zero fuzz/offset**; staged configure regenerates identical target |
| Snapshot audit | **2542/2542** immutable inputs unchanged |

Netplay uses the existing **test-only file-backed datagram transport**. No kernel
socket, Internet or live UDP result is claimed. The runtime binary does not link
the shim. Timing/rollback counts reflect a loaded shared host and are not a
performance acceptance result.

Rebuild and repeat from repository root, using existing local dependencies:

```sh
bash port/p421_build.sh
python3 port/p421_verify.py
python3 port/p421_verify_loader.py
python3 port/p421_verify_ui.py
python3 port/p421_probe_build.py
python3 port/p421_verify_sessions.py
python3 port/p421_verify_health.py
```

`p421_verify.py` pins RTC with the copied test clock input, runs the fresh stock
smoke, then off/identity through no-coin attract (9000 fields), game-over/second
game (9000), combat (2100) and legacy combat (2100). It checks all expected
indices and compares against `work/p411/regression/after/manifest.json`, with
no trace/pixel masking. Raw example captures use autoplay. The regression
reference and ROMs are owner-local inputs, not shipped in the deliverable.
The verification scripts use `work/p421/`; preserve evidence before rerunning.

## PM integration steps

The patches are **incremental against this job's retained snapshot**, not the
future final P4.19/P4.20 shared tree:

1. [p421-mod-runtime.patch](../../port/runtime-patches/p421-mod-runtime.patch)
   contains `runtime/...` paths, including new loader/codec/catalogue/SDK headers,
   frame/state/netplay wiring and launcher/overlay changes.
2. [p421-mod-convergence.patch](../../port/runtime-patches/p421-mod-convergence.patch)
   contains `generated/CMakeLists.txt` and `generated/bridge.cpp` changes against
   the **qualified round-4 generated output**, preserving all 216 adapters.
3. [p421-mod-harness.patch](../../port/runtime-patches/p421-mod-harness.patch)
   contains only `port/main_stv.cpp` changes against the captured harness.

First produce a reviewable reconstruction with independent copies:

```sh
python3 port/p421_stage.py --destination work/p421/pm-review
P421_WORK=work/p421/pm-review bash port/p421_build.sh
```

The staging helper copies `work/p421/snapshot/{runtime,port,converge-generated}`
to `{runtime,port,generated}`, applies the three patches at that common root
with zero fuzz/offset, copies the new SDK/examples/helpers, and checks all 17
result hashes. It refuses an existing destination. `p421_export.py` documents
export/reconstruction; `work/p421/patch-report.json` records every hash.
The verified `work/p421/patch-review/` and `stage-check/` are already available.

Then reconcile changes into the final integration tree deliberately: P4.19 and
P4.20 also own harness/runtime/frame/UI code. Keep their netplay/audio and
presentation updates; merge the HELLO/hash/state-part and VBlank callback
changes at the same deterministic points. Do not apply these patches directly
to shared `tools/saturn-recomp` or assume future offsets establish compatibility.
Use the full qualified generated output as the recompile source, not a lone
wrapper patch against the stock-generated tree. Preserve the convergence
CMake's complete C input hashing and P4.21's restored `STV_NETPLAY_CODE` define.

The existing default stock build stays opt-in with respect to convergence.
For a shipping mod-capable target, wire the qualified generated build and its
fingerprint into the owner's normal build/package workflow. Package the SDK,
example sources and exact `p421-target.txt`; leave ROMs, compiled enabled mods,
local selections and saves out of releases. Rebuild native example libraries
for the final merged target and repeat reference/netplay/save/UI tests there.
Actual macOS/Windows launch, architecture matching and file-manager opening
remain PM checks. No commit or publish action was performed.

## Open issues and limits

- Eight resumable functions are deliberately catalogue-only for native hooks;
  ABI 1 cannot replace their checkpoints or wrap suspended calls safely.
- Replacement is constrained by the existing residue/recipe contracts. Only
  the reviewed pure multiply return may intentionally differ; other replacements
  require original context/effects. SDK tail forwarding, custom register output,
  and richer per-function changed-effect contracts remain future work.
- Post/frame RAM patches provide behaviour mods now; arbitrary `actor_apply_damage`
  replacement is excluded because that function is not in the qualified set.
- ROM-table writes/shadowing and VRAM/MMIO hooks are unsupported and rejected.
  Patch the proven RAM copy of a table at a defined boundary instead.
- Native code is trusted and can bypass the SDK. No signing, dependency resolver,
  malicious-code sandbox, hot reload, automatic updater or crash isolation.
- Native deterministic behaviour is a modder obligation. Matching hashes cannot
  prove callbacks avoid nondeterministic host services; static gameplay state
  outside the SDK is not restored.
- Native binary hashes/exact targets prevent transparent cross-platform mod
  matching. Native macOS execution and Windows execution were not run here;
  Windows object/DLL compilation is narrower evidence than an app launch.
- UI interactions used software SDL. An actual desktop file manager, physical
  drag/drop, pads/audio and mixed-DPI systems remain platform checks. The
  launcher drop handler is intentionally active before gameplay only.
- This prototype packages ZIPs flat at the root, has 32 enabled-mod/64-hook
  limits and a fixed 256-byte serialized SDK state per mod. Callback overlaps
  cannot be statically rejected; ordering is stable by ID then descriptor order.
- General full-game ending/save/input-movie compatibility of every third-party
  mod is not established by the short acceptance runs. Existing convergence
  full-game proofs remain evidence for the unchanged adapters, not arbitrary mods.

1. Froze 2542 independent inputs at HEAD 8852d7f; no live owned source edits or commit.
2. Delivered an opt-in native C loader, generated SDK/catalogue and three source examples.
3. Preserved 216 convergence adapters; 208 nonpoll entries accept hooks, eight remain catalogue-only.
4. Added function replacement, observing pre-hooks, committed post-hooks, RAM data patches and frame callbacks.
5. Added the Mods tab, saved enables, metadata, folder action and validated disabled ZIP installation.
6. Both mods-off and identity match all 56 reference PNGs/four traces/four WAVs and the required smoke MD5.
7. Identity passes 20000 full-context vectors; health holds 1412 samples at 114 HP; screenshots inspected.
8. Save sets/counters restore correctly; mismatches reject; matching modded peers retain equal state/PNG/WAV under rollback.
9. All 169 runtime tests, six existing convergence probes and 32 loader cases pass; Windows loader/launcher/DLL compilation passes, native platform runs remain open.
10. Three snapshot-based patches reconstruct 17 files exactly; PM staging/merge steps and remaining ABI/platform limits are documented.

## P4.22 supersedes the snapshot integration instructions

The live tree now integrates these features over final P4.19. The three patch
files have been re-exported against that base, so the historical snapshot
staging commands and hashes above describe the original prototype only. Use
`port/p422_prepare.py`, the normal build/package scripts and
[the P4.22 integration note](p422_mods_integration.md) for current instructions,
default per-platform folders, the corrected HUD placement and release evidence.
The original patch copies remain under `work/p422/original-*.patch` for audit.
