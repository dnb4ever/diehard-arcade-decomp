# Functional decomp batch 12 (Opus, single worker)

Headline: **6 new verified functional `.c`** (all "extras" - these bands are out-of-universe), every
one compiling clean `gcc -c -fsyntax-only -Wall -Wextra` with **ZERO warnings** and hand-traced
statement-for-statement against bounded SH-2 disassembly (`m2c_fn.py --asm`), with **every computed
branch and PIC pointer resolved** (braf_resolve / ptr_resolve). This batch is the DUAL-VALUE target
from `notes/port/play_transition_status.md`: it puts the **player-character join FSM chain** - including
the single function that gates the entire attract->play transition - and the **g_0609866c scene-director
writer** into C. Ran as a single serial worker (no forks), per the standing process rule.

## Authoritative deduped counts (`project/progress_functional.py`, of 1272 members)
```
functional  (src_functional):  333    26.18%   (+42 extra)
nonmatching (src_nonmatching):  73     5.74%
byte-exact  (src files)      :   5     0.39%
byte-exact  (oracle EXACT)   :   3     0.24%
ANY C, member-covered (deduped): 340   26.73%
ANY C, out-of-universe extras:   42
ANY C functions total        :  382
```
Baseline at batch-12 start: functional 333 (+36 extra) / ANY-C member 340 / ANY-C total 376.
**Net this batch: +6 extras, +6 ANY-C total.** Member-coverage is unchanged by design: the player-actor
FSM family (cart 0x2229xxx / 0x22axxx) and the scene-director (cart 0x2205xxx) are **not in the fn.units
member universe** - which is precisely why the port's three prior agents could not reach them. All six
are real, executing functions; they count as "extras".

## Functions added (offset -> name)
| cart addr | runtime | name | role |
|---|---|---|---|
| 0x02229b60 | 0x0602DB60 | `player_join_arm` | arms the join countdown that 0x229c00 runs down |
| **0x02229c00** | **0x0602DC00** | **`player_join_commit_fsm`** | **SOLE setter of start-present flag g_06098688 - the port unblock key** |
| 0x02229df4 | 0x0602DDF4 | `player_join_next` | next state after commit; case-1 finalises the join, morphs to 0x22863e |
| 0x0222a300 | 0x0602E300 | `actor_anim_begin` | actor spawn/arm state; runs + morphs to actor_anim_step |
| 0x0222a418 | 0x0602E418 | `actor_anim_step` | cfg-record (8-byte) anim-script step w/ fp-table dispatch |
| 0x02205b20 | 0x06009B20 | `scene_director_build` | **writes the game-scene descriptor g_0609866c** (scene-director target #2) |

## 1. The player-character join FSM - the port's attract->play unblock key, now in C
`play_transition_status.md` pinned the exact blocker: the attract->play morph fires iff
`g_06098688 || g_06098689` is nonzero, and the **sole setter** of that flag is cart 0x2229cac inside the
join-commit state at cart **0x2229c00**. That function is now decompiled and verified:

- **`player_join_commit_fsm` (0x2229c00)** does exactly what the port note predicted: on the tick its
  per-player countdown (`entity->unk c8`) and the word counter `g_0609978c[idx-1]` both reach 0 it executes
  `g_06098688[idx-1] = 1` (RAISE), registers the actor via `g_06099590[idx]`, publishes `entity->unk8/C/10`
  into the 12-byte record `g_06099764[idx-1]`, ORs `g_0604ebe4[idx-1]` into `g_0609977c`, and **morphs its
  own task handler to `player_join_next` (0x2229df4)**. (One honest correction to the note: the
  `g_06099590[idx]` store writes **0** here, not the actor pointer - r13 is pinned to 0 for the whole body;
  the note's "=r13" read r13 as the actor. The START-flag fact, the key one, is confirmed exactly.)
- **`player_join_arm` (0x2229b60)** is the state **before** it: once `entity->unk54 & 2` (ready) it sets
  `entity->unk c8 = 60` (the join countdown) and `entity->unk cc = 1` (blink phase) - the exact fields
  `player_join_commit_fsm` consumes. So the arm->commit->next chain is now contiguous in C.
- **`player_join_next` (0x2229df4)** is the handler installed by the commit: it dispatches on
  `g_0609868a[idx-1]` (the byte 0x229c00 clears) to finalise (case 1) or cancel (case -1) the join and
  morph on to 0x22863e. Bounded at 0x22a032 (fn.extent over-runs into two trailing sibling fns).

**Progress toward the port's eventual unblock:** the entire "get a 0x0602Dxxx actor with entity->unk34=1
stepped to the join-commit" path that `play_transition_status.md` S7/S8 asked for is now readable C. A port
agent can now (a) read precisely which entity fields must be set for the arm->commit transition to fire
(unk54 bit1, unk c8 countdown, g_0609978c[idx-1]), and (b) trust that reaching `player_join_commit_fsm`
with the counters drained raises g_06098688 and triggers attract_handler's morph on the next frame. The
remaining gap is upstream (what spawns/steps the 0x0602Dxxx actor and the scene spawn-script), not this
flag - exactly as the note concluded.

## 2. The scene-director writer g_0609866c - target #2, partial
- **`scene_director_build` (0x2205b20)** is a clean standalone writer of the game-scene descriptor
  `g_0609866c`: it selects a scene index and sets `g_0609866c = 0x06072660 + idx*0x50` (scene table, stride
  0x50), driven by a switch on the current descriptor's `cur[43]` selector plus the player-join bytes
  g_06098674/75. All three computed branches + the PIC morph (install `f_2205812`) resolve, and **every
  callee is already in C** (f_220fdc8, sound_queue_push, f_220fece, f_2205812). This is the first piece of
  the scene-director (target #2) in C.

## 3. Resisted / deferred (honest)
- **0x2201d2c and 0x2204df4** - the MAIN scene-director builders named in `play_transition_status.md`
  target #2 - are **NOT standalone functions**. Both are shared-frame continuations / alt-entries: 0x201d2c
  starts `mov.l r4,@r12` and reads r11/r12/r14/r9 with no prologue while restoring r8-r10 it never pushed;
  0x204df4 starts `mov.l r2,@r3; bra ...` and performs `jsr @r10`-type indirect calls that read the
  caller's stack frame (`@(8,r15)`..`@(36,r15)`) with live r8-r14. Their ABI is >4 registers inherited from
  an un-decompiled parent (the boot-state machine func_022016a0 region). Not faithfully expressible as
  standalone C; **deferred until the parent frame is mapped** (per the "ABI unconfirmed -> defer" rule).
- **0x229f60 / 0x22a16c** - trailing sibling fragments inside 0x229df4's over-run TU (find_entry maps them
  back into 0x229df4); handled by bounding 0x229df4, not shipped separately.
- Callees called faithfully but still undecompiled (declared extern, 1-arg where the asm sets only r4):
  func_0223dd28, func_0220a9c2, g_06000640 (resident query fp), func_0222ad7a, func_0222db44,
  func_0223dd96/df30/e17c/dcc2/e2ea/a6dc/a9d0, func_02227704/27c5c/27c22/2ca98, func_0228508, func_022863e,
  func_022a5ca.
- Untouched per constraints: port/ + tools/saturn-recomp/, 0x24axxx dispatch band, 0x2896xx data-as-code,
  confirmed misseg, non-standard-ABI 0x221354/0x221384, read-only m2c_fn/fn/match.

## 4. Next-band recommendation
1. **Finish the player-actor FSM morph targets** now that the join chain is in: `func_022863e` (from
   player_join_next), `func_0228508` + `func_022a5ca` (from actor_anim_step). They are the direct next
   states and are almost certainly the same (task,entity) standalone ABI - a clean continuation of this band.
2. **The scene-director parent** (boot-state machine func_022016a0 region) - mapping its register frame is
   the key that unlocks BOTH the deferred builders 0x201d2c/0x204df4 AND the scene-director trigger
   (boot ctx[0x10] -> states 5-10) that the port needs. This is the single highest-leverage dig for the port.
3. **The join-FSM callees** func_0223dd28 / func_0220a9c2 / func_0222db44 and the resident query fp
   g_06000640 - small, and they complete the semantics of the commit path.
4. Keep running `find_entry.py` before every m2c feed and `decomp_bounded.py` for any fn.extent over-run
   (it correctly split 0x229df4 this batch); stay a single worker.
