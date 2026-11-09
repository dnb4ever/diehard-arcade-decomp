# Functional decomp batch: monsters2 (giant remaining functions)

Die Hard Arcade (Sega ST-V, dual SH-2, big-endian). Functional (not byte-exact)
decomp. All shipped TUs compile `gcc -O1 -c -Wall -Wextra -I project/src_functional`
with ZERO output. Single serial worker, no forks. Toolchain under tools/decomp/.

## Summary

8 target entries were assigned. Several were fn.extent OVER-SWALLOWS (one entry =
multiple real functions sharing a pool group). Net result:

SHIPPED WHOLE (9 TUs, all compile clean):
| file (src_functional) | name | notes |
|---|---|---|
| 0220c782.c | entity_phase_dispatch | RAM fn-ptr jump-table dispatch; sibling @0x20c858 left to over-swallow |
| 0223d768.c | effect_bounce_task | 2-state bounce physics; softfloat block MED |
| 0221f2bc.c | scene_intro_bootstrap_sm | band21 (dh_band21.h); 4-state bootstrap |
| 0220b0c6.c | camera_pose_reset_task | from over-swallow of 0x20b0c6 |
| 0220b0fc.c | camera_vel_integrate_task | " (sibling 2) |
| 0220b1c0.c | camera_pose_init_task | " (sibling 3) |
| 0239ce8.c | actor_state_select | combat state selector; anim-word swap |
| 0222ca98.c | draw_actor_hier_shadowed | 780-insn unrolled articulated-model draw |
| 022082be.c | scene_display_init | VDP init; installs handler 0x022083a0 |

DEFERRED (full CFG decoded + all targets resolved; see below):
| entry | name | why deferred |
|---|---|---|
| 0x0222c39c | actor_combat_cmd_handler | fully resolvable BUT m2c mis-maps one call (02223ee0 vs 02107dc) + 1 M2C_ERROR + two-loop structure needs asm-level reconstruction; combat logic, correctness-critical |
| 0x022083a0 | scene_math_task_2083a0 | ~756 insns fixed-point-math task, part of the 0x2082be over-swallow; not yet transcribed |

## Over-swallow boundaries (fn.extent bounded individually)
- 0x0220b0c6 group (code 0x28e) = THREE fns, shared pool 0x20b19a-0x20b1c0:
  - 0x0220b0c6..0x0220b0fc (camera_pose_reset_task)
  - 0x0220b0fc..0x0220b1c0 (camera_vel_integrate_task)
  - 0x0220b1c0..0x0220b354 (camera_pose_init_task; rts @0x20b350)
- 0x0220c782 group = 0x0220c782..0x0220c858 (real fn; braf tail) + separate fn @0x20c858.
- 0x022082be group (code 0x6cc) = 0x022082be..0x020832c (scene_display_init; braf tail)
  + VDP-transfer pool 0x20832e-0x2083a0 + 0x022083a0..0x0208988 (scene_math_task_2083a0).

## m2c corrections applied (shipped TUs)
- 0x0223d768: `*M2C_ERROR($r2)=func_0224a7a8()` is `entity->unk70 = dtoi()` (r2 held
  &entity->unk70 across the softfloat calls).
- 0x0221f2bc: `func_02215f6c(0xA00003FF, 0x060BACB4)` is `sound_queue_push(0xA00003FF)`
  — 0x060BACB4 is a stale r5 (the slot just cleared), not an argument.
- 0x0220b0c6 / b0fc / b1c0: `func_02214a7a(entity, <extra>)` 2nd/3rd/4th args are
  phantom (r5 is the zero-source register; r6/sp leftover from a clear loop) —
  mtx_build_fwd_inv takes `entity` only.
- 0x0239ce8: braf_resolve "computed bsrf @0x239dcc via shll r0" is the POOL CONSTANT
  0x0003f000 (phantom 0x0003=bsrf) — ignored. Array stores verified as mov.w / s16.
- 0x0222ca98: the 3 PIC fn-ptrs (0x02214a2e/0x022149e4/0x02214a50) were untyped;
  matrix_stack_push() takes NO args (m2c's first-call (mtx,w) args are stale r4/r5).

## Trap clearances (all shipped + deferred)
- No register-count shifts (shld/shad) anywhere -> ashl_var/ashr_var NOT needed.
- tst #imm8 checked everywhere: all imm < 0x80 (0x30,0x04,0x02,0x0D,0x0F,...), no
  sign-extend trap.
- Pool phantom bsrf (0x0003xxxx) seen & ignored at 0x239dcc (0x239ce8) and the
  0x22c3da-0x22c402 literal pool (0x22c39c, flagged @0x22c3e8) — NOT jump tables.

## Decoded tables
- entity_phase_dispatch (0x20c782): three 16-entry RAM fn-ptr tables indexed by
  (entity->unkB2 + i) & 0xF: guards @0x060520A4, next-handlers @0x060520E4,
  fallback @0x06052124; count @0x06052098.
- camera_pose_init_task (0x20b1c0): pos 0x06051E8C[sel->unk28] stride 12; angle
  0x06051F94[sel->unk29] stride 6; defaults 0x06051E78/0x06051E84; stage offset
  0x06060814[g_060B3D54] stride 44.
- actor_state_select (0x239ce8): cmd=g_060E0F0C[sel]; flags=g_0604EBE4[cmd]&0x3F000;
  anim recs via g_06091678[]; anim-word swap g_060E11E8[unk34]<->g_06096248[unk34].
- draw_actor_hier_shadowed (0x22ca98): rot recs 0x06096328+(s16)((s8)unk34*108)
  stride 6; trans recs 0x0607976C+(s16)unk5E*180 stride 0xC; shadow part-id words
  0x06096248+(s8)unk34*32 stride 2; body parts drawn via func_0222ec98(actor,0..0xA).

---

## DEFERRED: 0x0222c39c (actor_combat_cmd_handler) — decoded CFG

Single function 0x22c39c..0x22ca40 (rts @0x22ca3e). ABI: (actor r4, sel r6).
FULLY RESOLVABLE (no unknown targets). Deferred only for correctness risk — the
m2c body has a confirmed call mis-mapping and an M2C_ERROR. 347-line m2c body in
/tmp was the basis; regenerate with:
  decomp_bounded.py 0x22c39c func_0222c39c --end 0x22c42

Entry: actor=r4, entity fields via r14=actor. temp_r5 = 0x06098734 + (s8)((unk34-1)*0x18).
  if (actor->unk48 != 0) { func_0222adaa(actor, temp_r5, sel, unk34); return; }   // tail
  temp_r4 = actor->unk4C; temp_r6 = g_06096E90[unk34] (s16).
  if (temp_r4 == 0):
     if ((unk84 & 0xD) && (u16)temp_r6==0 && (temp_r5->unk4 & 1)):
        loop_93 (i=0..7 over the command queue g_060E0F0C/g_060E0F2C/g_060E0F4C):
           if slot active && marked==1 && g_060E0F4C[i]->unk10==1 &&
              g_060E0F6C[unk34*32 + i*4] < 0xA900:
              r = actor_state_select(actor, <r5>, i, unk34);   // PIC call -> 0x02239ce8
              if (r >= 0) { actor->unk43 = r+0x21; actor->unk84 |= 0x20000000; return; }
why loop_93: on no hit -> block_100.
     block_100:
        if (func_22c220(actor)) return;
        if (!(unk84 & 0x400000)) func_0223a9d0(actor);
        if ((unk84 & 4) && unk88==1 && unk45<=1) { func_22bfc2(actor); return; }
        if (unk45) return; if (!(unk84 & 0x100)) return; if (unk88 != 0x14) return;
        loop_115 / loop @0x22c8f0 (scan other actors, slots 1..6):
           ** m2c MIS-MAP: m2c renders func_02223ee0 here, but the asm @0x22c9ac is
              func_02107dc(actor->unk8, actor->unk10, other->unk8, other->unk10, &scratch);
              (func_02223ee0 @0x22c876 is a DISTINCT call in the same region.)
           gate: other->unk34>2 || g_0609979A!=0; then
              (other->unk84 & 0x40D) && !(other->unk84 & 0x237FBB00) && other->unk48==0 &&
              !(other->unk38 & 0x9E00) && other->unk4E==0 && |actor->unk10-other->unk10|<0x4000;
           on match: grab setup (actor->unk4B=other->unk34, other->unk4C=actor->unk34,
              other->unkE0=0x2000, other->unk84=0x40000, other->unk88=0x2C,
              actor->unk84=0x20000, unk41=1, unk43=0xF, unk5C=0x78/0x7D); return.
  else (temp_r4 != 0):
     other = g_060995B0[temp_r4]->unk8.
     if ((u16)temp_r6 != 0) { func_0223a9d0(actor); actor->unk52 = 0; }   // <- M2C_ERROR was here (bsrf r3 @0x22c7a2 -> 0x0223a9d0)
     actor->unk41=1; actor->unk10 = other->unk10 + actor->unkC8; unk78=unk7C=unk80=0;
     switch (actor->unk88) {  // throw-type grab alignment; each case:
        0x4B/0x4C/0x4D/0x4E/0x2B/0x2C/0x2D:
           unk43 = 0x16 (or 0x10 for 0x2C); if (actor->unk44==other->unk44) { flip unk44,
           unk5C=<id>, actor->unk8 = other->unk8 + <ofsA> } else { unk26 += 0x8000,
           unk5C=<id-1>, actor->unk8 = other->unk8 + <ofsB> }.  Per-case <id>/<ofsA>/<ofsB>
           literals are in the m2c body (e.g. 0x4B: 0x275/0xFFFEF5C3/0x10A3D). }
     if (!(actor->unk8C & 0xFFFFFF)) return;
     actor->unk74=0; actor->unk62 = unk44?0xC000:0x4000; actor->unk60=0x2000; unk45=0;
     func_02223e70(actor, 3, (u16)temp_r6, unk34);

Resolved calls: 0x0222adaa, actor_state_select(0x02239ce8, via mova+add 0x22c7e4+0xD504),
  func_22c220, func_0223a9d0, func_22bfc2, func_02107dc(0x22c9ac), func_02223ee0(0x22c876),
  func_02223e70(0x22c434). Globals: g_06096E90, g_06098734, g_060E0F0C/2C/4C/6C,
  g_060995B0, g_060995D0, g_06099590, g_0609979A, g_06099584.
TODO to ship: confirm actor_state_select arg count (asm @0x22c504 region), correct the
  02223ee0/02107dc mapping against the asm, verify per-case switch literals.

## DEFERRED: 0x022083a0 (scene_math_task_2083a0) — decoded CFG shell

Single function 0x2083a0..0x20898a (rts @0x208986). ABI: (arg0 r4). Installed as the
scene handler by scene_display_init (0x2082be). Fixed-point-math dominated:
  16x func_022131c8 (16.16 multiply) + 10x func_02213234 (16.16 divide) interleaved
  (a matrix / projection / physics build), then func_02210020 (rng), func_022214d2 (x2),
  func_0222142c, func_02213290, func_0221675e, func_02220ade (x2),
  sound_queue_push (0x02215f6c). 2 PIC ptrs: 0x0221632e, 0x022213a8 (mova+add).
Computed branch @0x208874 ("bsrf r0 via stc sr,r0") is a pool phantom (0x0002/0x0003
words) — NOT a jump table; verify & ignore. No tst/shift traps.
TODO to ship: bound --end 0x20898a, transcribe the 16.16 mul/div pipeline with the
  two PIC table pointers resolved by hand.
