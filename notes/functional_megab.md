# Functional decomp: mega colour builders B and C (clone-from-A pass)

Scope: builder **B = 0x235198** per the clone recipe in functional_mega.md, plus
the tractable 3rd function in B's compilation unit (which turned out to be a 3rd
builder, **C = 0x23579c**). Template was A (0x234b58, src_functional/02234b58.c).

## Result

| addr | name | file | status |
|------|------|------|--------|
| 0x235198 | vdp_colour_build_b | project/src_functional/02235198.c | DONE, 0 warnings |
| 0x23579c | vdp_colour_build_c | project/src_functional/0223579c.c | DONE, 0 warnings |
| 0x235dc0 | func_235dc0 | - | DEFERRED (extern) |

gcc -c -fsyntax-only -Wall -Wextra on both .c files: zero warnings.

## Unit map (confirmed)

`fn.py --units`: `235198-235f0c = 3 funcs`. Extents (fn.extent):
- 0x235198 builder B, code_end 0x23577c, pool to 0x23579c
- 0x23579c builder C, code_end 0x235d9a, pool to 0x235dc0
- 0x235dc0 func_235dc0, code_end 0x235edc, end 0x235f0c

The mega notes' claim that "0x23577c is not a function / pool garbage" was an
off-by-one-pool read: 0x23577c is builder B's trailing literal pool; the real 3rd
sibling entry is **0x23579c** (find_entry: "already real"). So there are THREE
builders in this family (A 0x234b58, B 0x235198, C 0x23579c), not two.

## What all three share (verified identical pool constants)

Tables: vec3 base 0x0607976C stride 0xB4 [idx MEM16(obj+0x5E)], euler base
0x06096328 stride 108 [idx (s8)obj+0x34], id base 0x06096248 stride 32, part-long
base 0x0609638C, SP write-pointer 0x06098878. Fields obj+0x30 (u16 out slot),
obj+0x34 (s8 part), obj+0x4C (s8 shift), obj+0x5E (s16 mesh). Prologue is the same
8-step sequence (SP+=0x30; load_abc; apply_vec_at8(obj); rotate_zyx_at24(obj);
push; matmul(0,partlong,0); rotate(atab); push). Helper set identical.

## How B and C each differ from A (the parts re-derived, NOT copied)

Derived from braf_resolve + ptr_resolve + an address-annotated capstone disasm,
reduced to an ordered semantic-event trace (id read / atab+=3 / vtab+=3 / 3x ashl
/ matmul / rotate / matrix_copy / hook / SP read-sub-write / push / rts). Each of
the 15 emit blocks was walked individually.

### Register allocation (pure, no behaviour change)
| | id tab | euler | vtab | SP ptr | matmul fn | rotate fn |
|--|--|--|--|--|--|--|
| A | r10 | - | - | pool each use | r8 | r9 |
| B | **r9** | r11 | r14 | **r12** (held) | **r10** | **r8** |
| C | **r10** | r11 | r14 | **r12** (held) | **r8** | **r9** |

### Block structure
- **A**: uniform 15 blocks, every block matmul+rotate+copy+hook+push.
- **B**: 15 vec3 blocks but only **13** matrix_copy_reverse + hook. Blocks **4 and
  5** (0-based 3,4) emit matmul+rotate+push ONLY (no copy, no hook). Modelled with
  an `emit[15]` mask = {1,1,1,0,0,1,1,1,1,1,1,1,1,1,1}.
- **C**: uniform 15 blocks (all copy+hook), like A - just a different rewind table.

### rewind[] (bytes subtracted from SP before each block's push), re-derived from
each block's `mov.l @r12,rX ; N x add #-48 ; mov.l rX,@r12` chain:
- A: {0,0x30,0,0,0x90,0,0,0xC0,0,0,0,0x90,0,0,0x120}
- **B: {0,0x30,0,0,0,0xF0,0,0,0x60,0,0x60,0,0x60,0,0xF0}**  (0xF0 = 5x0x30)
- **C: {0,0,0x60,0,0,0x90,0,0,0,0,0,0x90,0,0,0x1B0}**        (0x1B0 = 9x0x30)

The final block of both B and C rewinds (B 0xF0 / C 0x1B0) and RETURNS without a
push - same shape as A's last block (which rewound 0x120, no push).

### Last-block id-read skip (B and C both)
Unlike A's template (which reads the id every iteration), **B and C block 15 do
NOT read *itab++ and do NOT write obj+0x30** - the matmul/rotate (and, for C, the
copy+hook) in block 15 run with obj+0x30 still holding block 14's id. Modelled as
`if (i != 14) MEMU16(obj+0x30) = *itab++;` and `if (i != 14) { push; vtab += 3; }`.

### Pointer-advance timing (both)
- `atab += 3` at each block start, before that block's rotate (15 increments; the
  prologue rotate used the base). Matches A.
- `vtab += 3` between blocks (14 increments, none after block 1's use and none
  after block 15). In the asm it sits at the next block's start; modelled at the
  end of blocks 0..13 - equivalent, nothing reads vtab in between.
- `func_235dc0` is a near-direct `bsr 0x235dc0` here (A mixed PIC bsrf + bsr).

## Modelling choices / fidelity

- SP is volatile. For the rewind-0 blocks, B/C touch the SP pointer not at all, so
  the rewind is written `if (rewind[i]) MEMU32(SP) -= rewind[i];` - avoids the
  spurious volatile load/store that `-= 0` would emit, matching the asm exactly.
  (A's template always wrote; B/C are guarded because their asm genuinely skips.)
- ashl arg evaluation order in the ROM is z,y,x but the three results land in the
  x,y,z arg registers; `func_02213ca4(ashl(v0),ashl(v1),ashl(v2))` is correct
  since ashl_var is pure and C arg eval order is unspecified. Same as A.
- `sts.l/lds.l macl` save/restore and the 4-byte stack scratch for the shift count
  are calling-convention/temp artifacts with no C-level effect.
- fidelity: HIGH, functional (not byte-exact). Every block hand-traced vs the
  address-annotated disasm; SP write order (rewind in push's delay slot, so it
  precedes the push body) and block-15 no-id/no-push/return preserved.

## Resisted / deferred

- **func_235dc0 (0x235dc0): DEFERRED as extern.** Not trivial: multiple loops, a
  braf tail-call plus ~5 unresolved bsrf, four new tables (0x0609EBD0 id->record
  index map, 0x0609D7D0 stride-0x14 records, 0x060DA0C8 output ring, counter
  0x06098818) and obj fields +0x98/+0x9C/+0x20 that none of the builders touch. It
  is the per-block hook shared by A/B/C, not a blocker for the builders. Needs its
  own trace of those branch targets + table ABIs; out of this pass's scope. No M2C
  stub shipped - deferred with reason per the mission.
- Did not blindly copy A's rewind[]/block count: B is non-uniform (13 copies, 2
  push-only blocks) and C has a different rewind table; both were re-derived from
  their own disasm as the mission's warning demanded.
