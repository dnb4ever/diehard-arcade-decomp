# Functional decomp batch 3 (Opus) — bulk braf/bsrf rewriter + actor state-step family

Follow-on to batch 2. Goal (batch-2's #1 recommendation): build a thin wrapper that
rewrites m2c_fn.py's un-lowerable PIC computed branches into forms m2c CAN lower, then
use it to crack the `braf->0x22dc2c (animation_update)` actor-handler family in bulk.
All new TUs use the `dh.h` accessor style and compile clean on host gcc 15.2
(`gcc -c -fsyntax-only -Wall -Wextra`, **zero warnings**). Core read-only tools
(m2c_fn.py / fn.py / match.py) untouched.

## 1. The wrapper — tools/decomp/braf_rewrite.py (NEW)

Takes the GNU-as asm `m2c_fn.to_asm()` emits, runs `braf_resolve.resolve()` on the same
function, and substitutes each resolved computed branch with a direct `bsr func_<SH2>`
to the resolved target (SH2 = cart + 0x02000000). Also added `scan_family.py` (a NEW
helper) that classifies all 1272 units by resolvable-vs-table branches.

The transform **mirrors m2c_fn.py's proven `jmp @Rn` tail-call handling**: hoist the
branch's delay-slot instruction (it often sets the call's argument, e.g. `mov r13,r4`),
then emit the direct call:
  * `bsrf Rn` (computed CALL, mid-body): `<slot>; bsr target; nop` — control continues.
  * `braf Rn` (computed TAIL-JUMP): `<slot>; bsr target; nop; rts; nop`.
The dead pool DELTA load that used to feed Rn becomes an unused register write and is
dropped by m2c's DCE (verified — no dead temporaries in output).

### Critical correctness fix (the trap batch 2 flagged)
A first cut that just renamed `braf Rn -> bsr target` (leaving the delay slot in place)
is WRONG: a `braf` tail does **not** always sit at the function's physical end. In
`0x229644`/`0x229688`/`0x229714`/... the tail-jump lives inside a conditional block, so a
bare `bsr` falls through into the else-branch. The explicit `rts; nop` after the tail call
is REQUIRED and makes it a true tail call that returns (r0 survives `rts`, so value-
returning tails are preserved). This was caught during validation and fixed before shipping
any C — the fixed wrapper's output has no fall-through and no `unksp0` register-restore noise.

Usage (in the file header):
```
braf_rewrite.py 0xADDR [name] --asm      # print rewritten asm
braf_rewrite.py 0xADDR [name]            # rewrite + run m2c (sh2-gcc-c)
braf_rewrite.py 0xADDR --report          # list resolved / table / unresolved
```

### Validation against batch-2 hand-done versions
- **0x21db6e**: wrapper's m2c draft lowers the braf to `func_02246282(entity,...)` tail +
  the 3-state switch exactly as the hand file `0221db6e.c` (field widths, states, globals match).
- **0x22077e**: draft is field-for-field and arg-for-arg identical to hand file `0222077e.c`:
  pos copy from spawn source, `(g_060987BC & 0xFF) << 8`, gravity/spin, tail
  `func_022153f6(entity, 0x8000, 1)`.
  - NOTE: the hand file `0222077e.c` names that tail `func_02153f6` (the raw cart offset);
    the correct SH2 name is `func_022153f6` (cart 0x2153f6 + 0x2000000). Cosmetic naming bug
    in the batch-2 file; behaviour is identical. Left that file untouched (surgical); flagged here.
- **0x223dca (isr_timer0)**: body matches (page cycle, VDP2 toggle, tail sound_queue_service);
  m2c adds a spurious `goto` loop because it's an ISR (rte, no rts) — moot, already done in batch 2.

## 2. Scope of the unlock

`scan_family.py` over all 1272 units: **420 functions have ONLY resolvable braf/bsrf**
(fully wrapper-tractable), **382 of them not yet done**. Of those, **41 tail/call into
animation_update (0x22dc2c)** and were not done — the exact family batch 2 named.

## 3. Functions added this batch (20) — all actor state-step handlers / continuation

entity = r5 (ctx a0 = r4, passed through to animation_update), unless noted "task form".
Each runs/branches around `animation_update(a0,entity)` and a shared continuation.

| cart off | file | behaviour / resolved tails & calls |
|----------|------|-------------------------------------|
| 0x228508 | 02228508.c | **task form** (entity=task->unk8): func_02228536 + animation_update + func_0222ca98, advance task->unk0 -> func_0222863e |
| 0x2289b8 | 022289b8.c | on animation_update trigger, arm launch arc (unkC4/C8/78; anim 0x143/0x145 by facing) |
| 0x228cd6 | 02228cd6.c | home paired obj (g_060995B0[unk4B]->unk8) on X; snap if unk54&2 else ease halfway |
| 0x228e60 | 02228e60.c | home paired obj on X+Z; on contact spawn hit obj (func_0220ff72) & link; else ease + tail anim |
| 0x22881a | 0222881a.c | if unk54&1 set state 6; func_0222c220; tail animation_update |
| 0x228a0e | 02228a0e.c | gravity/landing arc (unkC4/C8/7C); on land reset slot g_06096C74[unk34] + landing anim |
| 0x228b38 | 02228b38.c | entry: func_022170ea + set 0x43/84/88, tail-**bra** into func_02228b68 |
| 0x228b68 | 02228b68.c | body (tail target of 228b38): unk48&unkBA0x40 -> bd54+anim+state9; else bdf2 + store frame to g_06099780[unk34-1] |
| 0x228d9c | 02228d9c.c | unk48 -> flip facing (unk26+=0x8000, unk44^=1) + func_0222adaa; else load pose + func_02227704; tail anim |
| 0x228fe2 | 02228fe2.c | unk48 -> func_0222adaa; else func_02227704 + set 84/88 + pose; tail anim |
| 0x2288e8 | 022288e8.c | func_0222c220 gate; nested unk54/unk56/unk8C(0x02000000) -> func_0223a6dc or pose; tail anim |
| 0x229644 | 02229644.c | anim; unk48 -> a394 + tail adaa; else on unk58==unk90 set state=f_239fc6+0x21 |
| 0x229688 | 02229688.c | anim; unk48 -> tail adaa; else on unk54&2 set 84/88/52/pose/state1 |
| 0x229714 | 02229714.c | **identical body to 229644** (distinct ROM copy; fn.py missegs its start as 0x2296da) |
| 0x229758 | 02229758.c | **identical body to 229644** (distinct ROM copy) |
| 0x22979c | 0222979c.c | anim; unk48 -> tail adaa; else on unk58==unk90 func_0223a3a2 + state 0x22 |
| 0x2297d8 | 022297d8.c | anim; unk48 -> tail adaa; else on unk58==unk90 state=f_223b01e+0x21 |
| 0x229816 | 02229816.c | anim; unk48 -> tail adaa; else on unk58==unk90 state=f_223b1c0+0x21 |
| 0x229854 | 02229854.c | anim; unk48 -> tail adaa; else on unk58>=unk90 (signed) tail func_0223ac9c |
| 0x22adaa | 0222adaa.c | **actor_pose_dispatch** (the braf->0x22adaa continuation): double fn-ptr-table dispatch (pose g_060911BC[g_0609979A], per-type g_060911C4[unkBA&0x3F]) + staleness timer on unk110/unk10C vs frame counter g_060987BC |

Every file cross-checked field-by-field against the raw SH-2 asm (`fn.py` listing) for
widths (M8/M16/M32) and signedness (cmp/pl/ge signed, extu.w/cmp/hi unsigned). Notable
verified details: signed `>>1` (shar) easing; unsigned `>0x1E` staleness (cmp/hi); the
`< 0xFFFF4000` land test is signed (cast `(s32)0xFFFF4000`); `(u16)unk58 == unk90` /
`(s32)(u16)unk58 >= unk90` width+sign mixing.

### Key structural findings
- **animation_update (0x22dc2c) confirmed 2-arg**: `animation_update(ctx_r4, entity_r5)`,
  returns a trigger-frame flag in r0 (used by 0x2289b8 `if (animation_update(...))`). Bumped
  to HIGH in symbols.txt.
- **Missegmentation**: fn.py's extent grabs the preceding literal pool for two starts —
  `0x2296da` is really `0x229714`, and `0x228d76` is really `0x228d9c`. Decompiled at the
  real prologues. (Consistent with batch 2's misseg note on 0x20e970.)
- **Tail-bra chaining**: `0x228b38` ends with a direct `bra 0x228b68` (tail jump into the
  adjacent function). m2c inlines it; I decompiled both as separate TUs with 228b38 tail-calling
  228b68 — faithful to the ROM's function boundaries.
- `0x229644 == 0x229714 == 0x229758`: three byte-behaviour-identical sibling handlers (same
  resolved targets), reached by different state-table slots.

## 4. Real counts (project/progress_functional.py)

```
functions total (fn.units) : 1272
  functional  (src_functional):  226    17.77%   (was 206 -> +20 this batch)
  nonmatching (src_nonmatching):   73     5.74%
  byte-exact  (src files)      :    5     0.39%
  byte-exact  (oracle EXACT)   :    3     0.24%
  ANY C written (deduped)      :  304    23.90%   (was 284 -> +20)
```

## 5. Resisted / not-taken (honest)
- **Bulk-handled vs available**: the wrapper makes all 20 trivial — each was a clean
  prologue with only resolvable braf/bsrf. Of the 41 anim-family not-done, I took the small/
  clean micro-handlers (0x2e–0x12a bytes) + the shared continuation. ~21 LARGER family members
  remain tractable but deferred for quality: 0x228168 (0x3a0), 0x229488 (0x1b8), 0x229988/
  0x22988e (0x2a2), 0x229bb0 (0x1e6), 0x229d9e (0x34e), 0x22a2e8/0x22a3e2/0x22a970/0x22a9fe,
  and the 0x241e82/0x242272/0x243066/0x2473ee/0x2482dc/0x248436 range — each has many more
  interleaved bsrf calls (to func_0222c220/0223a6dc/02227704/… and the 0x245xxx helpers) whose
  per-call field args need the same careful asm walk. Good next sitting; the wrapper already
  lowers them all.
- **Table-driven dispatch left alone**: the resolver flags multi-target `mov.b @(r0,Rn)` jump
  tables (dispatch_table_*) as TABLE/COMPUTED; the wrapper does NOT fake a single target for
  them (they stay as m2c M2C_ERROR / switch stubs). None were in this batch's set.
- Did NOT touch the misseg band (beyond the two re-pointed starts above) or the 0x24a3xx
  jump-table fragment cluster.
- **Minor**: hardened `braf_resolve.py` CLI to skip out-of-ROM addresses gracefully
  (was IndexError) — guards `fn.CODE_START <= start < len(fn.ROM)`.

## 6. Next-band recommendation
1. **The larger anim-family members** (list in §5): all wrapper-tractable now; they just need
   the per-bsrf arg transcription. A half-day sitting should add ~20 more.
2. **Decompile the shared callees** the family tails into — `func_0222c220`, `func_02227704`,
   `func_0223a6dc`, `func_02239fc6`/`func_0223a3xx`/`func_0223b0xx` (the `state = f(entity)+0x21`
   generators), and `func_0222aadc`. Naming these retroactively sharpens ~40 handlers.
3. **The 382 fully-tractable (non-anim) functions** `scan_family.py` lists — the wrapper now
   lowers every one; the boot/VDP/sound call graphs (e.g. the 0x206xxx–0x208xxx cluster) are a
   broad, low-risk runway.
Still avoid the misseg band and the 0x24a3xx–0x24a5xx jump-table fragment cluster.
