# P4.23: final presentation-only 30→60 interpolation attempt

2026-11-09. **Final NO-GO. Do not integrate or expose this prototype as an HFR
option.** Coordinated VDP1/VDP2 interpolation works in synthetic raster tests,
but accepted moving midpoints are **0/600 in a real first fight and 0/300 in the
rotation-floor fixture**. Every measured gameplay pair changes the palette;
command counts/order/identity also fail independently. The required majority
of genuine in-between pictures is not achieved. Investigation stops here.

## Snapshot, scope and crash recovery

Continued the existing `work/p423/` experiment after ENOSPC; did not resnapshot
or restart implementation. The original one-time committed archives remain:

- Main HEAD `e600540048d7872019334d0d72ccd903c710c9b9`, in
  `work/p423/committed.tar` and `committed/`.
- Nested runtime HEAD `b22688d0bef52fba31cda79fd23150e8905cf3c2`, in
  `work/p423/runtime-committed.tar` and `runtime-committed/`.
- `work/p423/snapshot.json` pins 157 files in the frozen **applied P4.20
  prototype** baseline, `runtime-base/` and `port-base/`. Experimental copies
  are `saturn-recomp/runtime/` and `port/`. The committed runtime predates
  several applied milestones; it is not interchangeable with the applied base.

Read STATE, the architecture map, P4.14/P4.15/P4.20 reports, and P4.20 sidecars
and patches. Retained the P4.20 private recorder/raster; replaced its rendered-
background-equality rejection with explicit private VDP2 temporal state.
The frozen generated code/dependencies remain those inherited from P4.20.
All legacy `STV_P420_*` controls must remain unset; the verifier clears inherited
`STV_*`/`SATURN_*` variables. P4.23 adds no culling or wider-aspect control.

Recovery audit found the zero-byte game-over hardware log and, additionally,
a game-over WAV with a valid header but **zero frames**. Its original PNGs
were intact. Repeated the complete regression, repairing the WAV and checking
all outputs against the unchanged oracle. `regression.log` records the failed
pre-resume run; **`regression-resumed.log` supersedes it**. The audit validates
both committed archives against their extracted files, all 157 pinned hashes,
sidecar copies, Python/shell syntax, PNG decoding, WAV payload lengths, raw
endpoint lengths, replay rows and machine-state framing. Expected empty Python
package files and the already committed `project/src_functional/e` are recorded
as inherited empties. No other empty regular file remains under this work tree.
See `work/p423/resume-audit.json` and `port/p423_audit.py`.

Disk started at about 53 GiB free and remained about 51 GiB after verification.
The verifier checks `/home` before and throughout captures and stops its own
child if free space falls below 15 GiB. It discards `hw-log.txt` through
`/dev/null` and suppresses unnecessary `STV_CAPTURE_FIELDS` device dumps.
Deleted 108,741,056 bytes of redundant regression dumps from the interrupted
run; hashes/sizes remain in `work/p423/removed-intermediates.json`. Small
endpoint diagnostics, images, WAVs and state evidence remain under `work/`.

Only `work/p423/`, new `port/p423_*`, new
`port/runtime-patches/p423-*.patch`, and this report belong to this job.
No existing shared `port/`, `tools/saturn-recomp/` or `project/` file was edited.
P4.22 files, including the truncations mentioned in the resume note, were left
to their owner. No commit, network, GPU, sudo, prohibited-directory access or
unrelated-process termination was used.

## Approach A: coordinated private endpoint state

`p423_video.inc` records executed VDP1 commands, their initial raster state and
actual draw-time textures for each hardware buffer. A displayed-buffer change
establishes a new endpoint. Matching requires equal scene/draw/count/control
state, draw attributes and used texture/LUT/Gouraud bytes. Moving vertices need
bounded displacement and a unique closest compatible previous command.
One failed command rejects the complete composition. It does not invent
identities or propagate another face's motion into unmatched geometry.

`p423_vdp2.inc` captures registers, 512 KiB VRAM and CRAM at the same endpoint.
The compositor's register/VRAM readers select a private interpolated copy only
while rendering an accepted sample. The implemented smooth-field whitelist is:

- NBG0/1 fractional X/Y scroll and zoom; NBG2 integer scroll.
- Enabled horizontal/vertical/zoom line-scroll entries at their configured
  interval and table base.
- RBG parameter sets A/B: screen starts, line/dot increments, matrix terms,
  viewpoint/center words, translations, scale, coefficient start/increments.
- Smooth coefficient values over a conservative union of both endpoint sample
  ranges, preserving coefficient control bits.

Changed non-whitelisted register/VRAM bits, CRAM, flags, table topology, large
steps, unsupported rotation-window selection or scene identity cause a snap.
NBG3 scroll stays fixed for the HUD; control/texture changes also reject rather
than move unknown HUD sprites. Full-VRAM comparison is deliberately
conservative and can reject unused writes. This is not a semantic HUD or
object-identity solution. Cut/large-motion/reset guards exist; their presence
does not establish broad real-scene transition coverage for an accepted mode.

Both layers use **the same alpha**, only when both matchers pass. For a two-
field game interval, age 0 renders the previous endpoint at alpha 0 and age 1
renders alpha 0.5. Rejection hides the private VDP1 raster and selects real
VDP2 state together. There is no frame blending, extrapolation or intermediate
guest tick. Attract's one-field cadence passes through without interpolation.
The public experimental control is `STV_P423_HZ=60`; supported scope is 1x,
non-interlaced ST-V, with no netplay. This remains an evidence control only.

Synthetic production-raster checks pass: VDP1 40→50 moves to 45; NBG and line
scroll 0→10 sample 5; an RBG Mx 400→410 boundary samples 405; changing
perspective coefficients produces the expected intermediate boundary. Tests
also check serialized VDP1/VDP2 isolation, palette/control/reset rejection,
count mismatch and ambiguous command identity. `work/p423/unit-resumed.log`
records the repeated pass. These independent layer tests do not prove useful
coordinated interpolation in the actual game.

## Coverage and reasons for stopping

`work/p423/coverage.json`, generated by `port/p423_analyze.py`, counts only
age-1 samples in stable two-field gameplay windows. Startup matches do not
count as moving interpolation. The direct-start Scene 9 fixture exercises a
rotation floor; it is not a claimed natural full-game traversal.

| Scene | Fields examined | Midpoint opportunities | Matched / fallback | Genuine moving ratio |
|---|---|---:|---:|---:|
| First fight, physical autoplay | VB901–2100 | 600 | 0 / 600 | **0%** |
| Rotation floor, training Scene 9 + physical autoplay | VB901–1500 | 300 | 0 / 300 | **0%** |
| No-coin attract control | VB901–1500 | 0 (one-field cadence) | Not applicable | Not applicable |

All 600 fight and all 300 floor opportunities reject VDP2 for **palette
change**. Captured gameplay endpoints each change one CRAM word, entry 364;
for example fight VB1800 changes 2→31, and floor VB1201 changes 21→18.
The RBG parameter block at byte address 389120 also changes 3–6 bytes in these
fight captures and 3–5 bytes in the floor captures. The code identifies smooth
VDP2 fields, but the palette guard required by this task prevents accepting
the complete pair. It was not disabled or bypassed to improve the ratio.

VDP1 matching is evaluated separately even when VDP2 rejects:

| Independent VDP1 rejection | Fight midpoints | Floor midpoints |
|---|---:|---:|
| Command count | 536 | 274 |
| Motion exceeds bounds | 37 | 14 |
| Attributes/order | 27 | 11 |
| Ambiguous identity | 0 | 1 |

Thus ignoring palette changes alone would still yield zero complete matches
in these windows. At fight VB1800 both lists have 336 commands, yet only 177
shared signatures are unique in both endpoints. Floor VB1201 has 481→476
commands, with 293 unique shared signatures. Depth sorting, visibility and
animation do not preserve a complete command sequence. Partial signature
matching cannot safely establish adjacency, ownership or newly visible faces.

The performance run reports 294 accepted compositions during startup, but
**zero genuine moving midpoints over all 2,100 fields**. Those static matches
must not be advertised as gameplay interpolation.

## Frame strips and visual inspection

Columns are previous real endpoint / requested midpoint / next real endpoint.
The middle column is explicitly labelled **REAL FALLBACK** because no candidate
passed. Captures retain both raw real output and presented output for every
field VB1800–1812 (fight) and VB1200–1212 (floor/attract).

![Fight: real / rejected midpoint fallback / real](../../work/p423/visual/fight-strips.png)

![Rotation floor: real / rejected midpoint fallback / real](../../work/p423/visual/floor-strips.png)

Opened and inspected both strips and the original-coordinate close crops:
`work/p423/visual/fight-floor-actor-crops.png` and
`work/p423/visual/floor-floor-actor-crops.png`. Fight: the airborne actor and
standing player's limbs step to the later pose; railing intersections and
floor/shadow details repeat. Floor: the white floor motifs, actor feet, dark
mesh shadows and wall junction repeat the later state rather than occupy an
intermediate location. HUD flashes/colour changes remain real-field updates;
the centre is not necessarily byte-equal to the adjacent earlier endpoint's
HUD, because its real field occurs one VBlank later.

**All 39 presented sample PNGs across the three scenes are byte-identical to
their same-VBlank real PNGs.** No new floor/actor split, crack, HUD swimming or
sprite ghost was seen in these fallbacks. This is evidence of safe rejection,
not a pass for interpolated image quality. There are no accepted moving pairs
on which to establish the requested absence of interpolation artifacts.

## Approach B: feasibility assessment, no runtime prototype

The decomp identifies useful capture sites: `view_build_display_transform`
(022225f6, runtime 060265f6) derives the VDP2 view transform and edge samples;
`vertex_project_dualmtx` / `vertex_transform_project` (022135a0, runtime
060175a0) consumes dual matrices around 060da040 and overwrites projected
scratch at 060debc8; `model_emit_mesh_part` (02235dc0, runtime 06039dc0) has
object/model/quad identity before emitting the deferred list. Camera task work
and actor positions alone do not retain each animated part's pose/matrix.

A read-only callback at these sites could record inputs without replacing any
guest calculation. A useful reprojector would additionally need per-object
lifetime/part/vertex keys, both matrices and source vertices, a mapping through
48-byte deferred records and depth sorting into 0x24-byte software command
records, plus visibility/texture/HUD policies. Re-running the guest projector
would write guest scratch and DIVU/device state and is excluded. An independent
host projector could avoid that write, but the current captured command stream
does not supply the required stable ownership mapping.

Most decisively, B still cannot accept the measured pairs while honoring the
explicit snap-on-palette-change rule: **every gameplay pair measured changes
CRAM**. Reprojection does not solve that disqualification. Under the task's
final-attempt boundary, B was therefore assessed as insufficient to meet
acceptance without a broader identity pipeline and changed palette policy.
No game-level runtime prototype, guest patch or relaxed guard is claimed.

## Guest/default identity and latency

The resumed default regression passes **56/56 PNGs, four complete traces,
four guest traces and four WAVs**, including 9,000 no-coin fields. Oracle:
`work/p411/regression/after/manifest.json`, unchanged. Results:
`work/p423/regression.json`, `regression-resumed.log`. The separate paid-input
smoke at explicit 1x yields shot-1900 MD5
**`9bd5386ea7439621c19674381086e8fd`**, in `work/p423/smoke.json`.

The fixed physical-input recording is replayed with the option off and on:
**2,100/2,100 full unmasked 2 MiB RAM hashes and semantic records match**;
all **22 serialized machine-state payloads** at VB2000 match byte for byte;
hardware dumps at VB1800/2100 and WAVs match. Only the state-file header/build
identity lies outside payload comparison. No device bytes or RAM addresses are
masked. Raw data and reports are under `work/p423/authority/` and
`work/p423/authority.json`; the resume audit independently recomputes these
comparisons from the retained files. This proves isolation for the tested
option/run, despite its failure to interpolate moving content.

Repeated runtime suite: **169 tests, zero failed**,
`work/p423/tests-resumed.log`. Layer midpoint/isolation probes also pass.

An accepted delayed composition would add **one game frame**, two runtime
fields: **33.430456 ms** using the 16.715228 ms field period. This is the
endpoint scheduler's intended extra visual latency, not measured controller-
to-photon latency. The measured gameplay output falls back to the current
real frame and does not incur that endpoint delay. No real display/input-
latency or high-refresh-monitor qualification was performed.

## 1x CPU cost

Sequential single-pass 2,100-field runs on AMD Ryzen Threadripper PRO 5975WX,
shared Linux host, explicit `--internal-resolution 1`, identical physical
autoplay, SDL dummy video/audio, **software** renderer, Sharp, frameskip off,
unpaced. All owned tests/captures finished first. GNU time supplies CPU totals;
100% is one logical CPU. Field rate includes startup and is not distinct moving
picture rate. The verifier's polling overhead is excluded by using GNU time's
child elapsed time. Reports: `work/p423/performance.json`,
`performance-summary.json`, and `performance/{default,hfr60}/`.

| Mode | Wall seconds | Guest fields/s | Process CPU | CPU ms/guest field |
|---|---:|---:|---:|---:|
| Default | 10.22 | 205.48 | 202% | 9.84 |
| P4.23 target 60 | 11.48 | 182.93 | 218% | 11.92 |

Added cost is **2.08 CPU ms/field, about 21.2%** in this pair of runs. Endpoint
copies, guards, diagnostics and disabled normal frame caching still cost CPU
when output falls back. These measurements do not estimate the cost of a
successful moving interpolation path or establish Mac/Windows/display results.
Shared-host load/frequency variation limits causal precision; no additional
benchmark pass was needed to decide this zero-coverage result.

## Reproduction and PM disposition

**Acceptance fails; no production integration steps are recommended.** Retain
the report, experimental sidecars and patches for review. Keep normal defaults
and player settings unchanged. This is the requested final no-go, not another
open HFR milestone or a disabled feature awaiting a checkbox.

Four patches have zero-fuzz, zero-offset, exact-byte reconstruction checks in
`work/p423/patches.json`:

- `p423-runtime-prototype.patch` (6 files) and
  `p423-harness-prototype.patch` (2 files) are incremental against the pinned
  applied-P4.20 `runtime-base/` and `port-base/`.
- `p423-runtime-snapshot.patch` (45 files) and
  `p423-harness-snapshot.patch` (2 files) reconstruct from the **committed**
  archives. The runtime patch includes inherited applied milestone changes
  required by the tested prototype. It is a reproduction patch, not a claim
  that those earlier changes belong to P4.23. The old unused `video.cpp.orig`
  backup is omitted. New P4.23 sidecars are supplied separately in `port/`.

Do not apply both patch pairs or apply them to the live P4.22 tree. The existing
`work/p423/` experiment already contains the changes. For an independent
reconstruction, use copies of the indicated frozen directories, matching
runtime/port layout and inherited generated code/SDL dependencies; apply the
corresponding pair once with `patch --batch --fuzz=0 -p1`. No source/dependency
fetch is necessary. `port/p423_export.py` performs and checks the per-file
reconstruction directly without touching shared sources.

From the repository root, against the retained experiment:

```bash
bash port/p423_build.sh
work/p423/build/p423_video_test
work/p423/tests/runtime_tests
python3 port/p423_verify.py regression
python3 port/p423_verify.py smoke
python3 port/p423_verify.py authority
python3 port/p423_verify.py visual
python3 port/p423_analyze.py
python3 port/p423_verify.py performance
python3 port/p423_export.py
python3 port/p423_audit.py
```

The build reuses frozen SDL and generated sources; evidence scripts require
Pillow, `cart.bin`, the frozen port cart, fixed RTC helper
`work/p415/fixed-time.so` and unchanged P4.11 reference manifest. No netplay
connection or other network operation is required. `STV_P423_CAPTURE=FIRST:LAST`
adds short paired endpoint/command/real/presented diagnostics. Leave it unset
for ordinary verification/performance. Do not use long capture ranges.

Final executable SHA-256:
`840e757c836dc35c1aa8d6c9e9c7f69af1546f7cbacb73fca2a221a276d67c85`.
`work/p423/artifacts.json` pins delivered sources, patches, executable and reports.

1. Resumed the frozen experiment; audited truncation and repaired the incomplete regression WAV.
2. Added private NBG/line-scroll/RBG/matrix/coefficient interpolation at the actor alpha.
3. Synthetic midpoint, state-isolation and rejection probes pass; 169 runtime tests pass.
4. First fight accepts 0/600 moving midpoint opportunities; all 600 fall back.
5. Rotation-floor Scene 9 accepts 0/300 midpoint opportunities; all 300 fall back.
6. Every measured gameplay pair changes CRAM; VDP1 correspondence also fails independently.
7. Inspected real/fallback/real strips and close crops; all 39 samples equal real output.
8. Defaults match 56 PNGs, all traces/WAVs and the required shot-1900 MD5; replay/state match.
9. Intended accepted-frame delay is 33.43 ms; measured 1x overhead is 2.08 CPU ms/field.
10. Final NO-GO: preserve evidence and defaults; do not integrate this prototype as HFR.
