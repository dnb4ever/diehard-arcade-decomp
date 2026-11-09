# Functional decomp report - BLOB SIBLINGS (worker: blobsiblings)

Scope: the sibling functions inside the two census blobs the lastgiants pass
mapped - siblings of the already-shipped func_02246ab8 (blob [0x246ab8, 0x2470da))
and func_02402c0 (blob [0x2402c0, 0x241556)). Rule followed: ship only
zero-logical-error work; defer the rest with a reason. Every shipped TU compiles
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` with ZERO output.

## Shipped (11 new .c files, all compile clean)

### 0x246xxx run
- **project/src_functional/02246cac.c** - func_02246cac(obj): state-init helper,
  called 3x by func_02246ab8. switch on func_243fe0(obj).
- **project/src_functional/02246d0c.c** - func_02246d0c(ctx, obj): slim dispatcher,
  sibling of func_02246ab8. Shares the func_22dc2c(ctx,obj) universal tail. Its
  computed call uses table **0x06090214** indexed by the **signed byte obj->0x6D**
  (NOT 0x06090230 / 0x100 like the others - a genuine difference).
- **project/src_functional/02246dbc.c** - func_02246dbc(obj): full state FSM. Two
  switches (func_244e2e, func_243fe0), a 2-arg braf tail func_249836(obj,ret), a
  computed call via 0x06090230[obj->0x100], and the 0x128-indexed active[]/obj[]
  block (func_24492c). Real fn [0x246dbc, 0x246f32).
- **project/src_functional/02246f32.c** - func_02246f32(obj): slimmer sibling of
  02246dbc (no 0x8C/0x40 pre-gate, no 0x20/0x128 block). Real fn [0x246f32,0x247036).
- **project/src_functional/02247036.c** - func_02247036(obj): one-line dispatcher,
  (*(0x06090230 + obj->0x100*4))(obj).
- **project/src_functional/02247064.c** - func_02247064(obj): leaf field-setter.
- **project/src_functional/022470a2.c** - func_022470a2(obj): leaf (the func_02246ab8
  case-3 tail; already decoded in symbols_lastgiants.txt - trivial ship, verified).

### 0x241xxx run
- **project/src_functional/0224124e.c** - func_0224124e(obj, out, arg): the shared
  tail of func_02402c0. Facing-sprite resolve through a 3-level pointer table
  (obj->0x164 -> [*out] -> [arg] -> [sel]) with two angle quantizers (>>14, >>11).
- **project/src_functional/02241324.c** - func_02241324(obj, out): sibling of
  0224124e; sets up motion state then resolves via obj->0x164 -> [*out] -> +12 ->
  deref -> [idx].
- **project/src_functional/022413e6.c** - func_022413e6(obj, out): sibling with a
  randomized launch angle (func_210020) and an optional PIC trigger (func_215f6c).
  Real fn [0x2413e6, 0x2414ee).
- **project/src_functional/022414ee.c** - func_022414ee(obj, out): leaf facing-init.

## Deferred (1)

- **0x0224148e - NOT a function.** find_entry.py reports it as "already real", but
  the address is **interior to func_022413e6**: it begins mid-expression with
  `shar r5` and has no prologue (no `mov.l r14,@-r15`). fn.py's extent for
  func_022413e6 (code 0x108 = [0x2413e6, 0x2414ee)) runs straight through 0x24148e.
  The task listed it as "0x0224148e?" - the question mark is resolved: it is a
  false-positive entry and is covered inside 022413e6.c, not shipped separately.

## Findings / corrections

- **Census over-swallow confirmed for both blobs.** fn.py extents merge siblings:
  func_02246dbc swallows func_02246f32; func_022413e6 swallows the 0x24148e label.
  Each real function was bounded with find_entry + braf_resolve + the real
  prologue/epilogue.
- **func_02246d0c uses a different handler table.** Its computed call is
  `(*(0x06090214 + (s8)obj->0x6D * 4))(obj)` - table 0x06090214, signed-byte index
  obj->0x6D - unlike func_02246ab8/dbc/f32/247036 which use 0x06090230 indexed by
  obj->0x100. Easy to conflate; kept distinct.
- **func_02246dbc 2-arg tail.** The branch after func_245d1c is a `braf` to
  func_249836 **with r5 = func_245d1c's return value** carried in the delay slot,
  i.e. func_249836(obj, ret) - a 2-argument tail call. m2c drops the 2nd arg.
- **braf_resolve phantom.** For func_02246dbc, braf_resolve reports
  `0x246e68 bsrf r0 -> TABLE/COMPUTED via mov.b @(r0,r8)`. No such bsrf exists in
  the instruction stream (the m2c/fn.py disassembly shows an extu.w/cmp sequence
  there). This is the known pool/phantom misread - ignored, per task guidance.
- **func_022413e6 dead-zero term (faithful).** The code computes
  `obj->0xF0 += (((u16)rand >> 16) & 0x7FF) + 0x1400`. The `(u16)rand >> 16` term
  is **always 0** because the compiled code zero-extends rand to 16 bits (extu.w)
  *before* the shlr16. Kept faithful with a comment - net effect obj->0xF0 += 0x1400.
  Verified against the raw fn.py opcodes (0x24142e extu.w; 0x241432 shlr16), so it
  is the real compiled behaviour, not a disassembly artifact.
- **tst #imm8 trap: not triggered.** No `tst #imm8 >= 0x80` occurs in any of the 11
  functions. The only tst-immediates are `tst #2` and `tst #32` (func_02246dbc/f32);
  all other tst are register-form. The sign-extend mask trap does not apply here.
- **shar counts verified.** The angle quantizers in the 0x241xxx family use 14 and
  11 `shar` (arithmetic >>14 / >>11); every shifted value is non-negative, so
  arithmetic == logical. Counts cross-checked against both m2c --asm and fn.py.

## Already shipped by the lastgiants pass (present, re-verified clean - not mine)

func_02241556, func_0224158c, func_022415ee (actor_motion_init), func_022416c4
(actor_facing_clamp), func_0224174e, func_022417a4, func_022417ea. All 7 already
existed in project/src_functional/ and compile clean; left untouched.

## Summary
- 11 new TUs shipped, all compile `gcc -O1 -Wall -Wextra` with zero output.
- 1 target deferred: 0x0224148e is not a distinct function (interior to 022413e6).
- Key corrections: func_02246d0c's distinct 0x06090214/0x6D table; func_02246dbc's
  2-arg func_249836 tail; the func_022413e6 always-zero obj->0xF0 term; and the
  ignored braf_resolve phantom at 0x246e68.
- Names/globals appended to notes/symbols_blobsiblings.txt. notes/symbols.txt,
  shared headers, tools/, port/ untouched.
