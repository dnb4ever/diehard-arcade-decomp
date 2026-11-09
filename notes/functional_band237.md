# Functional decomp report - band 0x02237000-0x02240000 (237 pass)

Worker: band237. All files in project/src_functional/, every computed branch
confirmed via braf_resolve.py, each compiled with
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` -> ZERO output.

## Added .c files (10)
| file | summary | fidelity |
|------|---------|----------|
| 022371c0.c | 8-slot HUD meter/popup manager: retire-one, 2 fixed-slot seeds, channel-bit allocation (incl. paired-bit re-home), per-slot ease+draw | HIGH |
| 02237828.c | 2-slot timer/meter updater (small sibling of 022371c0); scale_target + draw | HIGH |
| 02237be4.c | segmented gauge/bar renderer; short bar (<=0xC0) vs long bar (<=0x200) | HIGH |
| 0223c95c.c | stage/effect init: clears effect block, copies 3 config tables + scatter-copies a pose template selected by g_06098668/g_06098670 | HIGH |
| 0223b28a.c | launched-object state machine (arc; gravity 0x360, floor 0x800) | HIGH |
| 0223b5d8.c | launched-object state machine (spin; floor 0x1000; RNG seed + trig) | HIGH |
| 0223b9b8.c | launched-object state machine (floor 0x500, bounce >>1; state1 sig-5 early retire) | HIGH |
| 0223bcd4.c | launched-object state machine (fixed life 0x258, spin/trig) | HIGH |
| 02239fc6.c | "apply action" for a fighter slot (switch on action code unk88 'O'..'S') | HIGH* |
| 0223a3a2.c | "cancel/release action" (counterpart of 02239fc6) | HIGH |

## Key findings / corrections
- func_02237e6c is 1-ARG. Its prologue reads only unk0/unk2 of r4 and tail-jumps
  g_0609029c; m2c's 2/3/4-arg renderings are stale r5-r7. 02237828.c was corrected
  to the 1-arg form after confirming this.
- func_021632e is the target of every `jsr @rN`/bsrf in 022371c0 / 02237828 that
  m2c showed as a phantom pool pointer (mova 0x237224 + 0xfffdf10a = 0x0221632e).
- 02237be4: temp_r8 = func_021326c (0x02237c44 + 0xffdb628). The terminal
  g_0609029c call passes only r4 = &g_0609d7b0[3]; m2c's extra r5/r6 are stale.
- 0223c95c: the 2x/8x-unrolled ROM copy loops were rewritten as flat loops
  (20/80/80 shorts + 20 bytes); the final block is a 27-short+1-byte scatter from
  g_06091704[idx] into the live anim tables.
- The 0x23bxxx object family all share the draw tail g_06098878+=0x30 /
  func_02149aa/0214a40/0214a06/0215a2c / g_06098878-=0x30, and the slot handshake
  signal g_060e0f2c[unk34] (the array set up by 0223c95c). m2c literalised the
  field offset in `(0xCC >> 3)` etc. - these are really `func_0213190(unkCC) >> 3`.
  m2c also mis-rendered the RNG seeds in 0223b5d8 state-0 as `var_r0 = 0x10`; they
  are two independent func_0210020() calls into unkCC/unkCE.
- Naming: 0x23xxxx helpers use the 8-hex-SH2 form the 23hi pass established
  (func_0223cd0c, func_0223c6bc, func_0223e17c, func_0223b254); 0x20/0x21/0x22
  helpers keep their existing cart6 form (func_021632e, func_02173f4, func_02260c8).
- 0223a3a2: m2c's `.Lpool_23a4ac.unk8` spawn is a misread - the real code is
  `func_020feec(g_06099580, func_0228906)->unk8` (func_0228906 = constructor
  callback in r5, computed via mova+add). (*) 02239fc6 NOTE below.

## (*) 02239fc6 fidelity note
Field logic is HIGH-confidence. The one assumption: the shared tail calls
func_0223b254 with arguments that differ by entry path - r5 = unk34 for action
'O'(0x4F) but unk94 otherwise, and r7 = an absolute &g_0604ebe4[act] pointer for
'O'/'P'/'Q' but a plain index (act*4) on the default path. Both are reproduced
exactly as the ROM leaves the registers; func_0223b254's own ABI is not yet
decompiled, so how it interprets r5/r7 is unverified. (The `unk8C |=` operations
that use r7 as a dereference are unaffected - they always resolve g_0604ebe4[act].)
Also: on the dispatch-default path (unk88 not a known code), the 0223a3a2 tail
reads g_0608d3e8[chan] with chan (r13) never set by the ROM; chan is initialised
to 0 here, that path being inert in practice.

## Deferred (with reason)
- 0x0223bf86 (0x6a8): DECODABLE but high-risk, needs a focused pass. 8-state
  object machine driven by 6 ROM spawn-template pools (.Lpool_23c064/104/188/224/
  504/598) feeding a child-spawn mechanism, a per-type table g_060e120c lookup
  keyed by (unk5E-0xA), and a state-7 "hover" path where m2c shows stack-arg
  confusion (`unksp0`) around the func_0213190 trig calls. The pool layouts must
  be read as data and the state-7 stack pinned before it can be shipped with zero
  logical errors. Not safe to guess in this pass.
- 0x0223e690 (0x78a) and 0x0223f10e (0xaf8): jump-table dispatchers. braf_resolve
  shows many `braf`-based tail jumps plus an unresolved `bsrf r0 -> TABLE/COMPUTED
  via mov.b @(r0,r8),r0` index table. These are the m2c-defeating pattern already
  classed as HARD-TAIL/manual-reconstruction in band_claims.md; they need the jump
  tables rebuilt by hand, not a straight-line decomp.
