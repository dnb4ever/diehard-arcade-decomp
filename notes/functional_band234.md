# Functional decomp report - band 0x233-0x237 video-bootstrap / debug-editor cluster

Worker: band234. Serial, single worker, NO forks. Did NOT touch git, notes/symbols.txt,
shared headers, tools/, or port/. All files in `project/src_functional/`, named by full
8-hex SH2 address. Band symbols written to `notes/symbols_band234.txt`.

Every file compiles clean with the required gate (ZERO output):
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional`.
Every computed branch confirmed with `braf_resolve.py`; entries validated with
`find_entry.py`; variadic glyph thunk modelled per-call-site from the actual push count.

## Files added (6) - the entire deferred hard cluster

| file | role | notes |
|------|------|-------|
| 02234984.c | vdp2_stage_loader | out-of-cart DMA descriptors; multi-stack-arg calls (6+5 args) |
| 0223358c.c | video_menu_bootstrap_sm | the bsrf-r6-blocked one; 19 branches; 6 glyph (1 arg) |
| 022363e4.c | debug_obj_inspector | 14 branches; 6 glyph (2 args); state-0 tail call |
| 0223673a.c | video_menu_bootstrap_sm2 | 12 glyph (1 arg) + 3 glyph (2 args) |
| 02236af0.c | debug_cursor_editor | out-of-cart tables; switch glyph (1 arg) + 6 (2 args) |
| 02236e66.c | video_menu_bootstrap_sm3 | 9 glyph (1 arg); 4-state + state-2 spawn/poke |

## THE CRUX: "UNRESOLVED bsrf r6 @ 0x233740" is a false positive (resolved)

0x233738-0x2337af is func_0223358c's literal pool. The real code at 0x233730-0x233734
loads `r3=0x60997a4`, `r2=35`, then `bra 0x2337b0; nop`, and execution resumes at the
bra target 0x2337b0 (`mov.l r2,@r3`). Everything between is pool data holding the bsrf
deltas (0xfffdc7aa, 0xfffe2b72, ...) and pointer constants (0x60997a4, 0x608bef0, ...).

The bytes at 0x233740 are 0x0603 0x0700 = the 32-bit constant stored into
VDP2->unkF8 (0x0603) / VDP2->unkFA (0x0700) at code 0x23367c-0x233688 (m2c confirms
`unkF8 = 0x603; unkFA = 0x700`). braf_resolve disassembles the pool byte 0x0603 as
`bsrf r6`, but r6 is never loaded with a PC-relative delta anywhere before 0x233740, so
it CANNOT be a real computed branch. This is exactly the documented caveat
("braf_resolve emits spurious COMPUTED rows for bytes inside data pools"). All 19 genuine
computed branches of func_0223358c ARE resolved; the function ships.

## m2c corrections made

1. **Dropped call return values on bsrf-calls.** m2c systematically treats the r0 left
   over from a pre-call `mova` as the value stored, when the intervening bsrf-call actually
   clobbers r0 with its return. Corrected sites:
   - func_0223358c s0: `entity->unk64 = func_02227bc4(0, &data_2339f0)` (m2c: `&pool_233704`).
   - func_0223358c s1: `t=func_0220fece(...); t->unk8->unk50=-24` (m2c: `.Lpool.unk8->unk50`).
   - func_022363e4 s0: the state-0 `braf` is a TAIL CALL to func_02223ac0(&g_060e1324)
     (m2c mis-linearised the trailing pool into bogus `0xFFFED642->unk10 = ...` stores).
   - func_02236e66 s2: `child=func_0220fece(...); child->unk8->unk50=-0x20` (m2c: `.Lpool`).

2. **Variadic glyph-thunk argument counts.** m2c reported inconsistent arg counts (3/5/1)
   because it counts dead leftover registers (r4/r5/r6/r7 clobbered by preceding
   spawn/draw calls) as thunk args. The thunk *0x060902a0 takes its real arguments on the
   STACK; each site's count is read directly from the push sequence and confirmed by the
   batch `add #N,r15` cleanup:
   - 0223358c: 6 sites x 1 arg (add #24)
   - 022363e4: 6 sites x 2 args (add #48)
   - 0223673a: 12 sites x 1 arg (add #48) [s0] + 3 sites x 2 args (add #24) [s1]
   - 02236af0: switch cases x 1 arg (add #4) + 6 sites x 2 args (add #48)
   - 02236e66: 9 sites x 1 arg (add #36)
   The two latched globals g_060997a4 / g_060997a8 are modelled as explicit stores before
   each call (the thunk reads them as its other two parameters).

## Trap notes (per the brief)

- No ashl_var/ashr_var (func_0224a360/0224a420) or sdiv calls appear in this cluster.
- Mid-function literal pools present and handled as DATA in all six (fn.py flags them as
  `.byte`; the dispatch/unconditional bra skips them).
- Multi-stack-arg calls in func_02234984 (func_0221675e 6 args, func_0222156c 5 args,
  shared frame reclaimed by add #12,r15) verified against the known ABIs in symbols.txt.
- The out-of-cart data (0x22695d6x descriptors, 0x22700000 / 0x22700aa0 editor tables) is
  modelled as absolute volatile reads; stride/field offsets hand-derived.

## Nothing deferred

All six targets in the assigned cluster were decompiled cleanly. The sibling relationship
(0223358c / 0223673a / 02236e66 are three VDP2-bootstrap state machines that spawn the
inspector/editor tasks) cross-checked the shared conventions and increased confidence.
