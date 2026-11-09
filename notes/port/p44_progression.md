# P4.4: fan verdict, two missing recompiled entries, and progression to Scene 51

2026-11-06. Tested in this workspace on the build host. No commit made.
Every run uses `STV_BOOTMODE=0 STV_CFG798=40` with physical coin/START input and no
bridge knobs. No actor, HP, scene, progress, descriptor, director variable or VDP
state was forced.

Summary:

- **Fans (P4.3 open question):** they are real, live foreground ceiling fans. The game
  issues fresh VDP1 commands for them on every 30 fps frame, and they rotate. They are
  drawn opaque because VDP2 sprite colour calculation is off. Not a bug.
- **First gate past Scene 9: a missing recompiled entry.** At VB 7594, during the
  Scene 9 fight, the run died with `FATAL call to 060359B2, not an entry of an active
  module`. The game installs task step `func_022319b2` through a data pointer, and
  static call discovery never reached it. Fix: seed it. All 51 other
  `project/src_functional` functions missing from the generated table were seeded too,
  since they are the same class of gap.
- **Second gate: also a missing entry.** At VB 45519, in Scene 21, the run died with
  `FATAL call to 0604E928`. This soft-float helper is reached only through
  `mov.l @(pool),r0 … bsrf r0`, a pattern the recompiler's bsrf resolver does not
  follow. Fix: seed it. A scan of the generated code finds no other unresolved bsrf
  target of this kind.
- **Progression harness:** `STV_AUTOPLAY` is a read-only scripted player. It is off by
  default. It reads the actor slots, drives only the P1 raw panel word, and buys
  continues with COIN + START when `CONTINUE?` is on screen. Using it, P1 got from
  Scene 9 to **Scene 51** (entered at VB 82937, still in live combat when the 90000-VBlank run ended; 25 continues bought, 0 FATAL).
- **Sound plays during a round.** The SCSP gets 50–70 key-ons per 100 VBlanks across
  varied samples, and the round audio is non-silent.
- All regressions are byte-identical to P4.3: the P4.2 combat check, the paired
  reference, the no-coin attract run, the P4.2 4200-VBlank progression and the P4.3
  7500-VBlank progression (traces, PNGs and dumps). The runtime tests pass:
  `126 tests, 0 failed`.

## 1. Verdict on the brown shapes in the second area (Scene 5, VB ~4200)

**Verdict:** the brown shapes are two live three-blade ceiling fans in the foreground.
They are authored that way, not stale VDP1 data. In `port/run/P44/fans/shot-4190.png`
… `shot-4205.png` (side by side in `port/run/P44/fans/montage-4190-4200.png`) the
blades change angle every game frame, pivoting about two fixed hubs. The VDP1 dumps
for every VBlank from 4190 to 4205 show six commands with texture `srca = 0x3BD80`
(24x24, 8 bpp bank, `PMOD = 0x04A0`, distorted sprite `CTRL = 0x0032`). Their vertex D
is pinned at the hub, local `(100,-31)` for one fan and `(-300,-31)` for the other,
while vertices A–C rotate. The game re-sorts and rewrites them on every swapping VBlank
(even VBs change, odd VBs repeat, which is 30 fps manual frame change). Their command
slots move between `cmd[80..91]` from frame to frame, so these are fresh per-frame
depth-sorted commands, not a stale table (`port/run/P44/fans/fan-cmds.txt`,
`fan-freshness.txt`). They are opaque on purpose: the game's own VDP2 registers have
`SPCTL = 0x000F` (sprite type F), `CCCTL = 0x000A` (only NBG1/NBG3 colour calculation,
`SPCCEN` off) and `PRISA = 0x0505`. So the hardware also draws the blades as solid
priority-5 sprites over the room, and there is nothing to fix.

## 2. Root cause 1: data-pointer task step `func_022319b2` was never recompiled

`port/run/P44/auto1.log` (the first run past Scene 9's opening) ended at VB 7594:

```
FATAL] call to 060359B2, not an entry of an active module (from pr 06013E0E)
```

`0x060359B2` is cart `0x022319b2` (`project/src_functional/022319b2.c`), an
attach/projectile task step. Like `0x0223194a` and `0x02231a1c`, it is installed by
writing its address into a task (`*arg0 = …`), never by a direct call, so the
recompiler's call-graph discovery never generated it. It is not in `notes/symbols.txt`,
so `port/p41_build.py` did not seed it either. It runs only once Scene 9 enemies use
the move that spawns that attached object.

Fix (`port/seeds_extra.txt`, P4.4 block 1): seed `060359B2`. Every other
`project/src_functional/<addr>.c` function absent from
`port/run/P41/recomp/p_dh_table.cpp` (51 in total, the same class) was seeded at the
same time, so this gap does not stop the run again function by function. Rebuilt with
`python3 port/p41_build.py`.

## 3. Root cause 2: an unresolved `bsrf r0` soft-float helper

With root cause 1 fixed, the run cleared Scene 9 and continued through Scenes 10–21.
At VB 45519 (Scene 21, the washroom fight) it stopped
(`port/run/P44/auto5.log`):

```
FATAL] call to 0604E928, not an entry of an active module (from pr 06014C4C)
```

`0x0604E928` is cart `0x0224a928`, a register-saving soft-float helper next to `itod`
(`0x0224a830`). Its only callers, at `06014C48`, `06014E50` and `06014EF8`, load the
displacement into `r0` from the literal pool and then reach the helper through
`bsrf r0`, with unrelated `r2`/`r3` writes in between. The generated code
(`port/run/P41/recomp/p_dh_003.cpp`, line for `06014C48`) shows the recompiler left
this bsrf unresolved (`sh2_call(c, x)` with no `if (x == …)` arm). The neighbouring
`bsrf r3` and `bsrf r2` sites were resolved.

Fix (`port/seeds_extra.txt`, P4.4 block 2): seed `0604E928`.
`port/run/P44/tools/scan_unresolved_bsrf.py` lists every bsrf/braf whose register
comes from a pool constant within a few instructions, whose target is in cart code,
and which is not a table entry. Before the fix it listed only `0604E928`; the other
hits are in the cart header's data, decoded as code at `0x06004xxx`, and one braf
already resolved as a local label. A second scan of 4-byte literals that point at
plausible prologues found one candidate, `06040200`, referenced only from data tables
near `0x0604E344`/`0x0604EB04`. It was not seeded.

Both fixes stay inside the existing faithful-coverage convention: no recompiler or
runtime behaviour changed. If this bsrf pattern keeps appearing, the general fix is to
extend the recompiler's bsrf constant tracking to follow `r0` across non-aliasing
instructions (`tools/saturn-recomp/src/saturnrecomp`).

Stopping rule: the first fix unblocked twelve scenes (9 → 21) before the second gate
appeared, and the second fix unblocked Scenes 21 → 51 with no further gate inside 90000 VBlanks. Neither fix only exposed a deeper
gate, so the rule was not triggered.

## 4. Progression harness: `STV_AUTOPLAY` (`p44-autoplay-harness.patch`)

Fixed timelines broke down at Scene 9 because P1 (HP 12) faces three enemies with HP
92–113 and dies, then the game shows GAME OVER. Death by itself is normal game
behaviour, not an emulation gate. To keep progression reproducible without
hand-tuning thousands of VBlanks, `runtime/src/machine/bios.cpp` gained a default-off
scripted player:

- `STV_AUTOPLAY=VB`: from VBlank `VB`, whenever the `--stvinput` timeline holds no P1
  control, it chooses the P1 raw panel word (`0x06000708`). The cart's own cook
  (`func_0220f9d4`) consumes that word exactly as for the timeline tokens.
- It reads only what the `STV_P41TRACE` diagnostics already read: the actor slots at
  `0x060995B0+slot*4` (`ent+8` X, `ent+0x10` Z, `ent+0x56` HP). From these it walks
  toward the nearest live enemy, faces it, and alternates A/B. With no enemy on screen
  it taps A/B for QTE prompts and cut scenes.
- When the P1 credit state byte `0x06098674` becomes 3 (the `CONTINUE?` countdown), it
  queues the same COIN and START1 events a timeline would use. The coin goes through
  the faithful coin-detect gate in `deliver_coin`, and the game accrues the credit
  itself.
- World units are pixels << 12 (wave spawn X 160 = `0xA0000`). The direction mapping
  was measured in Scene 9 (`port/run/P44/dirprobe2.log`): RIGHT = −X, LEFT = +X,
  UP = −Z, DOWN = +Z, and diagonals are additive.

It never writes game state. Unset, the binary is byte-identical in behaviour, as all
regression traces and PNGs below confirm.

Side finding about coins: the gate in `deliver_coin` (credit byte 0 or 3) means a coin
inserted while P1 is alive is ignored, and logged as such. Only the `CONTINUE?` window
accepts a P1 coin, which matches the game's coin-detect routine.

## 5. Scene log of the furthest run (`port/run/P44/auto7`)

Director scene index (`SCENETRACE`, `g_06098668`) → first VBlank, from `port/run/P44/auto7.log` (90000 VBlanks, 0 FATAL):

| Scene | VB | | Scene | VB | | Scene | VB |
|---:|---:|---|---:|---:|---|---:|---:|
| 1 | 9 | | 18 | 33650 | | 35 | 65782 |
| 2 | 356 | | 19 | 34145 | | 36 | 66195 |
| 3 | 1179 | | 20 | 36286 | | 37 | 66254 |
| 4 | 3944 | | 21 | 36345 | | 38 | 70137 |
| 5 | 4003 | | 22 | 46894 | | 39 | 70190 |
| 6 | 6368 | | 24 | 47343 | | 40 | 70257 |
| 8 | 6945 | | 25 | 47410 | | 41 | 70318 |
| 9 | 7020 | | 26 | 51179 | | 42 | 70377 |
| 10 | 13175 | | 27 | 51238 | | 43 | 70450 |
| 11 | 13226 | | 28 | 51291 | | 44 | 70545 |
| 12 | 13285 | | 29 | 51350 | | 45 | 78596 |
| 13 | 13342 | | 30 | 51399 | | 47 | 79279 |
| 14 | 13414 | | 31 | 51466 | | 48 | 79342 |
| 15 | 13473 | | 32 | 51517 | | 49 | 82705 |
| 16 | 20366 | | 33 | 58876 | | 50 | 82774 |
| 17 | 20979 | | 34 | 58933 | | 51 | 82937 |

Continues bought by `STV_AUTOPLAY` (VB of `CONTINUE?`): 7585, 9923, 12781, 17588, 21652, 26722, 31512, 37114, 40154, 44234, 46464, 49481, 53992, 56830, 61292, 64418, 68011, 71284, 73436, 77046, 80909, 84308, 86090, 87856, 89974. Every one was accepted (P1 credit state 3 → 2 within 4 VBlanks).

What the scenes are, from the shots: 9–15 lobby, then the "WAY TO NEXT" map and a loading dock with weapon pickups. 16 is a `CAUTION`/`パンチ` QTE. 17 is a car park with firemen and a fire engine. 18–20 are a QTE (`FAILURE` branch) and a corridor. 21 is a washroom. 25 is a server room with spider-walker robots. 26–31 are short transitions (elevator). 32 is a pink lobby with SWAT enemies. 34 is a purple pool hall. 35 is a QTE. 37 is a green-floored office. 38–43 are transitions to the rooftop. 44 is the rooftop fight. 45 has yellow walker mechs on the rooftop. 47–51 are a glass rooftop pavilion fight. Scenes 7, 23 and 46 were skipped by the director on this route; they are probably QTE-success or alternate branches.

## 6. Sound during a round

`port/run/P44/sound/combat.wav` comes from the P4.2 combat input, 2100 VBlanks
(`port/run/P44/sound.log`). It is 44.1 kHz stereo, silent only during the first boot
second and the logo gap. It is continuously non-silent from VB ~360. During the round
(VB 1320–2100) the RMS is 296–585, peaks reach 1400–2800, and zero crossings run
1800–3600/s.
`--trace` run `port/run/P44/soundtrace.log` (1700 VBlanks) counts SCSP key-ons per 100
VBlanks: 50–103 in every bucket from VB 300 to 1700, using 9–16 distinct sample
addresses per bucket, with looping and one-shot voices. That is consistent with the
music sequencer plus effects. The long runs report about 97% non-silent samples
(`auto4`: 17.2M of 17.7M). **Verdict: sound runs in rounds.** The 68000 driver keys
voices from game commands for the whole round. Not verified: whether it sounds right
(no audition is possible headless), and the SCSP DSP effect path (every key-on seen
has `EFSDL 0`).

## 7. Visual and audio defects seen along the way (listed, not fixed)

1. **After GAME OVER the screen stays black forever** (`port/run/P44/explore1`,
   `shot-8800.png` … `shot-14000.png`). At VB 8453 the director object handler becomes
   `noop_handler` (`exec = 06005B8E`), meaning `boot_scene_director` (`022016ac.c`)
   state 4 finished. With DIP bit 5 clear, it calls BIOS vector `0x0600060C` and parks
   in state 13, handing control back to the ST-V BIOS. `bios.cpp` treats `0x0600060C`
   as a no-op, so nothing restarts the attract loop. This is a real BIOS-emulation gap
   for the coin-to-coin arcade loop. It does not block progression while continues are
   bought.
2. **The attract mode never leaves the title screen** (`port/run/P44/nocoin-long`, 8000
   VBlanks, `scene=1` throughout: "SELECT GAME WITH THE SELECT BUTTON / INSERT
   COIN(S)"). There is no demo play or ranking cycle. It is probably the same BIOS
   hand-off family as item 1.
3. **Black, undrawn floor regions** in interior rooms. Scene 9 lobby: right and bottom
   (`port/run/P43/final-stage-clear/shot-7400.png`). Scene 21 washroom: large black
   areas right of the tiled floor (`port/run/P44/auto5/shot-36400.png` …
   `shot-41200.png`). Scene 9/10: a white wedge at the frame bottom
   (`port/run/P44/auto4/shot-13000.png`). Scene 25 server room and Scene 32: black
   outside the floor polygons (`port/run/P44/auto7/shot-48500.png`, `shot-52500.png`). This continues P4.3 "what is next" item 1
   (RBG0 floor coverage, screen-over mode 1 / back-screen).
4. **Top band shows white backdrop** above the fans and walls in Scene 5
   (`port/run/P44/fans/shot-4200.png`), most likely the same RBG0/back-screen coverage
   issue.
5. **The QTE prompt is in Japanese (`パンチ`)** on what should be the US
   ("Die Hard Arcade") cart (`port/run/P44/auto4/shot-20800.png`, P4.3
   `shot-6600.png`). Other prompts are English (`CAUTION`, `SUCCESS`, `FAILURE`). It
   may be authored, or it may come from a region/area-code value the HLE BIOS reports.
   To verify against MAME.
6. Audio: no audible-quality check was possible. The SCSP DSP effect send is unused in
   all observed key-ons.

## 8. Reproduce from the repository root

Patch order against upstream `b22688d0bef52fba31cda79fd23150e8905cf3c2`:
`round-runtime-dh-portB.patch`, `p41-raw-panel-input.patch`, `p41-player-trace.patch`,
`p42-rbg-map-registers.patch`, `p42-dmac-block-count.patch` (applies with `patch -p1`;
`git apply` rejects its new-file header outside a git tree),
`p42-remove-retired-bridges.patch`, `p43-vdp1-vbe-erase.patch`,
`p44-autoplay-harness.patch` (all under `port/runtime-patches/`). Reconstructing all
13 touched runtime files from this chain matches the installed sources byte for byte
(`port/run/P44/patch-verification.log`). The two missing-entry fixes are in
`port/seeds_extra.txt` (the two P4.4 blocks at the end).

```bash
python3 port/p41_build.py > port/run/P44/build2.log 2>&1
cmake -S tools/saturn-recomp/runtime/tests -B port/run/P44/runtime-tests -DSATURN_NO_SDL=ON
cmake --build port/run/P44/runtime-tests -j8
port/run/P44/runtime-tests/runtime_tests > port/run/P44/tests.log 2>&1   # 126 tests, 0 failed

. port/run/P44/env.sh      # unsets STV_*, exports STV_BOOTMODE=0 STV_CFG798=40 + traces, sets BIN

# Fan evidence (Scene 5, consecutive VBlanks, PNG + VDP dumps)
$BIN --cart port/diehard_main.bin --cartrom cart.bin --vblanks 4206 \
  --stvinput "$(cat port/run/P43/progression-input.txt)" \
  --shots $(seq -s, 4190 1 4205) --dump $(seq -s, 4190 1 4205) --out port/run/P44/fans
python3 port/run/P44/tools/vdp1cmds.py 3BD80 port/run/P44/fans/dump-41{90..99}.bin

# Furthest progression: the P4.3 input prefix up to Scene 9 entry, then the scripted player
python3 - <<'PY'
from pathlib import Path
s = ['200:COIN', '340:START1', '1350:P1RIGHT:100', '6090:P1RIGHT:30']
s += [f'{v}:P1A:2' for v in range(1450, 7020, 12) if not (6070 <= v <= 6125)]
s += [f'{v}:P1B:2' for v in range(1456, 7020, 24) if not (6070 <= v <= 6125)]
Path('port/run/P44/prefix-7020.txt').write_text(','.join(s) + '\n')
PY
STV_AUTOPLAY=7020 $BIN --cart port/diehard_main.bin --cartrom cart.bin --vblanks 90000 \
  --stvinput "$(cat port/run/P44/prefix-7020.txt)" --shots $(seq -s, 7000 500 90000) \
  --out port/run/P44/auto7 > port/run/P44/auto7.log 2>&1

# Sound during a round
$BIN --cart port/diehard_main.bin --cartrom cart.bin --vblanks 2100 \
  --stvinput '200:COIN,340:START1,1350:P1RIGHT:100,1450:P1A:2,1500:P1A:2,1550:P1A:2,1600:P1A:2,1650:P1A:2,1700:P1A:2,1750:P1A:2,1800:P1A:2,1850:P1A:2' \
  --wav port/run/P44/sound/combat.wav --out port/run/P44/sound > port/run/P44/sound.log 2>&1

# Regressions: the five P4.3 commands with --out port/run/P44/reg-{combat,reference,nocoin,progression,stage-clear},
# then
port/run/P44/tools/verify_regressions.sh > port/run/P44/regressions.txt
```

Runs are deterministic: `auto4`, `auto5` and `auto7` share the same prefix and produce
the same scene timeline up to their stopping points.

## 9. Regression results (`port/run/P44/regressions.txt`)

| Check | Result |
|---|---|
| Runtime unit tests | **PASS**, `126 tests, 0 failed` (`port/run/P44/tests.log`) |
| P4.2 combat check (2100 VB) | **PASS**, 16,810 trace records identical to P4.3, 0 FATAL, 5 PNG + 1 dump byte-identical |
| Paired paid reference (2100 VB) | **PASS**, 16,810 identical, 0 FATAL, 6 files identical |
| No-coin attract (1800 VB) | **PASS**, 14,404 identical, 0 FATAL, `shot-1700.png` identical |
| P4.2 progression (4200 VB) | **PASS**, 33,617 identical, 0 FATAL, 6 PNG + 3 dumps identical |
| P4.3 progression to Scene 9 (7500 VB) | **PASS**, 60,027 identical, 0 FATAL, 7 PNG identical |

## 10. Evidence PNGs

| What | Path |
|---|---|
| Fans rotating, consecutive game frames | `port/run/P44/fans/montage-4190-4200.png`, `port/run/P44/fans/shot-4190.png` … `shot-4205.png` |
| Continue bought, Scene 9 fight goes on | `port/run/P44/auto4/shot-8000.png` |
| Scene 9 cleared, "WAY TO NEXT" map | `port/run/P44/auto4/shot-13400.png` |
| Scene 15 loading-dock fight (weapon HUD) | `port/run/P44/auto4/shot-13600.png`, `shot-14000.png` |
| Scene 16 `CAUTION` / `パンチ` QTE | `port/run/P44/auto4/shot-20400.png`, `shot-20800.png` |
| Scene 17 car park, firemen and fire engine | `port/run/P44/auto4/shot-21200.png`, `shot-24000.png` |
| Scene 18/19 QTE `FAILURE`, corridor | `port/run/P44/auto5/shot-34000.png`, `shot-35200.png` |
| Scene 21 washroom fight | `port/run/P44/auto5/shot-36400.png`, `shot-41200.png` |
| Scene 25 server room, spider-walker robots | `port/run/P44/auto7/shot-48500.png`, `shot-49500.png` |
| Scene 32 pink lobby, SWAT enemies | `port/run/P44/auto7/shot-52500.png`, `shot-54500.png` |
| Scene 34 pool hall | `port/run/P44/auto7/shot-60000.png`, `shot-62000.png` |
| Scene 35 QTE `FAILURE`; Scene 37 office | `port/run/P44/auto7/shot-66000.png`, `shot-66500.png` |
| Scene 43/44 rooftop approach and fight | `port/run/P44/auto7/shot-70500.png`, `shot-72000.png` |
| Scene 45 yellow walker mechs | `port/run/P44/auto7/shot-79000.png` |
| **Scene 51 (furthest), glass rooftop pavilion fight** | `port/run/P44/auto7/shot-83000.png`, `shot-86000.png`, `shot-88000.png` |
| GAME OVER, then permanent black (defect 1) | `port/run/P44/explore1/shot-8400.png`, `shot-9600.png` |
| Attract stuck on title (defect 2) | `port/run/P44/nocoin-long/shot-8000.png` |

## 11. What is next

1. **ST-V BIOS hand-off after GAME OVER and the attract cycle** (defects 1–2). Emulate
   what the real BIOS does when the game calls `0x0600060C` and parks
   `boot_scene_director` in state 13 (restart the game or attract). Use the recompiled
   US BIOS (`epr-17952a`, see `real_bios_spike.md`) as an oracle for that vector,
   instead of guessing.
2. **RBG0/back-screen floor coverage in interior rooms** (defects 3–4, P4.3 item 1).
3. Optionally, extend the recompiler's bsrf/braf constant tracking (r0 through
   intervening non-aliasing writes), so data-reached helpers no longer need hand
   seeds. Re-run `scan_unresolved_bsrf.py` after each rebuild.
4. Keep pushing with `STV_AUTOPLAY` past Scene 51 toward the stage 2/3
   bosses. The autoplayer has no weapon pickup or throw logic, so it buys many
   continues. A smarter policy, or a longer `--vblanks`, would cut run time.
5. Check the `パンチ` prompt and the region/area-code value against MAME (defect 5).
