# Functional decomp - 0x0222ef38 (gen7 deferral cleared)

## Result: SHIPPED
`project/src_functional/0222ef38.c` - `func_0222ef38` (actor_multi_hitbox_sweep_test).
Compiles `gcc -c -fsyntax-only -Wall -Wextra` with ZERO warnings. fidelity HIGH.

## What it is
A 10-iteration specialisation of the already-done sibling swept hit test
`func_0222f1d0` (hit_check_swept). Same point-to-segment distance math, but the
"point" is pulled from an array of the attacker's active sub-hitboxes (per-limb
hitbox positions in its pose record) rather than a single attacker position. It
returns 1 on the first (highest-index) hitbox that hits the target's swept motion
segment, 0 if none.

ABI: `s32 func_0222ef38(void *atk /*r4*/, void *tgt /*r5*/)`. r13=atk, r14=tgt.

## Why m2c failed (and the correction)
m2c reported "Cannot find branch target .L22f1b2" - a branch-into-branch. Root
cause confirmed: the two mid-body literal pools were disassembled as code.
  - pool @0x22f01a..0x22f046 (sits between the `bra 0x22f07a` at 0x22f016 and the
    real block at 0x22f048)
  - pool @0x22f182..0x22f1a6 (between `bra 0x22f1bc` at 0x22f17e and block 0x22f1a8)
fn.py already marks the genuine data words as `.byte`, but several pool words
still decode as plausible instructions (e.g. `22f01e add #80,r5`), and m2c built a
phantom branch target out of one, poisoning the whole CFG. Trusting fn.py's `>`
branch targets + braf_resolve TARGETs, the real CFG is a clean count-down loop.

## Control-flow map (real)
```
prologue 22ef38: push r8-r14,pr; frame -0x30
  seg[0..2] = tgt.4C/50/54 - tgt.08/0C/10         (slot24/28/32)
  r8 = lensq = dot(seg,seg)        [jsr r11=func_022132c4]
  radp = 0x0607C424 + atk.5E*0x40 + 0x24          (s32*, -4/iter)  (slot4)
  rec  = 0x06097D58 + atk.34*0x100 + 0x90         (u8*,  -16/iter) (r12)
  bra 22f1b2 ; r9 = 9                              (loop counter)

loop test 22f1b2: if (r9 < 0) -> 22f1ba (return 0); else -> 22efa8
body 22efa8:
  cand_r = *radp;  if (cand_r==0) -> 22f1a8 (continue)
22efb6: if (func_0221330c(0x6097444,0x6097438,rec,cand_r)==0) -> 22f1a8
22efca: rel = rec.xyz - tgt.08/0C/10 (slot36/40/44); proj = dot(rel,seg)
        r4=proj: proj<=0 -> 22f08c ;  else proj>=lensq -> fall(22eff8) ; else -> 22f048
  22f08c  (before) : dist2 = dot(rel,rel)
  22eff8  (after)  : closest = rec.xyz - tgt.4C/50/54 ; -> 22f07a
  22f048  (along)  : t=divide(proj,lensq); closest = rel - fixmul(t,seg) ; -> 22f07a
  22f07a           : dist2 = dot(closest,closest)
22f09a: rad = cand_r + tgt.64 ; if (dist2 >= fixmul(rad,rad)) -> 22f1a8 (continue)
22f0b0: HIT - register into atk, knockback, sfx, mark tgt -> return 1 (22f1bc)
22f1a8 (continue): r9--; rec-=16; radp-=4(one word); -> 22f1b2
22f1ba: return 0
```

## Resolved helpers
PC-relative (mova+add anchors, not in braf_resolve's list - resolved here):
  - r11 jsr -> 0x022132c4  vec3_dot_fixed   (mova 0x22ef90 + 0xfffe4334)
  - r10 jsr -> 0x022131c8  fixmul_16_16     (mova 0x22efa4 + 0xfffe4224)
braf_resolve.py (5 bsrf):
  - 0x22efbe -> 0x0021330c (SH2 0x0221330c) bounds_overlap_margin(a,b,c,margin)
  - 0x22f04a -> 0x00213234 sh2_hw_divide(num,den)
  - 0x22f104 -> 0x002107dc angle-from-2d (5 args: 4 reg + 1 stack)
  - 0x22f12c -> 0x0021055c lookat_angles_xz (8 args: 4 reg + 4 stack)
  - 0x22f150 -> 0x00215f6c sound_queue_push

## m2c corrections found
1. The whole function - m2c emitted asm only (no C). Phantom branch target from a
   mis-decoded literal pool; real CFG is a simple count-down loop (r9: 9 -> 0).
2. Dropped stack-arg marshalling on the two hit-response calls:
   - func_02107dc: 1 stack arg (&atk.62)
   - func_021055c: 4 stack args (tgt.0C, tgt.10, &atk.60, &atk.62)
   Both declared with full prototypes so the stack args are preserved. (The gen7
   warning about dropped stack marshalling applies here directly.)
3. No ashl_var/ashr_var in this function (checked); the variable-shift trap did
   not apply.

## Differences from sibling func_0222f1d0 (so they are genuinely distinct)
  - atk.49 = loop index i   (sibling: = 0)
  - tgt.5E = loop index i   (sibling: = 0)
  - extra store atk.138 = tgt.4C
  - special path does NOT set atk.60 = 0x2000 (sibling does)
  - point comes from the rec/radp candidate arrays, not a single atk position

## Cross-validation
Helper ABIs, the three-branch distance math, the field-copy block and both
knockback-angle calls are field-for-field consistent with the hand-verified
sibling `0222f1d0.c`; the gated-sfx table pattern matches `02248bb8.c`
(`g_06053F04[idx] << 8 | 0xA00000FF`, gate `g_060E0DDC`).
