# Band claims — coordination between orchestrators

Two Claude orchestrators share this tree:
- **Mac Claude** — role: VERIFY + HARD TAIL + backup. Does not spawn bulk
  generation agents anymore; owns the m2c-defeating functions + port-unblock,
  verify-compiles all output, and keeps the GitHub backup current.
- **p620 Claude** — role: BULK generation (incl. a local open-weight model).

Source of truth for "done" = filenames in project/src_functional/. Claim a band
here before working it; never take a band marked done or in-progress.

## Done (committed)
- 0x231xxx ; 0x22d draw units ; combo 0x248bb8/0x2492f8 ; colour-builders
  0x235198 + 0x23579c ; gen7 cluster 0x22d-0x230. Plus the partial coverage in
  0x200-0x20f, 0x210-0x21f, 0x220-0x22c, 0x240-0x248 (check census).

## In progress — Mac agents finishing, do NOT take
- 0x20fxxx
- 0x232000-0x237000
- 0x237000-0x240000

## Mac Claude HARD-TAIL queue (leave to Mac Claude)
- 0x0222e3b0  pose_matrix_blend_decompose (dynamic @-r15 stack frame; needs frame-offset tracking)
- 0x0222ef38  (m2c branch-into-branch failure)
- port round-unblock: condition Z (credit byte==4 + scene-director coordination)

## FREE for p620 bulk model (check src_functional first)
Gaps in 0x200-0x20f, 0x210-0x21f, 0x220-0x22c, 0x240-0x248, and the
0x248-0x24c pocket (but note most of 0x24a000 is jump-table phantoms).

## Rules
- Write per-band symbols to notes/symbols_band<X>.txt. Do NOT edit
  notes/symbols.txt directly — Mac Claude merges per-band files into it.
- fn.py / find_entry.py / braf_resolve.py / m2c_fn.py all take CART offsets
  (cart = sh2 - 0x02000000), NOT raw SH2 addresses.
- KNOWN SYMBOL FIX: func_0224a360 = ashl_var(val r0, count r1);
  func_0224a420 = ashr_var. m2c silently treats both as identity (WRONG) —
  route callers through them.

## Deferred hard cluster in 0x232-0x237 (needs careful handling, NOT naive bulk)
From the 0x232-0x237 pass — route to hard-tail / focused pass, do not let the bulk model guess:
- 0x0223358c  video/menu bootstrap — UNRESOLVED computed branch at 0x233740 (bsrf r6); must resolve before shipping
- 0x02234984  VDP2 stage loader — out-of-cart DMA descriptors, multi-stack-arg calls
- 0x022363e4 / 0x0223673a / 0x02236af0 / 0x02236e66  video-bootstrap / debug-UI family (variadic glyph thunk *0x060902A0, VDP2 shadow blocks)
- 0x02232610 (0x5b4, 36 branches) / 0x02232bc4 (0x55e, 30 branches)  large math dispatchers — dedicated pass

## Still-open genuine functions in 0x237-0x240 (available; some hard)
Doable now: 0x2371c0 (0x668), 0x237828 (0x3a4), 0x237be4 (string fmt 0x22a),
0x23b28a/5d8/9b8/cd4, 0x23bf86 (0x6a8), 0x23c95c (table-copy 0x254), 0x23e690
(0x78a), 0x23f10e (0xaf8), 0x239fc6 + 0x23a3a2 (action apply/cancel, dense bit-switch).
HARD-TAIL (jump-table bsrf m2c cannot lower; manual reconstruction): 0x2399bc,
0x239462, 0x23fcb6.
NOTE: 0x23db2a / 0x23d768 are NOT impossible - they call the r0/r1-ABI helper
family (func_0224a420 ashr_var, 0x24a518, 0x24a7a8, 0x24a830). Express by CALLING
the helper, as the 0x20f / 0x23mid passes did; do not inline the r0/r1 convention.

## Hard-tail DONE
- 0x0222e3b0 pose_matrix_blend_decompose - SHIPPED (was a 4x-unrolled loop, not a sliding frame).

## Hard-tail update
SHIPPED: 0x02239462 (sprite/anim sub-object handler), 0x0223fcb6 (hit/collision
decision handler), 0x0222e3b0, 0x0222ef38.
DEFERRED 0x022399bc (3D skeletal/aim transform): fully decoded EXCEPT its shared
tail does jsr @r9 -> 0x022179e4, which is a MID-FUNCTION alt-entry of
func_022179c0 (real prologue 0x2179c0, falls through to +0x24). That alt-entry
takes inputs in callee-saved r13/r14 and derefs r5 (undefined on this path) =
nonstandard ABI. Shippable only once func_022179c0 alt-entry ABI (esp. what r5
must be) is pinned. Focused single-fn job.
TOOLING CAVEAT: find_entry.py returns "real" for MID-function addresses (it
rejects pool phantoms but does NOT detect interior entries, e.g. 0x2179e4).
Confirm enclosing-function boundaries by disassembling upward.
NOTE: braf_resolve.py emits spurious TABLE/COMPUTED rows for bytes inside data
pools (saw 0x239a1c, 0x23fdfc) - those are data, not branches.
