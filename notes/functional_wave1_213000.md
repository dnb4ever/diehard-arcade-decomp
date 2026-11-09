# Functional decomp - Wave 1, band 0x213000-0x220000

Scope: functions whose REAL entry (find_entry.py corrected) lies in [0x213000, 0x220000).
All files compile with gcc -c -fsyntax-only -Wall -Wextra -I. at ZERO warnings and are
hand-verified against m2c_fn.py --asm. mac.l/dmuls.l modelled as s64 MAC; PIC
(braf/bsrf/mova+add) resolved with braf_resolve / ptr_resolve.

Band census (find_entry): 182 real entries, 97 already done on entry, 85 undone.
This wave added 22 (21 undone band members + 1 in-band sub-entry dependency).

## Functions added (22)

High-fidelity, self-contained / math / hardware:
- 0x022130b4 atan2_fixed        - fixed-point atan2 via SH-2 DIVU 64/32 + arctan table; octant {sign,base} table reproduced from ROM @0x02213124.
- 0x022145d0 dma_ch0_transfer   - SH-2 on-chip DMAC ch0 block copy (SAR0/DAR0/TCR0/CHCR0=0x5A11), misalignment logger, bounded TE/AE wait.
- 0x022143b8 func_022143b8      - empty leaf stub.

Render / VDP1 pipeline:
- 0x022150d4 func_022150d4      - upload_texture_groups: clear char table, build char entries + DMA textures to VDP1 VRAM, push cmd 2, poll status.
- 0x022153f6 func_022153f6      - draw_child_sprite: matrix push, transform (mtx_apply_vec_at8), clip_test cull, project_sprite_quad, emit VDP1 cmd.
- 0x022156da func_022156da      - build_vdp1_sprites: deferred list @0x060dc648 -> command buffer *0x060cf0c4.
- 0x0221582a func_0221582a      - build_vdp1_sprites_rev: list @0x060da0c8 -> *0x060cf0c8 (buffer filled backward).
- 0x0221660c func_0221660c      - flush 4 draw-lists via dma_ch0_transfer (double-buffered by @0x06099574).
- 0x02216154 func_02216154      - video_state_init: double-buffer bookkeeping + conditional VRAM clears (two bsrf -> func_02213290).
- 0x022161c8 func_022161c8      - PIC thunk: braf -> func_02213290 to clear VDP2 VRAM.
- 0x022161dc func_022161dc      - init 3 identity transform records @0x06098ba8 (sub-entry reached by bsr from 0x02216154; provided so the caller is self-consistent).
- 0x02216530 func_02216530      - run 3 init subs, toggle flag @0x06099574.

Geometry / vertex:
- 0x02213e0c func_02213e0c      - quad backface + depth/screen-box visibility test -> 0/1 (uses dispatch helper func_02213d90).
- 0x02213d90 func_02213d90      - dispatch through fn-ptr table @0x06017db4 (nonstandard r8/r9 live-in; modelled (sel,obj)).
- 0x0221400c func_0221400c      - 4-tap weighted vertex blend (vec3 s32 >>7 + 2x s16 >>15), weights @0x06053570.

Entity / state-machine update handlers (obj in r4, work record at obj[8]):
- 0x022199da func_022199da      - 2-state sprite-angle sweep (sin_lookup 0x213190).
- 0x0221a5f6 func_0221a5f6      - menu_set_text_ptrs by selector @0x06098568.
- 0x0221a6a8 func_0221a6a8      - 3-state limb/recoil animation; emits via func_022153f6.
- 0x0221ad58 func_0221ad58      - scan {id,value} table, spawn matched behaviours via func_0220fece.
- 0x0221e7d6 func_0221e7d6      - two-phase orbit update -> sprite record (sin_lookup).
- 0x0221e844 func_0221e844      - 3-state periodic spawner (func_02210020 rng, func_0220ff54 spawn).
- 0x0221707c func_0221707c      - register input handler into slot table @0x060b6d9c.

Confidence: see symbols_wave1_213000.txt (most high; med where a callee ABI or table
semantics is inferred rather than observed).

## Resisted (with reasons)

- 0x02215386 - braf is an unconditional tail-jump into 0x022148ac (another, already-done
  function); the 0x60 bytes after the delay slot are a shared tail / branch target from
  elsewhere, not this functions body. Needs cross-function flow analysis to attribute
  correctly; decompiling it as a lone unit would be wrong.
- 0x02215964, 0x02215a2c, 0x02215cb4 - interdependent sprite-submission cluster. Multiple
  bsrf calls to ALTERNATE entry points (func+offset, e.g. 0x213d24, 0x2135a0) and, for
  0x215964, a cross-band target 0x24a06c. The callee micro-ABIs (args in nonstandard
  registers, func+N entries) must be pinned first; guessing risks silent incorrectness.
- 0x0221632e, 0x02216b14, 0x0221b14a, 0x0221b5a0, 0x0221dc04, 0x0221e8c4 and other
  UNRES rows in the triage - contain braf/bsrf that braf_resolve could not resolve
  (register value not statically known: table dispatch or runtime pointer). Treat as
  table-dispatch; would need switch/M2C_ERROR stubs or data-table reconstruction.

## Band remaining (not attempted this wave)

~62 undone members remain. Triage by PIC complexity (tools/decomp triage, SH2 addrs):
- CLEAN (no braf/bsrf), large: 0x0221582a(done), 0x022170ea(0x2ce), 0x0221746c(0x3cc),
  0x022182c6(0x426), 0x0221a6a8(done) - the three big CLEAN ones (170ea/1746c/182c6) are
  tractable next targets (no PIC; m2c reliable) but large; left for a focused pass.
- Simple RES (1-2 resolvable targets, often to done funcs 0x213190 sin, 0x213354,
  0x2145d0 dma): 0x02217888, 0x022179b0, 0x02217ae4 (all -> 0x213354), 0x0221fd7a,
  0x0221fe00 (-> 0x214a40), 0x0221ffb0 (-> 0x2153f6), 0x0221c8a0, 0x0221e9c6 (-> 0x20feec),
  0x0221c8f4, etc.
- Heavy RES/UNRES entity cluster (many bsrf, targets include 0x22a5d0/0x22a708/0x22a878/
  0x22dc2c animation_update and cross-band 0x24xxxx): 0x0221898c, 0x02219c44, 0x02219f26,
  0x0221a1a0, 0x0221a452, 0x0221a80c, 0x0221bf72, 0x0221c2aa..0x0221f9c4, 0x0221f532 etc.
  These are the bulk; they need the shared callee ABIs (spawn/animation helpers) fixed
  before they can be done safely.

Deliverables written: 22 .c files in project/src_functional/, this file, and
notes/symbols_wave1_213000.txt. No shared files modified.
