# Functional decomp — 0x0222e3b0  pose_matrix_blend_decompose  (SHIPPED)

Hard-tail target previously deferred in `functional_banddraw.md`.  Shipped to
`project/src_functional/0222e3b0.c`; compiles `gcc -c -fsyntax-only -Wall
-Wextra` **clean (zero warnings)**.

## Outcome
SHIPPED.  The deferral reason (dynamic `@-r15` arg pushes that slide the stack
frame mid-function, bulk-cleaned by `add #64,r15`) dissolves once you see the
function is a plain 16-iteration loop that the compiler 4x-unrolled.  Modelling
it as a rolled loop with named C locals is exact and never touches a sliding
stack slot, so there is no fixed-slot hazard to get wrong.

## Signature
`void func_0222e3b0(s32 arg0, s32 arg1, s32 arg2)`
- arg0 = joint-table index (u34); buffer offset `off = (s16)(arg0*108)`.
- arg1 = fixed-point shift bits (caller passes `var_r12 = SHBASE + field`).
- arg2 = blend weight `w` (caller passes `(u8)entity->unk40`).
Matches the call site in `0222dc2c.c`: `func_0222e3b0(u34, var_r12, (u8)unk40)`.
Returns void (epilogue sets no r0; caller ignores result).

## What it does (per joint, 16 joints, stride 6 bytes)
Angle buffers, `off = (s16)(arg0*108)`:
  prev @0x0609661C+off, next @0x06096910+off, cur @0x06096328+off (output).
Active matrix cell = `*0x06098878`; pushed +0x30 at entry, popped -0x30 at exit.
Blend coefficients computed once: `w = arg2`, `b = (1<<arg1) - arg2`  (i.e. the
fixed-point `1.0 - w`, with the `1<<arg1` routed through `func_0224a360`/ashl_var
exactly as the asm does — m2c drops this shift).

Each joint:
1. `load_matrix_default(); mtx_rotate_zyx(prev)` -> matrix A; copy basis columns
   0 and 2:  A0={M[0],M[0x10],M[0x20]}, A1={M[8],M[0x18],M[0x28]}.
2. same from `next` -> matrix B columns 0,2 (B0,B1).
3. blend columns:  bl0[i]=A0[i]*w+B0[i]*b,  bl1[i]=A1[i]*w+B1[i]*b.
4. `lookat_angles_xy(0,0,0, bl0[0],bl0[1],bl0[2], cur+4, cur+2)` writes the Y/Z
   angle shorts to cur[+2],cur[+4].
5. `load_matrix_default(); mtx_rotate_y(-(u16)cur[+2]); mtx_rotate_z(-(u16)cur[+4])`.
6. `mtx_apply_vec(bl1[0],bl1[1],bl1[2])` -> transformed vec @0x06098B1C.
7. `cur[+0] = (s16)angle_from_vec2(*0x06098B24, *0x06098B20)` (the X angle).
8. prev+=6, next+=6, cur+=6.

## Frame map (post-prologue sp = SP0; frame = 104 bytes)
Prologue pushes r14,r13,r12,r11,r10,r9,r8,pr,macl (9 longs) then `add #-104,r15`.
Local slots, offsets relative to SP0:

| off   | meaning |
|-------|---------|
| +0x00 | ptr -> SP0+0x2C  (blend1 output) |
| +0x04 | ptr -> SP0+0x20  (blend0 output) |
| +0x08 | ptr -> SP0+0x44  (matB col2) |
| +0x0C | prev-angle buffer pointer (bumped +6/joint) |
| +0x10 | next-angle buffer pointer (bumped +6/joint) |
| +0x14 | loop counter, init 16, -1/joint |
| +0x18 | const 0x06098B24 (angle_from_vec2 arg1 ptr) |
| +0x1C | const 0x06098B20 (angle_from_vec2 arg2 ptr) |
| +0x20..+0x2B | blend0  (vec3) |
| +0x2C..+0x37 | blend1  (vec3) |
| +0x38..+0x43 | matB col0 (vec3) |
| +0x44..+0x4F | matB col2 (vec3) |
| +0x50..+0x5B | matA col0 (vec3) |
| +0x5C..+0x67 | matA col2 (vec3) |

### The sliding-sp mechanism (the reason it was deferred)
The loop body is 4x unrolled.  Each unrolled block pushes lookat_angles_xy's
four stack arguments with `mov.l rN,@-r15` and does **not** clean them per block.
So across the four blocks sp slides:
  block0 @ SP0,  block1 @ SP0-16,  block2 @ SP0-32,  block3 @ SP0-48,
then `add #64,r15` (0x22e9a2, in the 4th block, right after its lookat call)
restores SP0, and the epilogue `add #104,r15` (0x22e9f8) tears down the frame.
Every `@(disp,r15)` inside a block is written with a matching +0/+16/+32/+48
compensation, so **all four blocks address the identical frame slots and scratch
vec3s** listed above.  The four blocks are therefore behaviourally identical;
the scratch vec3s are fully overwritten each joint (no cross-joint carry besides
the three monotonic +6 pointer bumps and the counter).  Hence the rolled loop is
exact.  m2c cannot track the sliding sp and emits undefined `sp24/sp28/sp38`
(block2/block3 reads of slots it believes are still uninitialised) — these are
artifacts of the fixed-slot model, not real reads-before-write.

## lookat_angles_xy stack-arg order (verified from asm)
Pushes at 0x22e508/50e/514/51a (block0) load, in push order:
  cur+2, cur+4, blend0[2], blend0[1]  (so after the 4 pushes, lowest address =
  blend0[1]).  At the call sp = SP0-16, giving the SH-2 arg layout:
  r4..r7 = 0,0,0,blend0[0];  @sp..@(12,sp) = blend0[1],blend0[2],cur+4,cur+2.
=> `lookat_angles_xy(0,0,0, blend0[0],blend0[1],blend0[2], cur+4, cur+2)`.

## m2c corrections found
- m2c's `sp24/sp28/sp38 used-before-def` are the shifted block2/block3 accesses;
  resolved by the frame map (all map onto blend0/blend1/matB slots).  No real
  uninitialised read exists.
- m2c drops the `1<<arg1` (bsrf to ashl_var); re-added via `func_0224a360(1,arg1)`.
- m2c miscounts the decrements (`sp24 -= 1`, `sp34 -= 1`, etc. in different
  slots): these are all the single loop counter at SP0+0x14 seen through the
  shifted disps (@(36,r15)/@(52,r15)/@(68,r15)/@(20,r15)).
- The internal `bra 0x22e704->0x22e780` is only a jump over the mid-function
  literal pool (0x22e708..0x22e77e), not control flow.

## Callee ABIs used (all extern, owned elsewhere)
0x0224a360 ashl_var(val,cnt)=val<<cnt (val in r0, cnt in r1 — compiler helper;
  declared (value,count) to match sibling 0222dc2c).  0x02213fb4
  load_matrix_default().  0x022149e4 mtx_rotate_zyx(u16* angles, Z@4/Y@2/X@0).
  0x02213bf4 mtx_rotate_y(angle).  0x02213c4c mtx_rotate_z(angle).  0x02213538
  mtx_apply_vec(x,y,z) -> g_06098B1C (per symbols.txt: out=(M*vec)>>16 + col3).
  0x022130b4 angle_from_vec2(a,b) fixed-point atan2.  0x02210622
  lookat_angles_xy(6 vals + 2 out ptrs).

## Fidelity
HIGH.  Call order, args, the 6-element column copies, the blend arithmetic
(`A*w + B*b`, low-32 mul matching `mul.l`), the lookat/rotate/apply/atan2
decompose chain, the three +6 bumps and the matrix push/pop are all transcribed
1:1 from the SH-2 asm.  The only remodelling is roll-up of the compiler's 4x
unroll, which is provably behaviour-preserving (identical slots per block, scratch
overwritten each joint).  All 41 computed branches resolved via braf_resolve.py.
