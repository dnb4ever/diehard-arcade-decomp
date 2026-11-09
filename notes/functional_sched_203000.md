# Functional decomp - sched band cart 0x203000-0x206000 (attract->play transition cluster)

Mission: PORT-UNBLOCK - reproduce the genuine attract->play transition that installs & runs the
GAME scene as a live task. All 7 functions below compile clean under `gcc -Wall -Wextra`
(gcc 15.2, zero warnings) and were hand-traced against `m2c_fn.py --asm`.

## Functions added (project/src_functional/)

| cart / file     | name                        | notes |
|-----------------|-----------------------------|-------|
| 0x204868 / 02204868.c | attract_handler       | THE morph pivot |
| 0x204afa / 02204afa.c | round_controller_join | "credits present" morph target |
| 0x205232 / 02205232.c | round_controller_dual | free-play morph target; ENTER PLAY |
| 0x205812 / 02205812.c | scene_run_handler     | live-play task handler (thunk) |
| 0x2045f8 / 022045f8.c | attract_credit_join_service | per-frame coin/join edge svc |
| 0x20581a / 0220581a.c | player_slot_reset     | per-player state clear |
| 0x2058a8 / 022058a8.c | scene_bootstrap       | starts the whole state machine |

(scene_director_build 0x205b20 / 02205b20.c was already present; left untouched.)

## THE EXACT attract->play transition mechanism (port #1 unblock)

The game runs a SINGLE resident task object (`*g_06098678`, allocated by obj_install_noop_handler
0x201b6c). Its handler pointer lives at offset 0 of the object (`*(void**)object`). The whole
attract->play flow is a **self-morphing task**: each stage overwrites that handler pointer with the
address of the next stage, so no task is ever allocated or freed - the same object simply *becomes*
the next controller on the following frame. The task is invoked each frame with the object pointer
in r4 (`task`), and `entity = task->unk8` is the player/HUD entity holding the two countdowns
(`entity->unk40` = P1, `entity->unk44` = P2).

Stage graph (each arrow = `*(void**)task = <addr>`):

```
  scene_bootstrap (0x2058a8)
        | installs
        v
  attract_handler (0x204868) <--------------------------+-----------------+
        |  start flag g_06098688/89 set                 |                 |
        |                                               | join==4         | expire / idle
        |   credits (g_06099598+g_06099594)!=0          | or expire       |
        +---------------------> round_controller_join (0x204afa)          |
        |                              |  other player start              |
        |   else (free play)          +--> round_controller_dual (0x205232)
        +------------------------------------------------>  |             |
                                                            | countdown hits 0
                                                            v
                                                 scene_run_handler (0x205812) == PLAY
                                 (also installed by scene_director_build 0x205b20)
```

### 1. Bootstrap (scene_bootstrap, 0x2058a8)
Builds the scene descriptor `g_0609866c = 0x06072660 + (s16)(g_06098668 * 0x50)` (scene table,
stride 0x50), selects the per-frame scene-step function `g_0609867c` (0x204370 or warning/scene
step 0x2044cc per cfg byte g_060987e8 bit2), enables VDP2 display, and **installs attract_handler
into the task**: `*(void**)(*g_06098678) = 0x02204868`. From the next frame attract runs.

### 2. The morph fire condition (attract_handler, 0x204868)
Every frame attract calls `attract_credit_join_service()` then steps the demo scene
`(*g_0609867c)()`, then reads the two START edge flags:
```
if ((s8)g_06098688 + (s8)g_06098689 != 0)   // a START was pressed
```
That is THE trigger. On fire it immediately rewrites its own task pointer:
```
*(void**)task = (g_06099598 + g_06099594 != 0) ? 0x02204afa   // credits/coins present
                                               : 0x02205232;  // free play
```
then runs per-player "start pressed" bring-up: join state g_06098674/75 = 3, clears the start flag,
arms the countdown `entity->unk40/44 = 0x27f`, blanks the HUD, and notifies the system task
`(*g_06000688)(0x20 | 0x21)`. (Morph targets hand-resolved via ptr_resolve: mova anchor 0x2048d0 +
0x22a = 0x204afa; 0x20490c + 0x926 = 0x205232.)

### 3. Round controllers = two-player join countdown (0x204afa, 0x205232)
Both decrement `entity->unk40/unk44` each frame, print the countdown digit, and ramp the timer on
the frame-tick bits of g_06098794/g_06098734/g_0609874c. Terminal transitions:
 - **round_controller_join (0x204afa)**: a join byte reaches 4 OR a countdown expires -> morph back
   to attract (0x204868); the *other* player pressing start -> morph to dual (0x205232).
 - **round_controller_dual (0x205232)**: a countdown expiring -> **morph to scene_run_handler
   (0x205812)** -> ENTER PLAY. Its tail (a player in state 2) morphs to round_controller_join
   (0x204afa) if a countdown is still live, else back to attract (0x204868).

### 4. Player-actor spawn (port "spawn-script -> player actor")
Inside both round controllers, when `g_0609979a==1`, `(u8)(countdown + 0x82)==0`, `g_06098670==1`
and `g_06098830>=4`, the LATE-JOIN SPAWN fires: it takes the player actor `(*g_0609882c)->unk8`,
flags it (`g_06098830=4`, `(*g_0609882c)->unkD=1`, `actor->unkAE = 1 (P1) / 2 (P2)`) and randomizes
its spawn coordinates `actor->unkB0 = actor->unkB2 = rng_next() (func_02210020)`. join==4 / final
hand-off instead ZERO unkAE/B0/B2. The actual scene descriptor + spawn-script that drives the
player object are built by scene_director_build (0x205b20, already decompiled), which also installs
scene_run_handler (0x205812) into *g_06098678 - the same play handler the dual controller morphs to.

### 5. Entering play (scene_run_handler, 0x205812)
A 4-instruction thunk: `(*(void(**)(void))0x0609867c)();`. Once this is the task handler, every
frame the task steps the currently-selected scene (set by scene_director_build to the active game
scene). Reaching this handler is the state the port must reproduce to "run the game scene live".

## Jump-table dispatch resolution (KEY DIRECTIVE)

The round controllers were previously DEFERRED as "large table-dispatch state machines". **They
contain NO ROM jump tables.** `braf_resolve`/`ptr_resolve` flagged several `bsrf` sites as
"table/computed", but disassembling each (204cd0, 204cd4, 204e10, 204f78, 204fa4, 204fa8, 205164 in
0x204afa; 2052e4, 205474, 205620, 20573c in 0x205232) shows they all land **inside literal-pool
words** (surrounded by capstone `.byte`/garbage), i.e. they are pool DATA misread as instructions,
not dispatches. Every genuine computed branch back-traces to a single fixed pool-delta target and
was rendered as a direct named call. Likewise the two "bsrf" at 0x2048fc/0x2049f8 in attract and
0x205968/0x20596c in scene_bootstrap are literal-pool constants, not code. The real control flow is
a plain two-player countdown state machine (if/else), faithfully reproduced. **Nothing was
M2C_ERROR-stubbed; no case left unresolved.**

## Computed branches / PIC resolved (all via braf_resolve + ptr_resolve, hand-checked vs --asm)

 - Morph pointers (mova+add): 0x204afa, 0x205232, 0x205812, 0x204868, 0x204370, 0x2044cc.
 - bsrf/braf tail+call targets: f_238134 (0x238134), f_238176 (0x238176), sound_queue_push
   (0x215f6c), func_0228168 (0x228168), func_0223dcc2 (0x23dcc2), resident_cmd_issue_108
   (0x20aca8), rng_next (0x210020), sound_reemit_latched (0x216066), resident_cmd_reg_coords_70/74
   (0x20aa70/0x20ab34), func_0220f990 (0x20f990), func_0215f24 (0x215f24).
 - `mac.l`->s64: none in this cluster.

## Inherited-register ABI notes

 - All four task handlers (0x204868/afa/205232/205812) take the task object in r4; `entity =
   task->unk8`. They morph by storing to `*(void**)task` (offset 0 = handler slot).
 - attract_credit_join_service (0x2045f8) is called with r4 = task (attract) or r4 = 0x06098794
   (round controllers); the routine ignores its argument (operates on globals) - marked `(void)buf`.
 - In 0x205232, f_238176 is loaded once into r11 (mova 0x2053a4 + 0x32dd2 = 0x238176) and reused
   via `jsr @r11` throughout; rendered as direct `f_238176(7, n)` calls.
 - HUD print idiom (shared with existing HUD code): set g_060997a4 (color), g_060997a8 (mode),
   then `(*g_0609029c)(str)` (single arg) or `(*g_060902a0)(str, num)`. m2c's extra args on these
   calls are stale registers, dropped.

## Resisted / not done (with reasons)

 - 0x204370 scene_step_A: the per-frame attract/demo scene stepper pointed to by g_0609867c. Its
   unit (fn member 0x204362) is literal-pool-contaminated (find_entry real=False) and it is a demo
   renderer, not part of the transition/builder cluster - left for a later pass, referenced by addr.
 - 0x238134 / 0x238176 / 0x228168 / 0x23dcc2 / 0x20f990 / 0x215f24: out of band (>0x206000),
   declared extern with the args observed at the call sites; not decompiled here.
 - Byte-exact fidelity not attempted (this is FUNCTIONAL/equivalence-oriented decomp). Structure is
   from m2c with every call/branch/morph/spawn hand-resolved; countdown & spawn arithmetic
   hand-verified against the raw SH-2 disassembly.
