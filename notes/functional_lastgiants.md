# Functional decomp report - LAST GIANTS (worker: lastgiants)

Scope: the two final deferred large functions, func_02246ab8 and func_022402c0,
left deferred by notes/functional_band240giants.md and notes/functional_giants2.md.
Rule followed: ship only zero-logical-error work; defer the rest with decoded CFG.
Both shipped TUs compile `gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional`
with ZERO output.

## Headline finding: both "giants" were census-blob mirages

fn.py reports a *census extent* that runs to the next literal-pool gap, which for
these two addresses swallows a whole run of sibling functions. The prior workers
measured the blob, not the function:

- **0x02246ab8** fn.py "code 0x622 / 785 insn" -> the real function is only
  **[0x246ab8, 0x246cac) (~250 insn)**. The blob [0x246ab8, 0x2470da) also holds
  func_246cac, func_246d0c, func_246dbc, func_246f32, func_247036, func_247064,
  func_2470a2 (all now extern).
- **0x022402c0** fn.py "code 0x1296 / 2379 insn" -> the real function is only
  **[0x2402c0, 0x24043c) (~90 insn)**. The blob [0x2402c0, 0x241556) holds ~20
  sibling functions sharing a tail pool of helpers at 0x241556+.

Both target functions are fully resolvable and are SHIPPED WHOLE. The "58 computed
branches / func_22ab18 x30 / func_227704 x15" figures in the giants2 note are
blob-wide; func_02402c0 itself has 5 computed calls.

## SHIPPED

### `project/src_functional/02246ab8.c` - func_02246ab8(void *ctx, void *obj)
Counter / hit-interrupt FSM. ctx (r4->r13) is only forwarded to the shared tail;
obj (r5->r14) is the subject. `flags = M32(obj,0xE0)` is cached once (r11) and all
bit tests use that cached copy (memory writes to obj->0xE0 don't change it - must
be preserved). Structure:
1. nested-if cascade, each arm firing one call then jumping to the tail:
   `obj->0x4C!=0`-> func_2489d0; `func_246208(obj)`-> func_249a82;
   `g_06097418`-> func_249f08; `obj->0x54 & 2` -> func_245bcc gate
   (==0: func_24840a + obj->0x5C=obj->0x14A; else func_246180 fallthrough).
2. `obj->0xC8--`; if `old<=0 && !(flags&8)`: optional func_247d6e(flags&0x100),
   then `flags&0x20 ? obj->0xC8=(func_210020()&7)+8 : func_246cac(obj)`.
3. `flags&0xC`: `obj->0xC4--`; if pre-dec==0, `obj->0xE0 &= ~4`.
4. optional func_247d6e (flags&0x100); `flags&0x50` -> func_246cac.
5. `switch (func_244e2e(obj))`:
   - case1: `(*(void(**)(void*))(0x06090230 + obj->0x100*4))(obj)`
   - case3: tail-call func_2470a2(obj); **returns** (bypasses the shared tail)
   - case0/2/default: if `flags&0x20` and `g[0x06099590+idx]!=0`, call
     `func_24492c(obj, MP(g[0x060995B0+idx],8), 0x18000, 0x20000)`; if 0 ->
     `obj->0xE0 &= ~0x20; func_246cac(obj)`.  (idx = obj->0x128)
All non-case3 paths converge on `func_22dc2c(ctx, obj)`.

m2c corrections applied (m2c collapsed to 120 lines with 12 M2C_ERROR and lost the
switch variable + every computed-call return value):
- tail `(&.Lpool_246ad0 + 0xFFFE715C)` = **func_22dc2c (0x0022DC2C)**, NOT the
  giants2 note's 0x225c2c (miscalc; verified against sibling func_246d0c's tail
  0x246d1c+0xFFFE6F10 = same 0x0022DC2C).
- `switch(0xC8)` -> **switch(func_244e2e(obj))** (not obj->0xC8).
- `if (0x4C!=0)` -> `func_246208(obj)!=0`; `(u16)temp_r0==0` -> `func_245bcc(obj)==0`;
  `(0xC8!=0)` -> `func_247d6e(obj)!=0`; `(0xC8&7)+8` -> `(func_210020()&7)+8`;
  `if (0x128==0)` -> `func_24492c(...)==0`.
- m2c mis-attached `(elem->8, 0x18000, 0x20000)` to `.L246cac`; they are
  func_24492c's args. The real func_246cac call takes only obj.
No `tst #imm8 >= 0x80` occurs (only `tst #2`), so no zero/sign-extend mask trap.

### `project/src_functional/02402c0.c` - func_02402c0(void *obj, s32 *out)
Per-object expire / lock-on check. obj=r4(r14), out=r5(r12, an s32* written on
every path). Runs func_22ab18(obj,obj->0x64), func_227704(obj) (gated on
obj->0xE0&0x10000), func_24174e(obj); then:
- `hp=obj->0x64`; if `hp>=obj->0x146 || obj->0x56<=0 || hp==0` -> **expire**
  (obj->0xE0|=0x40000, obj->0x108=1, obj->0x84=0x1000, *out=1, tail).
- else if `func_241556(obj,6)!=0`: obj->0x84=0x800, obj->0x108=0, *out=0, tail.
- else: obj->0x10C=1; `toward = ((MP(g[0x060995B0+obj->0x4A],8)->8) - obj->8) < 0`;
  if `(obj->0x38 & 0x7800) && obj->0x44!=toward && g_06098564==3`: **seek/lock-on**
  (obj->0x5C=MP(obj,0x180)->0xA, obj->0x84=0x800, obj->0xE0|=0x2000000,
   obj->0x108=6, obj->0xBA|=0x100, if g_060E0DDC: func_215f6c(cmd), *out=0,
   **return** directly); else fall to expire.
Tail on all non-lock-on paths: `func_24124e(obj, out, 0)`.
`cmd = ((s16)g_0608DA40 << 8) | 0xA00000FF`.

m2c corrections: m2c stops ~60 lines in and SPLICES a later sibling (its
`func_2416c4` call and the giant `obj->0x5C` expression belong to func_24124e, a
different function - the real tail). Real func_02402c0 ends at 0x24043c.
`if (0x56!=0)` -> `func_241556(obj,6)!=0`. No `tst #imm8` (all tst register form).

## Deferred (none)

Both targets shipped whole, zero logical error, zero compiler output. No tractable
state was left behind for either.

Not shipped (out of scope, declared extern): the sibling functions inside the two
census blobs - func_246cac/246d0c/246dbc/246f32/247036/247064/2470a2 and the
0x240xxx sibling run plus tail helpers func_241556/2415ee/24158c/2416c4/24174e/
2417a4/2417ea/241324/2413e6/2414ee/24124e. func_2470a2 is already fully decoded
(see symbols file) and would be a trivial future ship; the rest need their own pass.

## Summary
- SHIPPED WHOLE: 0x02246ab8, 0x022402c0 (both compile clean, zero warnings).
- Both were mis-measured as giants: the census extent included sibling functions;
  the real functions are ~250 and ~90 insn and fully resolvable.
- Key correction: func_02246ab8 tail is **func_22dc2c (0x0022DC2C)**, not 0x225c2c
  (giants2 miscalc). Switch is on func_244e2e(obj)'s return, not obj->0xC8.
- m2c traps fixed: lost switch var, lost computed-call return values, mis-attached
  call args (func_24492c vs func_246cac), and sibling-splicing in func_02402c0.
- No tst #imm8 >= 0x80 occurs in either function - the known sign-extend mask trap
  did not apply here.
