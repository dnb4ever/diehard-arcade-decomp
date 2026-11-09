# Functional decomp - the two deferred braf jump-table dispatchers

Follow-up to `functional_852c.md`, which decoded both tables but deferred shipping
them (each table target is a dense block). This wave hand-decompiles both runtime
`braf` dispatchers and a third genuine entry found inside the 0x2492f8 byte span.

m2c / decomp_bounded collapse a runtime `braf Rn` table into the fall-through case
(+ `M2C_ERROR`), so both were hand-traced statement-by-statement against the
address-annotated `fn.py` listing, with every computed branch run through
`braf_resolve.py` (trusting resolved TARGET addresses) and each jump table decoded
DIRECTLY from ROM bytes. All three `.c` compile at zero warnings
(`gcc -c -fsyntax-only -Wall -Wextra`).

Shipped (project/src_functional/, function named `f_<cart>`):
- `02248bb8.c`  f_248bb8  actor_command_dispatch
- `022492f8.c`  f_2492f8  actor_combo_machine
- `02249836.c`  f_249836  actor_combo_begin

## 0x02248bb8 - actor_command_dispatch (double command table, ~0x3d0 bytes)
ABI (r4=task, r5=entity). r11=animation_update (ptr_resolve 0x248bd8+0xfffe5054=
0x0222dc2c), r9=func_02244988 (mova 0x248c44+0xffffbd44=0x02244988), r10=1, r12=0.
Top switch on entity[0xd8]: state0 (anim; if engaged entity[0x64]=0/entity[0xd8]=1),
state1 (dispatch), else return. State1 returns if entity[0x4c]!=0, else splits on
entity[0x56]: `<=0` is a cmp-chain on entity[0x88], `>0` is the runtime braf table.

Runtime braf table @ **0x248db8** (35 x s16), braf PC **0x248db2**,
`target = 0x248db6 + (s16)tbl[cmd-0x28]`. **Decoded DIRECTLY from ROM** (cmd=idx+0x28):

```
0x28           -> 0x248e9e      0x30           -> 0x248f34
0x3b,0x3c,0x3f -> 0x248dfe      0x3d,0x40,0x41 -> 0x248e30
0x42           -> 0x248ed2      0x43           -> 0x248eea
0x44           -> 0x248e4e      0x49           -> 0x248e66
0x4a           -> 0x248e7e      all other in-range cmd -> 0x248f72 (bare return)
```

**CORRECTION to functional_852c.md:** that note's cmd->target map had the command
indices SHIFTED (it listed e.g. `0x33,0x34,0x37 -> 0x248dfe`, `0x35,0x38,0x39 ->
0x248e30`, `0x3a->0x248ed2`, `0x3b->0x248eea`, `0x3c->0x248e4e`). The TARGET set is
right but the cmd assignment is wrong. The raw-ROM decode above (verified with the
braf formula and cross-checked against the block addresses in the listing) is
authoritative and is what the shipped `.c` uses.

cmp-chain path (entity[0x56] <= 0), on entity[0x88]: 0x3b/0x3c -> combo block
0x248c84 (falls through into the 0x248cca state-3 setup, shared with 0x4a), 0x44 ->
0x248cea, 0x49 -> 0x248d04, 0x42 -> 0x248d64, 0x43 -> 0x248d78, 0x3d/default ->
0x248d18 (tail func_02249cda). **Second correction:** the 852c note said 0x3d/default
tails to `func_0223ffcc`; braf_resolve + the pool (0x248d60=0xfa8, 0x248d32+0xfa8)
prove the 0x248d2e tail is **func_02249cda** (actor_enter_despawn). decomp_bounded's
braf-rewrite mislabeled it.

Combo sub-tables (block 0x248c84): 0x060540e8 (s8[] by entity[0x5e]), 0x06054080
(8-byte records), 0x06053f04 (s16[]), 0x060e0ddc (enable flag). The first bsrf
(0x248cb2 -> func_02215f6c) has a non-nop delay slot (`or r3,r4`) and was resolved by
hand. In-range bsrf: 248cb2->02215f6c, 248d10/248e72->0223ffcc, 248e18->02247d6e,
248e28->022469f2, 248eb8/248f54->02210020, 248f36->02227704; tails 248d2e/248ed2/
248eea bsr/braf->02249cda, bra->02249240.

## 0x022492f8 - actor_combo_machine (14-state machine, ~0x5a0 bytes incl. helper)
ABI (r4=task, r5=entity). Prologue: r12=animation_update (mova 0x249320+0xfffe490c=
0x0222dc2c), r6=0xffffdfff(~0x2000), r7=8, r8=7, r9=1, r10=3, r11=0. Two working
latches: `ec = entity[0xec]` (-> entity[0x14] in states 2/9/10), `engaged2 =
entity[0x54]&2` (read by states 11/12/13). Dispatch: if `(u32)entity[0xd8] >= 14`
return; else runtime braf table @ **0x249354** (14 x s16, braf PC **0x249344**,
`target = 0x249348 + (s16)tbl[state]`), decoded & verified (matches the 852c note):

```
 0 -> 0x249370   1 -> 0x249404   2 -> 0x24941e   3 -> 0x2494a4
 4 -> 0x2494da   5 -> 0x249590   6 -> 0x2495e0   7 -> 0x249600
 8 -> 0x24965a   9 -> 0x2496d8  10 -> 0x24972a  11 -> 0x249788
12 -> 0x24979c  13 -> 0x2497ec
```

Shared epilogues: **.L24981c** issues animation_update then returns; **.L249822** is
the bare return (anim already issued on that path) - note this is the OPPOSITE of the
852c note's labelling; traced directly from the listing. States 2 & 10 share the wrap
tail **.L249742** (entity[0x18]=entity[0xf0]; anim; ret). State 0 reuses the combo
sub-tables (same as 0x248bb8). State 4 does a secondary cmp-chain on func_022454e4()'s
return (0/1/3/4 -> 0x249514, 2 -> 0x24953e, 5 -> 0x24954a, 6 -> 0x249566, else ret).
State 9 branches on animation_update's return value (hence it is declared `s32` in
this TU). Tails to siblings: 0x249240 actor_enter_state4 (states 3>,8>), 0x249cda
actor_enter_despawn (states 3<=,8<=), 0x2499a6 actor_enter_state14 (state 7).
In-range bsrf all resolved (2493ba->02215f6c, 2493e8->022458ec, 2494f0->022454e4,
249592/2495e2/249602->022449b2, 249640->02245eb8, 249636/249652/24978e/2493f6->
022469f2, 2497a8->02245e58, 2497ae->0223d0c6, 249804->0223d0fe, 24980a->02245c88,
249818->02241d6e).

## 0x02249836 - actor_combo_begin (init helper, ~0x62 bytes)
A SEPARATE genuine entry (find_entry confirms; own prologue/epilogue, ABI r4=entity,
r5=arg1) that lives inside the 0x2492f8..0x249898 span the 852c note treated as one
function. It is NOT reachable from the dispatcher (no fall-through past the
dispatcher's rts at 0x249834, no in-function pointer to it) - the deferrer lumped it
in because its two bsrf (249854->02210020, 249886->02239ce8) sit in that address
range. Decompiled here so no genuine code is left undone in the span. Behaviour:
entity[0xd4]=13; entity[0xe0]&=0x1300; if (func_02210020()&1 && entity[0x38]==30)
-> state0 (entity[0x41]=1, entity[0x5c]=0x162) else state1 + entity[0x108]=5 +
func_02239ce8(entity, arg1, entity[0x12c]); entity[0x84]=1. func_02239ce8 is the same
3-arg callee the 0x249898 grab-lock handler uses (func_02239ce8(entity,&task,index)),
so arg1 is an opaque pointer and entity[0x12c] the index.

## Resisted / nothing deferred
Both target dispatchers + the in-span helper are shipped at HIGH fidelity, zero
warnings. No code remains undone in `[0x2492f8, 0x249898)` or in
`[0x248bb8, 0x248f88)`.
