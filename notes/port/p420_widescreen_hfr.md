# P4.20 widescreen and high frame rate

2026-11-09. **Investigation and rejected prototypes delivered; no production
widescreen/HFR enhancement is recommended for integration.** Combat/floors run
at ~30 Hz while attract runs at ~60 Hz. Wider rendering exposes incomplete
stage coverage and changed culling changes guest state. Conservative HFR has
zero useful moving-frame coverage in the measured runs. These directions were
stopped at those fidelity limits, as permitted by the task; they are not passed
acceptance criteria or silently disabled “completed” features.

Started against HEAD `8852d7f`. This is a snapshot job:
`work/p420/runtime-base/` and `port-base/` retain the runtime (including already
applied patches) and port copied once at the start; `saturn-recomp/` and `port/`
are the independent experiment copies. Source identities are in
`work/p420/snapshot.json`. Concurrent P4.19 edits are outside this experiment.
No existing file under `port/` or `tools/saturn-recomp/` is edited here.

## Phase A: findings recorded before compiling a prototype

Read STATE, architecture, P4.11/14/15/16, convergence round 4 and runtime-patch
instructions. The existing executable copied with the snapshot is being used
for the initial measurements; no prototype has yet been built.

The main loop is `game_boot_and_run` (02201020), task update 0220fde4,
collision sweep 0222ece4, both view transforms 022225f6, priority fill 022227c4,
and pulse/gate wait 02223c74. `060987bc` counts game-loop pulse calls, while
`0609869c` counts VBlank-outs since the last gate. They are different clocks.
The ISR 02223cf0 requests a manual frame change (FBCR=3) when the display-ready
gate is set; 02223d98 increments the interrupt count and commits the TV mode.
The retained original runtime already contains a default-off interpolation
path, but it uses nearest-signature matching and propagates motion to unmatched
faces. Its 512-column intermediate buffer cannot represent this game's
704-column 8-bit display. It must not simply be exposed as a finished feature.

Projection uses a 256 focal numerator, doubled horizontal results in
`vertex_project_dualmtx` 022135a0 and sprite projection 022139b8. A Hor+ view
keeps this focal length and vertical framing and adds columns on both sides;
shrinking every projected x would squeeze the original view. Frame headers
02215000 set user/system clip to 0..703 by 0..239 and local origin (352,120).
The software quad test 02213e0c rejects outside ±352,±120 (on its low-depth
branch); model submit 02215a2c separately checks z±x and z±y against -0x18000
and z against 0x1999 before enqueueing geometry. Renderer-only expansion
cannot recover objects discarded there. Existing 1024-byte-row VDP1 hardware
buffers must remain unchanged, and a 21:9 companion exceeds that row width.
A 16:9 companion can use 940 columns (118 new per side); 21:9 uses 1232 (264).
The 16:9 source-column rounding is 0.14%; host presentation should use the
requested physical aspect, preserving the original anamorphic dot shape.

VDP2 normal planes are 2×2 planes, each 1/2 pages of 512 dots (1024..2048-dot
maps); RBG maps are 4×4 planes, 2048..4096 dots before transformation. Larger
addressable maps do not prove that offscreen tiles were authored. Scroll,
line-scroll, zoom, wrapping, RBG coefficient tables and screen-over modes must
sample signed original-space x (output x minus left pad), retaining original y.
The normal and rotation layer samplers already support signed coordinates.
Windows need the same original-space interpretation. HUD text uses tilemaps
(02222fe0/02223080), while meters/counters also use sprite/deferred geometry
(02237828/02237be4/0223df30). NBG3-only movement is not a complete HUD policy.
Keep the original 4:3 HUD centered first; edge anchoring needs semantic
identification of both text and sprite pieces, including P2 join prompts.

Risks to check: preprojection model/sprite/quad rejection and edge pop-in;
missing stage geometry outside the authored view; enemies becoming visible
before their original entry; spawn, camera horizontal_range, scrolling and
stage triggers remaining tied to the original view; fixed-coordinate clears,
subtitles, QTE prompts, effects and menus; tilemap wrap into unused/other art;
RBG floor/VDP1 seam alignment; hardware framebuffer reads and CPU writes;
interlace/8-bit modes; draw-time texture changes; command count/order changes,
identical-signature meshes and sprite animation during interpolation; cuts,
fades, palette changes and independent HUD timing; state restore/rewind,
rollback, cache invalidation, input sampling and audio latency.

A game-side culling change writes different command buffers and performs more
work at the original safe points. In this runtime virtual time advances at
those polls, so keeping AI/spawn/scroll code untouched alone does not prove
identical gameplay. Widescreen with changed culling cannot be advertised as
presentation-only or mixed-settings netplay. A production implementation would
need equal peer settings in the handshake, or a separate read-only rendering
simulation. The experiment will reject widescreen during netplay rather than
silently allow divergent peers. No network test is needed to establish this
structural restriction.

Initial measurement commands/results and the build decision follow below.

Initial measurements completed **before the first prototype compile**. Source:
`work/p420/phase-a/{attract,fight,floor,intro,boss}/report.json`; exact commands,
all semantic records and timestamped draw/swap events are retained. Trace mode
preserves the original polls (idle batching is disabled), so these are virtual
cadence measurements, not host-throughput benchmarks. In stable windows:

| Route | Fields | Draws / swaps | Main-loop tick delta | Cadence |
|---|---:|---:|---:|---|
| No-coin attract, Scene 2, VB901..1200 | 300 | 300 / 300 | 299 | one frame/field (~59.83 Hz) |
| First combat, Scene 3, VB901..1200 | 300 | 150 / 150 | 150 | one frame/2 fields (~29.91 Hz) |
| Floor fixture, Scene 9, VB901..1200 | 300 | 150 / 150 | 149 | one frame/2 fields |
| Scripted route starting at 13, now Scene 15, VB901..1200 | 300 | 150 / 150 | 149 | one frame/2 fields |
| Scripted intro, Scene 13, VB651..700 | 50 | 25 / 25 | 24 | one frame/2 fields |
| Scene 72 fixture, VB901..1200 | 300 | 150 / 150 | 149 | one frame/2 fields |

Tick deltas exclude the first sample; 149/150 is the boundary phase of a
150-update interval, not a measured slowdown. Resource transitions add occasional
longer intervals (e.g. 148 swaps in a later 300-field window). Therefore gameplay
HFR means **30→60**, while attract already needs **60→120/144** to improve motion.
The runtime field period is 16,715,228 ns (263×63,556), not exactly 1/60 s.
The direct-start fixtures are labelled training scene selections; they do not
prove a natural full-game route or that every cinematic has the same cadence.

Build decision: prototype a separate read-only command recorder and private
8-bit-dot raster with signed wider sampling. Retain actual draw-time textures,
clips and local coordinates. Require stable order/attributes/counts, bounded
motion and unique nearest correspondence for moving commands; otherwise show
the real frame. Reject changing VDP2 backgrounds/palettes for interpolation
rather than move actors against a stepped floor. Use an interval of latency
between known endpoints; never extrapolate or change guest simulation speed.
Wider culling is a separate, explicitly state-changing generated-code experiment.
Both remain experimental until image/state/performance checks are assessed.

## Phase B: what was built, and why it is rejected

The pre-build findings are preserved separately in
`work/p420/phase-a-findings.md`. The final executable is
`work/p420/build-final/saturn_stv`; the first attempt remains in `build/`.
Two incremental patches reproduce the edits against the frozen snapshot:
`port/runtime-patches/p420-runtime-prototype.patch` and
`port/runtime-patches/p420-harness-prototype.patch`. New `port/p420_*` sidecars
supply the experiment. Nothing is applied to the live runtime or existing port.

The private VDP1 recorder receives only commands executed by the authoritative
walk, plus each draw's initial clip/local state and a 512 KiB draw-time texture
copy. Both framebuffer histories follow hardware erase/swap. Replay uses a
separate raster state, pixel buffer, stride and step counter. Its dots are
sampled only by the compositor; bus reads, hardware buffers, registers, command
budgets, interrupts and guest state never read those dots. Restore invalidates
the histories. The default branch bypasses allocation, replay and extra output.
This prototype is 1x, non-interlaced ST-V only; combining enhanced resolution,
CRT, other Saturn modes or launcher setting changes is not qualified.

Widescreen shifts the local origin by 118/264 dots, widens the private system
clip, translates the user clip, and replays into a 940/1232-column buffer.
VDP2 samples original-space signed x for normal and rotation planes and windows;
the output host uses 16:9/21:9. Original projection/focal scale and vertical
framing are retained. It is a Hor+ canvas, not an image stretch. Hardware memory
and its original clip remain authoritative and unchanged.

`port/p420_culling.py` separately fingerprints the **entire frozen generated
p_dh_004.cpp** and changes just four emitted expressions: the ±352 screen
bounds in `f_06017E0C` and the x terms in the two z±x tests in `f_06019A2C`.
The latter scale x by 704/new_width for the visibility comparison. The original
poll locations and continuation IDs remain. No `project/` source, recompiler,
AI, wave/spawn script, camera-tracking range or stage-trigger code is edited.
This broadens two proven rejection sites, not a claim to have found every
content-specific visibility gate.

**Wider culling is not presentation-only.** A fixed recorded input stream
produces different unmasked RAM from VB641 onward, different CPU/machine/VDP1
snapshot parts and different hardware command/framebuffer dumps. All 2,100
actor/scene semantic records and the WAV still match in this short fixture.
That is useful scoped evidence, not proof of future RNG, input, spawn or camera
equivalence. More offscreen objects are submitted, and originally offscreen
enemies can be seen earlier. Such a production option must be **forced equal
on both peers** in the netplay handshake, including any stage-specific policy.
The experimental harness rejects P4.20 controls with CLI host/join arguments;
no mixed-settings netplay, launcher/netplay integration or live peer test is
claimed. The existing P4.19 job's networking/audio work was not modified.

Without changed culling, wider presentation is guest-identical in the measured
run, but it cannot recover rejected objects. That mode is not offered as a
successful widescreen feature either. The experiment keeps HUD geometry in the
centered original rectangle. `STV_P420_HUD=edges` only experiments with splitting
NBG3 sampling between the left and right edges; it does **not** identify/move
VDP1 meter, portrait, prompt or subtitle commands. A complete edge-anchored HUD
option was stopped, not shipped or claimed. NBG3 is a layer, not a semantic HUD
identifier, so that control must not be exposed to players as a finished policy.

HFR matches commands by draw/list order, type, texture, size, draw attributes,
colour and Gouraud fields. Texture/LUT/Gouraud bytes are checked against the
actual draw-time copies. Counts/control/local changes reject the whole pair;
moving commands require bounded displacement and a uniquely closest previous
command of those attributes. Normal/scaled sprite width/height fields are not
mistaken for moving vertices. Ambiguous or large moves fall back to real output,
including at cuts. There is no nearest-neighbour motion propagation into
unmatched geometry and no extrapolation. A synthetic production-raster test
proves a 40→50-dot translation renders its midpoint at 45, while hardware and
all serialized VDP1 bytes remain unchanged; count and identity ambiguity fall
back. See `work/p420/unit-final.log`.

The presentation scheduler supports requested 60/120/144 samples/s on the host,
using the measured field interval and fractional sample times. Between two
known game frames it uses one frame interval of latency: roughly 33.4 ms in
combat, 16.7 ms for 60-Hz content. It does not advance guest time, audio or input
logic to manufacture frames. Headless captures generate the same fractional
samples without waiting. No actual high-refresh display was available; target
selection is an experimental environment control, not display negotiation.
Extra host presentation/event/audio servicing and real display pacing still
need dedicated validation if this design is revisited.

**HFR was stopped for lack of useful coverage.** The first attempt compared all
VDP2 VRAM/register bytes and produced no moving interpolation. The final
attempt compares the actual fully rendered sprite-free background plus CRAM
at successive real frames, avoiding rejection just for unused VRAM writes.
It still produces **zero moved coordinates** in each 2,100-field 60/120/144
first-fight run. The reported 294/883/1002 matched compositions are static
startup pictures, not moving gameplay. All **90 captured subfields** across
attract/fight/floor/boss/intro have `matched=0`. They are real-frame fallbacks.
All 45 primary images and 90 subfield PNGs also equal their real-frame originals
(`work/p420/fallback-equality.json`). The synthetic midpoint passes, but no successful real-game interpolated pair
is claimed or supplied under a misleading caption.

A moving RBG/normal background cannot stay at the real-frame endpoint while
VDP1 actors/geometry move through a delayed midpoint without risking visible
floor/actor separation. Removing that guard would conceal this unsolved layer.
The bounded next step is coordinated read-only interpolation of actual VDP2
rotation/scroll/line/coefficient state at the same alpha, with cuts, palette and
HUD policies, then stable object/mesh command keys where order is ambiguous.
The inherited generic interpolation path is not a shortcut: its 512-column
buffer and unmatched-face heuristics do not establish correctness for this game.

## Screenshots and visual decision

Sheets below are **4:3 / 16:9 / 21:9**, all at 1x. The original center view has
the same physical scale in each panel. Raw images, exact argv/options, logs and
hashes remain under `work/p420/visual/{scene}/{original,wide,ultrawide,hfr}/`.
The floor/boss/intro images are direct-start training fixtures (Scenes 9/72/13),
not newly played full-route evidence. Their INIs are retained. Widescreen
pictures use the widened culling experiment; HFR captures use the final build.
The wide/original images precede only the background rejection diagnostic
change, which is inactive when HFR is off.

![Attract, VB1200](../../work/p420/visual/attract-comparison.png)

Attract: new columns reveal additional/repeated city art. The helicopter and
subtitles stay centered at the original scale. This view alone looks plausible;
tilemap wrap is visible and is not proof that every stage has valid extra art.

![First fight, VB1800](../../work/p420/visual/fight-comparison.png)

First fight: wider floor sampling works, but the building wall ends at the old
view boundary and the city backdrop is revealed around it. A renderer clip/FOV
change does not supply the missing wall. Stretching it or painting replacement
art would invent content. This is a rejection, not an “obvious problem” left
unnoticed in an accepted feature. The white HUD rectangle exists at 4:3 too.

![Floor, Scene 9 VB1200](../../work/p420/visual/floor-comparison.png)

Floor: the extra sampled RBG area contains coherent floor motifs, but the room
ends expose large black corners. Tiles being addressable is insufficient to
make the wider view complete. A room/stage coverage policy or authored extension
would be needed. No obvious random tile garbage is established in these five
views; unused/overlapping art elsewhere remains unqualified.

![Roof boss fixture, Scene 72 VB1200](../../work/p420/visual/boss-comparison.png)

Boss fixture: the repeating roof floor extends relatively well; the wider view
reveals further roof edges. This one scene cannot justify enabling all stages.
The original HUD remains centered and is not relabelled as edge-anchored.

![Scripted intro, Scene 13 VB680](../../work/p420/visual/cutscene-comparison.png)

Intro: screen-fixed green brackets, prompts and lettering remain in the old
center rectangle. Moving the entire sprite layer to the edges would also move
world geometry; identifying the authored UI pieces is necessary.

![First fight subfields, real fallback](../../work/p420/visual/fight-hfr-samples.png)
![Floor subfields, real fallback](../../work/p420/visual/floor-hfr-samples.png)
![Attract subfields, real fallback](../../work/p420/visual/attract-hfr-samples.png)

Opened and inspected all five aspect sheets, individual wider fight/floor/boss
images, and the attract/fight/floor subfield sheets. Their first two subfields
are identical fallbacks; the third shows the next real field. The actor/floor
and rotor motion still step. These are failure evidence, not successful HFR
comparisons. No frame blending, invented geometry or relaxed matching was added
to hide those outcomes.

## Verification

Final default oracle: `work/p420/regression-final.log` and
`work/p420/regression.json`: **56/56 original P4.11 PNGs, four full traces,
four guest traces and four WAVs are byte-identical**. The no-coin schedule runs
9,000 fields. The PM's separate default paid-input VB1900 smoke also gives
**MD5 `9bd5386ea7439621c19674381086e8fd`**, in `work/p420/smoke-final/` (also repeated in the first build).
The first default regression also passes; nothing was rebaselined.

Final `work/p420/authority.json` compares a recorded physical-input stream,
replayed separately with default/60/120/144/wider-presentation/wider-culling.
Every field retains SHA-256 of both full 1 MiB RAM banks and the complete
semantic record. Only transport width/height are excluded from semantic
comparison because they describe presentation dimensions. No RAM bytes, actors,
RNG, counters or state parts are masked. A quick state at VB2000 compares all
**22 machine-state parts byte for byte**, excluding only the file's build/header
identity, not any device payload. Hardware dumps at VB1800/2100 and WAVs are
also compared.

| Option | Equal RAM/semantic fields | Equal state parts | Hardware dumps | WAV |
|---|---:|---:|---|---|
| HFR 60 | 2100/2100 | 22/22 | both equal | equal |
| HFR 120 | 2100/2100 | 22/22 | both equal | equal |
| HFR 144 | 2100/2100 | 22/22 | both equal | equal |
| Wider presentation, original culling | 2100/2100 | 22/22 | both equal | equal |
| Wider presentation + wider culling | 640/2100 | 19/22 | both differ | equal |

For wider culling, actor/scene semantics alone match 2100/2100; RAM does not.
HFR's state isolation is established even though its moving-frame acceptance is
not. No useful interpolation claim follows from unchanged guest state alone.

Existing runtime suite: **169 tests, zero failed**, `work/p420/tests.log`.
New midpoint/private-state/ambiguity probe: PASS, `work/p420/unit-final.log`.
Both patches reconstruct exactly with **zero fuzz and zero offsets**;
`work/p420/patches.json`. No tests, sources or generated files in the live
runtime/port/decomp trees were changed by this job.

## 1x performance and limitations

AMD Ryzen Threadripper PRO 5975WX (32 cores / 64 logical CPUs), shared host.
All this job's builds/correctness/capture runs finished before these sequential
measurements. Each runs 2,100 fields with identical physical autoplay, SDL dummy
video/audio and the **software** renderer, Sharp, frameskip off, unpaced, 1x.
GNU time supplies CPU totals; 100% is one logical CPU. These are single-pass,
whole-run measurements including startup, not isolated steady-combat or Mac
numbers. Exact commands, logs and CPU reports: `work/p420/performance.json`,
`performance-summary.json` and `performance/{option}/`. No GPU is used.

| Experiment | Guest fields/s | Process CPU | CPU ms/guest field |
|---|---:|---:|---:|
| Default | 163.82 | 210% | 12.81 |
| Wider presentation, original culling | 84.59 | 315% | 37.28 |
| HFR target 60 | 123.84 | 295% | 23.81 |
| HFR target 120 | 78.12 | 365% | 46.73 |
| HFR target 144 | 70.04 | 374% | 53.37 |

**Guest throughput is not distinct moving-picture throughput.** In particular,
120/144 target sampling repeats real pictures after rejection; these results
must not be advertised as successful 120/144 fps motion or high-refresh-display
validation. Repeated composition, background validation, copies and replay cost
CPU even when a frame is rejected. Unpaced dummy audio has its ordinary queue
resynchronizations; these measurements are not speaker/latency tests. Guest WAV
identity is established separately above. There is no new performance target
pass for these rejected features, and no Mac/Windows/GPU/display test claim.

## PM reproduction and integration decision

**Keep the feature defaults unchanged and do not merge these prototypes as
player options.** The useful deliverable is the measured mixed cadence, the
culling/state boundary, the inspected content limits, and the reproducible
private-raster/HFR failure evidence. Stage coverage and synchronized VDP2/HUD
interpolation are new work, not one more setting to expose.

The copied source tree is pinned in `work/p420/snapshot.json`; current live
P4.19/P4.21 work is not a replacement baseline. The two patches are experiments
against that frozen input, not a patch chain to apply blindly after P4.19.
`port/p420_export.py` reconstructs all eight changed runtime/harness files from
that baseline into `work/p420/patch-check/` with zero fuzz/offset and exact bytes.
The generated-code change is reproducible through `port/p420_culling.py`, not
an edit to the shared generated program or a production C override.

For a **fresh copy of the frozen baseline** laid out like `work/p420/`, apply
once (the current experiment already has these applied):

```bash
patch --batch --fuzz=0 -p1 -d work/p420/saturn-recomp < port/runtime-patches/p420-runtime-prototype.patch
patch --batch --fuzz=0 -p1 -d work/p420 < port/runtime-patches/p420-harness-prototype.patch
python3 port/p420_culling.py work/p420/port/run/P41/recomp
bash port/p420_build.sh
```

The builder copies only this job's new sidecars and builds the snapshot. It
requires the existing frozen SDL dependency and never downloads anything.
Reproduction of evidence, from the repository root:

```bash
python3 port/p420_verify.py regression
python3 port/p420_verify.py authority
python3 port/p420_verify.py visual
python3 port/p420_sheets.py
python3 port/p420_verify.py performance
work/p420/build-final/p420_video_test
python3 port/p420_export.py
```

The verifier reuses `work/p411/regression/after/manifest.json` as the unchanged
oracle and `work/p415/fixed-time.so` for the measured RTC origin. Frozen sources,
phase-A data, build logs, first-attempt evidence and the final artifacts remain
under `work/p420/`. `artifacts.json` pins the final binary, sidecars, patches and
reports. Final binary SHA-256:
`ef16905d2161766c11a79388067786be6c342cd5acb3ee643ece827d936d55ef`.

Experimental controls (only for reviewing this rejected snapshot):
`STV_P420_ASPECT=16:9|21:9`, `STV_P420_CULL=1`, `STV_P420_HZ=60|120|144`,
`STV_P420_CAPTURE=FIRST:LAST`, `STV_P420_HUD=edges`. Use explicit
`--internal-resolution 1`; saved higher-resolution/launcher combinations are
not qualified. An unset control preserves the original behavior. Ordinary
`--shots` captures primary fields; `sample-VB-SLOT.png` captures requested
subfields. CLI netplay rejection is demonstrated by `netplay-reject.log`, before
any connection attempt. No network operation was needed or performed.

If revisited after P4.19, first rebase only the private recorder/raster on the
final runtime and repeat default/state checks. Then solve VDP2 temporal sampling
and semantic HUD/mesh identities with real cuts/animation fixtures before
reconsidering HFR. For widescreen, inventory each stage's authored coverage and
all draw rejection gates; retain original spawn/AI/trigger logic, explicitly
accept early enemy visibility, and require equal peer settings if authoritative
culling is changed. A separate rendering-only scene pass could avoid that state
change, but is not implemented here. Do not promote presentation-only overscan
with known missing geometry as the requested complete widescreen mode.

No existing `port/` or `tools/saturn-recomp/` file was edited by this job. No
`project/`, `tools/decomp`, commit, sudo, network, GPU, prohibited directory or
unrelated process termination was used. Other jobs' visible working-tree changes
remain theirs. Only the requested report, new `port/p420_*`, new P4.20 patches
and `work/p420/` contain this job's output.

1. Froze the current applied runtime/port once; shared P4.19 sources stayed untouched.
2. Recorded Phase A findings before compiling the snapshot prototype.
3. Measured ~60 Hz attract and ~30 Hz combat, floors, intro and boss fixtures.
4. Built private wider buffers, signed VDP2 sampling and narrow horizontal culling experiments.
5. Wider culling diverges in RAM at VB641; equal peer settings would be required.
6. Inspected all requested scene comparisons; incomplete walls/rooms reject general widescreen.
7. Built isolated command interpolation and fractional host sampling; synthetic midpoint passes.
8. Real moving interpolation coverage is zero; all 90 captured subfields fall back honestly.
9. Defaults match 56 PNGs, all traces/WAVs, the required MD5; 169 runtime tests pass.
10. Delivered reproducible patches/evidence; neither enhancement is accepted for production integration.
