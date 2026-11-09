# Functional pass: band-residue scattered entries

Single serial worker, no forks. Each TU compiles with
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` (zero output).
Header chosen by band: `dh_band21.h` for 0x02210000-0x0221ffff, else `dh.h`.

## Done (4 new .c, all HIGH fidelity, compile clean)

| cart   | file                         | header        | name                          |
|--------|------------------------------|---------------|-------------------------------|
| 20c3e4 | project/src_functional/0220c3e4.c | dh.h        | func_0220c3e4 camera_track_pair |
| 208c44 | project/src_functional/02208c44.c | dh.h        | func_02208c44 perspective_strip_builder |
| 216c88 | project/src_functional/02216c88.c | dh_band21.h | func_0216c88 stage_build_object_grid |
| 216f7a | project/src_functional/02216f7a.c | dh_band21.h | func_0216f7a stage_build_actor_grid |

### 0x0220c3e4 camera_track_pair (twin of 0x20c17c)
Dual-focus camera: frames two slots (unkAE + unkB0), eye at their midpoint, pulls
back by nearby active actors. Same 4 calls as the single-target twin
(lookat_angles_xz, camera_smooth_lerp, camera_shake_decay, mtx_build_fwd_inv),
all resolved via braf_resolve. Shares the c17c literal pool.

Findings (all faithfully replicated):
- The first slot mask `cam->unkAE & 7` reads r3 **unset** in the isolated disasm -
  this entry's prologue has no `mov #7,r3` (c17c's does; the 2nd mask here reloads
  #7). Value 7 is certain (parallel unkB0 mask + the twin + 8-slot table); the SHC
  build left r3=7 live across the shared code region.
- The focus1 Y-bias indexes `g_06096C58[unkAE]` (the focus0 slot), **not** [unkB0].
  Confirmed in both asm and m2c - an original off-by-slot quirk, kept as-is.
- Empty-slot branch does a dead read of cam->unk88 into the focus Y then overwrites
  with 0x10000; dropped (as in c17c).
- Differs from single: lower Y clamp 0xC000 (not 0x8000); zoom cap 0x20000; unk9C =
  3*(zoom>>2); unkA0 = 0x8000 const; unkA4 = -(zoom*2)-(zoom>>2); tilt unkC4 sign
  from (unkAE+unkB0)&1, halved if zoom>0x18000.

### 0x02208c44 func_02208c44 perspective_strip_builder (sibling of 0x207bcc)
3-state VDP1 display-list builder. **Named func_02208c44** (not a descriptive name)
because 02208b98.c externs it by that name as the handler spawned via func_0220ff54.
m2c output for this TU is heavily corrupted (stack slots/scalars mistyped as
pointers, dropped calls), so the whole thing was hand-traced from fn.py disasm.

PIC calls resolved by hand (mova + add; none caught by braf_resolve except the bsrf):
- func_0221632e  = mova 0x208c60 + 0xD6CE  -> 0x21632e (FIXED 4-arg mode,cur,dst,count)
- fixmul_16_16   = mova 0x208ca0 + 0xA528  -> 0x2131c8
- sh2_fxdiv_16_16= mova 0x208ca8 + 0xA58C  -> 0x213234  (see correction below)
- func_022213a8  = bsrf 0x208ecc + 0x184D8 -> 0x2213a8

Correction: 0x213234 (notes/symbols.txt "sh2_hw_divide num/den") is actually a 16.16
fixed divide `((s64)num<<16)/den` via the on-chip DIVU - recorded in
symbols_bandresidue.txt, symbols.txt left untouched per task.

Note: state 2 plots via func_022213a8(0, unkC) **directly** - no negate/<<16 (unlike
the 0x207bcc sibling which passes (-unkC)<<16). Confirmed in the bsrf delay slot.
Compiler unrolled the state-0 build bands and state-2 row loops by 2; re-rolled.

### 0x0216c88 / 0x0216f7a (band21 workers of dispatcher 0x216c38)
These are the two large workers the census merged into the small dispatcher 0x216c38
(which already externs them). Both stage-init grid builders:
- 0216c88: clears the 32x32 static-geometry bucket grid (0x060B3D5C) and spatial-hashes
  the stage object list into it. Calls clear_vec4_array (0x213290).
- 0216f7a: clears the actor grid (0x060B659C) + box table (0x060B6D9C), builds up to 64
  slot records (0x060B769C) biased by the stage base, grid_bucket_insert_cached (0x217ca2)
  each, and installs command handler func_02217888 at 0x060B889C. Both per-loop
  "index*stride" address exprs the compiler emitted collapse to the base (index 0 at
  entry) with a +0x48 post-increment; re-rolled as base + k*0x48.

## Deferred

### 0x02225c2c - NOT a standalone function (honest deferral)
The task described it as "the braf dispatcher that owns the 0x225ee0 arm". It is not a
real entry: it is an **interior block / jump-table region of a large (~4KB) player
input/combat state-machine**. Evidence:
- No prologue; the very first instruction derefs r2, and r0/r1/r2/r3 are live on entry
  (actually set at 0x225c20-0x225c2a, reached by intra-function `bra` off the spilled
  entity pointer `@(52,r15)`).
- It reads 6+ caller stack slots (@(8/20/28/32/48/52,r15)) that only the enclosing
  function sets up.
- fn.py associates every address in 0x225000..0x225fe8 with one function (entry
  <= 0x02225000, code ~0xfac, extent reaching ~0x02225fe8); the braf table at 0x225d9c
  and the 0x225ee0 arm are interior to it too.
- m2c confirms the breakage: unset r0/r1/r2/r3 and fabricated register args.

Decompiling 0x225c2c in isolation would mean inventing its calling convention and the
meaning of those live registers/stack slots = exactly the silent-wrong outcome the
correctness mandate forbids. The correct unit of work is the whole ~4KB state-machine
function at <= 0x02225000, which is a dedicated task, not a "scattered single entry".

## Already done before this pass (confirmed, skipped)
- 0x02206d8c (02206d8c.c, scene81_cam_update) - complete, HIGH.
- 0x0220bdb4 (0220bdb4.c, entity-handler trampoline) - complete, HIGH.
