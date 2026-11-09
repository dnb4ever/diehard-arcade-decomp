# Die Hard Arcade scenario checker v1.01

P4.7 update (2026-11-07): the hook is now installed and rebuilt, captured in
`port/runtime-patches/p47-scenario-capture.patch`. It is headless only, off by
default, and streams protocol **1.01**. Use `--port-hook` for full port RAM and
initial RNG contents; no store-watch inference is needed on that route. The
v1.00 results and unapplied-hook discussion below are historical. Current
measurements, boot-state changes, remaining timing differences and repros are in
`notes/port/p47_mame_fidelity.md`. Comparison rules and the RAM mask are unchanged;
no mismatch was masked or resynchronised away. The source generator recognises
an installed hook, emits its compile copy under `work/scenario_v101/` and copies
the canonical runtime patch without applying or duplicating it.

Independent implementation; no code was copied from arcade-decomp. This checks the
USA Die Hard Arcade cart against MAME 0.285, with factory NVRAM, `epr-17952a`, and the
P4.5 single-cart route. It compares every captured VBlank, including the attract
and death/continue paths. Game-state fields are the primary diagnostic; exact
screen and work-RAM hashes are supplementary.

## Running

Run from the repository root with Python 3.10+ and Pillow. Existing local ROMs and
the extracted MAME package are required; this tool neither downloads nor modifies
them. The defaults match `notes/port/p45_bios_floors.md`.

```bash
python3 tools/scenario/check_scenarios.py --version
python3 -m unittest discover -s tools/scenario -p 'test_*.py' -v
python3 tools/scenario/check_scenarios.py --jobs 4 \
  --out work/scenario_local --png 11,316,1526,2100,5000,7177,9300
# Recompare saved captures, without launching either emulator:
python3 tools/scenario/check_scenarios.py --reuse --out work/scenario_local
# Quicker state-only diagnostic, still recording every VBlank:
python3 tools/scenario/check_scenarios.py tests/scenarios/scene1_combat.txt \
  --fields-only --out work/scenario_quick
```

Use a fresh output directory for each capture: existing captures are never silently
overwritten. `--port`, `--mame`, `--mame-lib`, `--rompath`, `--cart`, and `--cartrom`
select existing local artifacts. All paths in commands and authored files are
repository-relative. Outputs must be under `work/scenario*`. `--jobs 1..4` runs
independent scenario processes, without agents. Each child has a timeout; only
children started by the checker are terminated on failure or interruption.

Exit codes: **0** means complete equality with coverage satisfied; **1** means a
reported divergence or an unmet scenario expectation; **2** means unavailable
capture data or a harness error. A divergent run can also have unavailable fields:
check `missing_port_fields` in the result. Equality of the available P41 fields
alone is never reported as a complete pass.

`--png N,...` retains PNGs at those scenario frames; otherwise the port's temporary
`--shots` images are decoded, hashed and deleted. MAME hashes raw pixels and only
writes requested PNGs. MAME filenames contain its absolute frame number, i.e.
scenario frame plus the calibrated offset. `--ram-frames N,...` retains MAME's raw
2 MiB RAM images at requested scenario frames (both engines with `--port-hook`).
JSONL contains hashes for every
frame regardless of retained PNG/RAM images. `--fields-only` explicitly skips
screens and full MAME RAM; it does not imply those comparisons passed.

## Input scripts and coverage

Files in `tests/scenarios/` use inclusive VBlank ranges:

```text
frames 2100
expect paid
expect scene=3
600 605 0   0 01  # coin 1, held six VBlanks
700 705 0   0 10  # P1 START
1650 1749 200 0 0 # P1 right
1750 1751 1   0 0 # P1 punch (A)
```

Columns are `FIRST LAST P1 P2 SYSTEM`. Masks are hexadecimal, active high; omitted
frames release all controls. Overlapping rows OR their masks. P1/P2 use the port's
raw peripheral ABI: A/B/C = `1/2/4`, up/down/left/right = `40/80/100/200`.
SYSTEM uses COIN1/COIN2/TEST/SERVICE/START1/START2 = `1/2/4/8/10/20`.
Unknown bits, out-of-range events and malformed files are errors. These masks
compile to exact-duration `--stvinput` events for the port and to MAME physical
ioport fields, never writes to game RAM. Empty scripts use a coin event beyond the
end of the run because an empty port argument would activate its built-in coin
and START sequence.

| Script | VBlanks | Intended coverage |
|---|---:|---|
| `attract.txt` | 9300 | No coin; full FBI/intro/title/demo cycle, scene 10 and return to scene 1 |
| `scene1_combat.txt` | 2100 | Coin + START, first combat area, walk and punch |
| `game_over.txt` | 5600 | Idle paid game, decline continue, GAME OVER and return to attract |
| `scene3_plus.txt` | 10000 | Fixed combat/continue input through the third combat area, director scene 9 |

The first combat area is player-facing Scene 1 but director scene ID **3**; the
third combat area is director scene **9**. The long script is a frozen recording
of the existing scripted player, not an adaptive player used during comparison.
Recording used `STV_AUTOPLAY=100 STV_P41TRACE=1`, input beyond the run, and
`--watch 06000708:0600072F`; `tools/scenario/record_input.py` converted the BIOS raw
panel words at each P41 boundary into held ranges. Source and SHA256 are in the
script comments. Replays unset `STV_AUTOPLAY`, as well as every inherited `STV_*`
variable. Both machines receive exactly the same frozen input. The short scripts
are handwritten.

Expectations are separately reported for each machine: paid means P1 credit state
2 was reached; game_over means continue state 3 transitioned to 0; attract_return
means a valid scene 3..81 was followed by scene 1. The latter is a state-based
coverage check, not OCR of the GAME OVER text. MAME's pre-cart `SEGA` RAM test
pattern is excluded from attract-return detection.

TEST is representable in the format and supported by the MAME driver. The current
port silently ignores a `TEST` token, so the checker rejects such a scenario
explicitly. The optional patch adds the physical TEST token; the actual BIOS test
menu remains outside the port's implemented single-cart route. None of the four
acceptance scenarios presses TEST.

## Capture and alignment

Before replay, an input-free calibration finds the **first attract scene 1 -> 2
edge** on each machine. It does not fit offsets to later gameplay or resynchronise
at scene boundaries. The measured offset is **508 VBlanks**: port 316, MAME 824.
The first scene-1 boundaries differ by 509 (port 11, MAME 520), so a one-frame
startup mismatch remains visible. MAME runs the input at `port frame + 508`.
`--offset` is an explicit override for a previously measured offset; its use is
recorded. The input-free calibration evidence is in
`work/scenario_v100/smoke2/alignment.json` and its two calibration captures.

MAME's Lua machine-frame notifier samples fields and raw work RAM, then sets
physical ioport fields for the **next** frame. The port P41 trace samples at
VBlank-IN, after BIOS raw input publication and before the game ISR. These are
frame boundaries, not an assertion of cycle-identical CPU scheduling. A fixed
boot offset cannot remove within-frame input scanning or a later timing drift.
The checker compares from the port's first scene 1 (VB 11); MAME's BIOS-only boot
prefix is captured but excluded from semantic comparison.

`tools/scenario/mame_capture.lua` uses the MAME Lua
[memory API](https://docs.mamedev.org/luascript/ref-mem.html) and
[screen API](https://docs.mamedev.org/luascript/ref-devices.html). Raw RAM and native
screen pixels stream through a FIFO to Python. This avoids millions of Lua byte
hash operations and avoids retaining tens of gigabytes of frame dumps. No game
memory is written, HP pinned, RNG seeded or scene forced. Each scenario has its
own initially empty NVRAM/config directory. CLI selects `-bios us`, disables
configuration loading and runs `-video none -sound none -nothrottle`.

Addresses and widths were checked in `notes/symbols.txt` and functional sources:

| Field | Location / derivation |
|---|---|
| Scene | u32 `06098668` |
| P1/P2 credit state | u8 `06098674/75` (2 playing, 3 continue) |
| Armed flag | u32 `06098670` |
| Mode | u16 `0609979A` |
| Progress | u32 `06098830` |
| Cooked system edge | u16 `06098798` |
| RNG seed | u32 `0609957C`; `project/src_functional/02210020.c` |
| Game frame counter | u32 `060987BC`; `02201958.c`, `02223c74.c` |
| Actor slots 1..6 | u32 task at `060995B0 + 4*slot`; entity pointer at task + 8 |
| Actor identity / HP | u8 entity + `34`; u16 entity + `56` |
| Actor x/y/z | u32 entity + `08/0C/10`, unsigned bit patterns of signed fixed point |

Actor pointers are used to find records, not compared as semantic fields. Slot,
active/absent status, ID, HP and x/y/z are compared. MAME validates pointer ranges
before dereferencing; absent slots have zero fields. Port observations come from
its existing P41 diagnostics. Additional mode/progress fields can retain undefined
boot contents until the game first writes them; their early mismatches are
classified separately from gameplay bugs.

Each engine writes `frames.jsonl`, a process log and a command manifest. Results
include the first field divergence, the first divergence for each individual
field, the first masked RAM region divergence if available, the first screen
divergence, scene transitions and separate coverage outcomes. Frame gaps,
incomplete actor records and failed child processes are errors. The run manifest
records input and binary SHA256 values and the mask. New runs also fingerprint
checker sources. `--reuse` verifies input and mask identity.

Screens hash SHA256 of `LE32(width), LE32(height), packed RGB bytes` at the native
resolution. The MAME BGRX host pixels and decoded port PNGs are converted to the
same RGB representation; PNG metadata/compression are irrelevant. Geometry
changes are reported, not scaled away. Audio hashing is not implemented in v1.00.

## Existing-port limits and the unapplied hook

The checker uses the current port binary without rebuilding or changing it:

- `STV_P41TRACE=1`: every-VBlank scene, join, mode/progress and six actor slots.
- `--watch 060987BC:0609957F` with `STV_WATCH_CPU=0`: track master writes to the game
  frame counter and RNG. Ignoring slave watch output avoids a massive calibration
  busy-loop trace. The interval also contains unrelated fields; those writes are
  retained as evidence but do not become semantic comparisons.
- `--shots` on every VBlank: screen hash, optional retained PNG.

The parser tracks individual bytes and snapshots them at P41 lines. RNG is
**null until its first observed store**, rather than treating unobserved memory
as zero. `--peek` only runs at exit. An independent one-VBlank `--peek` probe
(`work/scenario_v100/initial-probe.log`) observed RNG 0 and counter 1; this is
supporting evidence, not invented per-frame trace data. The frame-counter writes
are available throughout the compared interval.

The ST-V executable has no per-frame RAM state export. Its `--dump` is VDP1/VDP2
memory only, and runtime save-state support in the general Saturn executable is
not exposed by this ST-V harness. Consequently **port full-RAM hashes are null**
in the executed runs, and masked RAM equality was **not tested**. A hash of the
captured semantic record (`state_sha256`) is explicitly separate from a RAM hash.

`work/scenario_port_hook.patch` is the proposed opt-in hook, **not applied**. It
changes only `tools/saturn-recomp/runtime/src/video/video.cpp` and `bios.cpp` in the
patch text. With `STV_SCENARIO_PIPE` unset it makes no captures. When enabled it
streams the same fields, 2 MiB RAM and composed pixels to the same receiver, before
raising the VBlank-IN ISR. The two SH-2 CPUs are not frozen into an atomic
cycle-level snapshot; the slave handshake remains a noisy diagnostic region. It also adds the missing physical TEST token. There is
no game-state forcing. The video source compile copy passed `c++ -fsyntax-only`,
and the patch passed `patch --dry-run -p1` from the repository root. A compiled
standalone fixture also verified the hook's binary framing, RAM byte order, actor
pointer traversal and RGB hash (`work/scenario_v100/hook-fixture-checks.log`). No runtime
source was edited or rebuilt for this task.

After the runtime owner reviews/applies the patch and rebuilds independently:

```bash
python3 tools/scenario/check_scenarios.py --port-hook --jobs 4 \
  --out work/scenario_with_hook --ram-frames 316,1526 --png 316,1526
```

`tools/scenario/make_hook_patch.py` regenerates only the patch and a compile copy
under `work/scenario_v100/`, against the current runtime source. It never applies
the patch. `--port-hook` must not be used with an unpatched executable: the missing
transport is reported as an incomplete capture, not a match.

## RAM mask

`tools/scenario/ram_mask.json` is versioned and copied into each run manifest.
The raw work-RAM hash covers 1 MiB at `00200000` followed by 1 MiB at `06000000`, in
ascending bus byte order. The masked hash zeros the following half-open ranges;
32 SHA256 region hashes cover successive 64 KiB regions in the same address order.

| Ignored range | Reason |
|---|---|
| `06000000..06003FFF` | Real BIOS copied services/vectors vs port sentinel services; BIOS bookkeeping/input work area and initial master stack at `06002000` |
| `060E0DD0..060E0DDB` | Slave command read/write pointers and busy handshake, sampled asynchronously; documented in `notes/port/freeze_diagnosis.md` |

This is a conservative, architectural mask, not a learned list of bytes that
happen to differ. Low work RAM, game code/data, actors, RNG, game counter, all other
stacks and scratch stay visible. No scene/HP mismatch can train the mask away.
Further BIOS or stack noise is expected and should be added only with address
ownership evidence. The mask has synthetic coverage verifying that a BIOS byte
is ignored while an RNG byte still changes the hash. Cross-engine validation of
the mask awaits the unapplied hook; whole-RAM equality is not a v1.00 pass claim.

## Executed results and first divergences

Completed on 2026-11-06 (local date). The existing native executable was copied
into `work/scenario_v100/saturn_stv` before recording/running, so concurrent port
builds could not change the executable under this experiment. Its SHA256 is
`2c0f41c91061eb298deb54135ddea56a7b547c140d92796738e3062e505b74dd`.
MAME reports `MAME v0.285`; executable/ROM-input hashes are in the run manifest.

After measuring the offset with the input-free calibration, the full run was:

```bash
python3 tools/scenario/check_scenarios.py \
  --port work/scenario_v100/saturn_stv --out work/scenario_v100/full \
  --offset 508 --jobs 4 --timeout 3600 \
  --png 11,316,698,1526,2100,5000,7177,9300 --ram-frames 11,316
python3 tools/scenario/check_scenarios.py --reuse --jobs 4 \
  --out work/scenario_v100/full
```

Both commands exited **1**, as expected for detected differences. All eight engine
runs completed: **27,000 port frames**, **29,032 MAME frames**, and **26,960
boot-aligned frame pairs** compared. There are no frame gaps, missing actor
records, missing screen hashes or native FATALs. Full MAME RAM hashes exist on
every frame. Port RAM remains unavailable; unknown RNG prefixes contain
864/700/700/631 frames for attract/game_over/scene1_combat/scene3_plus respectively.
These prefixes are marked null, not inferred from an exit-time peek.

| Scenario | Frames | Offset | First divergence (port/MAME frame) | Values (port / MAME) | Coverage | Unavailable port | Status |
|---|---:|---:|---|---|---|---|---|
| attract | 9300 | 508 | scene @ 11/519 | 1 / 0 | PASS | rng, ram | DIVERGED |
| game_over | 5600 | 508 | scene @ 11/519 | 1 / 0 | PASS | rng, ram | DIVERGED |
| scene1_combat | 2100 | 508 | scene @ 11/519 | 1 / 0 | PASS | rng, ram | DIVERGED |
| scene3_plus | 10000 | 508 | scene @ 11/519 | 1 / 0 | UNMET | rng, ram | DIVERGED |

The `UNMET` entry is specifically MAME's `scene=9` expectation: **port reaches
scene 9 at VB 7177; MAME reaches scene 5 but never scene 9 during the same input**.
All other coverage expectations pass. This failure is retained instead of
weakening the long scenario's target. The fixed replay exactly matches its
adaptive recording on all 10,000 P41 summaries and 60,000 actor-slot records
(`work/scenario_v100/frozen-input-check.log`).

All four scenarios have the same first scene mismatch, so the per-field results
are needed to understand what follows:

| Observation | Port frame / MAME frame | Both values or timing | Classification |
|---|---|---|---|
| Initial scene boundary, all scenarios | 11 / 519 | Scene 1 / 0; MAME enters scene 1 at 520 | Boot boundary timing: the first scene lasts 305 vs 304 VBlanks, so one constant cannot align both its start and end |
| Counter, all scenarios | 13 / 521 | 5 / 4 | Frame/task sampling phase difference; does not alone establish a game logic bug |
| Mode/progress before their first game writes | 11 / 519 | 0 / `4741`, 0 / `53454741` | Undefined initial memory: real BIOS `SEGA` RAM-test pattern vs zero-initialised port RAM |
| No-coin RNG after first observed store | 865 / 1373 | `8016B499` / `39D64C21` | Initial RNG state plus call timing candidate; investigate HLE boot memory initialization |
| Paid-combat and idle-game-over seeded RNG | 702 / 1210 | 533 / 536 | Timer-derived seed difference, likely input/frame scheduling; downstream combat is no longer an identical random experiment |
| Long-script seeded RNG | 633 / 1141 | 464 / 467 | Same timer-derived seed difference on a second input schedule |
| First actor-slot-5 position mismatch in paid short runs | 712 / 1220 | x 2590869 / 2590876; z 1561251 / 1561190 | Actor/cut-scene timing or RNG consequence; no separate physics fault established |
| First screen mismatch, all scenarios | 11 / 519 | 320x224 / 352x224, different hashes | Startup video-mode diagnostic, not evidence of a later floor-rendering bug |

The no-coin case has a useful independent initialization clue. Before any RNG
store, the one-frame port peek gives seed 0; MAME's RAM image at frame 519 contains
`53454741` at the seed. The first port PRNG output is `8016B499` (observed at VB 865,
store logged during VB 864), exactly `rng_next(0)`. MAME's first output after its
boot pattern is `BF86696E` at frame 1371, exactly `rng_next(53454741)`. This is
consistent with **different starting RAM**, not evidence of a faulty multiply or
rotate in the recompiler. The compared frame 865/1373 already includes two extra
MAME RNG calls. This initial-state mismatch can affect attract effects before the
game explicitly reseeds. The observed PRNG algorithm is documented in
`project/src_functional/02210020.c`.

For paid play, `project/src_functional/022058a8.c` explicitly assigns
`0609957C = 060987BC`. Thus the observed three-count seed difference is expected
to change later enemy decisions and damage even if the PRNG arithmetic is exact.
Input sampling and frame-counter scheduling are the next candidates to isolate;
forcing the seed to match would conceal this evidence and is not done here.

The idle game-over timing separates a correct fixed countdown from the preceding
combat divergence:

| Event | Port VB | MAME frame | MAME scenario frame (`frame - 508`) |
|---|---:|---:|---:|
| Playing | 720 | 1227 | 719 |
| First combat scene 3 | 1526 | 2033 | 1525 |
| Continue state 3 | 3615 | 3999 | 3491 |
| State 0 / GAME OVER | 4893 | 5277 | 4769 |
| Return to attract scene 1 | 5195 | 5576 | 5068 |

Both continue countdowns last **1278** VBlanks. GAME OVER -> attract takes
**302** port VBlanks and **299** MAME frames. Death occurs 124 scenario frames
earlier in MAME. The countdown agrees; earlier death is a gameplay trajectory
mismatch compatible with the seed difference, and the three-frame return gap is
a transition timing candidate. No failure to return to attract was reproduced.

The no-coin attract loops to scene 1 at **port 8872 / MAME 9367**, reproducing the
P4.5 observation. With the fixed +508 alignment, MAME's loop is 13 scenario frames
earlier. The checker deliberately preserves this accumulated drift.

On the long script, MAME enters its first fight at frame **1195** (scenario 687),
versus port **698**. Its first game ends and returns to attract at **4402**;
the second scripted coin/START starts a new game, with scene 3 at **5854**,
scene 4 at **7800**, and scene 5 at **7856**. The port's same inputs buy a continue
in scene 5 and reach scenes 6 -> 8 -> 9 at **6527 -> 7104 -> 7177**.
This is a **cascaded input/RNG/timing divergence and coverage failure**, not proof
of a scene-9 loader defect in MAME or the port. The sequence is an open-loop
regression stimulus, not a guarantee that a diverged character remains near the
same enemies. No HP or progression writes were used to repair the trajectory.

Exact screen hashes agree on only 33/21/21/20 aligned frames for
attract/game_over/scene1_combat/scene3_plus, mostly blank boundaries. Given the
state/timing differences and the startup geometry mismatch, these pixel counts
are diagnostics; they do not isolate a new renderer bug. Requested PNGs are
preserved alongside the JSONL for inspection. Whole-RAM cross-engine comparisons
number **zero** until the runtime owner supplies the optional hook.

Validation evidence:

- `work/scenario_v100/tests.log`: **12 tests pass**, covering input holds/releases,
  overlaps, invalid masks, no-coin default suppression, TEST rejection, boot
  alignment, noisy RAM masks, first actor divergence, gaps, unavailable data,
  screen-only divergence and BIOS-pattern coverage handling.
- All Python sources parse with Python 3.10 grammar.
- `work/scenario_v100/full-validation.log`: every frame interval, all retained PNG
  hashes and all retained RAM hashes checked; USA area, single-cart settings and
  boot mode verified from both retained RAM frames in every MAME scenario.
- `work/scenario_v100/hook-fixture-checks.log`: compiled optional-hook transport
  fixture passes; runtime syntax check and patch dry run also passed. The patch
  remains unapplied and has not been tested in a rebuilt game executable.
- `work/scenario_v100/full/results.json`, per-scenario `comparison.json` and
  `work/scenario_v100/full/summary.md`: machine-readable and tabular results.

P41 store-watch caller labels are approximate native address lookups and can be
misleading after compiler inlining; the checker uses the observed guest store
address/value and P41 boundary, not those labels. RAM-mask refinement and any
runtime timing/RNG fixes belong to a subsequent port change. This task changed
only `tools/scenario/`, `tests/scenarios/`, this note, and `work/scenario*` artifacts;
no port/runtime/functional source was edited, and no commit was made.

