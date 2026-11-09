# Functional decomp batch 2 (Opus) — braf-resolver + unlocked FSM family

Follow-on to batch 1. Goal: build the `braf`/`bsrf` PIC tail-branch resolver the prior
batch recommended, use it to crack the actor-FSM handlers m2c couldn't lower, and adopt
the deferred large copy routines. All new TUs use the `dh.h` accessor style and compile
clean on host gcc (`gcc -c -fsyntax-only -Wall -Wextra`, zero warnings).

## 1. The resolver — tools/decomp/braf_resolve.py (NEW; read-only tools untouched)

SH-2 PIC forms a computed branch target as **`target = braf_addr + 4 + Rn`**, where Rn is a
signed 32-bit DELTA loaded from the literal pool (`mov.l @(disp,pc),Rn ; ... ; braf Rn`).
The tool disassembles a function (via the existing `fn.py`), finds each `braf`/`bsrf`,
back-traces the branch register (following `mov rA,rB` copies) to its pool `.long`, and
computes the fixed target in cart-offset space (mod 2^32; add 0x02000000 for the SH2 addr).
When the register instead comes from a runtime table load (`mov.b @(r0,Rn)` — the 32-way
`dispatch_table_*` pattern) it is reported as table-driven, not a single target.

Usage: `tools/decomp/venv/bin/python tools/decomp/braf_resolve.py 0x21db6e [0x22077e ...]`

**Validation** (targets land on clean function prologues; one is a named symbol):
- `isr_timer0` 0x223dca  braf delta 0xffff212e -> **0x215f36 = sound_queue_service** (named!)
- 0x22077e  braf delta 0xffff4be4 -> 0x2153f6  (clean prologue)
- 0x21db6e  braf delta 0x286aa    -> 0x246282  (clean prologue)
- 0x246282  bsrf delta 0xfffce714 -> 0x2429aa  (confirms bsrf call handling)

**Scope of the unlock (scan over all 1272 functions):** 506 functions contain a
resolvable braf/bsrf. This is effectively the engine's whole PIC call graph — e.g.
`main_init_entry` (0x201020) resolves its ~30-call boot sequence to named services
(smpc_*, vdp1_*, sound_init, task_list_run, ...), and the actor-handler population tail-
branches to a small set of shared continuations: **0x22dc2c (animation_update), 0x2153f6,
0x2154fc, 0x22eea2 (drawlist_prepend), 0x20fdc8**. The resolver turns every one of those
`M2C_ERROR(braf)` dead-ends into a real call/goto.

## 2. Functions added this batch (13)

### braf-tail actor-FSM handlers (10)
| cart off | file | behavior / resolved tail |
|----------|------|--------------------------|
| 0x223dca | 02223dca.c | **isr_timer0** - page cycle + VDP2 toggle; braf -> sound_queue_service(0x215f36) |
| 0x21db6e | 0221db6e.c | func_0221db6e - 3-state spawn/link handler; braf -> func_02246282(entity) |
| 0x22077e | 0222077e.c | func_0222077e - projectile init + gravity/spin; braf -> func_02153f6(entity,0x8000,1) |
| 0x231d72 | 02231d72.c | func_02231d72 - lifetime-countdown mover; braf -> func_02154fc(entity,0x20000,0x10000,1) |
| 0x2324da | 022324da.c | func_022324da - follow-source-while-flag; braf -> func_022eea2(entity) |
| 0x23ef5a | 0223ef5a.c | func_0223ef5a - 12-frame scripted action; braf -> func_02153f6(entity,(f<<16)|0x10000,1) |
| 0x23efa0 | 0223efa0.c | func_0223efa0 - 8-frame table-pose action; braf -> func_02153f6(entity,0x80000,1) |
| 0x23efec | 0223efec.c | func_0223efec - 8-frame scripted action; braf -> func_02153f6(entity,+0x70+0x20000,1) |
| 0x2396e8 | 022396e8.c | func_022396e8 - directional projectile launch + arc; braf -> func_02153f6(entity,+0x40,1) |
| 0x2397c2 | 022397c2.c | func_022397c2 - sibling of 2396e8 (bigger kick); braf -> func_02153f6(entity,+0x40,1) |

### continuation leaf + copy routines (3)
| cart off | file | behavior |
|----------|------|----------|
| 0x22eea2 | 0222eea2.c | **drawlist_prepend** - push entity onto 32-entry 8-byte-node list (the braf target of several handlers above) |
| 0x214e2e | 0214e2e.c  | **entity_radix_sort** - 3-pass LSD radix sort of 0x24-byte entity records by the 24-bit key@+0x20 into 0x100-way linked-list buckets |
| 0x2227c4 | 022227c4.c | **displist_priority_fill** (was deferred) - OR a 7-bit priority into the top byte of 0xF0 display-list words; 3 modes |

Every file was built from the m2c draft, then **cross-checked field-by-field against the raw
SH-2 asm** (`fn.py` listing) for widths (M8/M16/M32) and signedness, with the braf/bsrf tail
rendered as a real call to the resolver-computed target. Notable faithful simplifications,
each verified equivalent against the asm:
- `displist_priority_fill`: the ROM unrolls modes 0/1 by 3 and mode 2 by 4, and mode 2 keeps
  two cursors that provably advance in lockstep (always equal) -> folded to one word loop.
- `entity_radix_sort`: m2c mis-scales the `void**` bucket stride; byte strides (node 0xC,
  record 0x24, bucket 0xC) were taken from the asm. Pass-2 keeps the ROM's literal uncached-
  mirror addressing (0x26xxxxxx = 0x06xxxxxx | 0x20000000).

## 3. Real counts (project/progress_functional.py — NEW, separate from progress.py)

```
functions total (fn.units) : 1272
  functional  (src_functional):  206    16.19%   (was 193 .c files -> +13 this batch)
  nonmatching (src_nonmatching):   73     5.74%
  byte-exact  (src files)      :    5     0.39%
  byte-exact  (oracle EXACT)   :    3     0.24%
  ANY C written (deduped)      :  284    22.33%
```
`progress.py` (the byte-exact oracle, 3 funcs) is unchanged and still works.

## 4. Resisted / deferred (honest)
- **0x23c95c** (round/scene init, ~0x300B, 80+ m2c temporaries): a long sequence of heavily-
  unrolled table copies ending in a block that scatters ~26 sequential shorts from a level-
  descriptor record (g_06091704[level]) into dozens of individually-addressed absolute fields
  (g_060E1080/10D0/10F8/1208/120C/106C ...). Because this is functional (non-byte-matching,
  so NOT verifiable by the permuter) and the field-by-field scatter is highly error-prone to
  hand-transcribe, I deferred it rather than ship an unverifiable transcription. Needs a
  dedicated pass walking the asm offset-by-offset (and ideally confirming the destination
  struct layout first).
- **0x20e970**: listed as braf-family by batch 1 but it has **no braf** - its fn.py extent has
  a garbled head (leading pool bytes) and the extent runs into what looks like the next
  function; misseg-suspect, left for the asm band.
- **0x231a06** and the large `braf->0x22dc2c`/`0x22adaa` handler family (0x228e60, 0x229688,
  0x22979c, 0x229854, ...): tractable now that the tail is resolved, but their bodies ALSO
  contain `bsrf` *calls* (to animation_update 0x22dc2c, 0x228106, 0x23a6dc, ...) that m2c_fn.py
  likewise can't lower, so each needs the asm walked for the call args. Good next target.

## 5. Next-band recommendation
1. **The `braf->0x22dc2c` (animation_update) handler family** - dozens of small (0x30-0x120B)
   enemy/object state handlers in 0x228xxx-0x229xxx. The resolver already gives every tail +
   every bsrf call target; a thin wrapper that rewrites `bsrf/braf Rn` in m2c_fn.py's asm
   output into resolved `jsr func_<t>` / tail-call (a NEW wrapper, leaving m2c_fn.py read-only)
   would let m2c lower their whole bodies and unlock the family in bulk.
2. **func_02153f6 / func_02154fc** - the two most-targeted motion+animate continuations; naming
   and decompiling them would retroactively sharpen ~20 handlers that tail into them.
3. **0x23c95c** with the careful offset-by-offset pass described above.
Still avoid the misseg band and the 0x24a3xx-0x24a5xx jump-table fragment cluster.
