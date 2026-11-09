# Die Hard Arcade (ST-V) RECOMP — real ST-V control-panel BIOS service + input→game-start

Date: 2026-10-19. Follows and builds on notes/port/spawn_diagnosis.md (root cause).
Port side only (port/ + tools/saturn-recomp/). project/ and tools/decomp/ untouched.

## TL;DR

- Implemented the **real ST-V control-panel BIOS service (0x06000660)** in the runtime: it
  now fills the panel-report buffer every frame and a scripted input source (`--stvinput`)
  injects coin/start. This is the **default** on the ST-V harness path (not env-gated); Saturn
  disc games are unaffected (a disc boot never routes the 0x06000600 window to this service).
- This **breaks the round-start freeze.** With the default `--stvinput "300:START1"` the game
  leaves the byte-identical frozen HUD (old md5 `0ff787a8`) and runs its real game-start /
  continue sequence (frames evolving: `CONTINUE 5 → 2`; with input during a demo game it reaches
  the game-over `CONTINUE?` prompt `5 → 1`). Confirmed by checksum — the frozen fixed point is gone.
- **Honest limit:** this reaches the game's **continue / game-over flow**, NOT a fresh,
  player-controlled fighting round with new on-screen characters. Every start path lands in the
  self-playing attract/**demo**'s continue logic, because the port never establishes **play mode**
  (a real credit). That is the next concrete blocker, named precisely below. No fighting-character
  playfield frame was produced.
- **New seed:** 1 (`06009812`, the game-over/continue code reached for the first time). Autolearn
  otherwise converges clean on the start path — the blocker is **game logic/mode, not a missing
  function.**

## 1. Decoded ST-V control-panel report format (buffer `0x060987D8`)

Derived by disassembling every consumer of the buffer (sh-elf-objdump of the loaded image;
runtime addr = cart + 0x03E04000). The authoritative consumers:
- boot config `func_0201b92` (== runtime `0x06005b92`): the exact source the functional-decomp
  agent already decompiled (project/src_functional/0201b92.c) — matches byte-for-byte.
- the live coin-wait handler `fn 06008868` (callers pr `060089CA`/`06008A88`, ~2×/frame during
  the freeze), and the dispatch consumer at `0x06009266` / countdown state `0x06009232`.

| offset | meaning | evidence |
|---|---|---|
| `buf[0x10]` | **status byte.** bit0 = no-data/invalid → the game ignores the read; bit2 = 2-player control panel present (gates whether P2 is checked) | `func_0201b92`: `if(!(buf[0x10]&1)){ if(buf[0x10]&4) …P2… }`; handler `0x6005bda` `tst #1` / `tst #4` |
| `buf[0x03]` | **P1 action code** (small enum). **2 = START/join.** The handler latches it into `0x0609868C` (prev) and calls `dispatch[buf[3]]` through the RAM jump table at **`0x06090304`** (edge-detected: fires when `buf[3] > prev`) | `0x6008a48`–`0x6008a74`; boot path treats `buf[3]==2` as P1 present |
| `buf[0x07]` | **P2 action code** (buf+4 is the P2 record; +3 = `buf[7]`). 2 = START/join | `0x6008a76`+ reads `@(3,buf+4)`; boot path `buf[7]==2` |
| `buf[0x00..2]` | P1 credit/count fields used only to draw the credit-display text (`buf[0]>1?dispatch[4]:dispatch[5]`, string table `0x60506F0`) | `fn @0x060084CC` |

Separately, the **cooked per-frame input word `0x060987BC`** (read by ~80 functions across all
gameplay, directions/buttons in bits 0–4) is the universal input bitmask. Its exact bit→button
map was **not fully pinned** (it is produced elsewhere, not by this service) and is **not needed**
to deliver coin/start; it is left idle (0). **This is the one genuine remaining format unknown.**

Cross-ref note: MAME's `src/mame/sega/stv.cpp` models ST-V controls through the IOGA at
`0x00400000`; **this game never touches the IOGA** (confirmed earlier: no cart function references
`0x00400000`/`0x02400000`). It reads only the BIOS service buffer, so the MAME IOGA bit map does
not directly apply — the format above is taken from the game's own consumers, which is authoritative.

## 2. What was implemented

**`tools/saturn-recomp/runtime/src/machine/bios.cpp`**
- `stv_input_script(spec)` — parses `"VB:TOKEN,…"` (TOKEN ∈ START1/START/START2/COIN/COIN2).
- `stv_panel_read(buf)` — the real 0x06000660 body: writes a full zeroed (valid, no-activity)
  report each call, then stamps scripted events active on the current VBlank. A press is held for
  a **2-VBlank window** because the coin-wait handler reads the panel only on alternate VBlanks —
  this guarantees exactly one handler read sees it (a single clean rising edge of `buf[3]`).
  - On START it sets `buf[3]`/`buf[7]=2` (the correct panel encoding — the handler DOES read it:
    verified `0x0609868C`←2 at the press VBlank) **and**, once per event, raises the player
    start-present flag `0x06098688`/`0x06098689`. The flag is raised because the scene state the
    port actually reaches does not bridge the panel to the join (its `dispatch[2]` is not the
    join handler); the start-present flag is the proven game-start trigger (spawn_diagnosis.md).
  - COIN is logged only (encoding not pinned; see §4). It deliberately does **not** poke
    `0x06098674` (that byte is P1/P2 join *state* 0..3, not a coin credit — corrupting it misleads
    the state machine).
- The 0x06000600-window sentinel branch now runs `stv_panel_read` for 0x660 **by default**;
  `STV_STVBIOS` still selects the old diagnostic overrides (`log`/`fill`/`join`).

**`port/main_stv.cpp`**
- `install_bios_workarea` now **always** routes the 0x06000600 service window to the runtime
  ST-V service sentinel (was env-gated to a silent no-op). A Saturn disc boot never installs this
  window this way, so the ST-V body is reached only on this harness path (gating by construction).
- New flag `--stvinput "VB:START1,…"`; default (if unset) = `"300:START1"`.

**`tools/saturn-recomp/runtime/src/machine/saturn.h`** — declares `stv_input_script`.

## 3. Results (checksums)

- Frozen baseline (no service): VB1200/1399 md5 **`0ff787a8`** (byte-identical, the freeze).
- Default (`--stvinput 300:START1`): VB900 = **`3ed28205`** = HUD + portraits + **`CONTINUE 5`**,
  VB1300 = **`2cc5026c`** = `CONTINUE 2`. Frames actively change — freeze broken.
  (A single start lets the countdown expire back to attract; sustained starts keep it alive.)
- Start during a demo game (`--stvinput 150:START1`, +seed `06009812`): reaches the game-over
  **`CONTINUE?`** prompt — VB700 `CONTINUE? 5`, VB1200 `CONTINUE? 1`, then a black inter-state.
- Autolearn on the start path: **0 further seeds** — clean 2200-VBlank run (no missing-function wall).

Frames in `port/run/`: `gamestart_continue5_vb900.png`, `gamestart_continue2_vb1300.png`,
`gameover_continue5_vb700.png`, `gameover_continueq_vb900.png`, `gameover_continue1_vb1200.png`
(also in the Mac scratchpad, same names, + `freeze_baseline_frozen_vb1200.png`).

## 4. Next concrete blocker (named, with evidence)

**Reaching a fresh player round requires an attract→play-mode (real credit) transition the port
never makes; forcing start only drives the self-playing demo's continue/game-over logic.**

Evidence:
- The only per-frame attract input handler reached is `fn 06008868`. Its top branches on the
  start-present flags; with a flag set it sets credit `0x06098674=3`, clears the flag, and
  transitions the task to the **countdown state `0x06009232`** (the `CONTINUE N` overlay). It has
  **no coin→credit logic at all** — so a coin in the attract state is never turned into a credit.
- `fn 06008868`'s start branch further forks on the **actor-pool count** (`0x06099594 + 0x06099598`):
  pool empty → the `CONTINUE` countdown; pool active (mid-demo) → a different handler that runs
  the game-over/`CONTINUE?` path (this is the `06009812` seed path). Both are **demo/continue**
  states, not a fresh stage load.
- Watching the actor pool `0x0609958C` with start injected: the pool populates **only** during the
  normal demo segments (VB441/693), identically to the no-input baseline. Forcing start spawns **no
  new round actors.** No fighting characters were ever rendered.
- The coin/credit→play-mode subsystem the skipped ST-V PIC/BIOS boot stub would set up (BIOS coin
  services `0x06000640`/`0x0600063C`, currently benign no-ops — and polled only once at boot, not
  per frame in attract) is not established, so there is no "credited play" mode to enter.

Secondary (independent) blocker for a visible playfield: VDP2 **rotation backgrounds (RBG0/RBG1)
are unimplemented** in the runtime, so a real stage ground/environment cannot render even if reached.

**Recommended next step:** reverse the attract↔play-mode flag (what a real credit sets so the game
waits for START and then stage-loads instead of auto-demoing), and/or implement the BIOS coin
services + the coin field in the panel report, so a scripted COIN produces a real credit. That is a
state-machine/mode piece, distinct from (and larger than) the panel-read service delivered here.

## 5. Files touched / seeds

- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — real 0x660 service + `stv_input_script`.
- `tools/saturn-recomp/runtime/src/machine/saturn.h` — `stv_input_script` decl.
- `port/main_stv.cpp` — route window to real service by default; `--stvinput` flag + default.
- `port/seeds_extra.txt` — +1 seed `06009812` (ST-V start/continue code path).
- Cleanup: removed the temp run dirs/logs created this session; left `run/watch12.log` and the
  pre-existing diagnosis frames/dirs intact. (Also removed the regenerable `run_*.txt` service
  logs from the prior spawn-diagnosis session — they are reproduced by re-running with `STV_STVBIOS`.)
