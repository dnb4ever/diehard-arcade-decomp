# Functional decomp report - combatSM worker

Target: the player-input / combat state machine at SH2 **0x02223ee0** (cart 0x223ee0).
Extent **[0x02223ee0, 0x02225fac)** code + trailing pool to 0x225fe8. One function,
one epilogue @0x225fa2, 0x20cc code bytes (~4150 insn). symbols.txt already lists this
address as `actor_behavior_fsm`; shipped here as `func_02223ee0` per task naming.

## RESULT: SHIPPED WHOLE (HIGH fidelity)

`project/src_functional/02223ee0.c` - the complete function. Compiles with
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` **ZERO output**.

Method: fn.py (patched limit 0x2200 - the default 0x2000 truncates the function at
0x225ee0 and makes m2c fail with "Cannot find branch target func_225f48") gave the full
disasm; m2c reconstructed ~95% of the cascade cleanly; I hand-decoded the one real jump
table, resolved all 5 computed transfers, fixed the m2c mask bug (below), and converted
to dh.h M-macro style. Preamble, jump table, all computed transfers, the combo-replay
tail and the epilogue were hand-traced against the disassembly.

## FUNCTION SHAPE

`void func_02223ee0(void *entity)`. Resolves slot = M8(entity,0x34); two input records
rec = g_06098734[slot-1] (stride 0x18), rec2 = g_060987AC[slot-1] (stride 6). Decodes
three button words (b32=cur @rec+0, b30=@rec+4, b2e=@rec+6). Working copies
cat = M32(e,0x84) (action-category bitfield) and st = M32(e,0x88) (state) drive a
pipeline of category dispatchers; both are committed back at the epilogue.

Pipeline (each stage re-reads the possibly-updated cat/st):
1. button remap if facing flipped (M8(e,0x44))
2. **early combo-replay goto**: `if (MU16(g_06096E90,slot*2)) goto L_combo` (braf r1)
3. `func_20fb98(rec, rec2)`  (bsrf@0x223fc2)
4. dispatch-1: cat&0x200 / !(cat&0x180)->{cat&0x40 switch | cat&0xC block} / else->JUMP TABLE
5. dispatch-2: !(cat&0xD)->{cat&0x10 switch | 0x20 | 0x100000 | 0x200000} / 0x410 edges / default
6. dispatch-3: !(cat&0x10000)->{cat&0x8000 switch 7} / else->{0x16900 drift physics + switch 6}
7. special-move init (switch 8), gated `!(cat & 0x037FBFF0)`
8. jump-start init, gated `!(cat & 0x037FBF70)`
9. button 0x100/0x80 edge detect (sets cat bit 2, M8(e,0x46/0x47))
10. **L_combo (0x225c12)**: if !M16(e,0x52) emit buffered records via func_225fac(e,src,base,b30)
11. replay bookkeeping (0x225f48): grow/flush g_06096CB0 ring, 4th func_23c7f6
12. epilogue: commit cat->+0x84, st->+0x88, clear +0x54, tick stale counter g_06096EB0 (clamp 0x1F)

## DECODED MAIN JUMP TABLE  @0x22403e  (the ONLY real table in the function)

Dispatch `switch (st)` for states 14..23, reached when `(cat & 0x180)`.
Guard @0x224028: `r0=st; r0-=14; if ((u32)r0 >= 10) -> default 0x2243ca`.
braf @0x22403e, r0 = (s16) table[(st-14)*2], target = **0x224042 + entry**.
Table words @0x224044 (big-endian s16) and resolved targets:

| idx | state | table word | target    | behaviour (summary) |
|-----|-------|------------|-----------|---------------------|
| 0   | 14    | 0x0016     | 0x224058  | if unk54&1 pick pose[d48]->+2, st=15; then dir -> unkC8 from da8/d90, cat=0x100, st=22/18 |
| 1   | 15    | 0x008C     | 0x2240ce  | dir(b30 2/1) -> unkC8 from dc0/da8 or d90/d78 by (unk88>13); else b32&0x100 pose[d48]->+4 st17; else unk54&1 st16 |
| 2   | 16    | 0x0180     | 0x2241c2  | dir(b30 1/2) -> unkC8 from d90/d78 or dc0/da8 by (unk88>14||<5)/(>16||<6); else !(b32&0x100) pose[d48]->+4 st17 |
| 3   | 17    | 0x0280     | 0x2242c2  | if unk54&2 -> idle (unk5C=g_06096C74[slot], cat=1, st=0, unk52=0) |
| 4   | 18    | 0x02FE     | 0x224340  | unk41=1; unk5C=unkCA; st = (unk45==0)?19:21 |
| 5   | 19    | 0x032A     | 0x22436c  | if unk88==9 st=20   (dead for st=19; faithful) |
| 6   | 20    | 0x0342     | 0x224384  | if unk88==16 st=21  (dead for st=20; faithful) |
| 7   | 21    | 0x035A     | 0x22439c  | if unk54&2 -> idle (as st17) |
| 8   | 22    | 0x02B2     | 0x2242f4  | unk41=1; unk5C=unkCA; st=23  (shares store @0x22440c) |
| 9   | 23    | 0x02CC     | 0x22430e  | if unk54&2 -> idle (as st17) |
|  -  | def   |    -       | 0x2243ca  | no-op (bra 0x224650) |

All 10 case bodies converge at 0x224650 (= start of dispatch-2). States 19/20 test
entity->unk88 which equals the case value, so those arms are structurally dead but are
reproduced verbatim (shared/templated pose-selector codegen).

## COMPUTED TRANSFERS (all resolved)

- braf r1 @0x223fb8 -> **0x225c12** (delta 0x1c56, pool@0x224010). Unconditional goto
  taken when the replay buffer is non-empty; maps to the L_combo label (m2c line 910).
- bsrf r3 @0x223fc2 -> **func_20fb98** (delta 0xfffebbd2). Args r4=rec(8734), r5=rec2(87ac).
- braf r0 @0x22403e -> the jump table above.
- bsrf r3 @0x224b06 / 0x225654 / 0x225716 / 0x225d36 -> **func_23c7f6(entity)**, returns
  r0 tested `!= 0`. (m2c lost the return - it reused a stale local as the condition;
  rewritten on the real return at all 4 sites.)
- bsr @0x225c6c -> **func_225fac** (already shipped, 02225fac.c);
  `func_225fac(entity, src_desc, arr_base, (s16)b30)` where
  arr_base = *(*(g_060862A8[slot]) + unk45*4) + (unk46+unk47)*4,
  src_desc = arr_base + MU16(g_06096EA0,slot*2)*0xC.

## m2c CORRECTIONS

1. **`tst #imm8` sign-extension bug (the critical one).** m2c sign-extends the 8-bit
   immediate of `tst #imm`, so it renders `tst #192` (0xC0) as `& ~0x3F` (=0xFFC0) and
   `tst #128` (0x80) as `& ~0x7F` (=0xFF80). The SH-2 zero-extends the immediate, so the
   true masks are **`& 0xC0`** and **`& 0x80`** (low byte only). This matters: these test
   bits 6-7 / bit 7 of the button words and of cat, and the high byte is frequently set
   (bits 8-9 = the 0x300 left/right buttons). 4x `~0x3F`->0xC0 (button remap x2,
   button-0x80 detect would-be) and 3x `~0x7F`->0x80 (drift-physics H test, jump-start
   accel, replay-drift) corrected. `tst #imm` with imm<0x80 (#1,#2,#64,...) and 32-bit
   pool AND/TST masks (0x037FBFF0, 0x037FBF70, 0x230000, ~0x402, ~0xD) are unaffected.
2. The four bsrf r3 return values (func_23c7f6) - rewritten (see above).
3. Pool phantoms (NOT calls/tables), confirmed and ignored:
   - "bsr 0x2237be" @0x224016  = low half of 0xfffebbd2 (func_20fb98 pool literal)
   - "bsr 0x22583e" @0x22585a  = pool word 0xbff0 (0x22583e itself is the pool const 0x84)
   - "bsr 0x225a36" @0x225b52  = pool word 0xbf70
   - "braf r0"      @0x225d9c  = 0x0023 word inside a literal pool (as giants2 noted)
   => the giants2 note's "shared blocks via bsr -> loc_22583e / loc_225a36" are BOTH
      pool phantoms; the only real direct call is func_225fac @0x225c6c.
4. fn.extent default limit (0x2000) truncates this 0x20cc function; must raise to >=0x20cc.

## FIELD MAP (entity, verified widths)

M8 : 0x34 slot, 0x36 flag, 0x41 anim-dirty, 0x44 facing, 0x45 char/move-set, 0x46/0x47 edge
M16: 0x26, 0x52 overflow, 0x54 event bits(&1,&2,&3), 0x58 range(u16), 0x5C pose id,
     0xCA, 0xCC, 0xCE
M32: 0x0C, 0x14, 0x18, 0x1C, 0x78, 0x7C, 0x80, 0x84 cat, 0x88 st, 0x98, 0xC4, 0xC8, 0x114
Record (rec/g_06098734): +0 cur-buttons(u16), +0xC/+0x10/+0x12 counters(s16).
Record (rec2/g_060987AC): +2 (s16), +5 (s8).

## DEFERRED: none for this target. Whole function shipped.

Globals -> notes/symbols_combatSM.txt. Shared notes/symbols.txt, dh.h, tools/, port/ untouched.
