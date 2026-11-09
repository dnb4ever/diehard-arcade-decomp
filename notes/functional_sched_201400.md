# Functional decomp - scheduler/scene-director wave, band cart [0x201400, 0x203000)

SH2 range 0x02201400..0x02203000. Dual SH-2, big-endian. Equivalence-oriented
(not byte-matching). Every file hand-verified vs capstone / `m2c_fn.py --asm`;
every computed branch classified; all PIC `mova+add` pointers resolved by hand.
All new files compile with `gcc -c -fsyntax-only -Wall -Wextra` at ZERO warnings.

This was the PORT-UNBLOCK cluster: the scene-director / task-scheduler core.

## Functions added (offset -> name)
| SH2 addr | file | name | notes |
|----------|------|------|-------|
| 0x022016ac | 022016ac.c | boot_scene_director | #1 target. 10-state jump-table machine; table @0x201738 fully decoded |
| 0x02202138 | 02202138.c | scene_vdp2_setup_sm | 8-state VDP2/scene setup; cmp/eq ladders, NO real jump table |
| 0x02202d14 | 02202d14.c | scene_timer_hud_task | compact scene timer; tail-calls func_0221d302 |
| 0x022027e4 | 022027e4.c | scene_bootstrap_sm | 4-state scene bootstrap; tail-calls func_0220fdc8 |

Only remaining undone real fn.units entry in band is now covered. (0x201d2c
resolved as not-a-function; see below.)

## How scenes get instantiated and ticked as tasks (THE port-relevant mechanism)

Tasks are objects whose first word `*obj` is the per-frame handler function
pointer. A task carries: `unk8` = "entity" sub-struct, `unk10` = state byte,
`unk50` (in entity) = a per-scene frame down-counter, `unk3c` (in entity) = a
bitfield of ready/ack flags (bit0 and bit1 are the common "phase done" acks).

**Spawning** is done through a small family that all RETURN the new object in r0
so the caller can seed its fields:
  - `spawn_obj_0x6c` (0x0220fece) - (parent, handler) -> obj
  - `func_0220feec` (0x20feec), `func_0220ff90` (0x20ff90) - siblings
The caller pattern is `obj = spawn(task, &handler); obj->unk8->unk50 = N;` i.e.
instantiate the child task with its handler and arm its countdown. These are
exactly the calls the port needs to turn a "scene" into a live ticking task.

**Ticking**: each handler is re-entered every frame; it switches on its own
`task->unk10` state, advances state when a phase ack bit in `entity->unk3c` is
set (set asynchronously by the spawned child tasks / BIOS), decrements
`entity->unk50`, and fires effects at specific counter values.

### boot_scene_director (0x022016ac) - the attract->play sequencer
Reached via boot_display_mode_task (0x02201590), which installs it as `*task`.
10-state machine (jump table @ cart 0x201738, base = braf_pc+4 = 0x20171a):

| state | cart tgt | behaviour |
|-------|----------|-----------|
| 1 | 0x20174c | reset director obj -> noop_handler; func_0220acf6() -> g_060987f4/g_06098800; func_0201b92(task); state->2; opt BIOS *0x06000680(1); FALLS INTO 2 |
| 2 | 0x2017a0 | func_0201c9c(task); if entity ready (unk3c&1): install handler 0x204868, state->3; return |
| 3 | 0x2017cc | mark ready; read BIOS timer *0x06000640 -> g_060987f4; state->4; BIOS *0x06000688(0); FALLS INTO 4 |
| 4 | 0x2017ec | func_02205b20(task); if ready: install noop, feed elapsed to func_0220a962/0220a992, state->1; opt BIOS *0x0600060c, state->13 |
| 5 | 0x2018a8 | arm director; spawn handler 0x23673a; state->6; FALLS INTO 6 |
| 6 | 0x2018d8 | if ready -> state 7 |
| 7 | 0x2018e6 | spawn handler 0x236e66; state->8; FALLS INTO 8 |
| 8 | 0x201906 | if ready -> state 9 |
| 9 | 0x201914 | spawn handler 0x23358c; state->10; FALLS INTO 10 |
| 10 | 0x201934 | if ready: zero g_0609880c/10 (u16), state->1 (loop) |

The odd/even pairing (5->6, 7->8, 9->10) is the generic "spawn a scene child,
then wait for its ack bit before advancing" idiom - this is the attract->play
transition the port is blocked on: play scenes are instantiated as spawned child
tasks and the director only advances once each child flags entity->unk3c bit0.

### scene_vdp2_setup_sm (0x02202138) and scene_bootstrap_sm (0x022027e4)
These are the per-scene setup machines the director's children run. State 0/1
program the VDP2 register shadow (g_06098f08: scroll/priority/display bits) and
then spawn the scene's layer loaders, HUD task (scene_hud_task_init 0x20340c),
gravity/object tasks (thrown_object_gravity_task 0x202ba8), etc. Later states
drive timed sound cues (sound_queue_push 0x215f6c with 0xA0xxxxFF command codes),
fade/scroll ramps (field unk84 += delta each frame), and finally enable the VDP2
display (TVMD reg 0x25f80000 = shadow->unk0 & 0x7fff) and raise the scene-ready
flag entity->unk3c |= 1 - which is the ack the director above waits on.

## Jump tables decoded vs stubbed
- **boot_scene_director 0x201738**: 10-entry s16 braf table - ALL 10 resolved to
  real targets and rendered as a C `switch` with the asm's fall-through chain
  intact. NONE stubbed.
- **scene_vdp2_setup_sm / scene_timer_hud_task / scene_bootstrap_sm**: NO real
  computed jump tables. The "TABLE/COMPUTED via stc sr,r0 / bsrf r0" that
  earlier waves saw are **false positives**: `braf_resolve.py` disassembles the
  function body linearly and mis-decodes the several EMBEDDED literal pools
  (verified they are `.long` pool slots, e.g. 0x2023dc-0x2023eb, 0x202640,
  0x202678/7c in 0x202138; 0x202e-pools in 0x2027e4) as bogus `bsrf r0`
  instructions. The real dispatch in all three is a plain cmp/eq ladder on the
  state byte (and inner cmp/eq ladders on the unk50 counter). Nothing stubbed.

## Key finding: fn.extent over-merges tail-called functions
`fn.extent(0x202d14)` reports 0x6f8 and m2c fails ("cannot find branch target
func_20394a") because the unit merges ~5 separate functions that are chained by
`braf` tail-calls (0x202d14, 0x202d7e, 0x202dda, 0x202dfa, ...). The real
func_02202d14 ends at its own `braf` tail (0x202d7a) and is ~0x6a bytes. Worth
teaching the unit splitter to break on a `braf`-via-pool tail followed by a fresh
prologue. The siblings past 0x202d7e are left for their own passes.

## Resisted / deferred (with reasons)
- None deferred in this band - all real fn.units entries in [0x201400,0x203000)
  are now decompiled. The handful of inherited-r4 calls (func_0215fac,
  func_0216d58 in 0x2027e4; *0x06090294, func_0214d26, func_023c95c in 0x202138,
  func_023c95c in 0x202138) are marked MED and flagged inline; they are only
  functionally exact if the preceding callee preserves r4 (SH2 ABI does not
  guarantee this, so the port should re-check those 5 arguments against the real
  callees once those callees are decompiled).

## Shared callees / globals referenced (consistent with notes/symbols.txt)
spawn_obj_0x6c 0x20fece (returns obj), func_0220feec 0x20feec, func_0220ff90
0x20ff90, sound_queue_push 0x215f6c, func_0220fdc8 0x20fdc8. New scene handlers
spawned: 0x20340c scene_hud_task_init, 0x202ba8 thrown_object_gravity_task,
0x2100f8/0x2101ce/0x21fe00/0x204308/0x2041d4/0x2191ec/0x23673a/0x236e66/0x23358c.
Director globals: g_06098678 (director obj, *obj=handler), g_0609866c (scene
descriptor), g_06098f08 (VDP2 register shadow), g_0609882c (scene obj),
g_06098664 (phase gate), g_06099578 (scene-active flag). See
notes/symbols_sched_201400.txt.
