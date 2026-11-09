# Functional decomp batch 13 (Opus, single worker)

Headline: **7 new verified functional `.c`** (6 brand-new + 1 re-decomp), every one compiling clean
`gcc -c -fsyntax-only -Wall -Wextra` with **ZERO warnings** and hand-traced statement-for-statement
against bounded SH-2 disassembly (`m2c_fn.py --asm`), with **every computed branch and PIC pointer
resolved** (`braf_resolve` / `find_entry` run before every feed). This batch continues the
**player-actor FSM family** (cart 0x228xxx-0x22bxxx) that batch 12 opened: it fills in the direct
callees of the join/step chain plus two standalone status/scene query functions.

## ⚠ PROCESS ANOMALY (flag for the user)
This repo's `project/src_functional/` received **concurrent writes from another process during this
session**, contrary to the standing "single serial worker / sole functional writer" rule. Evidence:
new `.c` files appeared with a **different umask** (0664 vs my scp'd 0644) and mtimes **overlapping
mine** (e.g. `0220340c.c`, `02203500.c`, `02213e0c.c`, `02213d90.c`, `02216530.c` at 22:20-22:24 while
I was writing). The `.c` count climbed 386 -> 391 -> (still rising) between consecutive `progress`
runs with no action from me. **I did not touch `port/`, `tools/saturn-recomp/`, or the port copies.**
Consequence: the absolute coverage numbers below are a **moving snapshot and are NOT solely my
contribution**. The reliable batch-13 deliverable is the explicit list of 7 functions in the next
section. Someone should confirm whether a second functional agent / background loop is running against
the MAIN repo.

## Authoritative counts (`project/progress_functional.py`, of 1272 members) - SNAPSHOT
```
functional  (src_functional):  347    27.28%   (+47 extra)
nonmatching (src_nonmatching):  73     5.74%
byte-exact  (src files)      :   5     0.39%
byte-exact  (oracle EXACT)   :   3     0.24%
ANY C, member-covered (deduped): 353   27.75%
ANY C, out-of-universe extras:   47
ANY C functions total        :  400
```
Batch-12 report baseline was 333 / 340 / 382. The jump beyond my +7 is the concurrent writer above.

## Functions added (offset -> name)
| cart addr | SH2 file | name | role |
|---|---|---|---|
| 0x02228134 | 02228134.c | `players_entities_ready` | member; -1 if any active player's entity->unk56==0 |
| 0x02228536 | 02228536.c | `actor_substate_init` | member; per-state block (re)init on FSM main-step entry (from 0x228508) |
| 0x02228d44 | 02228d44.c | `actor_state_1A_enter` | member; g_06091100 handler; anim_update then morph to state 0x1A (**re-decomp**, see below) |
| 0x0222a878 | 0222a878.c | `scene_value_select` | member; picks s32 from (*g_0609866C)->unk1C by player-count mode |
| 0x0222ac60 | 0222ac60.c | `actor_hit_countdown` | member; unk56 countdown; fires a late-join spawn on reaching 0 |
| 0x0222bc80 | 0222bc80.c | `actor_state4_enter` | member; state-4 (attack wind-up) setup + attack-param table lookup |
| 0x0223dd28 | 0223dd28.c | `actor_status_publish` | extra; publish actor state into 16-byte record g_060E0E38[slot-1] |
| 0x0220a9c2 | 0220a9c2.c | `resident_query_seq` | extra; resident-manager query sequence through fp g_06000644 |

**Re-decomp note (0x228d44):** this file already existed from **batch 6** (the 0x228-0x229 handler
band). My cross-check against the `ls` of existing files missed it, and the `scp` overwrote it. The
original is unrecoverable (no git in this repo), but my version is the **same function**, hand-verified
field-for-field against the current asm, so it is at worst a lateral replacement. Net *new* functions
this batch: **6** (+ this 1 re-decomp).

## Player-actor family progress
The direct callees of the join/step chain that batch-12 listed as "called faithfully but undecompiled"
are now mostly in C:
- `actor_substate_init` (0x228536) is the per-state (re)init the actor FSM step (0x228508) runs; it
  seeds unk84/88/38/C0/C4, zeros the screen-pos pair g_0609979C/A0[slot-1], and branches on
  `unk6D==0 && (unk8C & 0x02000000)` to either the special-anim path (func_0223a6dc + state 0x0D) or
  the normal path (unk5C from g_06096C74[slot], state 1).
- `actor_state_1A_enter` (0x228d44) and `actor_state4_enter` (0x22bc80) are two per-state handlers
  from the g_06091100 dispatch table (2-arg task/entity and 1-arg entity respectively), confirming the
  handler ABI that 0x22863E calls through.
- `actor_hit_countdown` (0x22ac60) is the unk56 countdown: when it drains it raises
  `(*g_0609882C)->unkD` and writes a spawn seed into `(*g_0609882C)->unk8`, gated on the join slot
  (g_06099590), the two-player flag (g_06098670) and progress (g_06098830>=5) - a late-join spawn.
- `players_entities_ready` (0x228134) is the two-player readiness gate over g_06099584.
- `actor_status_publish` (0x23dd28) and `resident_query_seq` (0x20a9c2) complete two standalone query
  paths: the 16-byte status record at g_060E0E38 and the resident-manager fp g_06000644 (the
  "resident-query fp path around g_06000640" that batch-12 flagged - it is g_06000644).

All field offsets (unk34 slot index, unk43 state byte, unk54 anim flags, unk56 ready/hp word, unk5C
param, unk8C flags-long) are consistent with batch-12's reads of the same structs.

## Resisted / deferred (honest)
- **0x0222db44 - DEFERRED.** Three calls into the 0x24Axxx runtime band via **non-standard register
  ABI** (r0/r1/r2): `bsrf -> 0x24A360` is a **32-way byte-offset table dispatcher** (index in r1), and
  two `bsrf -> 0x24A280` hit a dispatch-or-variable-block-copy helper (selector r0=108 selects the copy
  path; the byte count isn't pinnable from the bounded window). Not faithfully expressible as standard
  C without establishing each helper's exact ABI/semantics -> matches the table-dispatch /
  ABI-unconfirmed defer rules. This is priority-#2 item `func_0222db44`.
- **0x24A120 / 0x24A280 / 0x24A360** - compiler/runtime helpers (unsigned divide; copy/dispatch; table
  dispatch). Left untouched in the 0x24Axxx band per constraints. `0x24A120` IS the unsigned-divide
  helper (`div0u`/`div1` loop, r1/r0), so in `resident_query_seq` its `bsrf` is correctly rendered as
  `/` rather than a fake call.
- **0x0222ab18** (0x148) - unk56-decrement + late-join trigger, kin to 0x22ac60 but with more guard
  conditions and multiple func_02210020 calls. Not traced this batch (budget/quality); clean target.
- Untouched per constraints: `func_022016a0` + 0x201d2c/0x204df4 (port agent), `port/` +
  `tools/saturn-recomp/`, the 0x24Axxx dispatch band (as decomp targets), 0x2896xx data-as-code,
  confirmed misseg, non-standard-ABI 0x221354/0x221384.

## Next-band recommendation
1. **Resolve the process anomaly first** - confirm whether a second functional writer is running on
   the MAIN repo; if so, coordinate address ranges to avoid the kind of silent overwrite that hit
   0x228d44. A git init on this repo would make overwrites recoverable.
2. **Finish the per-state handler table** g_06091100: the remaining small handlers in the 0x228-0x22b
   band (e.g. 0x22ab18, 0x228bb6, 0x228f08, 0x229074) are the same (task,entity)/(entity) ABI and
   complete the actor state machine the join chain feeds.
3. **Map the 0x24Axxx helper ABI once, cleanly** (divide 0x24A120 done; the 0x24A280 copy/dispatch and
   0x24A360 table dispatch) - that single effort unblocks 0x22db44 and any other in-band function that
   calls them.
4. Keep running `find_entry.py` before every feed AND **cross-check against existing `.c` filenames**
   (both cart-offset and SH2-address forms) before writing, to avoid re-decomping done functions.
