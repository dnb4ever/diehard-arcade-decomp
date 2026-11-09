# Functional decomp - LARGE DEFERRED SINGLES wave

Single worker, no forks. Targets were the "resolvable-but-intricate" large functions prior
waves deferred for size/complexity. Workflow: `find_entry.py` first (all real entries); feed
CART offsets to `m2c_fn.py`; resolve every computed branch with `braf_resolve` / data ptr with
`ptr_resolve` (trusting resolved TARGET addresses, not capstone labels); decode the one real
jump table via direct ROM read; use `decomp_bounded.py --end <code_end>` for the pool-lumped
units that defeat plain m2c. Every file compiles at `gcc -c -fsyntax-only -Wall -Wextra` with
ZERO warnings and was hand-verified against `m2c_fn.py --asm`.

## Key lesson confirmed
Most of these "deferred" targets are NOT single functions - fn.extent lumps a whole shared-pool
unit under the first member because forward branches keep `far` past every intermediate rts.
The literal pools sit *inside* the byte range, so a linear m2c sweep either mis-addresses
(0x2473ee) or mis-reads pool words as a "double-delay-slot" (`bsr;bra`) and bails entirely
(0x242272 / 0x2425d0 / 0x243066). The fix is to cut the extent at the real per-function
code_end and run decomp_bounded, then hand-resolve the bsrf/braf/mova.

## Functions ADDED (10 functions across 7 files, all compile clean, HIGH fidelity)

### 02473ee.c - actor_move_init (0x022473ee)
Inits an actor's motion/anim state from a move descriptor; switch on the phase class
func_02460c8(entity) (0..3). m2c decoded the body cleanly; only the 3 computed calls
(func_02460c8, func_0223afc4, func_0223ac50) needed resolving. The in-unit siblings below were
swept into its extent and are decoded separately.

### 0247508.c - actor_move_launch0/1/2 (0x02247508 / 0x02247530 / 0x02247580)
Three entry-variant wrappers; each picks a random move descriptor from a sub-list of
*(entity+0x174) and tail-calls actor_move_init. (m2c bails: tail-call target out of range.)

### 02475a8.c - actor_move_step (0x022475a8)
Per-frame step of the move state machine (phase unkD8 0..3), tail-calls animation_update.
m2c gave the full control structure; the rng&7 roll (m2c had mis-read as "0xE0 & 7"), the
func_02245fc4 gate, func_0249240, and the animation_update braf were hand-resolved.

### 0247924.c - actor_face_resolve (0x02247924)
Decides facing toward the target then commits a turn/strike or re-dispatches by phase.
m2c mis-read the mismatch switch selector as the literal 0x160 and mis-attributed the rng &3
index to func_024840a; both corrected against braf_resolve.

### 023df30.c - hud_draw_player_status (0x0223df30)
Per-player HUD status builder (timer/score digits + a value) each frame. m2c decoded with no
errors; resolved the two mova PIC glyph-draw primitives (func_02380c0/func_02380fa, 4-arg
color,x,row,glyph), the globals, and confirmed func_0223e104/func_0223e17c are 1-arg (they
overwrite r5/r6 before any read).

### 023c95c.c - hud_round_data_init (0x0223c95c)
Pure data-init: fills/clears several HUD tables and copies four ROM source tables into HWRAM,
then scatters a layout row selected by globals. The compiler had fully unrolled the copy loops;
re-rolled to the real iteration counts taken from --asm and the pool-resolved absolute addresses.

### 0242272.c - actor_pose_hold_state (0x02242272)   [was "m2c double-delay-slot failure"]
Task-handler for a timed pose / hit-reaction hold (state unkD4 0..3), tails animation_update +
func_0222ca98. The "double delay slot" was the unit's literal pool mis-disassembled; cut the
extent at 0x2423da and decomp_bounded produced clean C. Routine name is a reasoned guess.

### 02425d0.c - actor_throw_sequence (0x022425d0)   [was m2c failure]
Task-handler for a thrown/launched actor, 8-state machine on task->unk10 (spawn, trigger,
timers, oscillate, launch w/ screen effect, ballistic arc, land). decomp_bounded --end 0x24288c.
All states hand-verified incl. the r6 reuse feeding func_02215f6c. Routine name GUESS.

### 0243066.c - actor_special_move_state (0x02243066)   [was m2c failure]
Task-handler for a special/grab move state machine on entity->unkD4 with a top-of-frame abort
check; scans actor slots 1,2 for an in-range target (func_0242fde). decomp_bounded --end
0x2433e8. Preamble constants (r5=3,r8=4,r10=5,r12=2,r13=1) resolved; vacuous `&& r13` dropped;
rng is nilary; unk5C is a word here. Routine name GUESS.

## RESISTED / DEFERRED (with reasons)

### 0x206308 - cutscene/attract scene director (~0x560 bytes, single function)
DEFERRED. Genuinely one function, but m2c loses tracking of every callee-saved register
(r8-r14 are persistent pointers/values held across the whole body) and emits `saved_reg_*`
placeholders plus 4 errors - the output is not patchable. Faithful decode requires a full
from-scratch hand-trace pinning r8 (an actor/mode object), r9 (a text-draw fn ptr), r10/r13
(draw x/y cursor state), r11/r12/r14 across ~731 asm lines. High error risk; not shipped
unverified. The 3 spawns (func_0220fece spawn_obj_0x6c with PIC handler 0x2101ce) and 3 PIC
pointers ARE resolved (braf/ptr_resolve) for whoever picks it up.

### 0x226eb8 - ~1836-byte state processor with embedded 11-way computed jump table
TABLE DECODED (see below); handlers pending. The jump table at 0x227088, indexed by
state = MU8(obj,16) (guard >=11), braf base 0x227076, resolves to 11 in-function handlers:
  state 0->0x22709e, 1->0x2270c4, 2->0x2271d0, 3->0x2272f8, 4->0x22736a, 5->0x2273a4,
  6/7->0x227444, 8->0x22748e, 9->0x2274d2, 10->0x22759c.
m2c reproduced the preamble but completely mangled the dispatch and the 11 handler bodies (it
rendered the braf as linear fallthrough). Each handler needs a full hand-trace from --asm; left
for a dedicated pass to avoid shipping an unverified 11-case machine. [If this session's later
pass completes it, the entry above moves to ADDED.]
