# Functional decomp report - HARD-TAIL jump-table pass (bandjt2)

Target set: the "computed-jump" functions the band200/band237 passes deferred as
m2c-defeating. Every computed branch was checked with braf_resolve.py; each shipped
file compiles with `gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional`
-> ZERO output. Names/globals in notes/symbols_bandjt2.txt.

## Headline result

The three "camera-mode family" deferrals (0x20ebde / 0x20ea06 / 0x20ef02) are ALL
shipped. None of them is actually a jump table - every `mov.b @(r0,rN); bsrf r0`
flagged by braf_resolve in this family is a **phantom**: braf_resolve disassembles
the function's literal pool, and the pool longs 0x0003xxxx decode as `bsrf r0`
(opcode 0x0003). The real dispatch is a plain `cmp/eq #0..#3` cascade. The three are
siblings of the already-shipped func_0220f28e / func_0220f454.

Two of the three big SH2-0x23xxxx dispatchers (0x23e690, 0x23f10e) have **no computed
jump table at all** - same phantom (the shared mask literal 0x0003ffff reads as
`bsrf r0 / mov.b @(r0,r8)`). They are large flag-gated guarded-tail-call routers whose
every real branch braf_resolve already resolves. This reclassifies them away from
"HARD-TAIL table reconstruction"; see corrections below.

## Shipped (3)

| file | summary | fidelity |
|------|---------|----------|
| 0220ea06.c | camera_mode_service_a: mode dispatch off g_06099584, ease pos /4, look-at via func_0221055c, by=0x14000 | HIGH |
| 0220ef02.c | camera_mode_service_b: world pos + velocity, slew /8, transform func_0220f45c, by+0x15000 | HIGH |
| 0220ebde.c | camera_mode_service_c: self-propelled; vel=arg2>>g_06096C90 (ashr_var), rotate+advance (func_0221080a -> g_06098b1c), transform func_0220e55c | HIGH |

All three share the dispatch:
```
G = g_06099584;  mode = G->unk10 + G->unk14;
 0 -> return
 1 -> src  = (*G->unk30)->unk8 ; kind=1
 2 -> src  = (*G->unk34)->unk8 ; kind=1
 3 -> src  = (*G->unk30)->unk8 ; src2 = (*G->unk34)->unk8 ; kind=2
 default(>3) -> kind=mode  (inert: no valid src; provably unreachable for mode 0..3)
```
Then: first frame (entity->unk68==1) seeds state; steady state eases the tracked
position triple(s) toward the source(s) and rebuilds the look-at angles. kind==2
feeds the midpoint of the two triples (bx=(90+9C)/2, bz=(98+A4)/2) - the SH-2
`addc/shar` round-toward-zero idiom, which is exactly C signed `/2`.

## func_0223b254 ABI (requested) - PINNED

`void func_0223b254(void *entity)` - **ONE argument** (r4). Body (0x23b254..0x23b288):
```
idx  = (s8)entity->unk6D;              // mov #109 ; mov.b @(r0,r4)
rmp  = (s8)g_060e106c[idx];            // mov.l =0x60e106c ; mov.b @(r0,r5)
src  = &g_0608cf3c[rmp*12];            // 12-byte record  (r5 = 0x608cf3c + 12*rmp)
dst  = &g_060996ec[entity->unk34 *12]; // (r3 = 0x60996ec + 12*unk34)
*(u32*)dst = *(u32*)src; dst[1]=src[1]; dst[2]=src[2];   // copy 3 longs, rts in delay
```
It is a leaf 12-byte record copy keyed entirely by entity->unk6D and entity->unk34.
**r5 and r7 passed by callers are dead** (never read).

### -> resolves the 02239fc6 caveat
The band237 note left 02239fc6 "HIGH*" because func_0223b254 received, per entry path,
r5 = unk34-or-unk94 and r7 = an absolute g_0604ebe4 pointer-or-index, with its ABI
"not yet pinned". Now pinned: func_0223b254 ignores r5/r7 entirely, so those
per-path differences are irrelevant - 02239fc6 can be upgraded to HIGH (no remaining
dependency on func_0223b254's interpretation of r5/r7). The `unk8C |=` ops that use
r7 as a g_0604ebe4 dereference were already independent and are unaffected.

## Decoded jump tables

Short answer: **there are no live computed jump tables in this target set.** Every
"TABLE/COMPUTED" braf_resolve emitted for these functions resolves to a literal-pool
long, confirmed by the `mov.l 0xADDR,rN ; =CONST` load that reads that same address:

| function | flagged "table" addr | actually | proof |
|----------|---------------------|----------|-------|
| 0x20ea06 | 0x20eb28 / 0x20eb2c | pool long 0x0003b590 / 0x0003b57a (sdiv32 PIC deltas) | loaded by `mov.l 0x20eb28,r2 ;=0x3b590` |
| 0x20ebde | 0x20ec30.. 0x20ed34.. | pool longs (sdiv/ashr deltas) | loaded by neighbouring mov.l |
| 0x20ef02 | 0x20ef54..0x20ef68 | ebde pool longs 0x0003b2xx | pool island embedded in ef02's range |
| 0x23e690 | 0x23e72c | mask literal 0x0003ffff | `mov.l 0x23e72c,r3 ;=0x3ffff` @0x23e6c6 |
| 0x23f10e | 0x23f3dc | mask literal 0x0003ffff | `mov.l 0x23f3dc,r3 ;=0x3ffff` @0x23f324 |

The one genuine `mov.b @(r0,r8),r4` in e690 (@0x23ec22) loads a byte from the HWRAM
per-type table into **r4 as data**, not as a bsrf index - not a jump table.

The only genuine state-selector tables are plain `cmp/eq` cascades, decoded here:

- **0x23bf86** state (entity=MP(arg0,8); state=entity->unk10):
  `0->0x23c010  1->0x23c090  2->0x23c428  3->0x23c4ce  4->0x23c560  5->0x23c5d2
   6->0x23c5f2  7->0x23c2c0  default->0x23c5f6`
  plus a nested sub-state inside state 0 off g_060e1080 (`0->0x23c09e 1->0x23c04a
  3->0x23c08a default->0x23c5f6`).

## Deferred (3) - with reason

- **0x0223bf86 (0x6a8, 1704B)** - 8-state object/spawn machine. Fully triaged (state
  table above; all 17 bsrf resolved; the "6 ROM spawn-template pools" are PIC literal
  pools, not static data; the per-type params come from HWRAM g_060e120c/0e88/1080/
  1209/0ddc indexed by entity->unk34 and unk5E). DEFERRED: it is a dense 1704-byte,
  8-state machine with a state-7 "hover" path that pushes a stack-arg frame around the
  func_0213190 trig calls (m2c's `unksp0`); shipping it with ZERO logical errors needs
  the dedicated focused pass the band237 note already called for, and guessing it would
  violate correctness-over-completion. Decode above gives the next pass a running start.

- **0x0223e690 (0x78a, 1930B)** and **0x0223f10e (0xaf8, 2808B)** - large guarded-
  tail-call command routers (36 / 46 computed branches, every one resolved by
  braf_resolve). Structure: test entity flag bits, call a predicate (func_0247d6e /
  0245d1c / ...), then `braf`-tail to the selected sub-handler. CORRECTION: these are
  NOT jump-table dispatchers (no computed table exists - see table above). They remain
  deferred only on size: 1930 / 2808 bytes of branchy flag logic is too much to
  transcribe with a zero-error guarantee in this pass. braf_resolve output + the
  phantom-table finding are the hand-off; the next pass can decomp them as ordinary
  (large) functions, not as table reconstructions.

## m2c / prior-pass corrections

1. **No jump tables in the camera family or in e690/f10e.** The band200 note
   ("0x20ebde/ea06/ef02 ... dispatches through a byte/long jump table
   `mov.b @(r0,r9),r0; bsrf r0`") and the band237 note ("0x23e690/0x23f10e ...
   unresolved `bsrf r0 -> mov.b @(r0,r8)` index table") were both misled by
   braf_resolve disassembling literal-pool longs. Every such flag is a phantom
   (0x0003xxxx == `bsrf r0`; 0x0003ffff mask). Verified against the `mov.l`/`mov.w`
   loads that read those exact pool addresses.

2. **func_0223b254 is 1-arg** (not the 4-arg `(void*, s8, void*, s32)` guessed in
   symbols.txt / band237). r5/r7 are dead. -> 02239fc6 upgraded to HIGH.

3. **func_0220f45c / func_0220e55c are real entries** (ef02 / ebde transform targets),
   8-arg lookat-shaped; added to symbols_bandjt2.txt as GUESS (shape confirmed, exact
   semantics not traced).

4. m2c cannot lower any of these (`M2C_ERROR: unknown instruction bsrf $rN`); all PIC
   calls were resolved by braf_resolve and the callees' pinned ABIs (sdiv32 r1/r0,
   ashl_var/ashr_var r0/r1, func_0221055c 8-arg, func_0221080a mtx_push_rotate_apply).
