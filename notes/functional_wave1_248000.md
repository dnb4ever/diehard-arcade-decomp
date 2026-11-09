# Functional decomp - wave1, band [0x248000, 0x28de20)

Single serial worker (no forks). Every `.c` below compiles clean with
`gcc -c -fsyntax-only -Wall -Wextra` (**zero warnings**) and was hand-traced
against bounded SH-2 disassembly (`m2c_fn.py --asm`); every computed branch /
PIC pointer resolved with `braf_resolve` / `braf_rewrite` / `ptr_resolve`.

## Band reality check (important for the next wave)
This band is overwhelmingly **data-as-code**. Of 430 fn.units members in range,
only ~5 were already done and the GENUINE-code pocket is confined to
**0x248000-0x24b000**. I screened all 285 "real-entry" candidates by full-body
`.byte`/invalid-mnemonic fraction:
- 0x248000-0x24b000: byte-fraction ~0.00-0.10  -> genuine SHC code.
- 0x24c000-0x2896xx: byte-fraction ~0.35-0.82  -> data (confirmed by spot-reads:
  e.g. the whole 0x267e92-0x26dcbe "family" of ~0x120-0x1d0 blocks disassembles as
  a sequential-value table incl. `stc tbr` / `nott` / incrementing `.byte` runs;
  0x24c81a-0x24cc9a likewise ~0.8 bytes). `is_real_entry` only checks the first
  word, so it badly over-reports "real" here - do NOT trust it alone in this band.
- 0x2896xx+ and 0x24a300-0x24a520 are the banned blocks (left untouched).

## Functions added (16) - SH2 addr -> name
| SH2 addr | name | role |
|---|---|---|
| 0x0224a06c | `sdiv32` | runtime signed 32-bit divide (div0s+32xdiv1), r1/r0->r0 |
| 0x0224a120 | `udiv32` | runtime unsigned 32-bit divide, quotient |
| 0x0224a1c8 | `umod32` | runtime unsigned 32-bit modulo, remainder |
| 0x0224ab5c | `shr15` | (u32)x >> 15 |
| 0x0224ab70 | `shr23` | (u32)x >> 23 |
| 0x0224ab84 | `shr27` | (u32)x >> 27 |
| 0x0224ab92 | `shr28` | (u32)x >> 28 |
| 0x0224aba0 | `shr29` | (u32)x >> 29 |
| 0x0224abac | `shr30` | (u32)x >> 30 |
| 0x0224abb6 | `shr31` | (u32)x >> 31 |
| 0x0224a7a8 | `dtoi` | soft-float double -> signed int (truncating) |
| 0x0224a830 | `itod` | soft-float signed int -> double |
| 0x0224806a | `actor_proximity_dispatch` | distance-band 2-D table -> response/fp dispatch |
| 0x0224a000 | `actor_pre_anim_service` | hit/ready service, tail animation_update |
| 0x022482dc | `actor_state_init_6` | post-spawn field init, tail func_02244988 |
| 0x02248436 | `actor_turn_flip_fsm` | 3-state flip FSM, tail animation_update |

### Notes on the groups
- **Divide library (sdiv32/udiv32/umod32):** the classic Hitachi div0s/div0u +
  32x(rotcl/div1) sequences. Register ABI (dividend r1, divisor r0, result r0) is
  the SHC compiler-internal convention; divide-by-zero writes error code 0x44e to
  g_060e132c and returns 0. Modelled as native `/` and `%`, which is bit-equivalent.
- **Shift-right helpers (shr15..shr31):** each is a fixed sequence of
  `shlr/shlr8/shlr16` or `rotl + and #mask` = `(u32)x >> N`. They are entered via a
  shared dispatcher prologue (the trailing `mov.l @r15+,r2` restores a reg the
  dispatcher spilled; it has no effect on the result), so the standalone C model is
  faithful. The irregular set of N (15,23,27,28,29,30,31) is because the intervening
  members are data, not code.
- **Soft-float dtoi/itod (MED):** genuine IEEE-754 double<->int conversions done in
  integer ops (exponent bias 0x3ff / base 0x41f, mantissa reassembly, sign fix). The
  double is passed/returned via the stack (SHC double ABI). Modelled as host
  `(s32)(f64)` / `(f64)(s32)`, which is bit-equivalent over the materialised range;
  flagged MED only because the exact stack-slot ABI is inferred.
- **Actor handlers:** standard (task, entity) task-handler ABI, matching the done
  `actor_set_state_7` (0x0224840a). All tail-call `animation_update` (0x0222dc2c) or
  `func_02244988`; those braf tails + all bsrf calls resolved to fixed targets.

## Resisted / deferred (honest)
- **0x02248104** (actor case-FSM): 10 of 11 computed branches resolve, but the call
  at 0x248258 is a genuine **table-dispatched** `bsrf` (index via `tst #52,r0`). Per
  the "table dispatch -> never faked" rule I did not invent a target; deferred rather
  than ship a function with an unmodelled dispatch in the middle.
- **0x0224a280** (fast block-copy / memcpy helper): dispatches through a 17-entry
  jump table with a computed `jsr @r3` (same table-dispatch class). Nonstandard ABI
  (r0=len, r1=dst, r2=src). Deferred.
- **0x0224a884** (soft-float double binary op, r4:r5 x r6:r7): genuine soft-float
  (add/sub/compare) but multi-register double ABI + 0x266 of exponent-align / mantissa
  logic; could not pin the exact op with confidence. Deferred, not guessed.
- **0x0224ab00** (0x09 byte-frac): shared-frame continuation - reads r0/r1/r2/r3 and
  `saved_reg_r8/r12` it never pushed (inherited frame, same class as batch-12's
  deferred 0x201d2c/0x204df4). Deferred until the parent frame is mapped.
- **0x0224852c** (code 0x193c) and **0x0224ccfe** (0x1726): very large bodies at
  ~0.09 / 0.35 byte-fraction - mixed code+embedded tables; too big to verify fully in
  this pass. Left for a focused dig.
- **0x24a300-0x24a520** dispatch cluster and **0x2896xx+** data-as-code: untouched
  per constraints. `src_functional`/`notes/symbols.txt`/shared headers/`tools/decomp`/
  `port`/`tools/saturn-recomp` untouched per constraints.

## Band remaining
- Genuine-code pocket 0x248000-0x24b000 still has: 0x248104, 0x24a280, 0x24a884,
  0x24ab00, 0x24852c (see deferred above), plus a few tiny fragments.
- Everything 0x24c000-0x2896xx screened as data; the next wave should NOT spend time
  there without re-segmentation evidence. If misseg is fixed upstream, re-run the
  byte-fraction screen before feeding m2c.
