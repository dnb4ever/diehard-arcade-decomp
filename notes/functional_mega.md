# Functional decomp: VDP colour/geometry "mega" builders

Scope: unblock the mega builders by resolving their ~5 shared helper ABIs, then
decompile the first one end-to-end as the clone template.

## 1. Shared helper ABIs (STEP 1) - ALL already present in src_functional, reused

| addr | name | ABI | notes |
|------|------|-----|-------|
| 0x2149aa | mtx_load_abc | void(void) | copy_longs(0x06098abc -> *0x06098878, 12) |
| 0x214a40 | mtx_apply_vec_at8 | void(u8 *obj) | vec3_array_matmul(obj+8,+12,+16) |
| 0x214a06 | mtx_rotate_zyx_at24 | void(u8 *obj) | rotate by u16 angles @obj+0x24/0x26/0x28 (Z,Y,X) |
| 0x214a2e | mtx_stack_push_dup | void(void) | *p += 0x30; duplicate top matrix (12 longs) |
| 0x213474 | matrix_copy_reverse | void(void) | 2 matrices row-swapped -> 0x060da040 + 3-long translate |

Two PIC runtime fn-ptrs in the builders (mova+add idiom, resolved with ptr_resolve):
- **jsr @r8 = 0x213ca4** `vec3_array_matmul` = `func_02213ca4(s32 x,s32 y,s32 z)`
  (transform vec by *0x06098878 in place; r8 = anchor 0x234bbc + delta 0xfffdf0e8).
- **jsr @r9 = 0x2149e4** `mtx_rotate_zyx` = `void(u16 *angles)` rotate active matrix by
  angles[2],[1],[0] in Z,Y,X order (r9 = anchor 0x234bd8 + delta 0xfffdfe0c).

Shift helper (ashl_var): **0x224a360** `func_0224a360(s32 value,s32 count)` returns
`value<<count` (count<0 -> value, count>=32 -> 0). NON-standard ABI: in r0=value,
r1=count, out r0. In the builders the value is set in the bsrf delay slot (r0) and
count comes from the obj field saved on the stack (r1) - it is `obj[0x4C]` for every
vertex coord. All 45 call sites are this single pure shift.

## 2. Builder A = 0x234b58  ->  vdp_colour_build_a (STEP 2, DONE)

File: project/src_functional/02234b58.c  (gcc -c -fsyntax-only -Wall -Wextra: 0 warnings;
every iteration hand-traced vs m2c_fn.py --asm).

ROM is fully unrolled: 1 prologue + 15 emit blocks (~800 asm lines, 84 computed
branches). Re-rolled to an equivalent 15-iteration loop.

### Object (r13) fields
- obj+0x34 (s8)  part index  -> euler/id table row select
- obj+0x4C (s8)  shift count -> ashl_var on each vec3 coord
- obj+0x5E (s16) mesh index  -> vec3 (vertex) table select
- obj+0x30 (u16) output slot  -> written each block from the id table

### Tables (HWRAM)
- 0x0607976C  vec3 rows, stride 0x0C (x,y,z longs)       index = obj+0x5E * 0xB4
- 0x06096328  euler rows, stride 0x06 (3 u16)            index = obj+0x34 * 108
- 0x0609638C  (= 0x06096328+100) one long per part rec   (prologue vec Y arg)
- 0x06096248  id halfwords, stride 0x20 (one u16/block)  index = obj+0x34 * 32
- 0x06098878  matrix-stack WRITE POINTER (volatile), bumped +-0x30

### Prologue
`*p += 0x30; mtx_load_abc(); mtx_apply_vec_at8(obj); mtx_rotate_zyx_at24(obj);
mtx_stack_push_dup(); vec3_array_matmul(0, *(long*)(0x609638C+obj34*108), 0);
mtx_rotate_zyx(euler_base); mtx_stack_push_dup();`

### Per-block body (uniform x15)
`obj[0x30]=*id++; euler+=3; vec3_array_matmul(ashl(v[0],c),ashl(v[1],c),ashl(v[2],c));
mtx_rotate_zyx(euler); matrix_copy_reverse(); func_235dc0(obj); *p -= rewind[i];
if (i<14) mtx_stack_push_dup(); vec+=3;`
- euler advances BEFORE use (+6 B), vec advances AFTER use (+12 B), id post-inc (+2 B).
- rewind[] (bytes, per block 1..15) = {0,0x30,0,0,0x90,0,0,0xC0,0,0,0,0x90,0,0,0x120}.
  The rewind store sits in the push bsrf's DELAY SLOT, so it precedes the push body.
- Block 15 rewinds 0x120 and does NOT push (function returns there).

Pointer-walk / stack-write ordering preserved exactly; this is why re-rolling is
behaviour-identical (only non-uniform state = rewind[] + the last-block push skip).

## 3. Cloning to builder B = 0x235198  (STEP 3 - recipe, NOT yet built)

Builder B is the SAME template but NOT a pure constant swap - do a full hand-trace:
- Same tables / same field offsets / same helpers / same prologue shape.
- Register allocation differs: B uses **r9** for the id table (A used r10).
- DIFFERENT unroll: B has **13** matrix_copy_reverse (A has 15) -> fewer/other blocks;
  its rewind[] pattern and push-skip MUST be re-derived from braf_resolve + fn.py, do
  not copy A's table.
- func_235dc0: A calls it every block (mix of PIC bsrf + near direct `bsr`); confirm
  B's per-block hook target before assuming it is the same (B's unit contains 235dc0).
- Recipe: `find_entry 235198`; `braf_resolve 0x235198` (histogram: 45 ashl, 16 push,
  13 matrix_copy); `ptr_resolve 0x235198` for its r8/r9 deltas; `fn.py 0x235198`
  for the annotated listing; trace each block's rewind (grep '@r12'/'add #-48') and
  whether it pushes; then instantiate the same loop with B's rewind[] and block count.

## 4. Correction to the task premise
There are **TWO** mega builders here, not three. **0x0223577c is not a function** -
it is literal-pool/body data inside the 0x235198 compilation unit (find_entry reports
"pool-contaminated", snaps to 0x235198; m2c disasm at 0x23577c is .byte garbage).
Unit map (fn.py --units): `234b58-235198` = 1 func (builder A);
`235198-235f0c` = 3 funcs (builder B @235198, func_235dc0 @235dc0, + one more).

## 5. Resisted / open
- Did NOT rush builder B: it needs its own exact trace (different block count +
  rewind pattern); rushing risked silent transcription bugs (mission's own warning).
- func_235dc0 (0x235dc0) left as extern - out of scope (a sibling, not a helper the
  builders are "blocked" on); decompile separately if a full link is wanted.
- The macl save/restore (sts.l macl / lds.l macl) and the 4-byte stack scratch slot
  for the shift count are calling-convention/temp artifacts with no C-level effect.
