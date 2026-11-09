# Functional decomp batch 6 (Opus) — the "GBR entity-base" problem was a MISDIAGNOSIS

Task-1 asked me to teach the pipeline about a GBR-held entity pointer blocking the
0x228xxx-0x229xxx actor state-step band. **There is no GBR entity base.** Investigating the two
named examples (0x2296da, 0x228d76) showed the real problem and a second, compounding one; both are
now fixed with reproducible tools. Task-2 (0x23ac9c, 0x23a3a2) is addressed: the 0x23ac9c tool
blocker is solved and 0x23a3a2's caller contract is established (both large bodies deferred — honest,
see §5). All new .c compile clean (`gcc -c -fsyntax-only -Wall -Wextra`, **zero warnings**). Core
read-only tools (m2c_fn.py / fn.py / match.py) untouched.

## 1. What the "GBR / bogus-constant" symptom actually is

m2c on 0x2296da / 0x228d76 emits garbage constants (0x4F22, -0x2D3C) and lost field refs. Cause:

- **No GBR.** Every handler in this band reaches the entity through its **r5 argument** (moved to
  r14), with the ordinary SHC large-offset field idiom `r0 = mov #imm ; mov.b/w/l @(r0,Rn)`. A
  ROM-wide scan for `ldc/stc GBR` finds nothing meaningful in 0x228-0x229; decompiling the corrected
  starts yields clean `entity->field` C (entity = arg, not GBR).
- **The start address was wrong.** `fn.units()` groups functions by shared literal pool; at a unit
  boundary the previous function's trailing pool sometimes spills a few words past where the grouper
  stops absorbing, so the next unit's reported first member lands **inside that pool**. Disassembling
  from there is pool-as-code garbage. The infamous `-0x2D3C` is literally the spawn-template delta
  (`0x228e84 - 0x226148`); `0x4F22` is the encoding of `sts.l pr,@-r15` read mid-pool.

So batch-5's "GBR" note mis-attributed a mis-segmented start. Validation: `fn.extent(0x228d44)`
ends at 0x228d9c, which is the real prologue after the contaminated unit-start 0x228d76; m2c on
0x228d9c produces clean `handler(ctx r4, entity r5)` C field-for-field matching the raw asm.

## 2. NEW TOOL — tools/decomp/find_entry.py (the real task-1 unlock)

Given a (possibly pool-contaminated) address it returns the REAL entry. Detection: a real SHC entry
disassembles to a genuine instruction; a contaminated address is capstone `.byte` skipdata and/or in
`fn.all_pool()`. Correction: the real entry is the previous function's `extent()[1]` (end of its
code AND trailing pool). `--band LO HI` lists corrected entries per unit.

```
0x2296da -> 0x02229714   0x228d76 -> 0x02228d9c   0x228efc -> 0x02228f08   0x22812e -> 0x02228134
```
All corrected members then disassemble as real instructions (`real=True`), and the named anchors in
the band (0x228508, 0x2289b8 actor_step_launch, 0x22a5d0 process_spawn_list) confirm the tool.

**Big finding via the tool's done-check:** the entire 0x228-0x229 state-step band is **already
decompiled**. Prior batches wrote those files under **SH2-address** names (`02228508.c`,
`02229714.c`, …) while `scan_family.py`/the batch-5 recommendation used a cart-offset done-check, so
they read as "not done". They are part of the 244. A normalized done-check (parse every filename as
hex, subtract 0x02000000 when >= that) shows **824 undone real entries remain**, 397 of them small.

## 3. NEW TOOL — tools/decomp/decomp_bounded.py (the 0x23ac9c unlock)

Decompiles with an explicit `--end` (monkeypatches `fn.extent` AT RUNTIME for the one start — clamps
code window, drops out-of-window branch targets; never edits the read-only sources) and **tailifies a
direct `bra` to an out-of-window address** into a tail call (`<slot>; bsr target; nop; rts; nop`,
mirroring braf_rewrite). 0x23ac9c needs both: its rts is at 0x23b01a, but at 0x23af08 it `bra`s to
the shared func 0x23b254 **over** the separate TUs 0x23b01e/0x23b1c0. With `--end 0x23b01e` it now
produces a complete, single-function body ending in the tail `func_0223b254(arg0)`.

## 4. Functions added this batch (14) — all compile clean, net +14 (244 -> 258, 20.28%)

From the GBR band (the only two genuinely missing there):
| SH2 | name | note |
|-----|------|------|
| 0x0222863e | actor_task_main_step | task handler; fn-ptr-table state dispatch g_06091100[unk43]; installed by 0x228508 |
| 0x02228e0c | actor_step_grab_follow | **hidden** by the 0x228d9c extent over-run; recovered via find_entry.py; sibling of 0x228e60 |

Fresh leaves/near-leaves (from the 397 small-undone list, main engine code, all asm-verified):
| SH2 | name |
|-----|------|
| 0x022131d4 | dist2_xz_fixed (16.16) |
| 0x022131fc | dist2_xyz_fixed (16.16, 3-axis) |
| 0x02214918 | smpc_cmd_sshoff |
| 0x02214944 | smpc_cmd_sshon |
| 0x022100a2 | str_to_hw_table |
| 0x02216018 | sound_emit_latch |
| 0x0221603e | sound_emit_gated |
| 0x02216110 | actor_sound_emit_slot |
| 0x0222f3c0 | actor_pair_propagate |
| 0x02214dae | vdp1 cmd-list terminate + radix-sort arena reset |
| 0x0222a91a | actor_level_band |
| 0x0221707c | actor_slot_record_reg |

Widths taken field-by-field from `m2c_fn.py --asm` (mov.b/w/l -> M8/M16/M32, extu/exts -> sign).
Note: the `--asm` dump interleaves `mov.w .Lpool,r0` offset-reloads; do NOT filter pool lines when
reading offsets (that hid an `r0=0xE4` reload during this batch and briefly looked like a bug).

## 5. Resisted / deferred (honest)
- **0x23ac9c actor_state_gen_special** — tool blocker SOLVED (decomp_bounded.py, §3); bounded C is
  structurally complete, all 4 bsrf + 6 mova/add pointers resolved (spawn_obj_0x7c 0x20ff28;
  templates 0x23d2a0/0x23d3d0/0x23d558/0x23d1b6; rng 0x210020; func_022260c8/0223cbb0; tail
  func_0223b254). **Faithful field transcription deferred:** ~0x37e bytes, 4-way spawn switch, a
  function-pointer spawn call, multiple anim-record tables, AND it stores **uninitialised
  callee-saved r12** into spawned->unk64 on some paths (same register-provenance class as 0x23a3a2's
  var_r13). Not cheap as batch-5 hoped; needs a dedicated full-asm width pass. Tool + resolved data
  make it a clean follow-up.
- **0x23a3a2 actor_state_release_gen** — caller contract established: its only caller 0x22979c
  (func_0222979c, now confirmed) calls it unconditionally for the release states, so unk88 is
  guaranteed 0x52/0x53 and the default path (where `var_r13` is read uninitialised) is dead; var_r13
  is always assigned (= unk6D or unk6C) before the shared tail. **Full transcription deferred:**
  826 bytes, two nested switches, a spawn (template 0x227906), many fields — mechanical but large and
  width-sensitive; deferred rather than rushed.
- **0x02213538 / 0x02213ce4 / 0x02213b5c** (vec matmul) — `mac.l` not modeled by m2c (M2C_ERROR);
  skipped.
- **0x02221384** — relies on r2/r3 surviving a call to dispatch_table_a (0x24a360); unverifiable
  register-preservation assumption, deferred.
- **0x02130b4** (atan via on-chip DIVU @0xffffff00 + arctan table 0x06078F28) — HW-divide-register
  semantics, deferred for a careful pass.
- Untouched per constraints: the 0x24a3xx-0x24a5xx jump-table cluster and the misseg/out-of-ROM band.

## 6. Real counts (tools/decomp/venv/bin/python project/progress_functional.py)
```
functions total (fn.units) : 1272
  functional  (src_functional):  258    20.28%   (was 244 -> +14, all new addresses)
  nonmatching (src_nonmatching):   73     5.74%
  byte-exact  (src files)      :    5     0.39%
  byte-exact  (oracle EXACT)   :    3     0.24%
  ANY C written (deduped)      :  336    26.42%
```

## 7. Next-band recommendation
1. **Fix the done-check everywhere.** `scan_family.py` and any triage still under-report because
   filenames mix cart-offset and SH2-address forms. Normalize (parse hex, subtract 0x02000000 when
   >= it). This batch's undone_scan logic (in the report's method) found 824 undone real entries,
   397 small — the real runway, not the already-done 0x228-0x229 band.
2. **Mop up the 397 small-undone leaves** (see find_entry `--band` + the normalized done-check).
   Many are already named HIGH in symbols with no .c yet (e.g. sh2_dmac_copy 0x2145d0, scu_dma0
   0x21478a, vec3_array_matmul-adjacent math, more SMPC/sound/VDP helpers). Pure transcription.
3. **Finish 0x23ac9c and 0x23a3a2** now that the tool (decomp_bounded.py) and the caller contract are
   in place — they just need a patient full-asm width pass (and a convention for the uninitialised
   callee-saved-register stores: init-to-0 under the documented contract).
4. Keep avoiding the 0x24a3xx jump-table cluster and the misseg/out-of-ROM band; always confirm an
   entry with find_entry.py before feeding m2c (prevents the batch-5 "GBR" false alarm recurring).
