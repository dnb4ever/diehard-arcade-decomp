# Functional decomp report - BAND 240 CALLEES + 0x02402c0 BLOB SIBLINGS (worker: band240callees)

Scope: the undone switch-selectors/callees named in the task plus the per-state
actor-setup siblings inside the func_02402c0 census blob [0x2402c0, 0x241556).
Rule followed: ship only zero-logical-error work; defer the rest with a reason.
Every shipped TU compiles `gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional`
with ZERO output.

## Shipped (12 new .c files, all compile clean)

### Named callee from the task list
- **project/src_functional/022469f2.c** - func_022469f2(obj): the "kick-to-walk /
  reset counters" shared tail. real fn **[0x2469f2, 0x246ab8)** (fn.py "code 0xc6";
  the reported 0x152 extent trails into func_02246ab8's own prologue - bounded at
  the rts epilogue 0x246ab4). Three-gate fast path (unk E4==6 && (unk8C&0x3FFFF)==0
  && unk6A<3) sets E0|=0x01000000 and braf-tail-calls func_02241e3c
  (actor_enter_statewalk); otherwise resets the actor to the idle base state
  (0xD4/0x84/0x88/0xE0/0x108/0xC4 plus 0xC8 = rng()&7).

### func_02402c0 blob siblings (per-state actor setup handlers)
All take (void *obj, s32 *out); r4=obj, r5=out. They run func_022ab18(obj,unk64),
optionally more helpers, set a few fields, write *out, and (usually) tail into a
facing-resolve helper (func_0224124e / func_02241324 / func_022413e6).

- **02240492.c** - func_227704 then 0x108/0x84; *out=1; -> func_0224124e(obj,out,1).
- **0224043c.c** - actor_motion_init; 0x84/0x108; conditional 0xBC/0xE0 on
  func_0224158c(obj,6)==0; *out=2; -> func_0224124e(obj,out,3).
- **022404ce.c** - 0xE0|=0x400000; early tail to func_02402c0 when
  g_06096c58[(s8)unk34] > 0x8000 (signed); else func_022417a4, a conditional
  facing-table lookup (0x0608D898 / 0x06086304), *out=3, plain rts.
- **02240588.c** - facing from (u16)rec_delta_angle_neg(obj): middle angle range
  -> 0x44=0/0x26=0xC000 else 1/0x4000; 0xBA&0x40 override path (rts) vs normal
  path (0xE0|=0x40000, *out=1, func_0224124e tail).
- **02240612.c** - leaf (no frame): g_060e0f2c[unk94]=1; -> func_02402c0(obj,out).
- **02240a4a.c** - func_227704 gated on unkE0&0x10000; -> func_02241324(obj,out).
- **02240a7a.c** - func_022417a4; func_02241324(obj,out) (bsr); 0xE0|=0x400000; rts.
- **02240ae0.c** - same facing-angle logic as 02240588; -> func_02241324(obj,out).
- **02240b2e.c** - g_060e0f2c[unk94]=1; -> func_02241324(obj,out).
- **02240b5a.c** - func_227704 gated on unkE0&0x10000; -> func_022413e6(obj,out).
- **02240b8a.c** - func_022417a4; func_022413e6(obj,out) (bsr); 0xE0|=0x400000; rts.

## Named-target dispositions (the task's callee list)

| target        | disposition |
|---------------|-------------|
| 0x02244e2e    | already shipped (02244e2e.c) |
| 0x022469f2    | **SHIPPED this pass** |
| 0x02245d1c    | already shipped (02245d1c.c) |
| 0x02247d6e    | already shipped (02247d6e.c) |
| 0x02244e50 ?  | **NOT a function** - 2-insn `rts; mov.l @r15+,r14`, no prologue (shared epilogue tail). Deferred/skip. |
| 0x0224492c    | already shipped (0224492c.c) |
| 0x02245bcc    | **already shipped, mis-named** as func_02245bbe (see Corrections) |
| 0x02246208    | already shipped (02246208.c) |

## Findings / corrections

- **0x02245bcc is func_02245bbe (naming mismatch to fix).** The census/fn.py entry
  0x245bbe is pool-contaminated (7 pool words: `ff ff / 06 09 ...`); the real
  prologue `mov.l r14,@-r15` is at **0x245bcc** and the already-shipped body in
  02245bbe.c IS this function. But func_02246ab8.c (and the lastgiants symbols)
  declare/call `func_245bcc`, which will NOT link against the exported symbol
  `func_02245bbe`. This needs a rename or alias so the func_02246ab8 call resolves.
  (I did not edit the existing 02245bbe.c - flagging for the caller.)

- **Two prologue variants in the blob.** (A) push-first: `mov.l r14,@-r15; mov
  #100,r0; mov.l pool,r3; mov r4,r14; ...`. (B) scratch-first: `mov #100,r0; mov.l
  pool,r3; mov.l r14,@-r15; ...`. For a (B) function the real entry is TWO
  instructions BEFORE the register push - a prologue-scan for `2fe6` under-points
  it. Entries were bounded by the previous sibling's tail (bra/rts + delay slot).
  Affected shipped entries: 02240492, 02240a4a, 02240a7a, 02240b2e, 02240b5a,
  02240b8a.

- **func_022ab18 takes (obj, M16(obj,0x64)).** Every sibling loads r5 from the
  16-bit field at 0x64 (`mov #100,r0; mov.w @(r0,r14),r5`) before the call -
  consistent with func_02402c0 (lastgiants) and the func_022ab18 extern decl
  `(void *actor, s32 amount)`.

- **No tst #imm8 >= 0x80 anywhere in this set.** The only immediate tst is
  `tst #64` (0x40, func_02240588, bit 0x40 of unkBA) and `tst #7` (func_02240054,
  not shipped); all others are register-form (`tst r3,r2`). The sign-extend mask
  trap does not apply.

- **braf/bsrf all clean, no phantoms.** All 53 computed branches in the blob
  resolve to real functions (func_022ab18, func_227704, func_02210020, func_215f6c,
  actor_motion_init, func_0224158c, func_022417a4, func_022417ea, func_02244df2,
  func_02241556, func_0224174e, and the facing-resolve tails). No 0x0003xxxx
  pool-phantom bsrf TABLE misreads occurred.

- **func_022469f2 tail is func_02241e3c.** braf delta 0xffffb414 at 0x246a24
  resolves to 0x241e3c (actor_enter_statewalk, already shipped), with r4=obj and
  pr/r14 restored first - a real tail call, not a phantom.

- **0x022416ac is not a function.** Pool-contaminated census entry; real entry is
  0x2416c4 = func_022416c4 (actor_facing_clamp), already shipped.

## Deferred (with reason)

- **func_02240054** (standalone, [0x240054,0x2401d6), census 0x24003e). ~190 insn,
  jsr through fn-ptr table g_060918a0, many cross-band calls (0x245c88, 0x22ca54,
  0x23ed94, 0x20ff54, 0x23e4a0, 0x23cbb0, 0x23a924, 0x23edce). Needs its own pass.
- **The 0x240622..0x24124e blob-sibling cluster** (~20 more). This run has
  mid-function shared literal pools and heavy compiler tail-merging: functions bra
  into the middle of their neighbours (targets 0x240748, 0x240d86, 0x240c4a,
  0x240c32, 0x240bf2, 0x240e02, 0x240de2, ...). Each needs individual bounding
  before shipping to avoid silent-wrong splicing - deferred rather than rushed.
- **The larger ambiguous band-240 entries** (0x240622, 0x240bba, 0x241e82,
  0x242272, 0x2425d0, 0x243066, 0x246308, 0x246678, 0x2473ee) are out of scope for
  this callee/sibling pass.

## Summary
- 12 new TUs shipped, all compile `gcc -O1 -Wall -Wextra` with zero output:
  022469f2 + 0224043c, 02240492, 022404ce, 02240588, 02240612, 02240a4a,
  02240a7a, 02240ae0, 02240b2e, 02240b5a, 02240b8a.
- Key correction: **0x02245bcc == func_02245bbe** (pool-contaminated census addr) -
  caller symbol mismatch to resolve.
- Resolved the two "?" targets: 0x02244e50 is a 2-insn shared epilogue (not a fn);
  0x02245bcc is already shipped (mis-named).
- Discovered the scratch-first prologue variant that under-points fn.py entries.
- Names/globals appended to notes/symbols_band240callees.txt. notes/symbols.txt,
  shared headers, tools/, port/ untouched. No git actions taken.
