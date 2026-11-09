# Functional decomp report - Wave 1, band cart [0x201400, 0x20a000)

SH2 range 0x02201400..0x0220a000. Dual SH-2, big-endian. Equivalence-oriented
(not byte-matching). Every file hand-verified statement-by-statement vs
`m2c_fn.py --asm`; all computed branches resolved with `braf_resolve.py`;
PIC/mova pointers resolved by hand. All 16 new files compile with
`gcc -c -fsyntax-only -Wall -Wextra` at ZERO warnings.

## Band census
- fn.units real entries in band: 58
- Already done before this wave: 17 (incl. 0x205b20)
- Undone at wave start: 41
- **Added this wave: 16**
- Remaining undone in band: 25 (+ 1 bonus entry 0x203c28 found but not a fn.units member)

## Functions added (offset -> name)
| SH2 addr | file | name | notes |
|----------|------|------|-------|
| 0x02201590 | 02201590.c | boot_display_mode_task | alloc + table state-select + BIOS display cb |
| 0x02201b6c | 02201b6c.c | obj_install_noop_handler | installs noop_handler into *obj |
| 0x02202a28 | 02202a28.c | keyframe_object_mover | keyframe tables 0x0604edc0/0x0604eec0 |
| 0x02202ba8 | 02202ba8.c | thrown_object_gravity_task | arc init + integrate + tail 0x21b330 |
| 0x02203500 | 02203500.c | rumble_dispatch_pulse | parity-gated *g_0609029c dispatch |
| 0x02203576 | 02203576.c | rumble_hud_service_2p | builds 0x0609d7b0 record, both players |
| 0x02203afc | 02203afc.c | rumble_hud_service_1p | sibling, ctx 0xB |
| 0x0220340c | 0220340c.c | scene_hud_task_init | selects handler from g_060987e8 cfg bits |
| 0x022044cc | 022044cc.c | warning_hud_blink | timed notice, g_06098680 countdown |
| 0x02207224 | 02207224.c | stage_vram_loader | DMAC loop into VDP VRAM + spawn |
| 0x02207772 | 02207772.c | scene_layer_loader_spawn | func_021675e loop + spawn 0x207884 |
| 0x02207884 | 02207884.c | draw_transform_stub | stub tail-calls 0x22213a8 |
| 0x02207b3e | 02207b3e.c | framebuffer_bringup | 2 DMACs + clear + submit |
| 0x0220825c | 0220825c.c | scene_layer_loader | loader, no spawn |
| 0x02208030 | 02208030.c | scene_layer_loader_spawn_a | spawn 0x2080ac, seed obj |
| 0x022089d0 | 022089d0.c | scene_layer_loader_spawn_b | spawn 0x208a52, seed obj |

## Key findings
- A family of **scene/HUD task handlers** sits at the low end of the band
  (0x203500/0x203576/0x20376c/0x203afc/0x203c28), all selected by the config byte
  g_060987e8 inside scene_hud_task_init (0x20340c). They build controller
  rumble/HUD "records" at 0x0609d7b0 and submit them through a global dispatch
  function-pointer slot *g_0609029c (context passed via g_060997a4/g_060997a8).
- A family of **VDP layer/stage loaders** (0x207224/0x207772/0x207b3e/0x20825c/
  0x208030/0x2089d0) DMAC ROM blobs into VDP VRAM via sh2_dmac_copy (0x2145d0),
  zero VRAM via clear_vec4_array (0x213290), emit sprite/char blocks via
  func_021675e (0x21675e), submit via func_022156c (0x22156c), and chain the next
  task via func_0220ff54 (0x20ff54, which RETURNS the new object in r0 - several
  callers then seed obj->unk8 fields).
- **func_0220ff54 returns the spawned object** (m2c renders r0 as the mova base
  because it does not model the call clobbering r0 - verified by the post-call
  `mov r0,r4; mov.l @(8,r4),r4` field writes).

## Resisted / deferred (with reasons)
- **0x02202138** (vdp2_scene_setup, 0x650) - 35 computed branches incl. several
  TABLE/COMPUTED `stc sr,r0`/`bsrf r0` dispatch. Needs switch/M2C_ERROR stubs and
  heavy verification; large. Defer.
- **0x02202d14** (0x6f8) - 52 computed branches, multiple table dispatch + data.
  Defer.
- **0x02204afa / 0x02205232** - 25/14 branches with `mov.l @(r0,r10),r0` jump
  tables and cmp/eq ladders (table dispatch). Defer (needs faithful jump tables).
- **0x02205c96 / 0x0220600e / 0x02206876 / 0x02206e2e** - large state machines
  (14-20 branches); tractable but long. Deferred for a later pass.
- **0x02209188** (0x11d2) - large, braf jump-table dispatch (mov.w @(r0,r1),r0).
  Defer.
- **0x02202027e4** (0x2027e4, 0x1c0) - multi-state scene bootstrap; many bsrf
  calls with inherited (unset) arg registers + a braf tail with inherited args +
  mova-formed handler pointers. Decodable but higher error risk; deferred to keep
  fidelity high.
- **0x020041d4** (0x2041d4) - MISSEG: fn.units unit holds 3 chained sub-functions
  (each ends in a braf tail to 0x2154fc) plus embedded data around the 0x11220
  pool that m2c mis-disassembles (.byte skipdata). Only the first body is the real
  0x2041d4 entry; not safely separable/verifiable as one function. Skip.
- **0x02207360 / 0x02207542 / 0x02207bcc / 0x0220825c-neighbours** - geometry/
  draw routines with some TABLE/`braf r2` dispatch; partial. Deferred.
- **0x022079d0** - fixed-point beam/line renderer (sin_lookup 0x213190 +
  cos_lookup 0x2131a8, writes vertex buffer *g_06099510); correct but needs
  careful shift/sign verification per vertex; deferred to avoid rushing.
- 0x02206c88, 0x02208a52, 0x0220903e(done), 0x02209100(done) and the remaining
  loader/handler siblings: left for a follow-up pass.

## Shared callees referenced (named consistently with notes/symbols.txt)
sh2_dmac_copy 0x2145d0, clear_vec4_array 0x213290, fill_vec4_array 0x2132ac,
alloc_array_zeroed 0x214bb8, sound_queue_push 0x215f6c, spawn_obj_0x6c 0x20fece,
rng_next 0x210020. New: func_0220ff54 (spawn w/ return), func_021675e,
func_022156c, func_0220ade, func_0220828, func_0221b330, func_0221080a,
func_02213a8 - see symbols_wave1_201400.txt.
