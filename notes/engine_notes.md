# Die Hard Arcade (ST-V / SH-2 BE) — engine structure map

Target: `cart_fpr.bin` imported at SH2 base `0x02200000`, SuperH:BE:32:SH-2, ~3538 functions.
All addresses below are SH-2 addresses. Cart offset = SH2addr − 0x02000000.

## How this was derived
1. `scripts/CatalogFns.java` — for every function: in-degree (call refs), size, out-degree,
   whether it contains a multiply, and **which hardware region it touches**. Hardware was
   detected by reading the 32-bit values out of each `mov.l @(disp,pc)` constant pool and
   classifying the normalized address (`v & 0x07FFFFFF`) against the Saturn/ST-V memory map.
   Output: `notes_catalog.tsv` (in project root on p620).
2. `scripts/DecompBatch.java` / `DecompBatch2.java` — decompiled the candidate set
   (top in-degree, each hardware region, all multiply leaves, largest functions).
3. Confirmed key globals by reading the constant pools straight out of the ROM image.

Caveat on the HW scan: a handful of pool constants with impossible top bytes
(e.g. 0xd5b0000a, 0xcdb40602, 0xcdc80603) masked into SCSP/VDP1 ranges — those are
float/data constants, **false positives**, and were discarded. All names below rest on the
actual decompilation, not the scan alone.

## Memory map used (normalized, cached 0x0x.. and uncached 0x2x.. both map here)
- `0x00100000` SMPC (regs at odd bytes 0x20100001..0x2010007f)
- `0x06000000` HWRAM (work RAM high; globals cluster 0x060987xx–0x060997xx, 0x060e0xxx)
- `0x25a00000` sound RAM (68k/SCSP program+work) · `0x25b00400` SCSP control regs
- `0x25c00000` VDP1 VRAM · `0x25d00000` VDP1 regs (TVMR/FBCR/PTMR/EWxx/EDSR)
- `0x25e00000` VDP2 VRAM · `0x25f00000` VDP2 CRAM · `0x25f80000` VDP2 regs
- `0x25fe0000` SCU (DMA levels 0/1/2, DSP, timers T0C@0x90, interrupt mask@0xa0)
- SH-2 on-chip regs: DIVU @0xffffff00, DMAC @0xffffff80, (these are the `_DAT_ffffffxx` globals)

## Overall architecture (what the cart image actually contains)
This is a **boot + resident-library + task/actor** design:

- At boot the game copies its hot code to **HWRAM 0x06000000** and runs there. Consequently a
  large number of calls in-cart go through constant-pool pointers into HWRAM (the little
  `(*(code*)(DAT_xxxx + <retaddr>))()` thunks all over the 0x02214axx / 0x0220fexx clusters).
  **The master per-frame executive with the big game-state switch appears to live in that
  HWRAM copy and is not present as a single switch in this image.** Do not expect to find one
  canonical "main switch(game_state)" here — flagged honestly.
- What *is* in-cart and cleanly identifiable: the engine **service layer** (math, RNG, I/O,
  DMA, VDP1/VDP2, sound) and a big population of **task/actor state handlers**.

### Task / actor system
- Actors are driven by a small **task node** (fields seen: +0 active/link, +4 state byte,
  +8/[2] = pointer to the entity data struct). Entity structs carry position (+0x14/18/1c),
  velocity, angles (+0x24..0x2e), a type/state at +0x34, flags at +0x3c, etc.
- `actor_state_dispatch` (0x02244e2e) is the canonical pattern: `handler[state](actor)` via a
  function-pointer table — this is *why* so many large handlers have in-degree 0 (they are
  only ever reached through fptr tables, not static calls).
- `process_spawn_list` (0x0222a5d0, indeg 67) consumes a spawn script (0x18-byte records):
  allocates an entity for each, copies its spawn params, and inits it.
- Example fully-read handler: `actor_update_thrown` (0x02211108) — a state machine
  (init → windup → physics) applying gravity (subtract from vel+0x18), velocity halving on
  bounce, and RNG-driven scatter angles. Classic thrown/ragdoll object.
- `animation_update` (0x0222dc2c, indeg 140) is the big shared per-actor animation/frame
  advancer.

## Subsystem notes

### RNG — `rng_next` @0x02210020 (indeg 133) — HIGH
Reads seed global `*0x0609957c`; if `(seed & 0xffff)==0` it XORs in `0x2a6d365b`
(Galois-style feedback), then applies a constant multiply-mix (the `*0x28 | …` expansion is
the compiler's strength-reduced multiply), folds the two halves, writes the seed back, returns
it. Not a textbook LCG — a custom XOR-feedback+multiply PRNG. Called from everywhere.

### Fixed-point math (16.16)
- `fixmul_16_16` @0x022131c8 (indeg 80) — `(a*b)>>16` via 64-bit product. The workhorse.
- `sh2_hw_divide` @0x02213234 (indeg 29) — hands a 64-bit dividend + 32-bit divisor to the
  SH-2 **DIVU** and returns the quotient. The engine's "divide".
- `vec3_dot_fixed` @0x022132c4, `vec3_array_matmul` @0x02213ca4 — 3D geometry; the matmul
  loops over a vertex array, saturating via the MAC S-bit.
- `sin_lookup`/`cos_lookup` @0x02213190 / 0x022131a8 — one trig table at **0x06075718**;
  cos uses the +0x800 half. Angle is 12-bit-ish: `(ang+8)>>4` indexes shorts, result <<1.
- `dist2_xz_fixed` @0x022131d4 — squared planar distance, used for range checks.

### Memory / DMA
- **No generic byte memcpy/memset exists in-cart.** Fills are typed:
  `clear_vec4_array` (0x02213290, memset-0 over 16-byte elems) and `fill_vec4_array`
  (0x022132ac, memset-value). Bulk block moves go through DMA.
- `sh2_dmac_copy` @0x022145d0 (indeg 66) — programs the SH-2 **on-chip DMAC ch0**
  (SAR/DAR/TCR/CHCR at 0xffffff80, kick via DMAOR 0xffffffb0). The primary RAM→RAM mover.
- `scu_dma0_transfer` @0x0221478a — **SCU** DMA level 0 (registers at 0x25fe0000). Used to
  push data to VRAM / across buses. `scu_dma_push` (0x0221482e) is a descriptor-fed variant.

### Input — SMPC
- `smpc_intback_read` @0x0220acf6 — issues **INTBACK** (writes IREG0, COMREG@0x2010001f=0x10,
  waits on SF@0x20100063, returns OREG3@0x20100027 >>4). This is the peripheral/status read
  used for controller input. (Command byte 0x10 = INTBACK per the standard SMPC command set;
  flagged because a minority of docs number it differently.)
- `smpc_cmd_sshon`/`smpc_cmd_sshoff` @0x02214944 / 0x02214918 — COMREG = 0x02 / 0x03, bring
  the **slave SH-2** up/down. Universal command numbers, HIGH confidence.
- `smpc_pdr2_set`/`clr` @0x02214970 / 0x02214980 — twiddle PDR2 (0x20100077) port bits.

### Sound — SCSP
- `sound_init` @0x02215da8 — copies the sound driver/tables into **sound RAM (0x25a00000)**
  and pokes **SCSP control (0x25b00400)**.
- Request path: `sound_queue_push` (0x02215f6c, indeg 162) writes a request into a 16-entry
  ring (write idx @0x060e0dec, base @0x060e0df0); `sound_queue_service` (0x02215f36) pops it
  (read idx @0x060e0de8) when the driver is idle and hands it to `sound_send_command`
  (0x02215e20), which copies the command block into the sound-RAM command area and triggers
  the 68k driver.

### VDP1 (sprite/polygon drawer)
- NOTE: 0x02223cf0 / 0x02223d98 were renamed `isr_vblank_in` / `isr_vblank_out` (see Interrupt
  section) — they are the VBlank ISRs, and the TVMR/FBCR framebuffer/mode work is what they do per
  interrupt; 0x02223dca was renamed `isr_timer0`.
- Per-frame draw: `vdp1_frame_begin` (0x02214cdc) sets erase area + plot-trigger (PTMR),
  `vdp1_build_cmdlist` (0x02214f90) walks 0x40 linked lists and DMAs 0x20-byte command-table
  entries into VDP1 VRAM, and `vdp1_draw_kick` (0x02214d26, indeg 13) fires PTMR and waits on
  the draw-end status. `vdp1_add_sprite_cmd` (0x022150d4) builds individual command entries.

### VDP2 (scroll backgrounds)
- `vdp2_scene_setup` @0x02202138 — the big one: programs TVMD, scroll/BG-mode and plane
  registers (0x25f80000) and CRAM; state-driven (acts like a scene/title init handler).
- `vdp2_update_a/b` @0x02201c9c / 0x02201f20 — smaller VDP2 register updates.

### SCU / executive
- `frame_sync_loop` @0x022013c4 — an **infinite loop** that pushes display commands (5, 7) to
  a command list and spins on a status byte, using SCU Timer0 (T0C @0x25fe0090). This is the
  closest thing in-cart to a per-frame executive / vsync loop; marked MED because the true
  top-level loop may be the HWRAM copy.

### High in-degree dispatchers — now characterized (MED)
- `dispatch_table_a` @0x0224a360 / `dispatch_table_b` @0x0224a420 are the two highest in-degree
  functions (180 / 146). Decompiled shape: both take `r1` = index and `r0` = operand, bounds-check
  `0 <= r1 < 0x20`, then do a `braf`-based computed goto through a **byte displacement table**
  (`DAT_0224a380` for `_a`, `DAT_0224a450` for `_b`) added to a code base, and **return a value**.
  So they are 32-way value/operand **decoders/generators**, not a game interpreter loop.
  - `_a` evidence: in actor-update `FUN_0221279c` it is invoked 9× in a row, each result added to a
    different entity field — position (+8/+0xc/+0x10), velocity (+0x14/+0x18/+0x1c), angle
    (+0x24/+0x26/+0x28) and masked-random angular velocity (+0x2a/+0x2c/+0x2e). It generates a
    successive stream of typed values selected by the index.
  - `_b` is the **signed** variant: it special-cases the `r0` sign bit, rebases the index by `-0x18`
    for the upper range, uses a single handler (`*DAT_0224a458`) for index>8 when `r0>=0`, and returns
    `-1/0` when the index is out of range. In `FUN_0222156c` its results populate sprite-attribute
    fields (palette/priority bits, entity +0x3c..+0x4e masked with 0x3f). 
  - The exact per-index opcode semantics (what each of the 32 handlers computes) were not fully
    disassembled — the jump tables' `braf` targets are undecoded — so MED, not HIGH.

## Interrupt / VBlank handler — RESOLVED (HIGH)
Found the interrupt setup and the handlers. The reset-init/main function **`main_init_entry`
@0x02201020** (not previously a defined function — it falls through from the PIC boot stub, which is
why earlier function-scoped scans missed it) does, in order:
  1. `FUN_02201958()` — clear engine globals.
  2. `stc sr / and 0xff0f / or 0xf0 / ldc sr` — raise SR interrupt mask to level 15 (block IRQs
     during setup).
  3. Calls a BIOS **"set user interrupt function"** routine, whose pointer is read from the Saturn
     BIOS work area at **`*0x06000300`**, three times with `(r4 = SCU vector number, r5 = handler)`:
       - vector **0x40 = VBlank-IN**  → **0x02223cf0** (`isr_vblank_in`)
       - vector **0x41 = VBlank-OUT** → **0x02223d98** (`isr_vblank_out`)
       - vector **0x43 = Timer-0**    → **0x02223dca** (`isr_timer0`)
     Handler addresses are PIC: `pool_disp + mova_anchor` (0x22ca0+0x2201050, 0x22d3c+0x220105c,
     0x22d62+0x2201068). These three functions were previously mislabeled vdp1_set_mode /
     vdp1_fb_flip / display_page_cycle — the VDP work they do *is* the per-interrupt display work.
  4. Programs **SCU Timer-0**: `T1MD @0x25fe0098 = 1` (enable) and `T0C @0x25fe0090 = 0x3ff`.
  5. More init, then `stc sr / and 0xff0f / ldc sr` → **unmask IRQs (I=0)**, then tail-`bra
     frame_sync_loop`.

Why no literal `0x25fe00a0` (SCU interrupt mask) anywhere in the cart: the mask is enabled by the
BIOS set-func routine when each handler is registered — the game never pokes it directly. Likewise
there is no `ldc …,VBR` in-cart: the master-SH2 VBR / vector table is owned by the BIOS, and user
handlers are plain C functions called by the BIOS ISR trampoline (which is why none of them end in
`rte`). (The 28-byte "rte-terminated" blocks at 0x02284900+ are a FALSE POSITIVE: they are u16 data
tables whose trailing word 0x002b disassembles as the `rte` opcode.)

Handler bodies (decompiled):
- `isr_vblank_in` @0x02223cf0 — re-entrancy guard, sets display flags, conditionally writes VDP
  registers (framebuffer/mode swap), frame-pacing compares, calls VDP sub-routines. The per-VBlank
  display commit.
- `isr_vblank_out` @0x02223d98 — increments the **frame counter** (`*DAT_02223e1c += 1`), sets a
  flag, conditionally commits a VDP register. The vsync frame-tick.
- `isr_timer0` @0x02223dca — advances a 4-entry display-page index (`idx = (idx+1) & 3`), writes the
  selected VDP2 value from a 4-entry table, toggles a VDP2 register bit. Raster/page cycling.

## Master per-frame game-state machine — mechanism identified; no single in-cart `switch(game_mode)`
`main_init_entry` → `frame_sync_loop` @0x022013c4 is the in-cart per-frame executive. Each frame it
pushes display commands (5 then 7), waits on the SCU Timer-0 status byte, and runs a pipeline of
service routines (resolved via PIC thunks). The identifiable per-frame stages in-cart are:
- **`task_list_run` @0x0220fde4** — the task/actor scheduler: walks the active-task linked list and
  calls each node's handler function pointer (`(*node)(node)`); nodes whose handler is NULL are
  unlinked to a freelist; the active count is stored at `DAT_0220feac`. **This is the per-frame
  actor dispatch** — the game-mode logic runs as one of these task handlers.
- **`collision_grid_pass` @0x0222ece4** — bins entities into a 0x20×0x10 spatial grid and runs
  pairwise collision (`FUN_0222ef38` / `FUN_0222f1d0`).
- **`sprite_transform_setup` @0x022225f6** — computes per-sprite affine (sin/cos of entity angles →
  2×2 matrix fields).
- **`displist_priority_fill` @0x022227c4** — fills the display list's priority/depth bytes.

A single canonical `switch(global_game_state)` (title/attract/play/…) is **not present as one
in-cart function**. The top-level mode handler is dispatched through task-node function pointers
(see `task_list_run`) and through `frame_sync_loop`'s inner PIC thunk into the HWRAM-resident copy,
consistent with the boot_mapping finding that hot code runs from HWRAM 0x06xxxxxx. The largest
in-cart `braf` state machine, **`actor_behavior_fsm` @0x02223ee0** (7610 bytes), switches on a
*per-actor* state field (entity+0x44/local), not a global mode — it is enemy/character AI, not the
master game loop. Candidate global mode variables observed across many `braf` handlers: `0x06098f08`
(written by main_init_entry and branched on by a cluster of 0x0222xxxx handlers) and the
`0x060997xx` block.

## Files
- `notes/symbols.txt` — `0xADDR name # confidence` lines (strip `#...` if your loader is strict).
- `notes/engine_notes.md` — this file.
- Supporting (project root on p620): `notes_catalog.tsv`, `decomp_batch.c`, `decomp_batch2.c`.
- Scripts added (new only): `scripts/CatalogFns.java`, `DecompBatch.java`, `DecompBatch2.java`,
  `ApplyNames.java`, and for this pass: `InvestigateEngine.java`, `ScanAndDecomp.java`,
  `FinalProbe.java`, `FindInit.java`, `ResolveHandlers.java`.
- This pass' decomp dumps (project `work/`): `decomp_engine.c`, `decomp_probe.c`,
  `decomp_handlers.c`.
