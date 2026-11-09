# Functional decomp batch 11 (Opus, single worker)

Headline: **10 new verified functional `.c`** (4 fn.units members + 6 extras), every one
compiling clean `gcc -c -fsyntax-only -Wall -Wextra` with **ZERO warnings** and hand-traced
against bounded SH-2 disassembly + cross-checked with m2c. **Finished the 0x244988 split**
(all three large processors + the 0x244aa8 alt-entry) that batch 10 deferred; shipped the
**obj / task-handler init family** (0x20b098 / 0x20b0a4 / 0x20b85c / 0x20bdb4 / 0x21f952 /
0x20feb0); and **taught ptr_resolve.py the boot-cluster halfword swap** for mova+add
function-pointer deltas (validated, non-boot cases byte-for-byte unchanged). Ran as a single
serial worker (no forks), per the standing process rule.

## Authoritative deduped counts (`project/progress_functional.py`, of 1272 members)
```
functional  (src_functional):  333    26.18%   (+36 extra)
nonmatching (src_nonmatching):  73     5.74%
byte-exact  (src files)      :   5     0.39%
byte-exact  (oracle EXACT)   :   3     0.24%
ANY C, member-covered (deduped): 340   26.73%
ANY C, out-of-universe extras:   36
ANY C functions total        :  376
```
Baseline at batch-11 start: functional **329 (+30 extra)** / ANY-C member **336** / ANY-C total **366**.
Net this batch: **+4 functional members, +6 extras, +4 ANY-C member-coverage, +10 ANY-C total.**

## 1. Task 1 - the 0x244988 split is FINISHED
The three large direct-branch processors (and the alt-entry) that batch 10 identified but left
below the bar are now shipped, each isolated with `decomp_bounded.py` at its real end and
cross-checked against m2c:

- **0x2449b2 `actor_script_step`** (end 0x244aa8) - advance one step of an actor's **4-byte-record**
  control-word script. Gate word `w = rec[1]`: bit 0x8000 terminate (clear actor+0x84 bit 0x200,
  return 0), bit 0x4000 timed-wait (bump tick actor+0x11e, wait `masked<=tick`), else a limit
  gate vs actor+0x58. Accept advances the index, loads the next word, applies its control flags
  via the shared helper **0x244c4e** (already in C), returns 1; `-1` = waiting.
- **0x244aa8 `actor_script_begin`** (end 0x244b1e) - alt-entry: zero the index/tick/duration and
  store the script base at actor+0x118, then fall into `actor_script_load`.
- **0x244abc `actor_script_load`** (end 0x244b1e) - load the current record: set actor+0x26 from
  `cfg[2]&0xf000`, mark actor+0x41, then dispatch on `w=cfg[0]`: `0xffff` -> use actor's own
  default (actor+0x148); `0xfffe` -> indexed table lookup `(**actor+0x158)[cfg[2]&7]`; else apply
  control flags.
- **0x244b1e `actor_script_step6`** (end 0x244c4e) - stride-**6** sibling of `actor_script_step`
  with an extra bit-0x2000 branch into the global dispatch table **g_0608db3c** (8-byte entries:
  key@0 / threshold@2 / callback@4). If `key==actor+0x58 && actor+0x40==0` it fires the entry's
  callback(actor) and returns -1; otherwise gates on the threshold. Accept tail-calls
  `actor_script_load` with the next record.

The 0x244988 fn-unit is now fully in C (member 0x244988 + helper 0x244c4e from batch 10, plus
these four).

## 2. Task 2 - boot block: ptr_resolve swap shipped; branch leaves honestly deferred

### 2a. `tools/decomp/ptr_resolve.py` boot-cluster halfword swap (shipped, validated)
The 0x200xxx boot pools sit at **2-aligned** offsets, so a 4-aligned `mov.l`/`mova` read grabs a
longword straddling two constants - the running value is `swap16(raw)` (same finding `pool_read.py`
already exploited for mov.l pools and bsrf/braf deltas). `ptr_resolve.py` now applies the same
swap to **mova+add DATA / FUNCTION-pointer deltas**, **gated** to `start in BOOT_LO..BOOT_HI`
(0x200400..0x201020) so every non-boot resolution is byte-for-byte unchanged (regression-checked
against the batch-4 cases 0x22a970 / 0x22a9fe). Inside the cluster it reports both raw and swapped
targets and recommends whichever lands in a valid address window (reusing `pool_read.classify`);
both/neither -> keep raw and flag. Backup at `ptr_resolve.py.bak11`.

Validated on 0x200668 (note the pick is **per-constant**, not a blanket swap):
- `add@200680`: raw -> 0xe7b40680 (invalid) / swap -> 0x0220ec14 (ROM) => **SWAPPED**. This
  *confirms* the bsrf sibling at 200684 -> 0x20ec1c: the computed fn-ptr is that callee's own
  alt-entry 0x20ec14.
- `add@200694`: raw -> 0x02505bd6 (valid ROM) / swap -> invalid => **RAW**.

### 2b. 0x200668-class branch leaves - deferred (below the verified bar)
With `pool_read.py --branches` + the new `ptr_resolve.py` the branch *deltas* and fn-ptrs now
resolve. The leaves themselves still cannot be shipped at HIGH fidelity because:
- their callees **0x20eade** and **0x20ec1c / 0x20ec14** are jump-table dispatchers built on the
  same misaligned-pool idiom (0x20ec1c carries an embedded byte jump table; 0x20ec14 is a second
  entry into it), and
- **func_020eade reads r7 which 0x200668 never sets** - the leaf passes r7 through implicitly, so
  its own ABI is >4 registers and unconfirmed.

The tooling is now ready for a focused boot pass once the eade/ec1c jump tables are mapped; I did
not ship speculative bodies.

## 3. Task 3 - obj / task-handler init family (callees already in C)
These were reached via a leaf scan for small undone members / find_entry-corrected entries whose
callees are decompiled:

- **0x21f952 `obj_advance_rebuild_mtx`** (member) - `obj=*(actor+8); *(s32)(obj+8)+=0x4000; tail
  mtx_build_fwd_inv(obj)` (braf -> 0x214a7a). fn.extent over-runs the unconditional braf; bounded.
- **0x20b098 `obj_rebuild_mtx`** (member) - `mtx_build_fwd_inv(*(void**)(actor+8))` (braf ->
  0x214a7a), dead one-slot frame. Bounded (fn.extent merges it with the next extra).
- **0x20b0a4 `obj_init_mtx_handler`** (extra) - `func_02214bb8(actor,1,0xdc)` pool-init; register
  actor at g_0609882c; install `*(void**)actor = &obj_rebuild_mtx` (mova+add -> 0x20b098).
- **0x20b85c `func_0220b85c`** / **0x20bdb4 `func_0220bdb4`** (members) - task-handler trampolines,
  modes 1 and 2, siblings of the already-done `func_0220b354` (mode 0): run `func_0220d8d0(task,N)`,
  temporarily install `entity(=task->unk8)->unk64` as `task->unk0`, call it, then reinstall self
  (mova+add -> self). Named `func_<addr>` to match the existing sibling's convention.
- **0x20feb0 `func_0220feb0`** (extra) - `p=f_220fc82(a,b); func_02214bb8(p,1,0x6c); return p`.
  The clean body begins at 0x20feb0; the fn.units member 0x20fe9e is trailing-pool-contaminated
  (find_entry maps it to the separate entry 0x20fece), so this is named at its own address.

## 4. Resisted / deferred (honest)
- **0x200668-class boot branch leaves** - see 2b (jump-table callees + implicit r7 ABI).
- **func_0220d8d0** (trampoline callee) and **f_220fc82** - called faithfully, bodies undone.
- **Not real functions** flagged during the pure-leaf scan: **0x268420 / 0x2644d6** disassemble as
  garbage (data-as-code / misseg) - skipped.
- **Untouched per constraints:** 0x24axxx dispatch band (the scan surfaced 0x24a39c/0x24a4aa/
  0x24a4c0/0x24ab5c... - all skipped), 0x2896xx data-as-code (0x28966a... skipped), confirmed
  misseg, non-standard-ABI 0x221354 / 0x221384, `port/` + `tools/saturn-recomp/`, read-only
  `m2c_fn`/`fn`/`match`.

## 5. Next-band recommendation
1. **0x200xxx boot branch pass** - now that deltas/fn-ptrs resolve, the blocker is the two
   jump-table dispatchers 0x20eade and 0x20ec1c/0x20ec14. Map their byte jump tables (they use the
   misaligned-pool idiom; a small `braf_resolve`/`pool_read` jump-table mode would crack them),
   pin the leaf ABI including the pass-through r7, then the 0x200668 class (~a dozen) falls.
2. **Task-handler / entity-task family** - func_0220d8d0 (the trampoline callee) and the entity
   `->unk64` handler system look like a sizeable vein once d8d0 and f_220fc82 are decompiled; the
   trampolines (0x20b354/85c/bdb4) are the tip.
3. **0x241xxx / 0x221xxx actor-init family** - still the steady runway (helper ABIs pinned in
   batches 9-10); members 0x241a10 / 0x241b66 / 0x241e82 / 0x2418fa remain.
4. Keep running `find_entry.py` before every m2c feed and `decomp_bounded.py` for any fn.extent
   over-run (the braf-tail merges 0x20b098 / 0x21f952 this batch); stay a single worker.
