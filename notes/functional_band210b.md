# Functional decomp report - band 0x210000-0x220000 (worker 210b)

Header: `dh_band21.h` (absolute-address accessors MEM8/16/32, PTR). All files in
`project/src_functional/`, each compiles `gcc -O1 -Wall -Wextra` with **zero output**.

## Delivered (10 functions, all HIGH fidelity)

| SH2 addr | file | role |
|----------|------|------|
| 0x0221fd7a | 0221fd7a.c | draw_model_list - walk model-record list, push/pop matrix, submit each |
| 0x0221fed4 | 0221fed4.c | camera/aim update - weighted_accum3 + look-at + mtx_build_fwd_inv |
| 0x0221885c | 0221885c.c | draw_scrolling_prop - 3-state cull+draw vs reference object |
| 0x0221b450 | 0221b450.c | prop_fade_in_sweep - 4-state ramp then draw |
| 0x02215964 | 02215964.c | emit_or_queue_sprite - fast-poke vs heavy FIFO path |
| 0x02215cb4 | 02215cb4.c | build_quad_drawlist - sub-quad visibility list + tail dispatch |
| 0x0221c8f4 | 0221c8f4.c | debris_ballistic_update - gravity/spin arc + land + draw |
| 0x022154fc | 022154fc.c | draw_billboard_quad - matrix setup by mode, project + emit sprite |
| 0x022186ec | 022186ec.c | spinning_pickup_update - 5-phase spin envelope + draw |
| 0x0221a452 | 0221a452.c | hud_timer_display_update - 2-sprite HUD counter + SFX |

Names/globals logged to `notes/symbols_band210b.txt`.

## How selected
`census.py --tsv` filtered to band, triage_cat in (callC,leafC), bytefrac_class==code,
minus the done-set. 37 census rows -> after `find_entry.py` real-entry resolution and
de-dup, **24 genuinely-undone** functions. Picked the densest tractable ones that could
be fully verified (control flow + every call target resolved).

## Findings / corrections
- **Many census entries are pool-contaminated.** `find_entry.py` redirected several:
  210b1a->211108 (already done), 216c36->216c38, 21276e->212b20, 219c3e->219c44,
  21895a->21898c, 21594e->215964, 215b0a->215cb4, 21f080->21f088, 21f22e->21f2bc.
- **0x0221c8f4**: the census/find_entry entry is 0x21c8f4, but the true first instruction
  is `mov #16,r0` at **0x21c8f2** (loads the state-field offset for the entry dispatch;
  0x21c8f0 is an inter-function `nop` pad). Kept the func name `func_0221c8f4` to match
  the project's keying, documented the +2 preamble.
- **m2c fails outright** on 0x21fd7a, 0x21fed4, 0x2154fc, 0x21885c, 0x21b450, 0x2186ec:
  inline PIC literal pools (mova+add) and pool-adjacent branch targets confuse its linear
  sweep ("Cannot find branch target"). All reconstructed from `fn.py` + `braf_resolve.py`.
- **mova+add PIC calls** (jsr @rN after mova anchor + pooled delta) are NOT caught by
  braf_resolve; computed by hand (anchor addr + delta, mod 2^32) and cross-checked against
  known function names - e.g. 0221fd7a's r9/r10/r11 -> 0x214a60/0x214a2e/0x215a2c.
- **Cache-mirror reads**: 0x26098818/0x2609881c/0x260e0dd0 are read while the base
  0x0609.../0x060e... is written. Preserved the exact (mirror,base) pair verbatim rather
  than collapsing them - they may or may not alias and that is a target detail.
- **Non-standard r0/r1 helpers**: sdiv32 (0x24a06c) and ashr_var (0x24a420) are called as
  the existing `func_...(dividend,divisor)` / `func_0224a420(value,count)` externs - the
  helper is called, the r0/r1 convention is NOT inlined (per band rules).

## Deferred (with reasons)
- **0x21dc04** (219): starts with a `braf r0` jump table (`mov.w @(r0,r1),r0`); m2c output
  is badly garbled. Jump-table resolution needed before it can be done correctly.
- **0x21f2bc** (71): game-state/mission transition - 11 computed branches incl. a `braf r2`
  tail jump (0x20fdc8), a VDP write to 0x25F80000, and ~8 bsrf into other bands. High blast
  radius; wants careful cross-band verification.
- **The big ones (0x21e256 704, 0x21a80c 678, 0x216c38 527, 0x212b20 473, 0x21d6b6 469,
  0x21bf72 412, 0x219c44 372, 0x21a1a0 345, 0x219f26 317, 0x21898c 308, 0x21cf56 272,
  0x21c4ac 242, 0x21c690 212, 0x21f088 171)** remain undone. Not blocked - just not yet
  reached under the correctness-first budget. 0x21e256/0x21a80c have indirect fn-pointer
  calls with varying arg counts (register-count shifts) that need extra care.

## Edge in 0x022154fc worth flagging
mode = `arg3 & 3`; modes 0/1/2 set the depth register from the top matrix. Mode 3 leaves
it holding a stale non-coordinate value (the PIC function-pointer constant) that always
fails the `0x10000 <= z < 0x1FBFFF` gate, so mode 3 is a provable no-op - modelled as an
early `return` (reproducing the target address on host would be meaningless).
