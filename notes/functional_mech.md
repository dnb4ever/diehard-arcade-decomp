# Functional decomp: scene content/completion mechanism (session report)

Target: Die Hard Arcade (Sega ST-V, SH-2, big-endian). Work in `project/src_functional/`.
Companion to `notes/port/scene_content_completion.md` (port-side behavior trace). Everything here
was decompiled with `m2c_fn.py` + `braf_resolve`/`ptr_resolve` and hand-verified against a
capstone address-annotated disassembly (computed-branch *labels* are unreliable, so every
bsrf/braf target was resolved and matched BY ADDRESS, per the workflow warning).

## 1. CORRECTED FILE: `220fece.c` (cart 0x20fece, f_220fece)

The committed file claimed f_220fece "ignores off14" and declared
`func_06013c3c(void)`. The raw asm shows the first call `bsrf r3` (-> func_06013c3c) has only a
`nop` delay slot, so the INCOMING r4=task and r5=off14 reach the allocator unchanged. The
`r5=1, r6=0x6C, r4=node` setup belongs to the SECOND call (func_06018bb8, the zero-init) only.

Corrected to:
```c
void *f_220fece(void *task, s32 off14) {
    void *node = func_06013c3c(task, off14);   /* was func_06013c3c() - args dropped */
    func_06018bb8(node, 1, 0x6C);
    return node;
}
```
Cross-checked against the allocator `f_220fc3c` (220fc3c.c), which is `(void *arg0, s32 arg1)` and
stores node[0]=arg1 (=off14, the handler) and node[0x14]=arg0 (=task). So f_220fece is the
off14-content-task installer the port trace identified. Compiles clean (gcc -Wall -Wextra, zero
warnings).

## 2. NEW mechanism functions

### `232610.c` - f_232610 (cart 0x232610, rt 0x06036610) - fade-scene content-task state machine
off14 handler for all 21 fade/transition attract scenes. arg0=task; task->unk8=content entity;
task->unk10=state (0..5); task->unk14=director task. Faithful 6-state machine:
- state 0: init VDP2 shadow (g_06098F08) + sync fade registers from PB (g_0609866C), state++.
- state 1: content setup - installs sub content tasks via f_220fece(task, 0x246308) and
  f_220fece(task, 0x22A120), calls func_02233250(task); state++; **falls through** into state 2.
- state 2: per-frame steppers (func_0222a8d8 / func_02216d58 / func_02243628). Leaves when
  content_entity unk3C bit3 is set AND resident != round_controller_dual (-> state 3), OR resident
  handler == scene_run_handler (-> state 5).
- state 3: unk50 frame-timer countdown, then one timeline step (f_220fece(task,0x2101CE)) -> state 4.
- state 4: **normal completion** - `director_entity->unk3C |= 1`, arm-flag g_06098670=1, tail-call
  func_0220fdc8(task->unkE + 0xFF).
- state 5: **play-enter completion** - `director_entity->unk3C |= 1`, arm-flag g_06098670=0.

Confidence HIGH. 36 computed calls resolved by address; 6 mova+add data pointers resolved; helper
arity verified from each callee prologue (the many no-arg steppers load their own operands).

### `2463c8.c` - f_2463c8 (cart 0x2463c8, rt 0x0604a3c8) - transition-timeline interpreter
**Important:** `find_entry` reports 0x2463c8 as the entry, but the register saves start at
**0x2463b8** (r12 is the live constant 7, set at 0x2463be, used in the brightness clamp). Decoded
from the true entry. Walks the scripted spawn timeline (cursor g_060973F0, column g_060973E0, step
g_060973E4, limit g_060973E8), gated per-frame by `g_06098564` (case 0 => early return = the exact
scene-3 stall) and driven by entity->unk40. Each matured record spawns a timeline object
(func_02227b60) and copies the record's fields in; then advances (cursor += 0x14). When the next
record's link (unk4) is 0 the script is done and it rebinds `task->unk0 = func_022465fc` - whose
next tick sets content-entity unk3C bit3 (the bit f_232610 state 2 waits on). Confidence HIGH.

### `233250.c` - func_02233250 (cart 0x233250) + func_022332d4 (cart 0x2332d4)
State-1 content builder (called by f_232610 state 1 via func_02233250(task)) and its tail-called
spawn-list walker. func_02233250 loads BG graphics (scene81_stage_setup / scene_bg_layer_build,
plus func_022078aa when scene index == 0x22), installs two content tasks (f_220fece handlers rt
0x060371c0 / 0x06037ea6), runs func_02233388 if te->unk4 set, then tail-calls func_022332d4 on the
spawn-list head. func_022332d4 spawns each 8-byte record (class = record->unk0 & 0xFFFF) via
func_0223cbb0, with a class-10/11 special store. Confidence HIGH. (This is the scene-8 content
handler 060372d4 that port note sec 3 named as the next seed wall - now decompiled.)

## 3. RESISTED / already-covered (no new file)

- **f_201cae** (cart 0x201cae): NOT a function entry. 0x201cae is mid-function - the real prologue
  (pushes r8-r14) is at **0x201c9c**, already decompiled as `0201c9c.c` (func_0201c9c, the fade
  sequencer). The 2P-arm write `g_06098670 = 1` the port note attributes to "f_201cae" is the
  `fade_scene_ready()` helper there, already correct and gated by `(g_06000798>>5)&1` +
  `g_0609880C==1`/`g_06098810==1`. Nothing to add.
- **func_06013c3c** (cart 0x20fc3c): already decompiled as `220fc3c.c` (f_220fc3c). Verified it
  matches the port trace (node[0]=off14 handler, node[0x14]=task). No change needed.
- **f_22a8d8** (cart 0x22a8d8): already decompiled as `0222a8d8.c` (func_0222a8d8), and it exactly
  matches the port note (writes g_06098564 from g_06098670 / join bytes). No change needed.
- **func_022465fc** (the content-entity bit3 setter, 0x246658): already decompiled as `022465fc.c`.
  f_2463c8 installs it; its `M32(obj,60) |= 8` is the bit3 set. No change needed.
- **Larger nearby helpers left for later**: func_0223c95c (>512 bytes), func_0223cd98 (~234 instr)
  and the resident handlers round_controller_dual (0x205232) / scene_run_handler (0x205812) are
  genuine but large; deferred. Steppers in the sibling-owned band 0x210000-0x218000
  (func_02216154/02214d26/022150d4/02215228/02215e20/02216b14/02216c38/02216d58/02215f6c/
  02215fac/0221603e) were NOT touched (sibling territory).

## 4. Files added/changed this session
- MODIFIED: project/src_functional/220fece.c (the fix)
- NEW: project/src_functional/232610.c, 2463c8.c, 233250.c
- NEW: notes/symbols_mech.txt, notes/functional_mech.md
All .c compile with `gcc -c -fsyntax-only -Wall -Wextra` at ZERO warnings.
