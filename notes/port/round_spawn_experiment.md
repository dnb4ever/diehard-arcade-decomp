# Die Hard Arcade (ST-V) RECOMP — the ROUND swing: synthesizing a player spawn-descriptor and running it through the REAL process_spawn_list

Date: 2026-10-20. Port side only (port/ + tools/saturn-recomp/). project/ + tools/decomp/ read-only.
Work in isolated copy ~/decomp/dh-portA/. Reference C read-only from project/src_functional/.
Address map: runtime(HWRAM) = cart SH-2 + 0x03E04000; cart SH-2 = cart.bin offset + 0x02000000.
Builds on: coin_arm_and_reel.md, player_spawn_scene_status.md, drive_real_flow_status.md,
functional_batch_report_12.md (join-FSM family in C).

## TL;DR — the SPAWN works, faithfully. The milestone this experiment set out to hit (instantiate a
## real player actor through the game's own spawn path and step its FSM) IS ACHIEVED, cleanly, with a
## synthesized type-1 descriptor. The DOWNSTREAM join→round chain does NOT follow from a spawn alone:
## the join is a player-actor STATE (entity->unk43), and reaching it needs the credit-join context the
## real START trigger builds — which forcing a few fields does not satisfy (it bad-returns). Two
## load-bearing CORRECTIONS to the mission premise are below. NOTHING was faked; no player was rendered.

## 0. Two corrections to the mission premise (both proven from the disassembly)

**(A) type-0 is NOT the player. type 1/2 are.** process_spawn_list (cart 0x222a5d0 / rt 0x0602E5D0)
dispatches on descriptor byte[0] (the "type"):
  - type 0  -> func_02227b60(&func_0222a300)  — the DYNAMIC-id registrar. It allocates the next free
    slot index >= 3 (table 0x06099590, count 0x0609958C, max 5) and sets entity->unk34 = that index
    (3 or 4). unk34 >= 3 FAILS the player-index gate in func_0222a300.
  - type 7  -> func_02227b60(&func_0222a970)  — dynamic, a different handler.
  - type 1..6 (and 8+) -> func_02227bc4(type-1, &func_0222a300) — the EXPLICIT-id registrar. It sets
    entity->unk34 = type at slot[type]. So **type 1 -> unk34 = 1 = P1; type 2 -> unk34 = 2 = P2** —
    these pass the `unk34 < 3` player gate (the per-player facing table 0x06099788 + control table
    0x060e0e38 path). So the player descriptor is **type 1 (or 2)**, not type 0.

**(B) func_0222a300 is the GENERIC actor-anim entry, and its FSM does NOT reach the join FSM.**
  func_0222a300 -> (per-frame) func_0222a418 (8-byte anim-script step) -> on the script's -1 terminator
  (idx<3) morph to func_0228508 -> morph to func_022863e (the player MAIN state); on a -2 terminator
  morph to func_0222a5ca (which NULLs *task = despawn). The join-commit FSM (func_02229c00, the SOLE
  setter of start-flag g_06098688) is NOT on this chain and is NOT installed by process_spawn_list.
  (See §4 for where it actually lives — this closes the "installed by NO code" open item from
  drive_real_flow_status.md / functional_batch_report_12.md.)

## 1. The 24-byte spawn-descriptor format (fully derived from process_spawn_list disasm)

process_spawn_list reads arg0->unk30 (offset 0x30) as a pointer to a packed list of 24-byte descriptors,
iterates until a descriptor's +20 word is 0 (loop-valid marker), and on exit writes the advanced cursor
back to arg0->unk30. Per 24-byte descriptor (offsets in bytes):

    +0  u8   type           (1 = P1 player, 2 = P2, 0 = dynamic non-player, 7 = type-7 handler)
    +1  u8   -> entity->unk5e (u16)   (also sets entity->unk ce = (unk5e==46)?1:0, the anim-fp index)
    +2  u8   -> entity->unk4f (u8)
    +3  u8   (used only on the unk34>=3 branch: signed arg to func_0x13ce8)
    +4  u32  hi16 (>>16, sign-ext) -> entity->unk98 (s32 countdown; inert unless entity->unk6d!=0)
    +8  u32  -> entity->unk8
    +12 u32  -> entity->unk0c
    +16 u32  -> entity->unk10
    +20 u32  -> entity->unk d0  (THE cfg/anim-script pointer) AND the loop-valid marker (0 = stop)

cfg (entity->unk d0) is an 8-byte-record anim script (4x s16): [0]=anim id, [2]=countdown, [4]=match
key, [6]=0xffff sentinel-or-unk26. func_0222a300 reads cfg[0] (via func_0222db44), cfg[2], cfg[6];
func_0222a418 reads cfg[0/2/4/6] and advances cfg += 8 on a matched step. A real example in ROM:
cfg @ rt 0x06063B98 = [1063,1,0,0x4000],[49,1,0,0x4000],[-2,0,0,0] (a 2-anim script ending -2).

## 2. The experiment (flag-gated diagnostic STV_SPAWNPLAYER, default OFF, byte-identical baseline)

Added to bios.cpp `stv_scene_probe_tick` (default-off; the whole function early-returns with no env —
verified byte-identical frame vs baseline). STV_SPAWNPLAYER="vb[:type[:cfgaddr]]": once at VB>=vb, it
builds a 24-byte descriptor (byte[0]=type, +20=cfg) + a terminator + a minimal context (ctx+0x30 = &desc)
in verified-zero scratch RAM (0x060FA000; aborts if not zero), then runs the GAME'S OWN process_spawn_list
(rt 0x0602E5D0) via sh2_call on g_master with full context save/restore. Everything from the registrar
down (func_02227bc4, the task allocator, func_0222a300 and its init callees) is real game code; only the
descriptor + cfg + the one-field context are synthesized. With a 3rd field it points +20 at a REAL ROM cfg.

HONEST split: synthesized = the descriptor bytes, the context's unk30, and (without the 3rd arg) the cfg.
Real = process_spawn_list, func_02227bc4, the allocator, func_0222a300 + func_0222a418 and every callee.

## 3. RESULT — a real player actor spawns and steps its FSM, cleanly (the milestone)

Run: `STV_BOOTMODE=1 STV_SPAWNPLAYER='300:1:0x06063B98'` (type 1 = P1, real ROM cfg), spawn at VB300
(slot 1 free; a --watch showed slot[1]=0x06099594 is free VB203..394, taken VB394..534 by a demo actor —
the VB400 first attempt collided and was correctly rejected by the registrar's slot-free check).

Observed (live --watch + sat_note):
 - process_spawn_list ran and returned CLEAN (ret_ok=1). No faults, no new walls.
 - A NEW task was allocated by the game's own allocator: task=0x060924C4, entity=0x060E2610.
 - entity->unk34 = 1 (P1) — passed the player gate. entity->unk d0 = 0x06063B98 (the cfg propagated).
 - task handler installed = 0x0602E300 (func_0222a300, the true player-actor entry).
 - Same frame: func_0222a300 EXECUTED (its unk34<3 player-init path: func_0222db44, func_0221707c,
   func_0223dd96/df30/e17c, animation_update — all clean) and MORPHED the task to 0x0602E418
   (func_0222a418). Confirmed by watching the handler slot 0x060924C4: 0 -> 0602E300 (allocator) ->
   0602E418 (func_0222a300's morph, pr 0602E3C4), all at VB300.
 - The actor then steps func_0222a418 every frame until VB634, when the attract reel loops/resets and
   tears down ALL actors (count 4->0) — my actor included.

=> A faithful player actor was instantiated through the game's real spawn machinery and stepped its FSM.
   This also proves the player-actor region is NOT behind an unreachable cycle — the attract DEMO already
   spawns player-family actors through this exact path (the VB400 collision found slot 1 already holding a
   func_0222a418 actor with a real cfg). The machinery is live; what the demo lacks is the JOIN state.

| experiment question | result | evidence |
|---|---|---|
| does process_spawn_list run on the synthesized descriptor? | YES, clean | ret_ok=1, no faults |
| is a player actor instantiated + registered in the slot pool? | YES | task 0x060924C4 @ slot[1], unk34=1 |
| handler = the player-actor entry func_0222a300? | YES | handler slot = 0x0602E300 |
| does it morph to func_0222a418 and step the FSM? | YES | slot 0602E300->0602E418 @VB300, steps to VB634 |
| does the join FSM raise g_06098688? | NO | --watch: 0x06098688 stays 0 all run |
| does attract_handler morph to the round controller? | NO | COV round_controller_join/dual ran=0 |
| a player character on screen? | NO | spawn frame byte-identical to baseline (see §5) |
| new walls when the player-actor region runs for real? | 1 real seed | func_022863e 0x0602863E (see §6) |

## 4. WHERE the join actually lives (closes the long-open "installed by NO code" item)

func_022863e (the player MAIN state, the convergence of both func_0228508 and func_02229df4) dispatches
every frame on entity->unk43 through a per-state function-pointer table at **0x06091100**, calling it
`(task, entity)`. A live dump of that table pins the join states exactly:
  - index **0x1D (29)** = 0x0602DB60 = **player_join_arm** (func_02229b60)
  - index **0x20 (32)** = 0x0602DC00 = **player_join_commit_fsm** (func_02229c00, raises g_06098688)
And player_join_arm sets entity->unk43 = 0x20 — i.e. arm -> commit is the unk43 transition that
functional_batch_report_12.md could not find. So the JOIN is a **player-actor STATE** (entity->unk43),
dispatched from func_022863e, NOT a separately-installed task. The real door into it is whatever sets a
credited player actor's unk43 to 0x1D (the credit-join trigger) — still un-located.

## 5. Driving the join by FORCING (STV_SPAWNJOIN) — bad-return boundary, honestly reported

To test the downstream directly, STV_SPAWNJOIN="vb[:handler]" (default off) forces the spawned actor into
the join state after func_0222a300 has initialised it: entity->unk43=0x1D, entity->unk54|=2 (ready bit),
g_0609978c[0]=1 (commit on first cycle), task handler=func_022863e. These four fields are FORCED/synthetic.

 - handler = func_022863e: immediate `FATAL: returned to 00000000, expected 06013E0E` (bad return /
   stack corruption) — func_022863e's full main-state body runs more than the unk43 dispatch (a HUD/record
   block + 3 sub-calls) and the forced actor lacks the state they assume.
 - handler = func_02229b60 (player_join_arm, isolation): runs the ARM with no crash (reaches ~VB660) but
   cannot progress to commit (the handler stays player_join_arm, so unk43=0x20 is never re-dispatched),
   then hits an unrelated corrupted call (0x0608E89C from pr 0x0007FB19 — a forcing artifact, NOT seeded).
 - In BOTH cases g_06098688 is NEVER raised; attract never morphs to the round controller.

HONEST verdict: the commit cannot be reached by forcing a handful of fields. func_022863e and the join
handlers depend on the broader credit-join context the real START path builds — the 12-byte record
g_06099764[idx-1], the active-slot publish g_06099590[idx], the paired object g_060995b0, the per-player
counters g_0609978c/g_0609977c, HUD/timer fields, plus a correctly-progressed actor (reaching func_022863e
NATURALLY via a318's -1 terminator, not a forced handler). I did NOT fabricate that whole context — doing
so would be deep forcing with no fidelity, and it still bad-returns. The remaining faithful work is the
credit-join trigger that (a) sets a credited player's unk43 to the join state and (b) sets up that context.

## 6. New seed (for main merge)

    0602863E   # player main-state func_022863e (cart 0x0222863e). Dispatches the per-player state table
               # 0x06091100 (incl. join-arm @0x1D, join-commit @0x20). Reachable in normal play via
               # func_0222a300 -> a418 -> 228508 -> 22863e; surfaced here when a spawned player actor is
               # driven into its main state. Real, cleanly-disassembled fn (project has 0x22863e.c-worthy).

No forcing-artifact walls were seeded (0x0608E89C deliberately NOT added).

## 7. Files changed this session (isolated copy ~/decomp/dh-portA/) — EXACT list

MODIFIED (source — reconcile into main tree):
 - tools/saturn-recomp/runtime/src/machine/bios.cpp — added STV_SPAWNPLAYER ("vb[:type[:cfgaddr]]") and
   STV_SPAWNJOIN ("vb[:handler]") to stv_scene_probe_tick. Both default-OFF and inside the function's
   default-off guard (verified byte-identical frame with no env). STV_SPAWNPLAYER synthesizes a faithful
   24-byte descriptor + cfg + context and runs the game's own process_spawn_list via sh2_call(g_master)
   with full context save/restore in verified-zero scratch (0x060FA000). STV_SPAWNJOIN is the forced-join
   diagnostic (§5). No change to the real flow when unset.
 - port/seeds_extra.txt — +1 seed 0602863E (func_022863e, §6).

REGENERATED (build artifacts; no manual edit): port/recomp/* (relearn_portA.sh added the 0602863E entry),
 port/recomp-build/* (cmake --build).

NEW (this report): notes/port/round_spawn_experiment.md

RUN ARTIFACTS (not source): port/run/round_spawn/ (spawn_realcfg_attractbg_vb450.png,
 baseline_attractbg_vb450.png — identical md5, the spawned actor is NOT rendered), port/run/spawnexp/,
 run/spawnreal/, run/spawnbase/, run/sj5/. Mac scratchpad: round_spawn_attractbg_vb450.png.

NOT modified: project/, tools/decomp/, notes/symbols.txt, port/main_stv.cpp, the other runtime sources.

## 8. Frames (honest; NO player rendered)

port/run/round_spawn/spawn_realcfg_attractbg_vb450.png — the spawn run (type 1, real cfg 0x06063B98) at
VB450 with the actor live in slot 1 stepping func_0222a418. It is BYTE-IDENTICAL (md5 b596d136...) to
baseline_attractbg_vb450.png (no spawn). The spawned actor steps its FSM in RAM but is invisible: the
synthesized/real-but-positionless cfg produces no sprite, and the playfield sprite/BG layer is the known
separate VDP1 rendering gap (per coin_arm_and_reel.md §5b, demo-fight sprites are garbled anyway). So the
frame shows the attract night-skyline RBG1 backdrop only. No player character was drawn; none was faked.

## 9. Reproduce (p620, in the copy)

    cd ~/decomp/dh-portA/port && cmake --build recomp-build --target saturn_stv -j"$(nproc)"
    # faithful player-actor spawn (clean; actor registers in slot 1, steps a300->a418):
    STV_BOOTMODE=1 STV_SPAWNPLAYER='300:1:0x06063B98' ./recomp-build/saturn_stv --cart diehard_main.bin \
      --cartrom ../cart.bin --vblanks 900 --out run/spawnreal --watch 06098688:06098690 --stvinput '200:COIN'
    # watch the handler-slot morph (task addr 060924C4 is deterministic for this run):
    STV_BOOTMODE=1 STV_SPAWNPLAYER='300:1' ./recomp-build/saturn_stv --cart diehard_main.bin \
      --cartrom ../cart.bin --vblanks 340 --out run/w --watch 060924C4:060924C8 --stvinput '200:COIN'
    # dump the per-player state table (join-arm @0x1D, join-commit @0x20):
    STV_BOOTMODE=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 350 \
      --out run/pk --peek 06091100:256 --stvinput '200:COIN'
    # forced-join diagnostic (bad-returns; shows the downstream boundary, §5):
    STV_BOOTMODE=1 STV_SPAWNPLAYER='300:1:0x06063B98' STV_SPAWNJOIN=360 ./recomp-build/saturn_stv \
      --cart diehard_main.bin --cartrom ../cart.bin --vblanks 900 --out run/sj --stvinput '200:COIN'
