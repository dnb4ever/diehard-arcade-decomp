# Deferred-function analysis: 0x022399bc

Status: DEFERRED. Fully decoded below; blocked on one nonstandard-ABI
mid-function call. No guess shipped.

## Entry / frame
- Real entry 0x022399bc (find_entry: real). Size: code 0x318, extent 0x32c.
- Prologue saves r8-r14, pr, macl; `add #-16,r15` (16-byte local frame; the
  @r15 local[0] is reused as a byte-string cursor).
- r9 = actor (arg r4). r14 = obj = *(actor+8). r11 = 0x6096328 (angle table
  base). r12 = &g_06098878 (draw-offset accumulator).
- r13 = e = *( g_060995b0[ (s8)obj->unk34 ] + 8 )   [RAM pointer table at
  0x060995b0, indexed by obj->unk34<<2, then +8 deref].

## Top-level dispatch (NOT a jump table)
`switch ((s8)actor->unk10)` via a plain cmp/eq ladder:
- 0 -> 0x239a34   (full aim/transform setup)
- 1 -> 0x239b60   (countdown obj->unk32; reload to 30 + bump actor->unk10)
- 2 -> 0x239b7e   (countdown obj->unk32; clear actor->0, or parity gate)
- default -> 0x239b9e (shared draw tail)
The delay slot of the dispatch (0x2399ea) always computes r13 = *(e_record+8).

## All computed branches (resolved)
bsrf (standard-ABI calls), cart targets:
- 0x239a40 -> 0x0213fb4  load_matrix_default() (no args); delay does g_06098878 += 48
- 0x239a46 -> 0x0214a06  mtx_rotate_zyx_at24(obj)
- 0x239a88 -> 0x0213ba0  mtx_rotate_x(-(u16)angle[0])
- 0x239a9a -> 0x0213c4c  mtx_rotate_z(-(u16)angle[2])
- 0x239aae -> 0x0213ba0  mtx_rotate_x(-(u16)e[0x24])
- 0x239ac4 -> 0x0213c4c  mtx_rotate_z(-(u16)e[0x28])
- 0x239ace -> 0x0213538  mtx_apply_vec(0x80000,0,0)   -> g_06098b1c[3]
- 0x239af8 -> 0x0210622  lookat_angles_xy(...) 8 ARGS (see below)
- 0x239b04 -> 0x0213538  mtx_apply_vec(0,0,0x80000)
- 0x239b1a -> 0x0213fb4  load_matrix_default()
- 0x239b30 -> 0x0213c4c  mtx_rotate_z(-(u16)obj[0x28])
- 0x239b3c -> 0x0213538  mtx_apply_vec(*g_06098b1c,*g_06098b20,*g_06098b24)
- 0x239b46 -> 0x02130b4  angle helper(*g_06098b24, *g_06098b20) -> obj->unk24
- 0x239ba4 -> 0x02149aa  mtx_load_abc() (no args); delay does g_06098878 += 48
- 0x239bac -> 0x0214a50  mtx_apply_vec_at0(e+8)
- 0x239bca -> 0x0213ca4  vec3_array_matmul(0, g_06096c58[(s8)obj->unk34])
- 0x239c9a -> 0x0214a50  mtx_apply_vec_at0(...)   (2nd loop body)
- 0x239cac -> 0x0214a06  mtx_rotate_zyx_at24(obj) (tail)
- 0x239cb4 -> 0x0215a2c  func_0215a2c(obj, 0)     (tail)
computed jsr via mova+add (hand-resolved):
- 0x239a5c anchor: r8 = 0x239a5c + 0xfffda198 = 0x0213bf4  mtx_rotate_y (legit
  separate callable entry inside the 0x213ba0 matrix-rotate unit). Called at
  0x239a90 and 0x239ab8 as mtx_rotate_y(-(u16)angle[1] / -(u16)e[0x26]).
- 0x239bb4 anchor: r9 = 0x239bb4 + 0xfffdae30 = 0x022179e4  <-- BLOCKER.

## The 8-arg call (reconstructed, for the record)
func_0210622 (lookat_angles_xy) prologue reads 4 register args (r4-r7) plus 4
stack args at callee @(36/40/44/48,r15) = caller stack[0..3]. The caller
marshals (at 0x239ad2-0x239af8):
  r4 = 0, r5 = 0, r6 = 0, r7 = *g_06098b1c,
  stack0 = *g_06098b20, stack1 = *g_06098b24,
  stack2 = &obj->unk28 (out ptr), stack3 = &obj->unk26 (out ptr)
i.e. func_0210622(0,0,0, *g_06098b1c, *g_06098b20, *g_06098b24,
                  &obj->unk28, &obj->unk26). It writes negated angles into
*stack2/*stack3. (This part IS expressible; it is not the blocker.)

## Why deferred
1. jsr @r9 -> 0x022179e4 is a mid-function alternate entry of func_022179c0
   (prologue 0x2179c0, continuous fall-through to 0x2179e4). The alt entry
   consumes inputs from callee-saved r13 (=e) and r14 (=obj) and reads a 2nd
   vector via r5; on the shared-tail path r5 is left undefined by any standard
   call (func_0214a50 clobbers r4-r7 just before). This is a nonstandard
   register ABI (implicit r13/r14), the same class band23hi deferred for the
   func_024a4xx r0/r1 helpers. Cannot be lowered faithfully to functional C.
2. The 2nd command-string loop (0x239c5c-0x239ca2) runs two cursors over the
   same byte stream (r10 via `mov.b @r10+`, and the @r15 local via r2) offset
   by one byte; the first byte is pre-read at 0x239be0 and entered at 0x239ca0,
   skipping the func_0214a50 call for that first byte. Faithful reproduction of
   this pipelined off-by-one is a silent-equivalence hazard layered on top of
   (1).

## Command-string tables (decoded, for whoever picks this up)
- 1st loop cursor base: 0x0608c622 + (obj->unk38)*8. Each nonneg byte `c`:
  rec = 0x06096328 + (s8)obj->unk34 * 108 + c*6; angles u16 @rec+0/+2/+4,
  negated, fed to rotate_x / rotate_y / rotate_z. Loop ends at first byte < 0.
- 2nd loop cursor base: 0x0608c5d2 + (obj->unk38)*8. Primary cursor r10;
  lagging cursor = @r15 local (same base). Per byte `c` (r10): rec2 =
  0x06096328 + (s8)obj->unk34 * 108 + c*6 -> func_022179e4(rec2). Per lagging
  byte `b`: arg = 0x0607976c + 180*(s16)e[0x5e] + b*12 -> mtx_apply_vec_at0.
