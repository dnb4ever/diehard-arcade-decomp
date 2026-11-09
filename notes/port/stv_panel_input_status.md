# Die Hard Arcade (ST-V) RECOMP — ST-V control-panel input decode + real-START investigation

Date: 2026-10-19. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Builds on credited_round_status.md, stv_input_gameplay_status.md, boottask_status.md.
Address map: runtime(HWRAM) = cart SH-2 addr + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.

## TL;DR — what this run established (and what it corrects)

- **CORRECTION: 0x060987BC is NOT the "cooked input word". It is the free-running FRAME COUNTER.**
  Prior notes/task framing called it "the universal cooked input word read by ~80 functions,
  directions/buttons in bits 0–4." The decompiled C says otherwise, unambiguously, in every
  consumer: `g_060987BC` is "a free-running frame counter" / "frame/seed byte source". The ~80
  readers use it for frame parity (`(ctr&1)<<8`), RNG/palette seeds (`ctr&0xFF`), and elapsed-time
  timers (`ctr - field[0x114]`). The attract/coin-wait handler itself reads it only to THROTTLE the
  credit-display refresh (`ctr & 0xF`, `ctr & 0x1F`). Nothing maps panel bits into it. (Evidence:
  project/src_functional/0224158c.c, 0222863e.c, 0222077e.c, 02228b68.c, 02245fc4.c, 02246026.c,
  02241556.c + the attract_handler disasm below.)

- **The ST-V panel format (BIOS report buffer 0x060987D8) is decoded** (unchanged from prior work,
  re-confirmed from the game's own consumers — the game has NO IOGA path, it reads only the BIOS
  service 0x06000660). See §1.

- **The real cooked gameplay-input block is 0x06098794** (decomp calls it the "input state block").
  Gameplay reads `MU16(0x06098794, 6)` = word @0x0609879A, tested `& 3` / `tst #2` as
  "a-control-is-pressed" (project/src_functional/0201f20.c state 1 + the scene-skip decoders). Its
  full direction/button bit map is NOT resolved (its writer was not located — it is produced below
  the panel layer, probably the SMPC/BIOS peripheral path; the game never writes it via a literal).

- **NEW: the start-present flag 0x06098688 setter is identified.** It is NOT set by any panel/
  attract consumer (prior notes said "setter not yet decompiled"). It is set by a **player-character
  actor FSM at cart 0x2229cac (runtime ~0x0602DCAC): `0x06098688[entity[0x34]-1] = 1`**, where
  entity+0x34 is the player index. i.e. the flag is an INTRA-GAME join signal raised by an
  already-spawned player actor — not a raw panel input. This is the root cause of why a panel START
  can never raise it directly, and it localises "Blocker B" one level deeper.

- **Implemented** a faithful, additive, flag-gated extension of `--stvinput` (buttons/directions/
  service tokens) — details §3. Defaults byte-identical.

## 1. ST-V control-panel report format — BIOS service 0x06000660 → buffer 0x060987D8

The game delegates its per-frame panel read to the ST-V BIOS service `*0x06000660(buf)` (the game
passes buf = 0x060987D8). With no BIOS ROM the harness supplies this service (bios.cpp). The report
layout, from the game's own consumers (boot config func_0201b92 == runtime 0x06005b92, the coin-wait
/ attract handler 0x06008868, and the menu/scene decoders at 0x060073xx..0x060069xx):

| offset | meaning | evidence |
|---|---|---|
| buf[0x00..0x02] | P1 credit/count fields (drive the credit-display text only) | credit-draw fn 0x060084CC |
| **buf[0x03]** | **P1 action code** (small enum; **2 = START/present/join**). Edge-detected (`buf[3] > prev@0x0609868C`) and dispatched via RAM jump table 0x06090304 | attract_handler 0x204a48–0x204a74; func_0201b92 `buf[3]==2` |
| buf[0x04..0x06] | P2 credit/count fields | symmetry + credit-draw |
| **buf[0x07]** | **P2 action code** (buf+4 record, +3). 2 = START/join | attract_handler credit>0 branch 0x204aa..; func_0201b92 `buf[7]==2` |
| **buf[0x10]** | **status byte.** bit0 = no-data/invalid (game ignores read); bit2 = 2-player control panel present (gates P2) | func_0201b92 `if(!(buf[0x10]&1)){ if(buf[0x10]&4) …P2… }` |
| buf[0x11] | per-player nav nibble (low nibble P1, high nibble P2); menu up/down navigation | menu decoders read `buf[0x11]&0xF` / `>>4`, `cmp/gt` → nav-step helper |

The "cooked" per-frame gameplay input is a SEPARATE block at **0x06098794** (word @0x0609879A holds
the verified "pressed" bits 0/1); the full bit map is unresolved (see TL;DR).

### MAME cross-reference (hardware layout)
MAME `src/mame/sega/stv.cpp` models ST-V controls through the **Sega 315-5649 I/O IC (IOGA) at SH-2
addr 0x00400000**, all inputs active-low, as a JAMMA control panel:
- P1 port (PORTA): bit0 B1, bit1 B2, bit2 B3, bit3 B4, bit4 Down, bit5 Up, bit6 Right, bit7 Left.
- P2 port (PORTB): same layout.
- System port (PORTC): bit0 Coin1, bit1 Coin2, bit2 Service(active-low)/Test, bit3 Service1,
  bit4 Start1, bit5 Start2, bit6/7 extra service/pause.
(MAME layout summarised via the current source; treat the exact bit numbers as the hardware
reference, not this game's format.)

**Reconciliation:** this game **never touches the IOGA** — no cart function references 0x00400000/
0x02400000 (confirmed by full catalog scan in prior sessions, re-checked here). The real ST-V BIOS
reads the IOGA/SMPC and hands the game the DECODED report at 0x060987D8 (action codes + status +
nav nibble). So the MAME IOGA bit map is the hardware truth but does NOT appear at the game layer;
the authoritative game-level format is the table above, taken from the consumers. The harness fills
0x060987D8 in that game-level format (so "coin/start" semantics are faithful without a BIOS ROM).

## 2. Who sets 0x060987BC / 0x06098688 / the credit — traced this session

- **0x060987BC (frame counter):** incremented by the per-frame executive (a `+1` writer in the
  main tick); consumed only as a timer/parity/seed. NOT an input. (See TL;DR evidence list.)
- **Credit 0x06098674:** COIN → coin-pending 0x0609880C=1 (real coin-detect; harness raises it for
  the scripted coin) → accrual fn **0x060045F8** (cart 0x2045f8, called first thing every frame by
  attract_handler) gates on 0x0609880C==1 && 0x0609880E==1, then `0x06098674++`, updates BIOS coin
  meter 0x06000640, clears pending. VERIFIED live: two coins → credit 1 → 3. (The earlier
  stv_input note's "0x06098674 is join-state 0..3, not credit" was wrong; this note corrects it —
  it IS the credit counter.)
- **attract_handler 0x06008868** (cart 0x204868) is a READER/consumer: at its top it reads
  0x06098688+0x06098689; if nonzero it does round-start bookkeeping — **sets 0x06098674 := 3**
  (NOT a consume), clears coin-enable 0x0609880E, clears the start flag, and calls the round/text
  dispatch 0x0609029C. In the attract (credit==0) state it reads the panel and edge-dispatches
  0x06090304[buf[3]] (which draws "PRESS START"/credit text).
- **0x06098688 (start-present flag) SETTER (new):** cart **0x2229cac** (`mov #1,r2; mov.b
  r2,@(r0,r3)` with r3=0x06098688, index = entity[0x34]-1) — inside a player-character actor FSM in
  the gameplay entity region (0x2229xxx). Every OTHER reference to 0x06098688 across the whole image
  is a CLEAR (0x06005958 boot-init, 0x0600981A per-player init = f_220581a, 0x02202c5c, attract_
  handler) or a READ. So the flag is raised ONLY by a spawned player actor, confirming it is a join
  signal internal to the round, not something a panel press can produce on its own.

## 3. What was implemented (port/ + tools/saturn-recomp/ only; additive, flag-gated)

**tools/saturn-recomp/runtime/src/machine/bios.cpp**
- `stv_input_script()` token grammar extended: now also accepts `SERVICE`/`SVC` and per-player
  face/direction tokens `P1A..P1C`, `P1X/Y/Z`, `P1UP/DOWN/LEFT/RIGHT` (and `P2*`; a bare face/dir
  token defaults to P1). START1/START2/COIN/COIN2 unchanged. (The task's "carry buttons/directions,
  not just coin/start".)
- `stv_panel_read()`: START (buf[3]/buf[7]=2) and COIN (coin-pending) delivery UNCHANGED and
  faithful. New: `SERVICE` logs (the BIOS 0x688 service is inert on this path, documented). New,
  **gated behind `STV_PANELBTN` (default OFF)**: a face/dir press sets **bit0 of the verified cooked
  "pressed" word 0x0609879A** — the one gameplay-input bit proven decodable (read by func_0201f20
  state 1 and the scene-skip handlers). This lets a `--stvinput` timeline satisfy "press to continue/
  skip" gates. Per-button identity is intentionally NOT faked (every face/dir token collapses to the
  same verified bit) because the full panel-buffer direction/button bit map is unresolved.

**port/main_stv.cpp** — `--stvinput` help text updated to the extended grammar. Default input and
all other behaviour unchanged.

Regression: the change is purely additive (new event kinds 5/6/7 + a default-off env flag); the
kind-1..4 code path is byte-identical, so the default/credited runs are unaffected. Verified: the
credited cold-boot run still renders the attract combat-HUD; the 7-token test (coin/start/P1A/P1UP/
SERVICE/P2B) parses and fires correctly; cold+cfg798=60 with coin/start/face-dir runs clean to 3000
VBlanks (no missing-function walls).

## 4. Per-item results (the honest bar)

| question | result |
|---|---|
| **START decoded?** | **YES.** Panel action code buf[3]/buf[7]=2, status buf[0x10], nav buf[0x11], plus the frame-counter correction and the 0x06098794 cooked-block location. Full format documented (§1) + MAME hardware cross-ref. |
| **Real START fires round-start (via the game's own logic)?** | **NO — and now we know why.** The start-present flag 0x06098688 is set only by a spawned player-actor FSM (cart 0x2229cac), never by the panel/attract path. A panel START (buf[3]=2) with credit present was delivered and watched: the game did NOT store 0x06098688 in response. So a legit panel START cannot fire round-start without the round's actor-spawn path first running. (The STV_STARTFLAG diag forces the flag directly as a stand-in.) |
| **Credit consumes on start?** | **NO.** With the flag forced (STV_STARTFLAG) + 2 coins, the attract_handler start branch SETS credit := 3 (bookkeeping for the demo/continue flow) rather than consuming a paid credit; credit reads 3 at end. This is the demo/continue path, not a paid-round consume. |
| **Actors populate a player round?** | **NO.** Pool 0x0609958C cycles 0→4 and drains identically to the attract demo (writer 0x0602AEB8, alloc pr 0x0602BB96, free pr 0x0602E56A); no sustained player-round population. |
| **Stage loads?** | **NO.** VDP2 RBG1 stage floor unimplemented (known); and the round-population/stage-load path is not entered. |

**Net:** decoding the panel format correctly — plus the two substantive corrections (0x060987BC is
a frame counter; the 0x06098688 setter is the player-actor FSM at 0x2229cac) — is the win for this
run. The full credited player round remains gated on the round's **actor-spawn / stage-load** path
(the independently-identified Blocker B), which is gameplay-logic work outside the input layer.

## 5. Next concrete blocker

Make the credited attract→play transition actually **spawn a player actor** so the actor FSM at
0x2229cac runs and raises 0x06098688 legitimately (instead of the demo/continue bookkeeping). Trace:
with credit>0, what should drive the attract task to instantiate a P1 player entity (entity+0x34=1)?
Candidate path: the credit>0 dispatch 0x06090304[buf[3]] when the table is in its credited state —
find the dispatch-table initializer (writers of RAM 0x06090304) and what index 2 points to once a
credit exists. That spawn is the missing link between a panel START and a real round; it overlaps the
stage-load piece (Blocker B). The input layer is now complete and correct up to that handoff.

## 6. Files touched / seeds / artifacts
- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — extended token grammar + flag-gated
  (STV_PANELBTN) cooked-press injection; faithful start/coin unchanged; new 0x2229cac setter note.
- `port/main_stv.cpp` — `--stvinput` help text for the extended grammar.
- `port/seeds_extra.txt` — no new seeds (path is code-complete; dated comment appended).
- Frames: `port/run/panelinput_attract_combatHUD_vb1500.png`, `port/run/panelinput_title_vb600.png`
  (+ Mac scratchpad, same names). These are the ATTRACT combat-demo HUD — honestly NOT a player
  round (no new round was produced; none was faked).
- NOT modified: project/, tools/decomp/, notes/symbols.txt. Pre-existing run/watch12.log + diag_*.png
  left intact.

## 7. Reproduce (p620)
```
cd ~/decomp/diehard-stv/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# extended input grammar + verified cooked-press bit:
STV_BOOTMODE=1 STV_PANELBTN=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
  --vblanks 700 --stvinput "150:COIN,210:COIN,300:START1,400:P1A,500:SERVICE,520:P2B" --out run
# find the 0x06098688 setter empirically (watch the flag range; it is NOT stored on a panel START):
STV_BOOTMODE=1 ./recomp-build/saturn_stv ... --watch 06098674:0609868C
# credit accrual on the cold path (0 -> 1 -> ... -> 3):
STV_BOOTMODE=1 ./recomp-build/saturn_stv ... --peek 06098674:1,0609958C:4
```
