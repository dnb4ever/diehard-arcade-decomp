# Functional decomp batch 10 (Opus, single worker)

Headline: **24 new verified functional `.c` files** (13 fn.units members + 11 extras), every one
compiling clean `gcc -c -fsyntax-only -Wall -Wextra` with **zero warnings** and hand-verified
against raw SH-2 disassembly. Unblocked the whole deferred **matrix-stack / rotate family** and the
**0x241xxx actor-init family** by pinning the callee ABIs that blocked them in batch 9; added a new
**PC-align-aware pool reader** (`tools/decomp/pool_read.py`) that cracks the 0x200xxx boot block's
halfword-swapped literal pools; and **split both multi-function merges** (0x244988, 0x244cd2) into
their real entries. Ran as a single serial worker (no forks), per the standing process rule.

## Authoritative deduped counts (`project/progress_functional.py`, of 1272 members)
```
functional  (src_functional):  329    25.86%   (+30 extra)
nonmatching (src_nonmatching):  73     5.74%
byte-exact  (src files)      :   5     0.39%
byte-exact  (oracle EXACT)   :   3     0.24%
ANY C, member-covered (deduped): 336   26.42%
ANY C, out-of-universe extras:   30
ANY C functions total        :  366
```
Baseline at batch-10 start: functional **317 (+19 extra)** / ANY-C member **326** / ANY-C total **345**.
Net this batch: **+12 functional members, +11 extras, +10 ANY-C member-coverage, +21 ANY-C total.**
(functional +12 vs 13 member-files: 3 of the members - 0x244988 / 0x244cd2 / 0x244d40 - were
promotions from pre-existing `src_nonmatching` drafts, which already counted under ANY-C-member, so
those lift `functional` without lifting ANY-C-member; one member address already carried a functional
file and was re-decompiled. ANY-C-member +10 = 12 new functional members - 2 already-ANY-C members +0.)
Total src_functional/*.c on disk: **360**.

## 1. Task 1 (highest-yield) - callee ABIs pinned -> matrix + actor-init family unblocked

The batch-9 blocker was "2 unknown callee signatures". Both now pinned by inspecting the callee
bodies and all call sites:
- **func_0213fb4 `load_matrix_default(void)`** - takes **no args**; copies the 48-byte (12-long)
  matrix template at g_0604ec64 into `*g_06098878`.
- **func_02149e4 `mtx_rotate_zyx(u16 *angles)`** - one angle-struct pointer (Z@4, Y@2, X@0).

That unblocked **0x21080a** (`mtx_push_rotate_apply`, member) and the entire merged matrix-stack
family under fn-unit member **0x2149a0**:
- 0x2149a0 `mtx_stack_reset` (member), 0x2149aa `mtx_load_abc`, 0x2149b6 `mtx_rotate_zyx_regs`,
  0x2149e4 `mtx_rotate_zyx`, 0x214a06 `mtx_rotate_zyx_at24`, 0x214a2e `mtx_stack_push_dup`,
  0x214a40 `mtx_apply_vec_at8`, 0x214a50 `mtx_apply_vec_at0`, 0x214a60 `mtx_apply_angles_hi`,
  0x214a7a `mtx_build_fwd_inv` (all extras - siblings merged into member 0x2149a0 by shared pool).
  All 24 bsrf/braf resolve to known primitives (load_matrix_default, copy_longs, mtx_rotate_x/y/z,
  func_02213538/ca4).

Also pinned the **2-arg actor-helper ABI** `func_0223a9d0(u8 *actor, s32 mode)` (big 0x8ba teardown;
called with mode=3), and confirmed 1-arg `func_0223d0c6(actor)` and `func_0245c88(actor)`. With those
in hand, decompiled init-family members **0x241dde `actor_init_rng_dir`** and **0x2415ee
`actor_motion_init`** (the speed/heading->motion-vector setup, sibling of actor_motion_resolve
0x22acf8; uses sin_lookup/cos_lookup/fixmul 0x213190/0x2131a8/0x2131c8, all already in C).

## 2. Task 2 - PC-align-aware pool reader (new tool) + 0x200xxx boot leaves

**Finding (bigger than batch 9 assumed):** the 0x200xxx boot/init block stores **every** mov.l pool
longword - address constants *and* bsrf/braf/mova deltas - with its two 16-bit halves swapped
relative to the running value (raw `0xe5700602` == real `0x0602e570`). Validation: only the swapped
value lands in a valid address window, and three of the swapped addresses (0x0602cdb4 / 0x0602cdb8 /
0x0602e570) also appear as clean 4-aligned constants elsewhere in ROM.

**`tools/decomp/pool_read.py`** (read-only, reuses fn.extent):
- default: per mov.l/mov.w load, prints raw vs halfword-swapped value and which is a valid window.
- `--swap-only`: just the recommended (valid) constant per load.
- `--branches`: swap-resolves bsrf/braf targets and flags which land on a real entry (e.g. 0x200668
  -> 0x20eade / 0x20ec1c).

Shipped the two clean, pure-store leaves whose pools are now trustworthy:
- **0x2005fa `scene_flags_reset`** and **0x200c62 `scene_flags_reset_b`** (byte-identical sibling) -
  zero a group of HWRAM render flags and set two to 1.

**Resisted:** the branch-containing 0x200xxx functions (0x200668 etc.) - the swapped bsrf deltas
resolve to real entries, but their callees (0x20eade, 0x20ec1c, ...) and the swapped mova+add
function-pointer deltas still need analysis. Left for a focused 0x200xxx pass (tool is ready).

## 3. Task 3 - the two multi-function merges, split

- **0x244cd2** -> 2 functions: **0x244cd2 `actor_init_grab`** (member; inits grab/throw state, calls
  func_0223a9d0(actor,3), tail-calls func_0245c88) and **0x244d0a `obj_latch_reset`** (extra; a
  gated one-shot latch + field clear). Both shipped.
- **0x244988** -> member + shared helper shipped: **0x244988 `actor_init_from_cfg`** (member; field
  init + tail `bra 0x244c4e`) and **0x244c4e `actor_apply_ctrl_flags`** (extra; the shared bit->flag
  helper, bsr'd from the processor and bra'd into from the init). The unit's three large
  direct-branch state/command processors (**0x2449b2**, **0x244abc** with its **0x244aa8** alt-entry,
  **0x244b1e**) are genuinely separate functions (all entries identified) but are big cmp/eq
  dispatchers; **deferred** rather than shipped below the verified-quality bar. They have no computed
  branches, so they are tractable with `decomp_bounded.py` bounding + direct-call rendering next pass.

## 4. Task 4 - small-leaf runway (asm-verified)

Members decompiled from the undone-leaf scan (all self-contained or known-callee):
- **0x20fd06 `list_node_alloc`** - free-list pop + active-list splice allocator over g_06092238 /
  g_0609223c (no external calls).
- **0x242f60 `mode_resolve_0x242f60`** - cmp/eq #0..#3 switch over g_06098564 (case 3 draws rng).
- **0x244d40 `slot_record_reset`** - reset an 8-byte record in g_06098838[(s8)actor+0x34].
- **0x244df2 `rec_delta_angle_neg`** - delta of a 0x100-stride record in g_06097d58 -> angle helper
  func_022130b4 (undecompiled, called faithfully) -> negated.
- **0x246208 `actor_flag_rng_1in4`** - flag-gated 1-in-4 rng predicate (called as func_0246208).

Noted as **not real functions** during the scan (skipped): 0x23cfdc / 0x24537a (shared epilogue
stubs), 0x213d90 (jump-table dispatch fragment), 0x207332 / 0x213d9e / 0x2006b4 / 0x200d78
(pool-contaminated entries - would need find_entry correction), and the 0x24axxx band (banned).

## 5. Resisted / deferred (honest)
- **0x244988 processors** 0x2449b2 / 0x244abc(+0x244aa8) / 0x244b1e - large direct-branch command/
  state dispatchers; entries identified, bodies deferred (next pass: decomp_bounded bounding).
- **0x200xxx branch functions** (0x200668, 0x200400, 0x200d1c, ...) - pool reader now handles the
  swap; callee analysis pending.
- **func_022130b4** (angle helper in 0x213xxx math band) - referenced by 0x244df2; undecompiled.
- **0x221354 / 0x221384** - VDP1 cmd-table bit set/clear that call into the **0x24a360** band via a
  non-standard r0/r1/r2 ABI; untouched (0x24axxx is banned and 0x24a360's ABI is unresolved).
- **Untouched per constraints:** 0x24axxx dispatch family, 0x2896xx data-as-code, confirmed misseg,
  `port/` + `tools/saturn-recomp/`, read-only `m2c_fn`/`fn`/`match`.

## 6. Next-band recommendation
1. **Finish the 0x244988 split** - the three processors (0x2449b2 / 0x244abc / 0x244b1e) via
   `decomp_bounded.py` with each real entry bounded; dispatch is direct cmp/eq, so m2c should lower
   them once bounded. Good value (command/state machines sharing the 0x244c4e helper now in C).
2. **0x200xxx boot cluster** - with `pool_read.py --branches` the swapped pools are solved; walk the
   branch-containing leaves (0x200668 class), pinning the 0x20exxx callees. ~a dozen init leaves.
3. **Continue the 0x241xxx / 0x221xxx actor-init family** - the 2-arg helper ABIs are pinned; the
   remaining members (0x241a10, 0x241b66, 0x241e82, 0x2418fa) now mostly flow.
4. **Teach ptr_resolve the boot-block halfword swap** for mova+add function-pointer deltas (0x200668
   needs it) - small extension mirroring pool_read's `--branches`.
5. Keep running `find_entry.py` before every m2c feed; address m2c with cart offsets; stay a single
   worker.
