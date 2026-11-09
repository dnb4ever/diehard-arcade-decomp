# Die Hard Arcade (ST-V) RECOMP — coin→credit path + attract→play-mode investigation

Date: 2026-10-19. Builds on notes/port/spawn_diagnosis.md and stv_input_gameplay_status.md.
Port side only (port/ + tools/saturn-recomp/). project/ and tools/decomp/ untouched (read-only ref).

## TL;DR

- **Approach chosen: B (targeted coin service), because Approach A is not reachable by a simple
  re-route** (see §4). I implemented the game's real coin→credit path and the coin now registers
  a credit **through the game's own accrual logic** — not by faking the credit value.
- **Credit registers: YES** (with evidence). **Mode partially transitions: YES** — the credited
  path escapes the attract freeze and reaches **10 new gameplay functions** the start-only path
  never touched. **A full playable round (dynamic fighting actors on a stage): NO.**
- The precise next blocker is named in §4: in the boot-shortcut's attract sub-state, the panel
  START dispatch (`dispatch[2]`) is **not** the join/round-start handler, and the game never
  schedules the coin-poll / title / play-mode state machine that the fuller boot would set up.
  This is a boot-init/state-machine gap (true Approach-A territory), larger than one pass.

## Address mapping used
runtime (HWRAM) = SH-2 cart addr + 0x03E04000 = cart-file-offset + 0x05E04000.
Disassembly here is file-offset based: disasm_addr = runtime − 0x05E04000 = SH-2cart − 0x02000000.
The recompiled image's literal-pool DATA pointers are already HWRAM (0x060xxxxx) — the cart is
linked for HWRAM and the code is PIC; this was verified from the pool words (e.g. 0x0609867C).

## 1. The real coin→credit chain (traced from the game's own code)

All of this is the game's logic, recompiled and running; I only had to feed the coin in.

| step | where | what |
|---|---|---|
| coin input | BIOS svc `*0x06000664(player)` | per-frame coin-slot read (r4=0 P1, r4=1 P2); returns nonzero on a coin |
| coin detect | fn **0x060065f0** | polls 0x664; on a coin raises the coin-pending flag **0x0609880C** (word=1) |
| credit accrual | fn **0x060045f8** (called every frame from attract handler fn 06008868) | gate `0x0609880C==1 && 0x0609880E==1` → increments credit byte **0x06098674**, calls BIOS coin meter `*0x06000640`, clears the pending flag |
| coin-wait / start | fn **0x060089b8** | top reads credit 0x06098674; ==0 → read panel + dispatch `0x06090304[buf[3]]` (buf[3]=2 is START); !=0 → the credit-present branch (monitors buf[7]/P2) |
| credit display | fn **0x060084CC** | draws the credit text from the panel buffer field buf[0] (`buf[0]>1?dispatch[4]:dispatch[5]`) |

Note `0x06000640`/`0x0600063C` (the services the task brief named) are the boot/meter services,
each called **once at boot** (0x63C by FUN_02201958 as `g_06098804 = *0x0600063C(3,&g_06098674)`).
The **per-frame coin input service is 0x06000664**, not 0x640 — this is the service that actually
reports an inserted coin. Both 0x63C and 0x640 are fine left as benign no-ops (the game tolerated
it: credit still accrued), so the minimal faithful service is **0x664**.

## 2. What was implemented (tools/saturn-recomp/runtime/src/machine/bios.cpp + port/main_stv.cpp)

- **`stv_coin_read(player)`** — a faithful body for BIOS svc **0x06000664**: reports a scripted
  COIN on the given slot. Wired into the default service dispatch (`sp==0x06000664 →
  c.r[0]=stv_coin_read(r4)`). **Honest reachability caveat:** in the attract sub-state the
  boot-shortcut lands in, the coin-detect fn 0x060065f0 is **never scheduled** — only
  0x610/0x660/0x668 are polled per frame (verified: `STV_STVBIOS=log` shows 0x664 called 0×,
  0x640/0x63C called 1× at boot). So 0x664 is correct but currently dormant.
- **`deliver_coin(player)`** — therefore the harness delivers the coin by raising the coin-pending
  flag **0x0609880C=1 directly** (the exact value the real coin-detect writes). This substitutes
  **only** the one unrun hardware-coin-switch→0x880C detect step; **everything downstream**
  (accrual fn 0x060045f8, credit byte, coin meter 0x640, credit display, the attract→play attempt)
  is the game's own code. A COIN event (`--stvinput "…:COIN"`) calls this from the per-frame 0x660
  service (stv_panel_read). Documented: if a fuller boot ever reaches the 0x664-polling state,
  drop the direct 0x880C raise to avoid double-crediting.
- **START** is now delivered as the real control-panel action (`buf[3]/buf[7]=2`), the faithful
  path — not by force-raising the start-present flag. The old force-flag behaviour is preserved
  behind the diagnostic env **`STV_STARTFLAG`** (default off) for comparison.
- **Default `--stvinput`** changed from `"300:START1"` to **`"200:COIN,340:START1"`** so the
  default path demonstrates the coin→credit→start sequence.
- Saturn disc games unaffected: the whole 0x600–0x700 service window (incl. 0x664) is only routed
  to these bodies on the ST-V harness path (main_stv's install_bios_workarea); a disc boot never
  points there.

## 3. Results (evidence)

- **Credit registers (YES).** Watch 0x06098674 on the default run: a COIN at VB201 raises 0x0609880C,
  then the game's own fn 0x060045f8 increments credit **0→1→2 at VB203** (store attributed to
  pr 06008886 = the bsr to 0x060045f8 inside fn 06008868). No input, no COIN → credit stays 0.
- **Escapes the attract freeze (YES).** Baseline (no coin) is the byte-identical frozen attract HUD
  by ~VB900 (md5 **0ff787a8**, frame freeze_baseline_vb2100.png). The credited path is a different,
  live state at the same VBlanks (e.g. VB1200 md5 **2ee08f91**, paid_credit_hud_vb1200.png).
- **Mode partially transitions / reaches new code (YES, partial).** Paid-path autolearn
  (`port/autolearn_paid.sh`) learned **10 new indirect-call targets** that only the credited
  gameplay path reaches, then runs clean to 6000 VBlanks: 06049C88, 06048AA8, 0604A2BC, 06049DCC,
  06049E80, 0604892C, 060172D4, 0601730C, 0602ED7A, 0602FDF2. (The start-only path, by contrast,
  surfaced ~1 seed.) Frame paid_pleasewait_vb600.png shows a **PLEASE WAIT** game-start transition.
- **Actors spawn into the combat pool (NO).** The slot-pool active count 0x0609958C only cycles
  through the normal demo segments (VB30/443/695) then settles at **2 and holds static for 6000
  VBlanks** — no dynamic combat spawning. The per-frame spawn-context writes (fn 0602BCA8 on
  0x06099585/87/88) are animation/state counters, not new actors. g_06099580 shows no freelist
  churn. So no fresh fighting characters spawn.
- **Stage renders (NO).** The playfield stays the attract city-skyline NBG background; no stage
  floor, no character sprites (VDP1 sprite lists empty). This is partly the known RBG1 gap, but
  it is downstream of the logic blocker below (the round never actually loads).

## 4. The precise next blocker (named, with evidence)

**In the boot-shortcut's attract sub-state the panel START is not wired to the join/round-start,
and the coin-poll / title / play-mode state machine the fuller boot sets up is never scheduled.**

- The dispatch table 0x06090304 **is** populated (dispatch[2]=0x06050EEC for START, mode handler
  0x0609867C=0x060084CC) — peeked live. But START on the credit==0 path dispatches dispatch[2],
  and **dispatch[2] does not join**: START-only latches buf[3]=2 (0x0609868C=2 at VB501) yet the
  join byte 0x06098674 stays 0 — dispatch[2] in this state is some attract ack, not the round-start.
- On the credit>0 path the handler monitors **buf[7] (P2)**, not buf[3] (P1), so once a credit
  exists P1's START isn't re-dispatched here either — the actual P1 game-start must come from a
  different (play-mode) state that this attract state never transitions into.
- main_init_entry (cart 0x02201020, the harness entry) runs only FUN_02201958, installs the ISRs,
  programs VDP2, and tail-branches frame_sync_loop. It does **not** run the fuller boot state
  machine **func_022016a0** (runtime 0x060056a0 — a 10-state machine dispatched on a task's
  state byte ctx[0x10]) which calls **func_0201b92** (runtime 0x06005b92: sound init +
  per-frame panel read + controller-present 0x06098668 + config-byte setup). Evidence: func_0201b92
  never runs under the harness (its tell-tale store 0x0609868F=2 never fires; 0x0609868F is only
  written 0 by fn 06009B20).
- Routing more boot in doesn't trivially help: the PIC boot stub (cart 0x20013c) ends by jumping
  to the same main_init_entry (the image is already HWRAM-relocated, so the stub only re-copies +
  runs the security CRC), and func_022016a0 is a **task-state machine** that cannot be cold-entered
  — it needs the boot task to be installed the way the real boot does. So Approach A requires
  reproducing the **boot task install + state-machine drive**, not just a different `--entry`.

**Recommended next step (Approach A, properly):** install and run the boot state machine
func_022016a0 as the real boot does (find who installs its task + initial state; drive its states
incl. func_0201b92), with the security CRC at ~cart 0x200748 neutralised. That establishes the
title/insert-coin/play-mode states so that coin→credit→START reaches the real round-load. The coin
service (0x664) implemented here will then be exercised on its real polling path (drop the direct
0x880C raise at that point). Secondary, independent: VDP2 RBG1 for the stage floor.

## 5. Files touched / seeds / frames

- `tools/saturn-recomp/runtime/src/machine/bios.cpp` — `stv_coin_read` (0x664 body), `deliver_coin`
  (0x880C coin delivery), COIN handling in stv_panel_read, START as panel action (STV_STARTFLAG
  diag for the old force-flag), 0x664 wired in the service dispatch.
- `port/main_stv.cpp` — default `--stvinput` = `"200:COIN,340:START1"`.
- `port/autolearn_paid.sh` — new: autolearn loop on the credited path (STV_PAIDTEST removed; it
  runs the plain default coin+start script now).
- `port/seeds_extra.txt` — +10 paid-path seeds (listed in §3).
- Frames in `port/run/` and the Mac scratchpad: `paid_pleasewait_vb600.png`,
  `paid_credit_hud_vb1200.png`, `paid_credit_hud_vb2500.png`, `freeze_baseline_vb2100.png`.
  (Honest: these show the non-frozen credited HUD state, NOT fighting characters on a stage —
  that state was not reached.)

## 6. Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
cmake --build recomp-build --target saturn_stv -j"$(nproc)"
# default: coin -> credit (game accrues) -> start:
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 2600 \
    --shots 600,1200,2500 --out run
# prove credit accrues via the game's own logic:
./recomp-build/saturn_stv ... --watch 06098674:06098676   # 0x06098674: 0 -> 1 -> 2 at VB203
# which ST-V services are polled (0x664 is NOT, in this attract state):
STV_STVBIOS=log ./recomp-build/saturn_stv ...             # grep "STVBIOS svc"
# learn the credited gameplay-path walls:
./autolearn_paid.sh 30 2000 "200:COIN,340:START1"
```
