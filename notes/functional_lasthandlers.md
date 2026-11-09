# Functional decomp: last handlers + duplicate-pair arbitration

Die Hard Arcade (Sega ST-V, dual SH-2, big-endian). Functional (not byte-exact)
decomp. Single serial worker, no forks. Every changed/new TU compiles
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` with ZERO output.
braf_resolve / ptr_resolve TARGETs are treated as authoritative throughout; the
m2c body and the decomp_bounded `--asm` rewriter both systematically mis-assign the
pool-relative deltas of computed (bsrf) calls, so every call was re-mapped by
address against braf_resolve and matched to the defining TU's real signature.

## JOB 1 - the two deferred big handlers: BOTH SHIPPED

### 0x0222c39c  actor_combat_cmd_handler  (SHIPPED, project/src_functional/0222c39c.c)
Grab/throw main handler, [0x0222c39c,0x22ca40) (rts @0x22ca3e), bounded --end
0x22ca42. ABI: actor=r4, r5 unused, sel=r6 (sel is only forwarded to a 1-arg tail,
so dead in a faithful rendering).

m2c corrections (all confirmed by braf_resolve - the m2c AND the monsters2 note
both had these wrong; the three funcs were effectively rotated):
- @0x22c434 -> **func_02223e70** (1-arg). This is the M2C_ERROR slot in the
  slot4C!=0 / angidx!=0 branch (`func_02223e70(actor); actor->unk52 = 0;`). The
  note called this func_0223a9d0 - WRONG.
- @0x22c876 -> **func_02223ee0** (1-arg). block_100's
  `if (!(unk84 & 0x400000)) func_02223ee0(actor);`. The m2c AND note called this
  func_0223a9d0 - WRONG.
- @0x22c9ac -> **store_angle_between** (0x022107dc, 5-arg). The loop_115 two-actor
  probe: `store_angle_between(actor->unk8, actor->unk10, other->unk8,
  other->unk10, &scratch)`; `(u16)scratch[0] >> 15` picks the facing. The m2c
  rendered func_02223ee0 here (the documented mis-map).
- @0x22c7a2 -> **func_0223a9d0** (2-arg, r5=3). The end-of-else call
  `func_0223a9d0(actor, (void*)3)`. The m2c rendered func_02223e70 here - WRONG.
- func_0222adaa / actor_state_select are 1-arg / 3-arg per their defining TUs
  (0222adaa.c, 0239ce8.c); the extra registers set at the call sites are spurious
  (pre-computed temp values / forwarded-but-ignored params).
- the 0x22c3e8 "bsrf via shll r0" is the 0x0003xxxx pool phantom (ignored).
- tst #imm all < 0x80 (0x0D, 0x04, ...), no sign-extend trap.

Switch body (slot4C!=0 throw alignment, unk88 in {0x4B,0x4C,0x4D,0x4E,0x2C,0x2B,
0x2D}) transcribed from m2c and spot-verified vs fn.py --asm (e.g. the 0x2B-else
reuses the 0x2C constants 0xFFFF3C74/0xC38C - confirmed in ROM, not an m2c error).

### 0x022083a0  scene_math_task_2083a0  (SHIPPED, project/src_functional/022083a0.c)
Perspective "road/floor" scene task, [0x022083a0,0x20898a) (rts @0x208986),
bounded --end 0x20898a. State machine on task->unk10 (0..6); obj = task->unk8.
State 0 builds a 0xF0-row perspective table into the ring buffer at cursor
0x06099510 (3 longs/row: 1/z-scaled intensity, y, 1/z); the ROM's 4x loop unroll
is collapsed to a single-row `for (idx<0xF0)` loop (0xF0 % 4 == 0, identical).

m2c corrections (braf_resolve / ptr_resolve authoritative):
- @0x208886 -> func_02213290(0x25E78000, 0x400)   (m2c mis-named / 6-arg).
- @0x20889a -> func_0221675e(0xC00, 0x25E78000, 0x2267607C, 0xB, 6, 0x200)
  (m2c named it func_02213290).
- @0x20890a -> sound_queue_push(0xA01402FF)        (m2c named it func_02220ade).
- @0x208970 -> func_022214d2(0x160,0x5F,0,0,vr,vr) (m2c named it func_02215f6c).
- @0x2087e4 -> func_02210020() rng, no args (m2c passed a spurious arg).
- the two PIC fn-ptrs resolve (ptr_resolve) to func_0221632e (4-arg) and
  func_022213a8 (2-arg).
- @0x208874 is the 0x208870-0x20887e literal pool (0x00038000 / 0x00018cd0 /
  0x00018c00) mis-decoded by capstone as `bsrf` - a jumped-over phantom, the
  M2C_ERROR in m2c case 4. NOT a real call. Matches the monsters2 note.

func_0221632e and func_02220ade are not yet in src_functional (declared extern).

## JOB 2 - divergent duplicate-pair arbitration: all 3 resolved

Each pair is the same ROM entry decompiled twice; the census sibling in all three
cases was the WRONG one - it had been produced under an fn.extent over-swallow and
inlined the bodies of the SEPARATE functions that the canonical file correctly
tail-calls. Verified statement-by-statement vs fn.py --asm.

| ROM entry  | KEPT (canonical)            | DELETED (census)           | why |
|------------|-----------------------------|----------------------------|-----|
| 0x022473ee | 022473ee.c (func_022473ee)  | 02473ee.c (actor_move_init)   | cases 2/3 are `bra 0x247a16`/`bra 0x247a3e` tail calls (confirmed @0x2474ec/.L2474f6). The census inlined func_0223afc4/func_0223ac50 + field writes, which are the bodies of the distinct functions 0x247a16/0x247a3e (already their own TUs 02247a16.c/02247a3e.c). |
| 0x022475a8 | 022475a8.c (func_022475a8)  | 02475a8.c (actor_move_step)   | genuinely equivalent decomps (both carry the same rng&7 m2c correction, same unkD8 state machine, tail-call to animation_update @0x22dc2c which is external); per the "equivalent -> keep canonical" rule. |
| 0x02247924 | 02247924.c (func_02247924)  | 0247924.c (actor_face_resolve)| identical over-swallow trap to 0x2473ee: switch cases 2/3 are `bra 0x247a16`/`bra 0x247a3e` tail calls (confirmed @.L2479fa/.L247a04); census inlined the 0x247a16/0x247a3e bodies. |

Real function boundaries in the region (fn.extent over-swallows all three):
0x2473ee..0x247508, 0x2475a8..0x247924, 0x247924..0x247a16; 0x247a16 and 0x247a3e
are their own functions (02247a16.c / 02247a3e.c).

Caller redirect: `02247508.c` was the only external caller of a deleted name
(`actor_move_init`); redirected to `func_022473ee` (extern + 3 call sites).
`actor_move_step` / `actor_face_resolve` were referenced only inside their own
(now deleted) files, so no other redirects were needed.

## Verification
All changed/new TUs compile clean (zero -Wall -Wextra output):
0222c39c.c, 022083a0.c, 02247508.c, 022473ee.c, 022475a8.c, 02247924.c
(plus the untouched siblings 02247a16.c / 02247a3e.c re-checked). A tree-wide grep
finds zero remaining references to actor_move_init / actor_move_step /
actor_face_resolve. Not touched: git, shared headers, tools/, port/, notes/symbols.txt.
