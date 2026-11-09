# Functional decomp report - bandbig23 pass

Targets: the four "large flag-gated router/state" functions the prior passes
(band237 / bandjt2) decoded but deferred. Every computed branch confirmed with
braf_resolve.py. Each SHIPPED file compiles with
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` -> ZERO output.
Names/globals in notes/symbols_bandbig23.txt (NOT symbols.txt).

## Shipped (2)

| file | summary | fidelity |
|------|---------|----------|
| 02243fe0.c | tripled per-actor collision/path sweep (sibling of 022444b6) | HIGH |
| 0223bf86.c | 8-state summon/projectile object machine | HIGH |

### 0x02243fe0 - path_sweep_tripled
Real entry. **fn.py reports size 0x4d6 = 1238B, not 619B as the brief stated.**
Three sweep variants selected by `flags = ent->unkE0`:
- `flags & 0x40`      -> PATH D: target = (*g_060E0F4C[unk12C])->unk8; single sweep
  (count<5) over g_0608DBC0 seq; writes tbl10 step-words + target ptrs; then
  `func_02244670(ent,tgt,tbl10,count,(s16)-angle)` -> `return 2` on 0.
- `!0x40 && 0x10`     -> PATH B: target = (*g_060995B0[unk128])->unk8, with a
  `/2+/4` clamp when `!(flags & 0x80)` and |diff|>0x2CCCC; publishes step to
  g_0609740C; same sweep+func_02244670 as D.
- `!0x40 && !0x10`    -> PATH C: `func_02244874` reachability gate. Reachable ->
  short sweep (count<3) over g_0608DBD4, status g_0608DBF4==2 flips selector.
  Blocked -> sweep (count<5) over g_0608DBC0, then switch g_0608DBDC {0:
  `func_02244820`, 2: flip} writes selector. (No func_02244670, no early ret 2.)
- Shared tail (block_42/52): `unk44`/`flags&0x20000`/`unk8 vs tgt->unk8` may
  `return 1` or `step += 8`; then direction rehome from `*(*ent->unk158)[(step>>1)&7]`
  with a `func_02210020()&7` RANDOM re-pick when the opposite dir == last-used
  (`ent->unk100`).

m2c corrections (in file header too):
1. `tst #128` is **`flags & 0x80`** (bit 7), m2c rendered it `& ~0x7F` (wrong).
2. the final bsrf (0x24447e) is **`func_02210020()` (RNG)**; the re-pick is
   `var_r4 = func_02210020() & 7`. m2c dropped the call and reused the stale
   compare value `(var_r4+4)&7` -> a genuine logic bug in m2c's listing.
3. `func_02244820` is **2-arg** (ent,tgt)->s16, not 4-arg.

### 0x0223bf86 - obj_state_machine_8
Real entry. arg0=task, ent=arg0->unk8, other=ent->unkD0. Dispatch on
`M8(arg0,0x10)` is a plain cmp/eq cascade 0..7 (confirmed: NO jump table).
All 17 bsrf/jsr resolve to ordinary calls.

- The six "ROM spawn-template pools" the prior passes feared are **PIC
  descriptor pointers**: `func_020ff0a/func_020ff28(task, desc)` returns a child
  entity; m2c's `.Lpool_23cNNN.unk8` is really `child->unk8`. Descriptors
  (mova+disp): 0x0223C650 (data template; states 0.sub1 / 3 / 4.sub1),
  0x0212350 (callback; state1 spawn-and-die), 0x022319B2 (callback; state1
  launch helper).
- The **state-1 launch trig** (the deferral blocker) is resolved: the
  `mov.l r5,@-r15 ... jsr @r9 ... mov.l @r15+,r5` around `func_0213190` is a
  save/restore of r5 (=(s16)other->unk64<<9) across the call. The real code is
  two distinct calls: `unk14 = -func_02131c8(func_0213190(other->unk62), k)` and
  `unk1C = func_02131c8(func_02131a8(other->unk62), k)` with k = other->unk64<<9
  (other->unk64 is zeroed just before, so k==0). m2c fused them into one phantom
  `temp_r9(... unksp0)` call.
- **state 7** writes to unkC / unk2A / unk2C each follow a `func_02210020()`
  (RNG) bsrf that m2c dropped (it reused stale regs). Corrected.
- Draw tail is the shared 0x23bxxx idiom: `g_06098878 += 0x30; func_02149aa();
  func_0214a40(ent); func_0214a06(ent); func_0215a2c(ent,0); g_06098878 -= 0x30;`
  (matched to shipped siblings 0223b28a/0223b5d8).
- `func_023cd0c`'s 2nd arg varies by path (child ptr in state1-die, int 2 in
  states 3/4) - both reproduced exactly as the ROM leaves r5.

## Deferred (2) - with reason + resolved structure

Both are large guarded-tail-call command routers. The prior pass deferred them
"on size"; this pass confirms them decodable but **not safe to transcribe with a
zero-logical-error guarantee in one pass**: m2c gives no usable scaffold
(e690 fails outright; f10e is all M2C_ERROR), so they are 100% hand-transcription
of ~1930 / ~2808 bytes of flag logic mixed with heavy *inline* construction
(spawn/RNG/sfx clusters), not pure dispatch. Per "silent-wrong > honest
deferral", deferred. Every branch IS resolved (below) - ready for a focused pass.

Confirmed phantoms (reclassify the old "index table"): e690 @0x23e72c and
f10e @0x23f3dc are the mask literal **0x0003FFFF** (`tst r3,r2` with r3=0x3FFFF),
NOT a bsrf jump table. The real `mov.b @(r0,r8)` data loads elsewhere are table
reads into data regs, not dispatch.

### 0x0223e690 (0x78a = 1930B) - 36 computed branches
Shape: `entA=ent->unk158; r13=entA->unk4; f=ent->unkE0`, then a chain of
guarded tail-calls (epilogue-then-braf) and predicates:
```
if (f&0x100) { if (func_0247d6e(ent)) goto 0x23e86e; else TAIL func_022469f2(ent); }
else if ((ent->unk8C & 0x3FFFF)==0 && (f&0x40)) {
    r5=func_0245d1c(ent); if(r5) TAIL func_0249836(ent,r5); } ...
```
Resolved targets (addr bsrf/braf -> SH2):
247d6e(call) 2469f2(tail) 245d1c(call) 249836(tail) 244e2e(call) 2470a2(call)
247036(tail) 2470a2(tail) **243fe0(call @0x23e7ae = my shipped handler)**
24840a(call) 249fb0(tail) 2473ee(tail) 245dcc(call) 240054(call) 22d450(call)
22eee4(tail) 246208(call) 249a82(call) 2489d0(call) 24840a(call) 20ff0a(call)
2462bc(tail) 2170b4(call) 227c22(call) 210020(RNG) 20ff28(call) 215f6c(call x3)
210020(RNG) 20ff28(call x2) 2153f6(tail) 215f6c(call x2).
Branch @0x23e72c "TABLE/COMPUTED" = phantom (mask 0x0003FFFF). m2c unusable
("Failed to parse: stc tbr,r1" - a mis-disassembled pool datum).

### 0x0223f10e (0xaf8 = 2808B) - 46 computed branches
Same family, larger. Structure: triple-replicated guard blocks
(func_0245dcc + func_023ffcc/func_02401d6 + func_022ca98 + TAIL func_022eee4),
then the e690-style predicate chain, then heavy inline construction
(func_024a360 reg-count helper, func_0228106, func_020feec, trig/sfx, RNG).
Branch @0x23f3dc = phantom mask 0x0003FFFF. Also **tail-calls func_02243fe0
@0x23f3b8** and uses the reg-count-shift helper **func_0224a360** (the documented
trap) at 0x23f530 / 0x23f9be. m2c emits M2C_ERROR for every bsrf.
Full resolved target list is the braf_resolve.py output for 0x23f10e (46 rows).

## Cross-check bonus
Both deferred routers tail-call the newly-shipped **func_02243fe0**, confirming
it is a genuine command handler (an AI/path sub-step) in this family.
