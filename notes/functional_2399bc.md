# 0x022399bc — shipped (prior deferral was a transcription error)

Status: SHIPPED to project/src_functional/022399bc.c. Compiles clean
(gcc -c -fsyntax-only -Wall -Wextra, zero warnings).

## The deferral was a phantom
The prior analysis (notes/functional_jumptable_analysis.md) deferred 0x022399bc
on the shared-tail `jsr @r9`, believing r9 = 0x022179e4 — the interior (+0x24)
of func_022179c0, read as a nonstandard-ABI alt-entry taking callee-saved
r13/r14 and dereferencing an "undefined" r5.

That target is WRONG by one digit. The mova+add at 0x239bb2-0x239bb4 computes
r9 = (0x239bb4 + 0xfffdae30) & 0xffffffff = **0x022149e4**, not 0x022179e4.

Verified three ways:
1. Arithmetic (braf_resolve-style): the same PC-relative scheme resolves the
   neighbouring bsrf's exactly to their known targets (0x2149aa mtx_load_abc,
   0x214a50 mtx_apply_vec_at0), and the jsr to 0x2149e4.
2. find_entry.py 0x2149e4 -> "real entry"; disassembly shows a clean standalone
   prologue (mov.l r14,@-r15; mov r4,r14; sts.l pr,@-r15) and a normal epilogue
   — NOT an interior address.
3. 0x022149e4 is an already-named, already-shipped helper: mtx_rotate_zyx
   (func_022149e4), listed "(existing)" across notes/symbols_band23mid,
   banddraw, bande3b0, megab, gen2_218. It is called identically by the shipped
   func_02239082 (spin_projectile_handler) and draw_actor_hier_model (0x0222d0e8)
   as func_022149e4((u8*)x + 0x24).

func_022179c0 is entirely unrelated; it was NOT shipped and needs no work here.

## Pinned ABI of the call target
void func_022149e4(void *angles) — standard ABI, single pointer arg (r4).
Body: mtx_rotate_z((u16)angles[4]); mtx_rotate_y((u16)angles[2]);
mtx_rotate_x((u16)angles[0]) (last is a tail-call; angles NOT negated). No r5,
no implicit r13/r14. It is the base+0 twin of the sibling mtx_rotate_zyx_at24
(0x214a06, which reads +0x24/+0x26/+0x28); called here as func_022149e4(e+0x24)
it is exactly mtx_rotate_zyx_at24(e).

## The r5 question — resolved
No ambiguity. r5 was only "undefined" relative to the WRONG function
(0x2179c0's alt-entry). The correct target 0x2149e4 never reads r5.

## Two-cursor 2nd command loop (0x239c5c-0x239ca2) — modelled exactly
Primary cursor pc (r10, `mov.b @r10+`) drives mtx_rotate_zyx(rot_base + c*6) via
the SAME r9 (=0x2149e4) still live from the shared-tail jsr. A lagging cursor
one byte behind (the @r15 local, same base 0x0608c5d2 + obj->unk38*8) drives
mtx_apply_vec_at0(0x0607976c + (s16)e->unk5E*180 + b*12).
- Byte[0] is pre-read (0x239be0) and the loop is entered at the rotate (bra
  0x239ca0), so byte[0] gets a rotate but its apply is SKIPPED.
- The bottom test peeks *pc (no advance) and exits on the first negative byte,
  so the LAST byte's apply is never reached.
- Net call order: rotate(b0); apply(b0),rotate(b1); ...; apply(b[k-1]),rotate(b[k]).

## Equivalence hazards handled
- case-0 angle_from_vec2 (0x239b40-48) reads g_06098b20/b24 FRESH from memory
  after mtx_apply_vec(vx,vy,vz) overwrote g_06098b1c -> passes POST-apply globals
  g_06098b1c[2],[1], not the vx/vy/vz snapshots (confirmed via the delay-slot
  read at 0x239b48).
- case-0 lookat reads g_06098b1c live right after mtx_apply_vec(0x80000,0,0);
  snapshots vx/vy/vz taken only after the later mtx_apply_vec(0,0,0x80000),
  matching the asm reload.
- lookat writes obj->unk28/unk26 via out-pointers; the subsequent rotate_y/z
  read those updated fields (call-sequence ordering preserved).
- matrix push/pop balance: case-0 +0x30/-0x30 internal; shared tail +0x30/-0x30;
  case-2 early returns skip the shared tail entirely (never push).

## Dispatch summary
switch ((s8)actor->unk10): 0 full aim setup (then tail); 1 countdown obj->unk32,
reload 30 + bump on <=0 (then tail); 2 countdown, clear *actor + return on <=0,
else parity gate (odd return / even tail); default tail.

Names/globals: see notes/symbols_band2399bc.txt.
