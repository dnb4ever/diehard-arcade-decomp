# Functional decomp - shift-helper caller pass (single worker)

Mission: decompile functions unblocked by the SHC variable-shift helpers
`func_0224a360` (ashl_var) and `func_0224a420` (ashr_var), now locked to C with
the non-standard `(value r0, count r1) -> r0` bsrf ABI.

**7 new functional `.c`**, every one compiling clean under
`gcc -c -fsyntax-only -Wall -Wextra` (and even `-Wconversion`) with ZERO warnings,
each hand-traced statement-for-statement against `m2c_fn.py --asm` with every
computed branch resolved (`braf_resolve.py`) and every entry checked
(`find_entry.py`).

## Added (project/src_functional/)
| file | name | shift calls | notes |
|------|------|-------------|-------|
| 02221bd4.c | vdp2_shadow_pack6_arr  | 33 ashr | VDP2 shadow 6-bit packer, array arg |
| 0222184c.c | vdp2_shadow_pack6_args | 33 ashr | same packer, 16 scalar args (mission 0x221842; real entry 0x22184c) |
| 0222156c.c | vdp2_shadow_pack4_sel  | 17 ashr | same family, 4-way bank selector (real entry 0x22156c; 0x22154e was pool-contaminated) |
| 02238e94.c | obj_anim_step      | 1 ashl + 1 ashr | size/shrink FSM, tail-calls func_022386fc |
| 0222ea94.c           | rescale_active_slots | 4 ashl | shift 8 active slots' byte by level delta |
| 0221d176.c | obj_retract_step   | 1 ashl + 4 ashr | 2-child retract FSM (0->1 fallthrough) |
| 0223115c.c | obj_expand_draw4   | 1 ashl + 4 ashr | pop-in FSM + 4-quadrant sprite submit (PIC jsr -> func_022153f6) |

Shift-helper call sites cleared this pass: **33+33+17+2+4+5+5 = 99** bsrf calls
(**92 ashr_var + 7 ashl_var**), all lowered to
`func_0224a360`/`func_0224a420(value,count)`.

### Modelling choices
- Every shift call rendered as a faithful helper call `func_0224a360/func_0224a420`
  (consistent with the precedent set by 02221354.c), NOT inlined as `<<`/`>>`.
  For the three VDP packers the count is provably 11..20 (so the helper edge cases
  never fire and a plain shift would be equivalent) - documented in each header -
  but the helper call is kept for one-to-one faithfulness.
- For the FSM functions (0x2238e94 / 0x222ea94 / 0x221d176 / 0x223115c) the count is
  the runtime global `g_06096C90` (or a derived delta), so the count<0 / count>=32
  edge cases ARE reachable: the helper call is semantically required there.
- The packers' unrolled per-field bodies are expressed as `for` loops where the
  original is a straight unroll with identical values/order (observably identical).
- PIC computed calls resolved and kept: `func_022386fc` (0x2238e94 tail call, void),
  `func_022153f6` (0x223115c draw callee - mova(0x23129c)+0xFFFE415A, confirmed
  3-arg: it never reads r7, so the leftover r7 at the call sites is irrelevant).

### Naming correction (process note)
First draft of 5 files was mis-named with a leading `00` (cart offset padded) instead
of the SH2 address (`02` prefix). Corrected to the repo convention before final scp
(`func_02238e94`, `func_0222ea94`, `func_0221d176`, `func_0222156c`, `func_0223115c`;
externs `func_022386fc`, `func_022153f6`). NB: a pre-existing `func_002386fc` spelling
exists elsewhere in the tree from an earlier pass - the correct form is
`func_022386fc`.

## Resisted / deferred (with reason)
The three mission "priority" VDP sprite/colour builders were NOT shipped this pass:

- **0x02234b58, 0x02235198, 0x0223577c** (the "~45 ashl_var each" builders;
  813 / 796 / 826 asm lines, 84 / 77 / 79 computed branches).
  Reason: these are **only partially unblocked** by the shift helpers. Each is a
  large, *non-uniformly* unrolled sprite/VDP command builder dominated by calls to
  OTHER still-unresolved helpers - `0x214a2e`, `0x213474`, `0x2149aa`, `0x214a40`,
  `0x214a06`, `func_02235dc0` - plus two PIC computed function pointers
  (`jsr @r8` ~= 0x213ca4, `jsr @r9`), and they maintain a VDP command-list write
  pointer at `*0x06098878` that is bumped by +0x30/-0x30 at per-iteration-specific
  points. The ashl_var sites are trivial, but a *faithful, hand-verifiable* lowering
  needs (a) the arg conventions of those ~5 helpers, (b) r8/r9 resolved and typed,
  and (c) each of the ~8-10 distinct unrolled iterations transcribed exactly
  (the counter deltas and the intermittent func_02235dc0 calls are NOT uniform).
  That is a focused per-function pass in its own right; rushing 2400 lines of C
  across the three would risk exactly the kind of silent transcription bug the
  quality bar forbids. **Recommend a dedicated follow-up**: resolve r8/r9 and the
  helper ABIs first (several are likely already in src_functional), then do one
  builder end-to-end as the template for the other two (they are near-identical).

- Already decompiled by prior passes, so skipped: 0x210876, 0x2109d4 (6 ashr each),
  0x2425d0.

## Scan result
Full caller census (all fn.units members, bsrf -> 0x24a360/0x24a420): **39 caller
functions**. Of the "pure" callers (every computed branch IS a shift helper, i.e.
fully unblocked), all tractable ones are now done except none remaining; the
remaining un-done callers are the mixed ones (shift + other unresolved helpers) led
by the three mega-builders above and a handful of mid-size mixed functions
(0x20e6b2, 0x21c8f4, 0x231fd4, 0x231c30, 0x22db44, 0x238f... etc.) that each still
depend on one or more non-shift computed callees.
