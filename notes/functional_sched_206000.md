# Functional decomp — scene-81 GAME cluster (cart 0x206000..0x209000)

Date: 2026-10-20. FUNCTIONAL (equivalence, not byte-matching) decomp.
Addresses: SH2 = cart.bin offset + 0x02000000; runtime(HWRAM) = SH2 + 0x03E04000.
All files compile clean with host `gcc -c -Wall -Wextra -std=gnu11` (ZERO warnings).
Each hand-transcribed and cross-checked against `m2c_fn.py --asm`; PIC branches/pointers
resolved with braf_resolve / ptr_resolve.

## Functions ADDED this pass (all in project/src_functional/)

| cart | file | name | fidelity |
|---|---|---|---|
| 0x206876 | 02206876.c | scene81_game_sm | HIGH |
| 0x206c88 | 02206c88.c | scene81_off16 | HIGH |
| 0x206d8c | 02206d8c.c | scene81_cam_update | HIGH |
| 0x206e2e | 02206e2e.c | scene81_stage_setup | HIGH |

These four are one SHC compilation unit (shared literal pool, 0x205ab6..0x207224).
`find_entry.py` MERGED 0x206c88+0x206d8c into one unit (their pools interleave); I split
them by the real prologue at 0x206d8c (0x2fe6 = `mov.l r14,@-r15`) and verified both decode
cleanly with valid resolved branch targets. 0x20686a (tiny leaf) was already done.

## scene81_game_sm (0x206876) — the GAME-scene state machine (PRIMARY target)

ABI: handler(task). entity = task->unk8. Installed as scene-81's off14 (descriptor 0x06073fb0).

### Part 1 — the START gate (0x206876..0x2068f0)
Reads cooked control word g_06098798 (= [g_06098794 + 4]). bit4 0x10 = P1 start-enable,
bit5 0x20 = P2 start-enable.
- If (g_06098798 & 0x30) == 0 → skip entirely (panel NOT polled).
- Else call the ST-V BIOS panel service `(*0x06000660)(0x060987D8)`, then:
  - if (g_060987E8 & 4)  [2-player panel present]:
      if ((bit4 && g_060987DB==2) || (bit5 && g_060987DF==2)) → entity->unk3c=0; task->state=5
  - else: if ((g_06098798 & 0x30) && g_060987DB==2) → entity->unk3c=0; task->state=5
So a legitimate START (with start-enable set) jumps straight to state 5.
This exactly matches the hand-disasm in notes/port/cooked_input_gate_status.md §1 and
player_spawn_scene_status.md §5 — now a compiling C function.

### Part 2 — state dispatch (task->unk10)
A cmp/eq chain (NOT a jump table) — rendered FAITHFULLY as a C `switch`. Progression
0 → 4 → 1 → 2 → 3 → 5. A common tail runs every frame after the switch:
`(*0x06090298)(); g_0609869C = 1;`.

- **state 0 (init):** g_06099578=0; video_state_init(-1); program the game display shadow
  (*0x06098F08): unkE, unk10..unk1C, unkF0..unkFC, unk114..unk118, unkE0, unk0 register
  fields; vdp2_setup_largefont(8); call display hook (*0x06090294); vdp1_draw_kick();
  func_02216b14(0x060506DC); seed g_0609882C aux obj from scenedesc->unk10; upload tile
  groups upload_texture_groups(scenedesc->unk0) + func_02215228(scenedesc->unk4) +
  func_02223ac0(scenedesc->unk8); entity_system_reset(); func_0223c95c();
  func_02216c38(scenedesc->unk20, scenedesc->unk24). → state 4.
- **state 4 (stage load):** optional SCSP upload (if scenedesc->unkC) sound_send_command;
  **scene81_stage_setup(task, scenedesc)** — loads the stage/character graphics (below);
  g_060B8B90->unk10=0x8000; g_0609861C=1; g_06099578=1; arm a 0x6C-frame timer (entity->unk50);
  sound_cue_dispatch(); clear the character selector g_0609856C and g_06098580. → state 1.
- **state 1 (intro timer):** count entity->unk50 down; while >0 just return via tail. On expiry:
  clear display words 0x114/0x116/0x118, spawn_obj_0x6c(task, 0x022101CE) (an intro object),
  seed spawned->unk8->unk50 = 8; if (g_060E0DDC) sound_queue_push(0xA01403FF). → state 2.
- **state 2:** if (entity->unk3c & 2)==0 [not busy]: set display words to 0xFF,
  spawn_obj_0x6c(task, 0x022100F8), seed spawned->unk8->unk50 = -16, entity->unk60=0x20000.
  → state 3. (If busy, nothing; stays in state 2.)
- **state 3:** entity->unk60 -= 0xF0F; if not busy: clear display words,
  spawn_obj_0x6c(task, 0x022101CE), seed spawned->unk8->unk50 = -2. → state 5.
- **state 5 (cart 0x206c40 — the GAME-START COMMIT):** if (entity->unk3c & 2)==0:
  * blank the VDP2 display: `*0x25F80000 = disp->unk0 & 0x7FFF` (clears the TVMD DISP bit);
  * clear the start/EDSR gates: g_06099578 = 0, g_0609861C = 0;
  * **set the scene-director advance bit: `task->unk14->unk8->unk3c |= 1`** (task->unk14 is
    the parent/director task; this is the "advance now" signal that scene_director_build
    (0x205b20) consumes to leave the GAME scene);
  * **kill all live objects: `kill_objects_above_id(0)` (0x0220fdc8)** — tears down nodes
    whose id byte > 0.

### IMPORTANT correction for the port team (state 5 ≠ literal player spawn)
The port notes (player_spawn_scene_status.md) hypothesised that scene-81 state 5 "installs/runs
the player spawn-script." The faithful disasm shows state 5 is the **game-start COMMIT**:
blank display + clear the start gate + signal the scene director to advance + tear down the
intro/attract objects. It does NOT itself allocate the player actor. The player spawn is
therefore installed DOWNSTREAM of the director advance that state 5 triggers (the director,
seeing g_06098668==81 and the advance bit, routes onward per scene_director_build). So:
  reaching state 5 (START through the gate, OR the natural 0→4→1→2→3→5 run with the intro
  objects clearing entity->unk3c bit1) → director advance → the next handler installs the
  player actor. The remaining port blocker is still what cooked_input_gate_status.md concluded
  (legitimate start-enable bits + a live scene-81 task actually ticking), plus tracing the
  handler the director runs AFTER this commit for the actual 0x0602Dxxx spawn.

Precondition decoded for state 5 body: **entity->unk3c bit1 (0x02) must be CLEAR**. Bit1 is
the "intro busy" flag held set by the spawned intro objects (templates 0x22100f8/0x22101ce)
while their animation plays; states 2, 3 and 5 all wait on it clearing. So the natural path
only reaches/executes state 5 after the intro objects finish.

## scene81_stage_setup (0x206e2e) — how the scenes 75/76 char/stage state is CONSUMED

Called by state 4 as `func(task, scenedesc)`; reads only `task` (its stage config fields),
which the earlier character/stage-intro scenes (75/76 per the scene graph) populate:
- task->unk30 (s32 flags): bit0 = enabled (early return if clear); bit8 = tiles via
  tilemap_copy_wrap_w80 vs straight DMA; bit5 0x20 = 8-byte vs 0x10-byte cell stride; bits
  1/2/3/4 = extra VDP2 register config blocks; bits >0x7F = another config block.
- task->unk34 (s32 flags): bit0 enables a SECOND BG layer (same build, VRAM 0x25E5F080/780).
- task->unk38 (s32): tile base / layer select — ==0 ⇒ layer A (cache g_06098690, VRAM
  0x25E00000), !=0 ⇒ layer B (cache g_06098694, VRAM 0x25E18000).
- task->unk3A (s16): cell-column count (loop bound).
- task->unk3C (ptr): stage-asset bundle → {unk0/unk4 cfg, unk8 pattern table {count,src},
  unkC palette table {count,src}, unk10 array of 8-byte cell records {.,src}}.

Behaviour (gated on the per-layer "already loaded" cache keyed on unk3C): DMA the pattern
table into VDP2 VRAM (sh2_dmac_copy), upload each cell record's tiles to 0x25E40000+, DMA the
palette to CRAM mirror 0x25F00400, clear 0x25E5C000, clear the display DISP bit, run the VDP2
register-config loaders (func_022220be from blocks 0x060507xx/0x0605082x) and plane builders
(func_02221f68/02222052/02221bd4/display_apply_cmdlist), build a packed 16-colour palette
(source 0x06050878) through the CRAM cursor g_06099510. Mark layer-dirty flags
g_060BACAC/B0/B4.

**Port takeaway:** to make the GAME scene show a stage, the port must have scene 75/76 fill
task->unk30/34/38/3A and task->unk3C (the asset bundle with pattern/palette/cell tables).
Those are the exact fields scene81_stage_setup reads; reproducing them is the "character/stage
state the spawn needs" the mission asked about. (This is the VDP2 BG path; the RBG1 playfield
is a separate known-missing chunk.)

## scene81_off16 (0x206c88) and scene81_cam_update (0x206d8c)

- **scene81_off16** (descriptor off16, rt 0x0600ac88): 4-state (task->unk10) camera/object
  intro move — seeds a pose, raises z then y with func_022105ca (aim) and snaps at limits,
  tail-calls func_02214a7a(obj). cmp/eq → switch.
- **scene81_cam_update** (same unit): per-object 2-state orbit updater keyed on (s8)task->unkF;
  uses sin_lookup for the orbit phase and tail-calls func_022154fc. Likely the update handler
  of one of the intro objects spawned by the state machine. cmp/eq → switch.

## Jump tables — decoded vs stubbed
- scene81_game_sm, scene81_off16, scene81_cam_update: the state dispatch is a **cmp/eq chain**,
  not a table — decoded faithfully as C `switch`. NO jump table present; nothing stubbed.
- All PIC computed CALLS (`bsrf Rn`) in the four functions were resolved via braf_resolve and
  rendered as named/func_<addr> calls. All mova+add DATA pointers resolved via ptr_resolve
  (intro-object templates 0x22100f8/0x22101ce, DMA anchors). ZERO M2C_ERROR stubs remain in
  the shipped files. (The one M2C_ERROR m2c emitted in game_sm state 4 was braf_rewrite's
  delay-slot artifact for sound_cue_dispatch @0x215fac — resolved by hand.)

## Resisted / deferred (in range, NOT shipped) — with reasons

- **0x20600e scene_credit_join_sm** (0x85c bytes): the credit / player-join UI state machine
  (states 0..3). Fully identified (see symbols file): polls panel *0x06000660, reads credit
  flags g_06098674/75 and panel buffer 0x060987D8/DC, renders insert-coin/join/player text via
  UI fn-ptr tables *0x060902A0 and *0x0609029C with string-ID tables 0x06050DF8..0x06050E39,
  and dispatches through a true fn-ptr table @0x06090304 indexed by player-ready state. Shares
  the scene81 init family and spawn_obj_0x6c templates; refs attract controller 0x204868.
  DEFERRED because it is large, uses a stack-local scratch struct (sp+8 byte fields) and many
  as-yet-unnamed UI fn-ptr/string tables — a faithful hand-conversion needs its own focused
  pass (and naming of the UI table entries) to meet the zero-bug bar. Not stubbed, not faked.
- **0x207360, 0x207542** (0x1e2 / 0x230): m2c reports an unresolved branch target
  (`Cannot find branch target` — a jump table or data-as-code extent); need decomp_bounded +
  manual table read. Deferred.
- **0x2079d0** (0x16e): object orbit/pose update (func_022213a8/0222142c, clamps ±0x4000);
  clean in m2c but non-trivial; not scene-81-specific. Deferred.
- **0x2080ac** (0x18e): state machine with one unresolved bsrf (`stc sr,r0` misparse);
  resolvable but deferred. 0x208a52 (0x146): clean cmp/eq switch, deferred.
- **0x207bcc (0x432), 0x2082be (0x6cc), 0x208b98 (0x4a6):** large general scene/engine
  handlers, not on the scene-81 spawn path. Deferred.
- Already done before this pass (in range): 0x207224, 0x2072ce, 0x207772, 0x207884, 0x207b3e,
  0x208030, 0x20825c, 0x2089d0, 0x20686a.

## Verification
Each shipped .c: `cd project/src_functional && gcc -c -Wall -Wextra -std=gnu11 <f>.c` → exit 0,
no warnings. Logic hand-checked line-by-line against `m2c_fn.py <addr> --asm`. braf/ptr
resolutions reproduced (targets land in valid ROM/HWRAM/HW windows).
