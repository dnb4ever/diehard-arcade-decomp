# Decomp → port convergence pilot

2026-11-08. The mechanism works for a bounded slice: **41 v1.06 PASS functions are
linked through readable-C overrides; all 22,200 frames in the four reference
schedules match frozen stock pixels, full RAM and semantic records. All 56 PNGs
and four deterministic traces match too.** An optional one-function C mod
visibly changes rendering. Default port sources/builds are unchanged.

This is a staged convergence pilot. **Five functions execute directly from C
with small ABI recipes; 36 still execute generated register residue as well as
C.** Their public stores and return values come from C, while the residue keeps
the machine interface exact. It would be misleading to call all 41 independent
of the recompile, or to call this a completed readable-C port.

Everything was built in `work/converge/`. `tools/saturn-recomp`, `port`,
`project/src_functional`, the difftest harness/venv and symbol/floor notes were
copied once before implementation. The cartridge was copied too. No hardlinks
to mutable source were used. The final comparison rechecks every entry of
`work/converge/snapshot.sha256.json`; no frozen-input drift was found. Snapshot
manifest SHA256:
`feb1b22086c134b4b20758086f20a88a1b53dbfcf493b7c3227f518e0e4db838`.

Only new paths under [port/converge](../../port/converge/) and this report were
delivered. No existing `port`, `tools` or `project` source was edited, so no
runtime patch is required. The concurrent P4.11/decomp work was left alone.
No commit, sudo, GPU, prohibited-directory access or process termination was used.
The private tree occupies approximately **3.0 GiB**, including the requested
full port snapshot. RAM/video streams were hashed through FIFOs and discarded;
no enormous raw-frame archive was retained.

The build and calling-convention bridge
--------------------------------------

[build.py](../../port/converge/build.py) offers `stock`, `pilot10`, `pilot` and
`mod` variants. [overrides.txt](../../port/converge/overrides.txt) contains cart
addresses; `pilot10.txt` is the starting subset. The generator translates each
address using difftest's canonical mapping, for example `02213190 → 06017190`.
It copies the frozen generated recompile into a private output directory,
replaces the selected **definitions** with forwarding wrappers and adds the C
and glue translation units to that copied CMake project. Direct compiled calls,
module-table lookup and continuation dispatch therefore all reach the same
override. Changing only the module lookup table would miss direct calls.
There are no manual edits to generated code.

The C lowering is the frozen v1.06 difftest's actual `preprocess`, `parse`,
`prepare` and `Generator`, rather than a second memory-macro implementation.
It resolves `dh_names.h`/`dh_work.h` accessors and named globals through the
copied headers and `symbols.txt`. It retains guest pointers as **32-bit guest
addresses**, computes pointer strides from guest types and lowers dereferences
and stores to `dt_read`/`dt_write`. The generated C is compiled as C with
`-fwrapv -fno-strict-aliasing`. This pilot was tested with GCC on Linux.

The difftest executable normally supplies separate high/low RAM arrays,
big-endian accessors, hashed callee stubs and a synthetic private C frame. Here:

- Entry arguments come from r4–r7, then call-time guest SP for later words;
  integer/pointer results return through r0. Globals resolve to actual runtime
  addresses. No host pointer is written into guest memory.
- `dt_read`/`dt_write` use per-invocation transactions backed by the existing
  runtime memory, with big-endian byte assembly and physical/cache-through/HWRAM
  mirror normalization. Only work RAM and immutable cartridge reads are allowed;
  speculative hardware reads and hardware stores abort.
- `dt_callv` invokes the **real compiled callee**, with its prepared machine
  state. No hashed stub, Python CPU or SH-2 interpreter runs in the port.
- Fixed-address C stack locals from difftest's synthetic frame are rejected.
  That test fixture cannot safely become a recursive or dual-CPU port stack.

Ordinary C function ABIs do not specify the SH-2's residual r1–r7, T, MACH/MACL,
private spills, PC or continuation fields. Some callers consume those values.
Returning the right r0 alone is insufficient. The bridge handles this in two
explicit stages:

1. **Direct adapters, five functions.** Reviewed, body-hash-pinned recipes in
   [abi_recipes.json](../../port/converge/abi_recipes.json) reconstruct the small
   machine residue around the C result: sine/cosine argument advancement,
   table-base registers and T; multiply MACH/MACL/r1; angle-test scratch; and
   the return-zero stub. They set PC from PR and clear entry continuation state.
   Normal execution does not execute their original instruction bodies.
   `CONVERGE_AUDIT=1` additionally runs the retained reference residue and
   compares every byte of `SH2Context`. The optional sine mod uses the direct
   path without that original-result audit.
2. **Transactional adapters, 36 functions.** Automatically transformed generated
   code runs against a second transaction and a context copy. Its register
   calculations and branches are retained, but its public writes never reach
   guest memory directly. The C runs from the same initial RAM with its own
   write overlay. Returns and chronological public byte-write traces must agree.
   The bridge commits **C's public data**, at the original bus widths and store
   positions, plus the original private spill bytes, and keeps the residue's
   complete register state. The private window is SP−0x1000 through SP+0x100;
   C effects aliasing it abort rather than silently being discarded.

This second stage duplicates computation. It is a conservative way to make
readable C authoritative for observable data without losing undocumented
register behavior; it is also the largest remaining obstacle to removing the
old machine bodies. The five direct recipes demonstrate that the residue can
be removed once its boundary contract is small and verified.

Two selected near-leaves use terminal tail branches. `0221047c` calls the
remaining compiled square-root helper. `022462bc` chooses the actual actor
hierarchy/shadow drawing routine. The transformed residue stops at the tail
dispatch after its delay slot, preserving the prepared argument/scratch/stack
state. C's selected target and arguments must agree before the real `sh2_call`.
Post-tail guest reads/writes and multiple calls are rejected. A scoped active
session restores the adapter context if a runtime control exception unwinds it.

Timing is exact **relative to this stock port's scheduler**. Eligibility
requires zero `SH2_POLL` sites in the selected body. Such a body has no guest
interrupt/slave handoff while its RAM effects are computed. The adapter neither
spends budget nor adds safe points. These original tail edges do not poll;
their compiled callees retain every existing poll, continuation and return check.
The caller's polls are also unchanged. This does not establish hardware cycle
accuracy: the stock runtime advances virtual time at poll-budget boundaries.

Qualification is fail-closed. The checked-in
[qualification.json](../../port/converge/qualification.json) pins the original
C hashes, ROM and difftest. A changed source needs requalification; changed
recipe bodies need ABI review. Failed generation leaves an existing usable
output intact, identical generation preserves source mtimes, and output paths
outside the private work tree are rejected. Those cases were exercised in
[generator-checks.json](../../port/converge/evidence/generator-checks.json).
The generated build also adds the lowered C/glue to the existing quick-state
fingerprint. Normal and mod builds have distinct state identities; no existing
serializer file was edited. Save/load UI and cross-platform builds were not
acceptance tests for this pilot.

Functions and coverage
----------------------

The ten-function starting build passed combat, including **2,100/2,100** frame,
RAM and semantic records, five PNGs and the trace. The expansion initially had
42 entries. At 64 vectors, `02245fc4` stopped at vector 26 with an unmapped table
address `3E5DCD58` in the reference. It had passed the original 16-vector gate
and the 42-function combat run. It was removed rather than weakening its inputs
or treating that SKIP as proof. The two VDP2/DMA tail-copy candidates
`0221654a`/`02216582` were excluded because their live prefix accesses hardware.

Final independent qualification is **41 PASS / 0 PASS-void / 0 FAIL / 0 SKIP**,
64 vectors each, **2,624 vectors** total. The copied v1.06 uses varied stubs and
deterministic initial-pointer retry. Logs and full branch/effect data are in
`work/converge/qualification/`; the certificate retains branch-direction counts.
Passing vectors do not prove every input/path. Selected functions have no
open-floor/lever-pending rows in the copied floor registry.

The table counts calls across the four standard schedules plus the 12,000-VB
autoplay run. `D` means direct C plus an ABI recipe; `R` means C plus generated
register residue. Zero means the function was installed but not reached by
those game schedules. **31/41** were reached, for **693,734 calls**; the four
standard schedules alone account for **402,841 calls**. Counts come from the
actual wrappers, with separate master/slave counters in the raw logs.

| Cart address | Readable function purpose | Bridge | Game calls |
|---|---|:---:|---:|
| `0220c858` | Camera bounds / turn snap | R | 5 |
| `0220cab0` | Camera bounds / position snap | R | 1 |
| `02210020` | PRNG feedback/mix | R | 6,421 |
| `0221047c` | 2D magnitude → compiled square root | R | 0 |
| `02213190` | Sine lookup | D | 214,917 |
| `022131a8` | Cosine lookup | D | 110,227 |
| `022131c8` | 16.16 multiply | D | 84,669 |
| `022131d4` | XZ squared distance | R | 55,559 |
| `022131fc` | XYZ squared distance | R | 46,484 |
| `0221330c` | Bounds with margin | R | 33,120 |
| `02213354` | 3D AABB overlap | R | 65,622 |
| `02215630` | Current-matrix clip test | R | 11,511 |
| `02228134` | Player entity readiness | R | 2 |
| `0222a04e` | Fighter stat / stun arbitration | R | 0 |
| `0222a8d8` | Ready-player mask/count | R | 11,484 |
| `0222c220` | Special-move input gate | R | 7,797 |
| `0222f3ea` | Counter / flags 43800 | R | 601 |
| `0222f40e` | Counter / flags B800 | R | 9 |
| `0222f432` | Counter / flags 3800 | R | 67 |
| `0222f456` | Counter expiration | R | 32 |
| `0223b1c0` | Grab/hold animation state | R | 0 |
| `0223c74a` | Partner counter decrement | R | 22 |
| `0223c7ba` | Conditional partner counter | R | 9 |
| `0223e5bc` | Wrapped angle ordering | D | 0 |
| `02241556` | 60-frame debounce | R | 29 |
| `0224158c` | 90-frame debounce | R | 20 |
| `02243fba` | Two-axis comparison mask | R | 5,473 |
| `02244874` | Actor box overlap → pointer | R | 146 |
| `022448d0` | Actor box overlap → boolean | R | 17 |
| `0224492c` | Point in actor box | R | 2,762 |
| `02245382` | Directional proximity / fifth stack argument | R | 0 |
| `02245a6a` | Facing/side selection | R | 33 |
| `02245aec` | Alternate facing/side selection | R | 0 |
| `02245b48` | Reference target selection | R | 0 |
| `02245bcc` | Facing alignment test | R | 148 |
| `02245dcc` | Pre-attack/cooldown gate | R | 19,067 |
| `02246026` | Elapsed-time threshold | R | 0 |
| `022460c8` | Animation-limit classifier | R | 10 |
| `0224617a` | Return-zero stub | D | 0 |
| `022461d8` | Player-slot object lookup | R | 0 |
| `022462bc` | Actor draw/shadow dispatch → compiled renderer | R | 17,470 |

The five direct adapters also passed **50,000 complete-context checks** across
random register states, both CPU IDs, random RAM table data and signed/overflow
boundaries. The magnitude adapter passed **256 real C→compiled-sqrt calls**,
checking its result, preserved stack/registers, PR/PC and unchanged poll cost.
See [abi-probe.txt](../../port/converge/evidence/abi-probe.txt). These probes
exercise three of the ten schedule-unreached entries; the remaining seven have
difftest evidence only. The draw dispatcher was reached **17,470 times** in the
game schedules. Its large drawing callees remain compiled; this is a rendering
dispatch slice, not a converted HUD or complete renderer.

End-to-end equivalence evidence
------------------------------

The schedules are the P4.5 schedules preserved by `p48_verify.py`: no-coin
attract, paid game over → attract → second game, combat and legacy multi-cart
combat. [verify.py](../../port/converge/verify.py) reuses the existing read-only
scenario FIFO to hash every composed BGRX frame, both complete 1 MiB RAM banks
and the semantic record. It stores hashes rather than raw payloads. Every
captured frame index must be present. The deterministic trace filter is the
existing reference filter: SCENETRACE, coins, fatal/stop lines, SCSP and sound
sample-count lines; it is not an instruction trace.

| Schedule | VBlanks / identical frame+RAM+semantic records | Identical PNGs | Trace |
|---|---:|---:|---|
| No-coin attract cycle | 9,000 / 9,000 | 30/30 | identical |
| Game over → attract → new game | 9,000 / 9,000 | 16/16 | identical |
| Combat | 2,100 / 2,100 | 5/5 | identical |
| Legacy multi-cart combat | 2,100 / 2,100 | 5/5 | identical |
| Total | **22,200 / 22,200** | **56/56** | **4/4** |

All runs exit 0, with no FATAL or convergence mismatch. All pilot runs enabled
direct ABI auditing. Full per-frame hash lists and logs are under
`work/converge/runs/{stock,stock-long,pilot}/`. Portable complete hashes,
per-function counts and source hashes are in
[equivalence.json](../../port/converge/evidence/equivalence.json).

SHA256 of each identical `frames.jsonl` (frame index, pixels, full RAM and
semantic hashes, in order):

```text
nocoin        7d040b8e950aac27edfe4cbc925e76087ea917a8e42a61fb998ee33c4ab3a6fb
gameover      8a6052304c56e6cf98e479f18425386f7248d42083c80bb73f7762e262b41e1d
combat        5b31a8de38d44299c6b1649621bf23432d8d9ee76b4b299f2ff3385cef18177b
legacy-combat 66b2dd4dde533bc78fdf2c9e79bb19e9c813708093a16a5a855b82a78fd17f52
```

Matching deterministic trace SHA256:

```text
nocoin        132788f301ff790e8c164cd25f09272af45473d55acf6976a9a297b0837da443
gameover      eb1b91d3958f54f6494111659c2d7d438fd86e1eed5274a631be905051cca4de
combat        345b13217e784d7cc27b60d39dab4dcce049b399cf4c4d373c1356eb503a8f06
legacy-combat 77be9aa1838f91294c67126bc325b9410ed57d4a5d9cd5608ec51abade2497f4
```

An additional `STV_AUTOPLAY=100` run from boot to **12,000 VBlanks** matches
all five PNGs and the trace, including continued play in Scene 9. Its trace
SHA256 is `ae515bf7d49acc4bc0829384aa6c4461e0664a01c1f01c34970b43e829400117`.
This extra run used sampled screenshots, not every-frame RAM/video capture.
The approximately 170,000-VBlank full-game/ending run was **not performed**;
no ending coverage is claimed. The final delivery executable was additionally
rechecked against stock for all 2,100 combat frame/RAM/semantic records after
the build/state-identity integration was finished (`pilot-delivery/`).

The reference here is **fresh stock from the same frozen working tree**.
That stock already differs from archived P4.10 in **54/56 PNGs and all four
trace hashes**, with overrides disabled. For example, archived/current combat
have the same sample count but different sound non-silent counts/68000 PC and
SR in the stop line. Those pre-existing differences are preserved, not fixed
or attributed to the convergence bridge. The earlier
[P4.10 report](p410_upgrades.md) also records the historical distinction from
P4.5 after P4.7. This proves convergence against stock using the reference
**schedules**, not recovery of older archived picture hashes or MAME fidelity.

The optional C mod
------------------

[mods/sin_half.c](../../port/converge/mods/sin_half.c) is a readable copy of the
sine helper with one semantic edit: remove its final `<< 1`. The `mod` build
selects that file; `pilot`, `pilot10` and the default port do not. The direct
adapter carries the modified C result to real callers. No frame postprocessing,
guest instruction patch or renderer modification produces the effect.

At combat **VB1320**, this changes the rendered floor from the stock textured
green floor to a largely black area as the altered sine affects transforms.
The images were inspected. **46,492 of 168,960 pixels differ**. The complete
final mod run exits 0 and produces all five scheduled PNGs. This is a clear
mechanism demonstration, not a proposed artistic change or a HUD feature.

![Stock/unmodified C at left; optional sine-amplitude C mod at right](../../port/converge/evidence/mod-demo.png)

```text
Unmodified VB1320 PNG 400d7c87b3a0def0ec382f69af1e6845bed586b727315a39ddf79c20663af7ed
Modified VB1320 PNG   52d499ee56b106907531d681e8ca374205931796b749a4f481046aa45d304052
Comparison image     bab8a742e8a651a3710782da2968d1d77705de34dae46cef20296c7ca888971f
```

Source image provenance and pixel counts are retained in
[mod-demo.json](../../port/converge/evidence/mod-demo.json). The modified
function's original-result audit is intentionally not enabled for this run.
Normal and mod quick-state build IDs differ (`9d6d72a21a82dd0b` versus
`befe94b5cc5fc22c` in this build).

Cost, blockers and scaling
--------------------------

For the 36 residue adapters, adding an eligible function costs one list entry
and a qualification/runtime review; no C correction or per-function handwritten
glue was needed here. The five residue-free adapters additionally needed a small
reviewed ABI recipe and boundary tests. The generated semantic bodies total
**810 lines of C** and the retained/audit residue **1,762 lines** for the 41
entries. Cached-symbol generation takes roughly three seconds on this host.
The executable grows from **5,492,592 to 5,537,792 bytes**: **45,200 bytes**, about
0.82% overall or 1.1 KiB per selected function. These are measurements of this
slice, not a linear size/performance forecast.

The bridge currently pays for context copies, transaction vectors, byte-level
checks and duplicate register computation. Correctness captures overlapped
other CPU work: stock/pilot wall seconds were 259.5/266.5 (attract), 276.8/239.5
(game-over), 65.1/54.2 (combat) and 60.9/52.0 (legacy). They cannot establish a
speedup or isolated overhead. No performance improvement is claimed. Removing
register residue and avoiding transaction allocation are the useful next
performance steps, after ABI coverage is retained.

Modding scope is bounded too. The five supplied direct adapters are pure-result
adapters; adding memory effects needs an expanded contract. Residue adapters
still require the original write addresses/order and tail target/arguments to
agree, and normal builds also require identical values. A mod that changes
those control-flow/effect shapes cannot use this bridge unchanged. The supplied
demo deliberately changes a direct function's result. General subsystem
moddability requires the residue-removal and continuation work below.

The original 1,099-function gate has 118 PASS, 730 PASS-void and 251 SKIP. A
static port inventory is in [inventory.json](../../port/converge/evidence/inventory.json):

| Gate status | No poll in port body | Has a poll | Missing port entry |
|---|---:|---:|---:|
| PASS | 44 | 74 | 0 |
| PASS-void | 173 | 557 | 0 |
| SKIP | 63 | 186 | 2 |

The three unused no-poll PASS candidates are the two hardware tail copies and
the stronger-vector timer-classifier SKIP described above. Among PASS-void,
110 no-poll functions have no declared calls, a useful next review cohort.
Five no-poll PASS-void bodies had no observed effects; those need no-op or
coverage review, not automatic promotion. No-poll alone does not establish
safe memory, ABI or path coverage.

PASS-void is not inherently unsuitable. Difftest compares memory footprints,
ordered stores and callee traces for it. A void adapter must preserve the
original residual r0 instead of inventing a C return; the existing residue
provides that. The larger blockers are scheduler polls, hardware/device effects,
uncovered entry gates and nonstandard live registers. This pilot deliberately
does not relabel or install any PASS-void entry.

SKIP functions still require diagnosis: unresolved/nested pointers, private
stack addresses escaping to callees, initialized static tables, variadic or
aggregate interfaces, compiler helpers/custom r0/r1 and multi-result ABIs,
hardware/DMA waits and segmentation/entry issues. The round-2 examples include
the r8/r9→r1 quad dispatcher, sin/cos returning r4/r5 and a caller consuming
live r5. An ordinary r4–r7/r0 bridge cannot infer those contracts. More random
tests cannot supply a missing ABI or side-effect model. The new 64-vector
`02245fc4` exclusion illustrates why a prior PASS is evidence with a bounded
input model, not universal permission to swap.

A practical subsystem conversion sequence is:

1. **Finish the math/collision boundary contracts.** Replace residue with
   reviewed recipes or typed multiple-result interfaces, and retain complete
   register/stack audit vectors. The square-root loop is blocked by its polls;
   the sin/cos pair and atan table need their explicit ABI/static-data support.
   This pilot does not yet convert the whole math subsystem.
2. **Add effects-verified void leaves.** Review the no-poll/no-call cohort,
   preserve live r0 and reject inactive bodies/stack aliases. Convert a coherent
   matrix or command-buffer cluster, whitelist only proven passive memory
   regions and repeat every-frame/full-RAM comparisons. Plain VRAM data and
   trigger registers must not share a speculative-MMIO rule.
3. **Make ordinary calls and polls resumable in C.** Add explicit original-edge
   checkpoint identities and per-site call bridges, preserving live registers,
   delay-slot order and exact budget consumption. C loops may represent
   originally unrolled code, so inserting a poll per C iteration is wrong.
   Save/load needs C-local/continuation serialization for functions that can
   suspend, not just the current compiled continuation IDs. This is substantial
   generator/runtime work; copying a difftest stub cannot solve it.
4. **Convert subsystem interiors to normal typed C calls.** Keep SH2Context
   wrappers at the remaining compiled boundaries, establish which scratch
   outputs are actually consumed there and remove residue inside the converted
   call graph. Start with math/collision, then input/task helpers and text/HUD
   command construction. Convert guest-backed structures to native ownership
   only after pointer/global/callback boundaries have explicit layouts.
5. **Expand acceptance.** Add structured vectors for missing paths, real
   callback/stack fixtures, save/load/audio checks and the full autoplay ending
   run. Keep byte-identical baseline and deliberate opt-in mod tests separate.
   Remove a wrapper only after both its C semantics and its machine boundary
   are exercised; maintain the readable sources as the editable authority.

Reproduction commands, list selection and dependencies are in
[port/converge/README.md](../../port/converge/README.md). The existing decomp
venv supplies pycparser; builds use at most `--jobs 4`. Selecting `stock` links
no overrides. `compare.py --include-autoplay` checks all evidence, frozen hashes
and the final executable's combat recheck before exporting the portable report.

1. All implementation/build work stayed in the private snapshot and new delivery paths.
2. The pilot grew from 10 entries to 41 independently qualified PASS functions.
3. Five execute directly from C; 36 retain generated register residue.
4. Every one of 22,200 reference frames matches pixels, full RAM and semantics.
5. All 56 reference PNGs and four deterministic traces match frozen stock.
6. Twelve thousand autoplay VBlanks match sampled PNGs/trace; no full ending run is claimed.
7. Thirty-one overrides executed 693,734 times across the game schedules.
8. ABI validation passed 50,000 full-context checks and 256 real tail-callee checks.
9. An opt-in one-function C edit visibly changes rendering; the normal build omits it.
10. Poll continuations, custom ABIs, coverage and residue removal gate subsystem-scale conversion.
