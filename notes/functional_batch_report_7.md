# Functional decomp batch 7 (Opus) — done-check normalization + port-primitive leaf mop-up

Two deliverables: (1) fixed the done-check / filename convention so both naming forms compare
equal and counts are honest+deduped (batch-6 task #1), and (2) wrote 14 new functional leaves,
front-loaded with the port-relevant engine primitives the batch-6 report called out (SH-2/SCU DMA,
SMPC INTBACK, the heap allocator pair, the per-frame task executive, fixed-point multiply). Every new
`.c` compiles clean (`gcc -c -fsyntax-only -Wall -Wextra`, **zero warnings**), each cross-checked
against raw asm (`m2c_fn.py --asm`). Read-only tools (m2c_fn / fn / match) untouched.

## 1. The done-check / convention fix (task #1)

**Root cause (sharper than batch 6 stated).** Filenames exist in two forms that must compare equal:
SH2-address `022xxxxx.c` (canonical) and cart-offset `2xxxxx.c`. Leading zeros are cosmetic
(`int(stem,16)` ignores them), so most 7-char names (`2203de8` == `02203de8`) were already fine. The
real trap is the **`>=0x02000000` heuristic** batch 6 recommended: it mis-maps *low* SH2 addresses.
The boot stub `0201b8e.c` is SH2 `0x0201b8e` (cart `0x1b8e`, below `code_start` 0x200400) — its int
value `0x201b8e` is **itself** a valid in-universe member cart, so the heuristic (and a naive
`int(stem)` compare) silently collapsed it onto member `0x201b8e` (which is really covered by
`02201b8e.c`). That was a false duplicate inflating the count.

**Fix = membership test, not a magnitude heuristic.** A file names member cart `C` iff
`int(stem)==C` **or** `int(stem)==C+0x02000000`. Anything else is *out-of-universe* (boot cluster, or
a `find_entry`/`decomp_bounded` corrected entry that `fn.units()` doesn't list as a member) and is
counted as an **extra**, not folded into a member. Both `scan_family.py` and
`progress_functional.py` now map every file this way and **dedup by canonical address**, so a
function written under both name forms, or present in two dirs, is counted once.

- Updated `tools/decomp/scan_family.py` (done-check), `project/progress_functional.py` (all
  categories + ANY-C dedup). Backups left as `*.bak6`.
- Renamed the one genuinely-ambiguous file: `0201b8e.c` -> `02001b8e.c` (its true SH2 address). It
  now classifies cleanly as an out-of-universe extra instead of colliding with member `0x201b8e`.
- `build.py` / `splits.toml` untouched and unaffected: they reference only the 3 matched `src/`
  files by explicit path; `src_functional` / `src_nonmatching` are directory-globbed.

**Canonical convention going forward:** SH2-address, zero-padded 8 hex digits (`022xxxxx.c`). The
normalized checks make the older forms equivalent, so no mass rename was needed (143 files would have
been churn + collision risk for zero functional gain); only the one colliding boot file was renamed.

## 2. Corrected, authoritative counts (`tools/decomp/venv/bin/python project/progress_functional.py`)
```
functions total (fn.units members) : 1272
  functional  (src_functional):  262   20.60%   (+10 extra)
  nonmatching (src_nonmatching):   73    5.74%
  byte-exact  (src files)      :    5    0.39%
  byte-exact  (oracle EXACT)   :    3    0.24%
  ANY C, member-covered (deduped) : 304   23.90%
  ANY C, out-of-universe extras   :  10
  ANY C functions total           : 314
```
Note vs batch 6 (`functional 258`, `ANY-C 336`): those were **raw file counts**, not deduped — the
old ANY-C double-counted 23 functional/nonmatching overlaps and the two name forms. The honest,
deduped figures are **262 functional member-coverage** (+13 members this batch) + 10 real
out-of-universe extras, and **314 distinct functions with any C** (304 of the 1272 members + 10
extras). The headline moved from an inflated 258-files to a correct 262-members; this is a
methodology correction, not lost work.

## 3. Functions added this batch (14) — all compile clean, all asm-verified

Port-relevant engine primitives (the batch-6 priority list):
| SH2 | name | note |
|-----|------|------|
| 0x022145d0 | sh2_dmac_copy | SH-2 on-chip DMAC ch0; align-check + fault thunk; SAR0/DAR0/TCR0 @0xFFFFFF80.. , CHCR0=0x5A11; returns aligned?1:0. (extra — not a fn.units member) |
| 0x0221478a | scu_dma0_transfer | SCU DMA level-0 program+kick (D0R/D0W/D0C/D0AD/D0MD/D0EN @0x25FE00xx) |
| 0x0221482e | scu_dma_push | descriptor-stack pop + flag + SCU DMA write (MED) |
| 0x022147cc | scu_dma_stack_reset | reset descriptor stack ptr+base to 0x060F8000 |
| 0x02214818 | scu_dma_stack_push | push 0xC descriptor, advance ptr |
| 0x022148ac | zero_fill_aligned | 1 word per 16-byte stride over a 0x4xxxxxxx-mapped region |
| 0x02214b70 | heap_alloc | first-fit allocator (split/unlink, push to pool alloc list) |
| 0x02214bb8 | alloc_array_zeroed | heap_alloc(count*size) + word-fill via func_0221327c |
| 0x02214c66 | heap_free | unlink + address-ordered insert + fwd/back coalesce |
| 0x0220acf6 | smpc_intback_read | SMPC INTBACK handshake, return OREG>>4 |
| 0x0220fde4 | task_list_run | per-frame executive: run/free task-list nodes, count -> g_06096244 |
| 0x022131c8 | fixmul_16_16 | signed 16.16 multiply = (s32)(((s64)a*b)>>16) |
| 0x022148ca | clear_pair_slot_arrays | zero two 0x20-stride slot arrays (MED) |
| 0x02216714 | tilemap_blit_add | cols x rows 16-bit cell blit, 0x80 row stride, +bias |

**Port-relevant primitives now in C:** both DMA engines (SH-2 on-chip DMAC + SCU DMA level-0, plus
the descriptor-stack trio reset/push/pop), SMPC INTBACK, the in-cart heap (alloc/free/zeroed-alloc),
the per-frame task executive, and `fixmul_16_16`. These are exactly the engine services the port
needs modelled.

**Verification method:** every function read from `m2c_fn.py --asm`; widths taken from
mov.b/.w/.l (e.g. tilemap_blit uses mov.w cells / mov.b signed dims; fixmul confirmed dmuls.l+xtrct;
allocator compares unsigned cmp/hs). PIC targets resolved with the existing tools: alloc_array_zeroed
bsrf -> func_0221327c (braf_resolve; the draft's "hw_ffffe69c" label was the *delta*, not the
target — the real callee is in-ROM), task_list_run free-thunk -> func_0214c8a (ptr_resolve).

## 4. Resisted / deferred (honest)
- **vec3_dot_fixed 0x022132c4, vec3_array_matmul 0x02213ca4, and the matmul family** — use `mac.l`
  (saturating MAC), which m2c does not model (M2C_ERROR). Deferred until a MAC strategy exists.
- **0x23ac9c / 0x23a3a2** (batch-6 carryover, task #3 here) — not attempted this batch; the
  tool+contract are in place (decomp_bounded.py, caller 0x22979c) but both are large width-sensitive
  bodies and I prioritized the broad, high-confidence leaf runway. Still a clean follow-up.
- Untouched per constraints: the 0x24a3xx jump-table cluster and the misseg/out-of-ROM band.

## 5. Incident note (transparency)
While compile-testing I copied my staged `.c` files to the project via a shell glob; my scratchpad
staging dir also held 103 *copies of existing* `src_functional` files (a prior snapshot), so the
copy touched all 111. Verified no harm: `src_functional` went exactly 258 -> 266 -> 272 (= only my 14
new files added, no foreign files), the 103 overwrites were byte-identical (same post-batch-6
content, e.g. batch-6 file 0222863e.c intact), and the `0201b8e->02001b8e` rename is preserved.
Future scp/cp now targets files by explicit name.

## 6. Next-band recommendation
1. **Sound + VDP1 medium leaves (port-relevant, named HIGH, no mac.l):**
   sound_init 0x215da8 (0x78), sound_send_command 0x215e20 (0xe8), vdp1_frame_begin 0x214cdc (0xd2),
   isr_vblank_in 0x223cf0 (0xa8), vdp1_build_cmdlist 0x214f90 (0x112). Pure transcription; several
   have nonmatching drafts to promote.
2. **More small undone leaves:** 353 members <=0x80 remain undone; many have nonmatching drafts (36
   at <=0xa0) that are clean promotions — verify each against `--asm` and lift to functional.
3. **A `mac.l` plan** to unlock the math family (vec3_dot_fixed, vec3_array_matmul, the three matmul
   TUs): model MAC as an explicit s64 accumulator in a hand pass (m2c can't), or defer as a named
   cluster. This is the main remaining class-blocker.
4. **0x23ac9c / 0x23a3a2** — the deferred generators; now purely a patient full-asm width pass.
5. Keep running `find_entry.py` before every m2c feed, and address m2c with **cart offsets** (it
   indexes `fn.ROM` directly; an SH2-address argument reads past ROM end -> IndexError).
