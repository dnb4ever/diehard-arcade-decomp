# Die Hard Arcade (ST-V) RECOMP — credit accrual + credited-round investigation

Date: 2026-10-19. Builds on boottask_status.md, credit_playmode_status.md, spawn_diagnosis.md.
Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only reference.

## TL;DR

- **Sub-goal 1 (credit accrual on the cold-boot path): SOLVED, and it was a HARNESS CONFIG BUG.**
  `install_bios_workarea` lays the ST-V BIOS low area (0x06000000..0x06000800) down as placeholder
  interrupt-vector pointers, but the game reads two *real* BIOS config words out of that same area:
  the region word **0x06000248** and the DIP/config word **0x06000798**. They were reading vector
  garbage (0x1248 / 0x1798). The clobbered DIP word's **bit5 (0x20)** was clear, which routed the
  cold-boot state machine (func_022016a0 state 4) to a BIOS terminal/idle hand-off — the attract/
  coin-wait task **attract_handler 0x06008868 never ran**, so credit never accrued. Writing the
  correct ST-V DIP default **0x06000798 = 0x60** (bit5 proceed + bit6 sound) on the cold-boot path
  makes attract_handler run, **credit accrues (0x06098674: 0→1→2→3)**, and the **attract combat-demo
  HUD (character portraits + green/red health bars) renders**. Verified; the warm/default path is
  left byte-identical (no regression).
- **Sub-goal 3 (actors spawn into the pool): DEMONSTRATED in the attract combat-demo.** With the
  config fix the slot-pool **0x0609958C cycles 0→4** during the attract intro and holds ~2 combat
  actors during the combat demo (two fighters, two health bars). So the spawn machinery works and
  **combat actors do populate the pool** — but in the self-playing attract DEMO, not yet in a
  credited player round.
- **Sub-goal 2 (credited START → a player round): NOT reached. Two blockers, both now precisely
  localised** (see §3). (a) The harness's START delivery is *wrong* — it feeds the SMPC peripheral-ID
  byte, not a START button. (b) Even when the real round-start trigger (the start flag 0x06098688) is
  forced with a real credit, the round-start code runs but **the round does not populate** — it falls
  back into the attract cycle. This is the "one genuine logic unknown" the task flagged, and it
  resisted a bounded effort.

## Address mapping
runtime(HWRAM) = cart SH-2 addr + 0x03E04000. cart SH-2 addr = cart.bin offset + 0x02000000.

---

## 1. Sub-goal 1 — credit accrual (SOLVED)

### Root cause (new, corrects the prior "attract_handler is a state-reach issue" guess)
The cold-boot path (STV_BOOTMODE=1) and the warm/default path were taking two *different* top-level
game-flow branches (full coverage diff: ~75 functions ran on one path but not the other). The cold
path ran the 0x06006xxx/0x06007xxx init family and **went idle**; the warm path ran the 0x06008868
attract/coin-wait/0x0602Exxx-spawn family. The fork is in **func_022016a0 state 4** (cart 0x022017EC):
```
r3 = *0x06000798;  r3 >>= 5;  if (r3 & 1 /*bit5*/ == 0) { jsr *0x0600060C; ctx[0x10] = 13 /*terminal*/ }
```
Our harness had 0x06000798 = 0x1798 (vector-fill garbage), bit5 = 0 → terminal. func_0201b92 (cold
init) *also* reads bit5 and bit6 of the same word. The word is genuinely BIOS-provided config the
real boot stub would set; the vector-fill clobbering it is the bug.

### Fix (port/main_stv.cpp)
When STV_BOOTMODE is set, write the correct ST-V DIP default **0x06000798 = 0x60** (bit5 = normal-
operation/proceed, bit6 = sound-enable). Env `STV_CFG798` overrides for A/B; `STV_CFG248` overrides
the region word (left at placeholder by default — forcing nibble 4 enters an unseeded SMPC-read branch
that walls at 0x06026ECC; a side quest, not needed for credit). The warm/default path (no bootmode)
is untouched — 0x06000798 stays 0x1798 and credit still accrues there as before.

### Evidence
| path | attract_handler | credit 0x06098674 | pool 0x0609958C |
|---|---|---|---|
| mode-1, DIP clobbered (old) | ran=0 | 0 (coin-pending 0x0609880C=1 stuck) | ends 0 |
| **mode-1, DIP=0x60 (fixed)** | **ran=1** | **0→1→2→3 accrues** | cycles 0→4, holds ~2 |
| warm default (unchanged) | ran=1 | 0→1→2 accrues | cycles, holds 2 |

Frames (port/run/credited/ + Mac scratchpad): `credited_coldboot_title_vb600.png`,
`credited_coldboot_combatHUD_healthbars_vb1500.png`, `credited_coldboot_combatHUD_healthbars_vb2500.png`
— the last two show the attract combat demo: two character portraits + green/red health bars over a
black/skyline playfield (RBG1 stage unimplemented, as expected).

## 2. Sub-goal 3 — actor spawning (works in the demo)

With the fix the pool populates: writer fn 0602AEB8 (alloc at pr 0602BB96, free at pr 0602E56A).
`0609958C` goes 0→4 during the attract intro, drains/refills across demo segments, and the combat
demo holds two fighters. **So "combat actors spawn into the pool" is satisfied in the attract demo.**
The prior notes' "slot-pool stays empty / static at 2" was the pre-config-fix state; the fix changed it.

## 3. Sub-goal 2 — credited round (NOT reached; blockers localised)

### Blocker A — START input is mis-delivered (corrects prior notes)
The harness delivers START as `buf[3] = 2` in the panel buffer 0x060987D8. **buf[3] is the SMPC
peripheral-ID byte (2 = standard control pad), NOT a START code.** The handler's "dispatch table"
0x06090304, indexed by buf[3], is a table of **credit-display STRINGS** — 0x06090304[2] = 0x06050EEC
= the ASCII "PRESS START", drawn via the text routine g_0609029C=0x06027080. So START-via-buf[3] only
makes the game draw "PRESS START"; it never joins. (Disassembled attract_handler 0x06008868 +
coin-wait 0x060089b8 + the string table; literals confirm 0x06050EEC/0x06050E9C/... are text.)

The real round-start trigger is the **player start flag 0x06098688 / 0x06098689**: attract_handler
checks it at its top (0x0220489a) and, if set, clears coin-enable, sets the continue count, and calls
round-init g_0609029C. The *setter* that turns a real panel START button into that flag is **not yet
decompiled** (all decompiled writes to 0x06098688 are clears), and the ST-V panel button-bit layout
in the 0x060987D8 buffer is the genuine remaining format unknown. Next concrete step: find the
per-frame panel-button decoder (reads 0x060987D8 button bytes, writes 0x06098688) in the cart and
implement the matching byte layout in bios.cpp `stv_panel_read`. (Diagnostic `STV_STARTFLAG=1` raises
the flag directly as a stand-in — the exact substitution the old "join" probe used.)

### Blocker B — even a forced credited START does not populate a round (the deep unknown)
With real credit + `STV_STARTFLAG=1`, if START lands **before the demo takes over** (VB≈210–450,
while attract_handler still owns the flow) the flag **is consumed** (0x06098688→0, credit 0x06098674→0)
and round-init runs — but the pool cycles **identically to the attract** and ends empty; no sustained
player round, no new fighters. (If START lands during a demo segment the flag is never consumed and
stays stuck at 1 — attract_handler isn't the active task then.) So the round-start code executes
shallowly but the round never materialises. Most likely the credited round needs the stage/level-data
load + spawn-script setup (process_spawn_list reads ctx+0x30 → spawn-script → script+0x14, and exits
if null) that is coupled to the stage-load path, which overlaps the RBG1 work. Next concrete step:
trace round-init g_0609029C=0x06027080 forward to find where it should set the spawn-script pointer /
load level data, and watch why process_spawn_list exits without allocating in the credited path.

## 4. Is RBG1 now the ONLY thing left? NO.
Honest answer: between the current state and a visually-complete, *player-controlled* round there are
THREE pieces, not one:
1. **ST-V panel button decode** — deliver a real P1 START (Blocker A). Bounded RE.
2. **Credited round population** — make round-init actually load the stage data + spawn the fighters
   (Blocker B). This is additional *logic* work, partly entangled with the stage-load path.
3. **VDP2 RBG1** — the stage floor render (as before; the HUD/VDP1 sprite path already works).
The attract combat-demo already exercises the spawn + HUD + sprite paths, so once a credited round
loads its actors, the sprites should draw on a black playfield even before RBG1.

## 5. Files touched / seeds / artifacts
- `port/main_stv.cpp` — cold-boot ST-V DIP/config write (0x06000798 = 0x60 default when STV_BOOTMODE
  set; STV_CFG798/STV_CFG248 overrides); STV_COVDUMP (dump ran[] to out/cov.txt for coverage diffs).
  Warm/default path verified byte-identical.
- `port/autolearn_cold.sh` — NEW: autolearn loop carrying the DIP/config fix (and optional region),
  to seed the gameplay-fight walls the config-fixed path reaches.
- `port/seeds_extra.txt` — +2 seeds: **0604DFB0, 06041D28** (attract-demo fight code reached for the
  first time via the config fix). Cold path then runs clean to 5000 VBlanks (no missing-function walls).
- Frames in `port/run/credited/` and the Mac scratchpad (names in §1).
- NOT modified: project/, tools/decomp/, notes/symbols.txt.

## 6. Reproduce (p620)
```
cd ~/decomp/diehard-stv/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# cold boot + credit accrual + combat-demo HUD (DIP default 0x60 applied automatically):
STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 2600 --stvinput "200:COIN,260:COIN,340:START1" --shots 600,1500,2500 --out run/credited
# prove credit accrues on the cold path:
STV_BOOTMODE=1 ./recomp-build/saturn_stv ... --watch 06098674:06098674   # 0 -> 1 -> 2 -> 3
# forced credited START (diagnostic), lands but does not populate a round:
STV_BOOTMODE=1 STV_STARTFLAG=1 ./recomp-build/saturn_stv ... --stvinput "150:COIN,160:COIN,280:START1"
```
