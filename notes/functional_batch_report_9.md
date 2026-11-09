# Functional decomp batch 9 (Opus, single worker)

Headline: **14 new verified functional `.c` files** (12 fn.units members + 2 extras), all
compiling clean `gcc -c -fsyntax-only -Wall -Wextra` with **zero warnings**, each hand-verified
against raw SH-2 disassembly. Plus the **shared-header `uintptr_t` fix** (the 10 legacy
`-Wint-to-pointer-cast` warnings are gone and nothing else broke) and **`dmac_copy_preroll`
0x214678 upgraded MED -> HIGH**. Ran as a single serial worker (no forks), per the batch-8 lesson.

## Authoritative deduped counts (`project/progress_functional.py`, of 1272 members)
```
functional  (src_functional):  317    24.92%   (+19 extra)
nonmatching (src_nonmatching):  73     5.74%
byte-exact  (src files)      :   5     0.39%
byte-exact  (oracle EXACT)   :   3     0.24%
ANY C, member-covered (deduped): 326   25.63%
ANY C, out-of-universe extras:   19
ANY C functions total        :  345
```
Baseline at batch-9 start: functional **305 (+17 extra)** / ANY-C member **315** / ANY-C total **332**.
Net this batch: **+12 functional members, +2 extras, +11 ANY-C member-coverage, +13 ANY-C total.**
(The +11 vs +12 is one member that already had a nonmatching draft counted under ANY-C; the dedup
counts each address once, so promotions lift `functional` without moving ANY-C-member for that one.)
Total src_functional/*.c on disk: **337** (323 -> 337), all warning-free.

## 1. Task 1 - shared-header `-Wint-to-pointer-cast` fix (done, verified)
- `decomp.h`: added `#include <stdint.h>`; changed every `MEM*`/`PTR*` macro cast from `(u32)(a)`
  to `(uintptr_t)(a)` (matching the `dh_band21.h` pattern).
- `dh.h`: added `#include <stdint.h>`; hardened the `M*`/`MU*`/`MP` accessor macros to cast the base
  through `(uintptr_t)` as well (defensive - latent same-class bug).
- Fixed the handful of inline `(void*)/(u8*)/(const s8*)(0x... + idx)` int->pointer casts in the 10
  flagged files (`02238176`, `0223ee1a`, `2203de8/e0a/e5e`, `220581a`(none left)/`220903e`(none)
  `220cab0`, `220d98e`(none), `220e000`) by inserting `(uintptr_t)` before the integer expression.
- **Verified:** a full `-Wall -Wextra` pass over all 337 TUs now emits **0 warnings / 0 errors**
  (was 11 `-Wint-to-pointer-cast` warnings across 7 files). This edit touches shared headers, so the
  whole tree was recompiled.

## 2. Task 2 - mac.l matmul cluster finished (0x2135xx-0x213cxx)
Model confirmed from batch 8: `clrmac; mac.l @a+,@b+ ...; sts mach/macl; xtrct` == signed s64
multiply-accumulate with `xtrct` = middle-32 bits = `(s32)(acc >> 16)`. m2c is NOT needed (and its
C-context parser rejects dh.h's `#ifndef`), so these are written by hand against `--asm` and
validated field-by-field. `0x213694` was already done (concurrent agent, `vertex_transform_project`);
the rest of the band is now complete:

- **0x213570 `vertex_project_dualmtx`** - transform+project; selects matrix M0@g_060da040 (w<=0) or
  M1@+0x30 + coord offset @+0x60 (w>0); recip = (0x100<<32)/depth via DIVU 64/32; x doubled for
  aspect; 16-byte stride out -> g_060debc8.
- **0x21377c `vertex_project_dualmtx_m`** - same, matrix base = arg3, out g_060dfbc8, pre-zeros 8
  longs (16-byte stride) at (M+0x40000000).
- **0x2138b8 `vertex_transform_project_m`** - the 0x213694 algorithm with matrix base = arg3, out
  g_060dfbc8 / scratch g_06098b5c, 32/32 DIVU per component, pre-zeros 0x40 words at (M+0x40000000),
  no w-branch.
- **0x2139b8 `project_sprite_quad`** - single-sprite projection of the active matrix translation +
  scaled extents into a packed 3-long descriptor @g_06098b1c (swap.w/neg/shlr8/shll8 bit-packing).
- **0x213a70 `project_quad_corners`** - 4 corners (+/-bx,+/-by) projected to packed screen positions
  g_06098b1c[0..3].
- **0x213ba0 `mtx_rotate_x`** / **0x213bf4 `mtx_rotate_y`** (extra) / **0x213c4c `mtx_rotate_z`**
  (extra) - three independent matrix-rotation entry points merged under one fn.units member; each
  `sincos_lookup(-angle)` (0x213f90) then a 2x2 column rotation over 3 rows of M=*g_06098878; angle 0
  is a no-op (shared `rts` @0x213ca0). Written as three separate functions (y/z fall outside the
  member list -> counted as extras).

Cluster status: **COMPLETE** for the 0x2135xx-0x213cxx band (0x213538/213694/2132c4/213b5c/213ca4/
213ce4 were already done; all remaining members now have C).

## 3. Task 3 - manual reconstruction + merge assessment + DMAC upgrade
- **0x2143bc `lz_stream_step` (reconstructed by hand; m2c decode had failed).** A resumable,
  queue-driven LSB-first LZSS decompressor that advances one token (or one match-copy byte) per call
  via an 8-field state struct @g_06091e14 + an 8-byte-entry job ring at +28. Short and long match
  encodings, stream-end (word 0) -> dequeue next job, and the 3 distinct return codes (0/1/2) are all
  modeled faithfully, including the shared external `rts` @0x2143b8 (= return 0) and the SH-2
  dt/shlr bit-reader idiom (primed with count 9). Compiles clean.
- **0x214678 `dmac_copy_preroll` upgraded MED -> HIGH.** Pinned the fault-thunk call: the site loads
  the fn ptr from pool 0x2147e4 (= 0x060902a0) and **pushes exactly three stack args**
  (0x06053d98, src, dst) then cleans 12 bytes (`add #12,r15`), so the `(s32,s32,s32)` signature is
  unambiguous. The function's control flow and all register/memory effects are verified; the only
  non-static piece is the runtime-installed thunk *body* at `*g_060902A0`, which is outside this TU.
  Comment updated accordingly.
- **0x244988 / 0x244cd2 - assessed and DEFERRED (not misseg, but genuine multi-function merges).**
  - 0x244988 (0x34a bytes) is really 2-3 separate functions sharing a tail `rts`/`bra 0x244c4e`:
    a small struct-init block (244988-2449b0) + a large state/command processor with a cmp/eq
    dispatch (from 2449b2). Needs splitting into distinct entries.
  - 0x244cd2 (0x6e) is an actor-init that `bsrf`s one helper then `braf`-tail-calls another, with a
    *second* function (a cmp/eq checker from 0x244d0a) merged in after the tail `braf`.
  Both require entry-splitting + computed-jump (bsrf/braf) target resolution and carry callee-ABI
  uncertainty; shipping them now would be below this batch's verified-quality bar. Left for a focused
  split pass (recommended tooling note below).

## 4. Task 4 - small-leaf runway (asm-verified)
Built a leaf scanner (`/tmp/leaf2.py` pattern) over undone fn.units members. Findings worth
recording for future batches:
- Many "undone small members" are **not real functions**: the 0x2896xx-0x2897xx block is data
  misdisassembled as coherent-looking code; several 0x4-byte members are bare `rts` stubs / shared
  tails; the whole **0x24axxx region** is the jump-table dispatch family (fragments that pop r2 with
  no matching push - interlinked, do not decompile standalone; consistent with the 0x24a3xx ban).
- The early **0x200xxx** init functions (e.g. 0x2005fa, 0x200c62) reference literal pools at
  2-aligned (not 4-aligned) addresses, so capstone's reported pool value is **word-order-ambiguous**
  vs the CPU's 4-aligned PC-relative read (0xe5700602 <-> 0x0602e570). Needs a PC-align-aware pool
  reader before trusting those; skipped this batch.

Shipped 5 clean leaves (valid pools, understood control flow):
- **0x22ea0e `record_copy_48w`** - copy a 96-byte (48 s16) record idx*108 from g_06096910 to
  g_06096328 (muls.w/exts.w index, 16 triples).
- **0x217838 `dispatch_cmd`** - write ctx to g_060b88a0 (in the bt/s delay slot), 3-way cmp/eq
  dispatch to func_02217888/022179b0/02217ae4 (direct bsr), reset g_060b88a0=-1, return result.
- **0x241da8 `actor_init_random`** - uses rng_next (0x210020, known): actor+0xc8 = rng&15, clears
  +0xe0 bit20, +0x41=1, copies (s16)+0x148 -> +0x5c, +0xd8=0 (in the bsrf delay slot).
- **0x241e3c `actor_enter_statewalk`** - set +0xd4/+0xd8/+0x84, call func_0223d0c6(actor), mask +0xe0.
- **0x2418a0 `actor_deactivate_if_active`** - gate on (+0x8c & 0x3ffff), teardown func_0223a9d0
  (actor,3), clear +0x41, tail-call func_0245c88(actor).

## 5. Resisted / deferred (honest)
- **0x244988, 0x244cd2** - multi-function merges, need entry-splitting + bsrf/braf resolution (see 3).
- **0x21080a** - matrix-stack push / call / `vec3_matmul_out` / pop wrapper; bsrf targets resolve
  (0x213fb4, 0x2149e4, 0x213538=vec3_matmul_out) but the first two callees' signatures are unknown,
  so deferred rather than guessed.
- **0x221354 / 0x221384 / 0x222052 / 0x21ce1c** - actor/object helpers using non-standard call ABIs
  (args in r0/r1/r2) or a `braf` jump table; need callee-ABI pinning first.
- **0x24axxx family, 0x200xxx PC-align-pool block, 0x2896xx data** - not touched (reasons in 4).
- **Untouched per constraints:** 0x24a3xx jump-table cluster, confirmed misseg band, `port/` +
  `tools/saturn-recomp/`, the read-only `m2c_fn`/`fn`/`match`.

## 6. Next-band recommendation
1. **Split pass for the 0x244988 / 0x244cd2 merges** - teach a small helper to emit each real entry
   (prologue-detected) as its own TU and resolve bsrf/braf targets, then decompile bodies faithfully
   with a switch for the dispatch; good value (command/actor processors).
2. **PC-align-aware pool reader** - a tiny fix so the 0x200xxx init cluster (and any 2-aligned-pool
   function) reads the CPU-correct 32-bit literal; unblocks ~a dozen clean init leaves.
3. **Pin the 2-arg actor-helper ABIs** (func_0223d0c6, func_0223a9d0, func_0245c88, func_02217888/
   9b0/ae4, 0x213fb4, 0x2149e4) - once named, 0x21080a and the 0x241xxx/0x221xxx init family flow.
4. **Continue the `g_06098878` matrix-stack family** around 0x21080a / 0x2149e4 (3D transform glue
   now that the whole mac.l cluster is in C).
5. Keep `find_entry.py` before every m2c feed; address m2c with cart offsets; stay a single worker.
