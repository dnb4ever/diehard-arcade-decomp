# Functional decomp - scheduler wave 2: attract->play scene/credit handlers

Date: 2026-10-20. FUNCTIONAL (equivalence, not byte-matching) decomp.
SH2 = cart.bin offset + 0x02000000; runtime(HWRAM) = SH2 + 0x03E04000.
All files compile clean on host `gcc -c -Wall -Wextra -std=gnu11` (ZERO warnings).
Each hand-transcribed and cross-checked vs `braf_rewrite.py <addr> --asm`; PIC
branches/pointers resolved with braf_resolve / ptr_resolve. `find_entry.py` run first
on every target (all were already-real entries).

This wave finished the PORT-RELEVANT scene/credit handlers the scheduler/transition wave
deferred - they complete the attract->play picture.

## Functions ADDED this pass (project/src_functional/)

| cart | file | name | fidelity |
|---|---|---|---|
| 0x20600e | 0220600e.c | scene_credit_join_sm | HIGH |
| 0x22992c | 022992c.c  | actor_launch_dispatch | HIGH |
| 0x229af4 | 0229af4.c  | actor_launch_countdown | HIGH |
| 0x204370 | 0204370.c  | scene_step_A | HIGH |
| 0x208a52 | 0208a52.c  | disp_shutter_sm | MED |
| 0x2080ac | 02080ac.c  | line_scroll_fx_sm | HIGH |
| 0x207542 | 0207542.c  | aux_obj_orbit_update | HIGH |
| 0x207360 | 0207360.c  | scene_bg_layer_build | HIGH |
| 0x2079d0 | 02079d0.c  | wave_distort_update | HIGH |

## PRIMARY #1 - scene_credit_join_sm (0x20600e): the credit / player-join UI

ABI: handler(task). entity = task->unk8. Per-frame switch on task->unk10 (cmp/eq chain,
NO main jump table). Common tail every frame: (*0x06090298)(); g_0609869C = 1.

- **state 0 (init):** EDSR gate off; video_state_init(-1); program the game display shadow
  (*0x06098F08) VDP2 register fields (unkE/unk0/unkE0/unkF0..unkFC, (disp+0x10) block);
  vdp2_setup_largefont(7); (*0x06090294)(7); vdp1_draw_kick; upload stage groups from the
  scene descriptor (*0x0609866C); seed the aux display obj (*0x0609882C) (unk0=ROM data
  0x0220686A); entity_system_reset; func_0223c95c; stage_vram_loader + scene_layer_loader_spawn
  + func_022082be(task, sd); clear entity timers; arm the scene flags (g_06098670=1,
  g_060987D0=1, g_0609861C=1, EDSR gate=1); spawn intro obj (tmpl 0x022100F8, unk8->unk50=0x40);
  g_06098580 |= 0x20000. -> state 1.
- **state 1:** when the intro obj finishes (entity->unk3C bit1 clear), arm the coin service
  (*0x06000680)(1). -> state 2.
- **state 2 (MAIN, see credit_render()):** poll the panel (*0x06000660)(0x060987D8); render the
  header prompt (string 0x06050DF8 + 0x06050E0C/E20 by panel type), the credit line(s) (string
  0x06050E30 with a value from g_06090300 or g_060902F4[action]), and the per-player join slots;
  then handle coin-insert (countdown expiry) and START-button joins. -> state 3 on commit.
- **state 3 (COMMIT):** when the exit obj has finished, install **attract_handler (0x02204868)**
  as the next task handler (`*(*0x06098678) = 0x02204868`), set the scene-director advance bit
  (`task->unk14->unk8->unk3C |= 1`), blank the VDP2 display (`*0x25F80000 = disp->unk0 & 0x7FFF`),
  clear the start/EDSR gates, `kill_objects_above_id(0)`, `g_060987D0 = 0`. If NOT finished
  (busy bit set or unk60==0) the original BRANCHES BACK INTO the state-2 render for this frame;
  reproduced by calling the shared `credit_render()`.

### THE 0x06090304 DISPATCH (faithfully decoded, per directive - NOT stubbed)
`g_06090304` is a RAM-resident descriptor table of per-player "join / insert-coin digit"
glyphs (the SAME table attract_handler @0x02204868 maintains). It is **indexed by player-ready
state**: for a panel slot whose count byte (panelbuf->unk0) is <= 1 the code selects entry
**[4]**, otherwise entry **[5]**, and passes the selected descriptor to the glyph thunk
(*0x0609029C). Entries 4/5 are the "not-ready" / "ready" join glyphs. Because the table lives
in RAM (populated at runtime, not ROM constants), the faithful rendering is the real indexed
indirect load+call - the index computation IS the dispatch (rendered in `join_digit_desc()`).
Each slot also draws the ROM player-number glyph g_060506F0[count] and a name glyph that is
scratch-built from a ROM 3-byte record (0x06050E39/E33/E36) offset by the panel's unk1/unk2,
or the default string 0x06050D50. The name glyph deliberately sets NO cursor - it reuses the
number glyph's auto-advanced cursor (confirmed in asm).

### Join / coin logic
- countdown timer = `g_06000654 & 0x7F` (0x7F = idle); when it hits 0 and not yet committed,
  consume a coin per un-joined player ((*0x06000664)(player)) and set the coin-accept latches
  g_0609880C / g_06098810.
- while the countdown runs, a START press (panel action byte ==2) with the cooked-control
  start-enable bit (g_06098798 bit4/bit5) consumes a coin and latches the join; two-player vs
  single-player panel paths (g_060987E8 bit2) handled separately.
- on any latch, spawn the exit obj (tmpl 0x022101CE, unk8->unk50 = -0x14), set entity->unk40=1,
  task->unk10 = 3.

### UI thunk ABI note
The two HUD thunks take their screen position from the cursor globals written immediately
before each call: g_060997A4 (color) / g_060997A8 (mode) (confirmed by dmac_copy_hw and the
attract notes in symbols.txt). ui_printf (*0x060902A0) is variadic `(fmt, ...)`; ui_glyph
(*0x0609029C) takes one descriptor pointer. Modeled as per-call-site casts.

## PRIMARY #2 - actor_launch_dispatch (0x22992c) + sibling actor_launch_countdown (0x229af4)

Per-actor launch/knockdown physics handler (task, entity); part of the 0x228xxx-0x229xxx actor
state-step family; tail-calls animation_update (SH-2 braf). The real entry 0x22992c is confirmed
(the fn.units grouper over-runs 0x22988e's extent); extent 0x22992c..0x229af4 ends at the braf
tail. The hidden sibling begins at 0x229af4 (fn.units merges it in; my find_entry/rts scan pinned
it) - the mission's "~0x229ad0" estimate was the merged extent; real = 0x229af4.

### The dispatch (faithfully decoded, per directive)
- **Preamble (RAM fn-ptr table):** when `unkCC==0 && unkB0!=0`, fire a one-shot hit reaction
  through the RAM-resident table **g_060911BC indexed by the mode flag g_0609979A**:
  `(*g_060911BC[g_0609979A])(entity, 0xE)` (the same reaction table actor_pose_dispatch uses).
  Rendered as the documented indexed indirect call (table is RAM, populated at runtime).
- **Secondary switch on entity->unk88 (s32), cmp/eq chain -> C switch:**
  - `0x54` airborne arc: integrate (zone-record flag nudges unk78; accel unkCE/unkC4/unk7C while
    unk58<8, else decel via unkC8); on land (unk58>8 && unk7C<0) -> unk88=0x56,
    actor_facing_resolve, unk41=1, unk5C = g_06087A9A[unk45*8].
  - `0x55`/`0x56` settle: reset arc fields, unk5C = g_06096C74[unk34], then branch on pose state
    (unk6D / unk8C bit 0x02000000): actor_pose_load + actor_facing_resolve + unk43=0xD, or
    unk84=1 / unk52=0 / unk43=1.
- **Sibling (0x229af4):** animation_update first, then if `unk54 & 2` run a 60-frame countdown on
  unkC4 that on expiry sets unkCE=1, unkC8=0x3C, unkCC=1, unk43=0x1E (the settle re-arm).

NB: SH-2 `mov #imm` immediates here are DECIMAL in capstone output (#84=0x54 etc.) - offsets
were confirmed against m2c's labelled output.

## scene_step_A (0x204370) - the demo join-prompt stepper (g_0609867C)

Clean m2c decode (no M2C_ERROR, no jump table, no PIC calls). Confirms the credit-SM join-UI
decode: it reuses the SAME g_06090304 digit table, g_060506F0 number table and ui_glyph thunk.
Gated on `(g_06098794->unk6 & 3) && !(g_060987E8 & 1)`: reads the panel and draws the P1
(cols 0xA/0x12) and P2 (0x16/0x1E) join slots (name scratch from ROM 0x06050D4C / 0x06050D53),
arms the g_06098680 prompt timer (0x3C), and on expiry draws the final glyph 0x06050D58.

## VDP2 / effect handlers (0x206-0x209 band)

- **line_scroll_fx_sm (0x2080ac):** line-scroll distortion SM. Writes an alternating +/- wobble
  via cursor g_06099510, flushes to line-scroll VRAM 0x25E3FC00 (func_0221632e), decays the
  amplitude; hold timer; slide-out with func_022213a8 then detach. (NB: braf_rewrite MISLABELS
  the state-2 calls - the true targets were resolved from the pool deltas against braf_resolve:
  state-2 call 1 = func_022213a8, call 2 = func_02221384.)
- **disp_shutter_sm (0x208a52):** display SM keyed on g_06098580 bit 0x40000; eases entity->unk10
  to 0x10000 + func_022214d2 draw, decays a unk32 timer into disp->unk108, else func_02221384(1)
  + detach. (MED - the exact gameplay role of func_022214d2's 6-arg call is unconfirmed.)
- **scene_bg_layer_build (0x207360):** VDP2 BG-layer/nametable builder (task,entity). m2c's
  "Cannot find branch target .L2074bc" was a `bra`-to-bottom do/while LOOP, NOT a jump table -
  rendered as a plain for-loop. Loads reg-config blocks (func_02220828, sel by entity->unk40),
  DMAs palette+pattern (func_022145d0), clears+fills the nametable (func_022132ac +
  tilemap_copy_wrap_w80 loop over unk4A cells), func_0222156c, then spawns aux_obj_orbit_update.
- **aux_obj_orbit_update (0x207542):** the spawned per-frame updater; sin-orbit position relative
  to the aux display object; tail-calls func_022213a8(x,y). (The 0x207542 target was a small
  standalone fn; braf_rewrite over-ran into the following builder.)
- **wave_distort_update (0x2079d0):** builds a per-scanline sin-wave line-scroll/rotation
  coefficient table (0xF0 lines x2 longwords -> VRAM 0x25E3F800 via func_0221632e + cursor
  g_06099510). Note the two inner sin calls (sin_lookup 0x213190, func_022131a8) store their
  result into the cursor slot, not into a callee arg - m2c mis-rendered this as a 2-arg call.

## Jump / dispatch tables - decoded vs stubbed (per the DIRECTIVE)

- scene_credit_join_sm, actor_launch_dispatch: both carry RAM fn-ptr tables (g_06090304 and
  g_060911BC). Both are faithfully decoded as the real indexed indirect load/call (the index
  computation is the dispatch); NOT M2C_ERROR-stubbed. Their main state machines are cmp/eq
  chains rendered as C switches (no code jump table).
- scene_bg_layer_build's apparent jump table (.L2074bc) was a loop-back, not a table - rendered
  as a for-loop.
- ALL bsrf PIC calls in every shipped file were resolved (braf_resolve + manual pool-delta
  computation where braf_rewrite mislabeled). ZERO M2C_ERROR stubs remain in shipped files.

## Resisted / deferred (with reason)

- **0x207bcc func_02207bcc** (0x432, ~614 asm lines): large VDP2/tilemap builder in the 0x207xxx
  effect family, NOT on the scene-81 spawn path. braf_resolve MISFIRES: 0x207d66 "func_082a02d6"
  is actually a `mov.l #0x0609856C` (character-selector global) misparsed as a bsrf delta; 0x207ea4
  "mov.l @r15+,r2" is a stack-pop misparsed as a computed branch. m2c also emits a 10-argument
  indirect fn-ptr. Faithful zero-bug conversion needs a focused pass untangling those misfires +
  the 10-arg call ABI. Not stubbed, not faked.

## Verification

Each shipped .c: `cd project/src_functional && gcc -c -Wall -Wextra -std=gnu11 <f>.c` -> exit 0,
no warnings (all 9 re-checked together). Logic hand-traced line-by-line against
`braf_rewrite.py <addr> --asm`; branch/pointer resolutions reproduced (targets land in valid
ROM / HWRAM / VDP2 windows). Note for the maintainer: braf_rewrite's instruction LABELS are
unreliable in these functions (it mislabeled resolved bsrf targets in 0220600e, 02080ac and
elsewhere) - the authoritative targets are braf_resolve's report + direct pool-delta computation,
which is what the shipped C uses.
