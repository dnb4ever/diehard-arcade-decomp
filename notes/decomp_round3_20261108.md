# Functional decomp round 3 — 2026-11-08

v1.07 is promoted after the frozen full gate, with v1.06 retained verbatim for rollback. Prior round-2 changes remain in the tree: all 38 previously edited C sources match the fresh-baseline hashes, and none overlaps this round’s 19 C edits (`prior_work_preservation.json`). All work is within `project/src_functional/`, `tools/decomp/`, `notes/` excluding `notes/port/`, and `work/` excluding `work/converge/`. No commit, GPU use, privileged command, unrelated process termination, delegation, or port/runtime/toolchain edit was performed. Scans and builds use at most four jobs.

## Status and promotion gate

| Full scan, 16 requested vectors plus inherited/additive profiles | PASS | PASS-void | FAIL | SKIP |
|---|---:|---:|---:|---:|
| v1.06 fresh baseline | 118 | 730 | 0 | 251 |
| v1.07 frozen final gate | 157 | 774 | 0 | 168 |

Transitions: **44 SKIP -> PASS-void**, **39 SKIP -> PASS**. All **848** starting PASS/PASS-void functions retain their exact status. The fresh v1.06 scan exactly reproduces the supplied baseline. Three newly enabled PASS-void entries still take inactive states; their separate coverage floors are listed below, so not every status gain is new body verification. `audit.json` compares every address and every frozen C/header/symbol/tester/ABI/hint hash; no unexplained change or input drift remains. The exact gated v1.07 file is copied to `difftest.py`; `difftest_v106.py` and its hints remain unchanged. `promotion.json` records the copy hashes.

Evidence paths below are relative to `work/decomp_round3_20261108/`, except installed tools and notes. Full reports retain actual vectors, input setup, branch directions, source hashes and effects. Passing bounded vectors are not a claim of complete path equivalence.

## Starting 251 SKIPs: root-cause histogram

This is a mutually exclusive primary-blocker inventory, not just an error-string count. `skip_inventory.json` retains all 251 addresses/reasons, and `class_outcomes.json` retains each cohort's final statuses. Two private-stack errors were actually the sin/cos output-pointer representation; `0224a000` is an ordinary actor handler accidentally covered by the runtime-address guard. Those are classified by the inspected root cause. Deeper blockers can remain after a class's first limitation is removed.

| Primary blocker | Starting SKIPs | Final PASS | Final PASS-void | Still SKIP |
|---|---:|---:|---:|---:|
| unmapped pointer / input setup | 52 | 1 | 0 | 51 |
| compiler runtime register ABI | 35 | 27 | 1 | 7 |
| hardware / device access | 26 | 6 | 4 | 16 |
| escaping private stack | 25 | 2 | 4 | 19 |
| callee / indirect signature | 22 | 0 | 0 | 22 |
| C generator / aggregate or scalar types | 21 | 0 | 4 | 17 |
| undeclared live registers / SR state | 18 | 1 | 7 | 10 |
| variadic interface | 16 | 0 | 10 | 6 |
| image write / pointer provenance | 10 | 0 | 0 | 10 |
| entry bytes / segmentation | 9 | 0 | 0 | 9 |
| initialized static tables | 5 | 1 | 4 | 0 |
| custom / multiple-result register ABI | 5 | 1 | 4 | 0 |
| bounded loop / asynchronous wait | 5 | 0 | 5 | 0 |
| symbol spelling | 1 | 0 | 1 | 0 |
| overbroad compiler-runtime entry guard | 1 | 0 | 0 | 1 |

The five bounded-loop cases are **two radix-sort loops** (`02214e2c`, `02214e2e`), two texture upload/ready loops (`022150d4`, `02215228`) and external pulse calibration (`02223c74`). The prior round's description of the radix-sort pair as DMA waits was inaccurate. ROM `06018e74..18edc` walks entity lists and buckets.

## Tester changes

`tools/decomp/difftest_v107.py` was developed separately. Its explicit contracts are `difftest_abi_v107.json`, and inputs remain separate in `difftest_hints_v107.json`.

- **Register recipes:** entry argument register lists, logical return register, register outputs represented by C pointers, explicit internal dispatcher targets, callee argument register/stack recipes, result-field stores, and proven constant register residue. Integer divide uses dividend r1/divisor r0; shifts use r0/r1. The r8/r9 dispatcher returns r1. Sin/cos entry compares both r4/r5 values through the C output pointers; its callers execute the original leaf bytes while C uses an independently transcribed table recipe. The live-r5 facing leaf likewise executes original bytes, including its actor+0x4d store; C uses the exact field recipe. `entry_abi` in results explains when the legacy `r0_checked` flag means a different logical return register.
- **Runtime entries:** reviewed integer shift sled entries can receive r0 inputs directly, including the saved-r2 epilogue slot. Unsupported soft-float, 64-bit aggregate and exceptional register interfaces remain SKIP. The broad address guard no longer automatically rejects the ordinary actor entry `0224a000`; that entry then reaches an unresolved pointer setup.
- **Escaping stack objects:** per-callee `read` descriptions snapshot exactly the declared bytes at call time, replace only private pointers with stable argument tokens, and hash/compare the payload. `out` descriptions fill caller storage deterministically on both sides, so post-call consumption is checked. Implemented vector/box, angle-output and particle-position contracts enable meaningful callers. Undescribed escapes and private addresses stored into ordinary guest memory remain SKIP. This is not blanket stack-address normalization.
- **Static tables:** file-scope and local initialized const arrays, including nested arrays, live in instrumented candidate-private guest storage. C initializer data is independent of ROM data, so wrong constants fail through their uses. Mutable statics are rejected pending persistent-state modelling; no immutable program bytes are patched.
- **Variadic/stack-only formatter:** concrete word call sites are supported; known formatter callees read their arguments from call-time SP. Multiple arities are selected by parsing the original immutable format string and checking the candidate-declared set, including escaped percent and widths. DMA diagnostics have three stack words; stale r4 is ignored. Aggregate/by-value and the formatter implementations' `stdarg.h`/variadic entry lowering still require explicit interfaces and remain SKIP.
- **DIVU and DMA fixtures:** opt-in signed 32/32 and 64/32 quotient/remainder calculations with signed saturation; DMA/SCU status gives a deterministic busy/ready transition. Only listed register windows are mapped. VDP1 register programming is included. Hardware read order/values are compared as well as writes. These verify programming boundaries; DMA transfers, device timing, video/audio execution and interrupts are not simulated.
- **External waits:** explicit per-address read schedules model changes to shared signal/ready bytes independently of CPU instructions. Both sides consume the same samples; widths and read traces must match. Finite radix lists and zeroed bucket heads are initial-state fixtures, not live loop repair. The upload fixture uses a valid first group followed by -1: the ROM processes the first group before testing a subsequent sentinel, so an empty-list fixture was rejected.
- **Remaining limits:** nested/provenance-heavy pointer graphs, ambiguous callbacks without an original signature, escaped objects without a payload contract, mutable/static aggregate state, floating/aggregate interfaces, protected program-image writes, unimplemented buses/devices and invalid entry bytes are still explicit floors. More permissive memory mapping or fabricated entry bytes are not used to manufacture passes. The `fixmul` and `lookat_angles` spellings are resolved by ROM-backed symbol aliases. Taking the address of an instrumented memory accessor preserves its guest address and offset.

The final files contain **35 entry recipes**, **12 callee recipes** and **219 input profiles**. Schema and footprint limits remain explicit in the code/tests.

## ROM triage and installed corrections

**19 installed C files** changed. **18 original sources FAIL under the final model; one originally passing source is rejected for a missing output argument.** All corrected files pass **64 requested vectors**; `0221a6a8` additionally repeats them under its coverage fixture (128 executions). `fix_replays.json` retains both original and corrected results; originals are saved verbatim in `originals/`. Seventeen corrections started as SKIPs, and two (`0221a6a8`, `0223a806`) were previously passing with inadequate observations.

| C function(s) | Evidence and correction |
|---|---|
| `02213d90` | `abi_initial.asm`, `runtime_abis.asm`: `06017d9c` is the six-entry jump table; `06017db4` is its first code target. Replace dereferencing code words as function pointers with selector max/min/average/back-copy logic; retain quad+0x20 store and r1 result. |
| `0224a4aa`, `0224a4d4`, `0224a4e6`, `0224a4f6` | `newfails.asm` `0604e4aa..4e502`: ROTL followed by OR sign-fill masks. Signed `>>31` incorrectly filled low bits for negative inputs. Rotate an unsigned word, then cast the result. |
| `0220e7c4` | `fixmul_alias.asm` `0601289e..1291e`: repeat the sine call before each of six fixed multiplies. The C had collapsed six observable calls into one. The symbol aliases and guest-memory address-taking support expose this mismatch. |
| `0220f5b8` | `newfails.asm` `060136e6..136e8`: the call delay slot sets r4=2, not 4. Restore the debug-camera service selector. Vector payload comparison first made the preceding stack call verifiable. |
| `022139b8` | `newfails.asm` `060179e8..179ea; 060179f6..17a0a`: SHLL16 truncates each width/height to a 32-bit word before DMULS.L. Restore that truncation before the 64-bit host product. |
| `022144b8`, `02214510`, `022145d0`, `02214678` | `newfails.asm`, `runtime_abis.asm`, `dma_residue.asm`: retain DMAOR read after the first CHCR clear, and CHCR read before the final clear/kick. Final footprint had matched but volatile read traces had not. The 14510/145d0 diagnostic prototypes also drop fabricated r4 arguments and represent the three original stack words. |
| `02214e2e` | `waits.asm` `06018e8e..18e96` and the later bucket passes: fill node next/entity/key before publishing the bucket head. The helper had published the head early. The bounded two-record fixture tests all radix passes and ordered stores. |
| `0221a6a8` | `a6a8.asm` `0601e744/1e772/1e7a0`: frame IDs are MOV.W loads at four-byte strides. Replace MOV.L followed by low-half truncation; the old default vectors never entered the body. |
| `02222c08` | `builders.asm` `06026c7e..26ce8`: program the DIVU pipeline and read the preceding quotient in ROM order. Direct host division omitted hardware effects and saturation. `06026d0c..26d16` additionally proves the conditional intermediate midpoint store; retain it only on the ROM path that executes its delay slot. |
| `02234b58` | `traces_02234b58.json` and `builders.asm`: zero rewind entries do not write the matrix cursor. Remove extra writes of an unchanged cursor, preserving nonzero rewinds. |
| `02235198`, `0223579c` | `bc_full.asm` `06039714..3971c` and `06039d22..39d2c`: the last block still loads/stores the part ID. Restore the omitted final halfword stores; push remains omitted. |
| `0223a806` | `a806.asm` `0603e818`: push actor+0x62 as the fifth argument to `store_angle_between`; `06014800` stores the angle through it. The former four-argument C caller had silently omitted this output. The new strict contract first SKIPs that source; the corrected caller preserves PASS-void with the output store observed. |

Readability applies to every installed C edit: a purpose/evidence header, semantic temporary/helper names, existing `dh_names.h` address/callee spellings, task accessors where the task layout is proven, and `DhWorldPose` fields at the confirmed camera/actor position sites. Quad/effect/transform offsets keep their actual layouts. `notes/symbols.txt` adds evidence-backed dispatcher, DMA, angle-caller, shift-sled names and the `fixmul` alias; linker identities are preserved.

### Artefacts distinguished from C bugs

The first full v1.07 scan (`gate1`) has 15 FAILs: six (`0220a0b0`, `0220a21c`, `0220a390`, `0220a4f0`, `0220a648`, `0220af94`) compare register words against a stack-only formatter. `formatter_abis.asm` shows the original `02223460` stack-format load and the `02233122` pushed argument; formatter r4–r7 are ignored; its format/arguments are at SP. Correcting that boundary makes four PASS-void and exposes later pointer faults in the other two. No C edits were made to those six. The rejected `gate2` results are retained; promotion was blocked rather than accepting its two SKIPs of former passes. The other nine first-gate FAILs are corrected C behavior above; earlier targeted scans found the other corrections.

The first negative-only upload profile is invalid for the ROM's initial do/while group. `waits.asm` proves the first descriptor is dereferenced before the sign test on the next group; the profile was replaced with group 0 followed by -1. For `02215228`, a subsequent busy-wait mismatch was an omitted live-r7 stub output: `06018730` sets r7=2 and the DMA epilogue never restores it. The new constant-result recipe is checked against the original DMA bytes. The ordinary short ready schedule now passes. **The timeout-limit path remains a drafting/coverage floor:** C starts that counter at zero, and LZ-return r7 residue is also not qualified. The passing bounded fixture is not a claim that the timeout behavior is fully corrected.

## The 25 effectless passes

All **21 non-noops** now have matching effects in the default v1.07 scan; the original input vectors are retained before additional state fixtures. **Four are exact `rts; nop` leaves**, documented with `noops.asm`; artificial effects would be incorrect. No other member of this 25-function cohort is labelled dead code.

| Address | Observed v1.07 writes / calls (vectors) | Scope |
|---|---:|---|
| `02201b8e` | 0 / 0 | exact rts/nop, true no-op |
| `02201f20` | 16 / 16 | parent notification/cancellation state |
| `022143b8` | 0 / 0 | exact rts/nop, true no-op |
| `022191ec` | 16 / 0 | initialization stores |
| `0221a1a0` | 16 / 0 | controller acknowledgement/state transition |
| `0221a452` | 16 / 16 | active HUD state and draw calls |
| `0221a6a8` | 16 / 16 | recoil draw body; exposed/fixed halfword load |
| `0221d302` | 16 / 16 | initial task service/state advance |
| `0221f2bc` | 16 / 16 | safe state-1 fixture |
| `02223cf0` | 16 / 16 | VBlank input branch, callbacks and register writes |
| `02223d98` | 16 / 0 | VBlank output latch/frame updates |
| `02226144` | 0 / 0 | exact rts/nop, true no-op |
| `02229640` | 0 / 0 | exact rts/nop, true no-op |
| `0222a120` | 16 / 0 | paced-list countdown |
| `0222bfc2` | 7 / 16 | eligible NPC, angle out-parameter, grab-link stores |
| `02231b14` | 16 / 6 | flight state, integration and optional drawing |
| `02232610` | 16 / 16 | safe state-1 fixture |
| `0223358c` | 9 / 9 | teardown state with mapped VDP2 shadow |
| `02234984` | 16 / 16 | active stage-transform state |
| `02236246` | 16 / 16 | projectile motion, drawing and collision queue |
| `022363e4` | 16 / 16 | inspector initialization |
| `0223673a` | 16 / 16 | teardown state with mapped VDP2 shadow |
| `02236e66` | 7 / 7 | parent notification/cancellation gate |
| `02237ea6` | 16 / 0 | reset-state stores |
| `022384e4` | 16 / 0 | slot/animation state-1 gate |

Three newly enabled functions outside the original 25 remain effectless: `022119a4` (animation-sequence task), `022371c0` (eight-slot HUD manager), and `02237828` (two-slot HUD manager). Their random task states all bypass states 0/1; they are active-code coverage gaps, not no-ops or dead code. Per-function coverage floors request structured task/control/slot fixtures. The interface modelling gain is retained, without claiming those bodies are verified.

These observations qualify the selected states. Broader hardware branches, timeout boundaries, deep spawn chains and inspector text paths are not implied to pass. The retained round-2 focused tests and floors still describe those broader paths. `audit.json` gives all 25 before/after effect counts.

## Floors and validation

Added **261** floor rows: **244 tool-limit**, **6 noise**, **8 drafting-gap**, **3 lever-exists**. Updated **83** precise resolved prior rows; unrelated and broader-path entries remain. `floors_added.json` retains all changes. Remaining repeated SKIPs have explicit next-evidence requirements; the deeper grid-node pointer faults are registered instead of repeatedly retrying unstructured lists. `ledger_merge.json` records merging scan attempts into `work/attempts/<fn>.jsonl`, with targeted probes/replays already appended there.

- **1,099/1,099 objects compile; `ld -r` links the complete tree.** There are 16 diagnostic-bearing files, matching the baseline count. See `build_final.log` and `build/report.json`. An early build caught a missing `dh.h` include in the edge-builder rewrite; it was corrected before the final gate.
- **111/111 tests pass:** 54 inherited oracle tests, eight v1.06 contracts run against v1.07, 18 v1.07 contract/mutation tests, and 31 noise-report tests. See `suites_final.log`, `suites.json`; `promoted_suites.log` / `promoted_suites.json` repeat all 111 through the live module. New tests cover original-register inputs/results, both sin/cos outputs, custom dispatch, payload mutation sensitivity, out-parameter fills/consumption, static-table mutations, const persistence rejection, formatter arities, device read order, quotient saturation, shared-memory schedules and original DMA r7 residue.
- **19 C corrections pass 64 requested vectors each**, with additive vectors retained where configured; all saved original-source comparisons remain visible in `fix_replays.json`. The formerly missing angle-output caller is explicitly distinguished from the 18 old-source FAILs.
- An intermediate full scan (`gate2`) had zero FAIL but two status regressions. `02233122` deliberately exposes four ignored formatter register words followed by its first stack argument; it requires a per-caller register/stack adapter rather than the default stack-only recipe. `0223cf70` calls the particle allocator with flag zero, where the position argument is unused; the payload descriptor now requires that argument only when flag equals one (`optional_particle.asm` `06040ce2..40cf2`: compare flag with one before position loads). Focused regressions exercise both. The final **full `--all --jobs 4`** gate has no FAIL, no passing-status regression and no frozen-input drift. Promotion occurs only after those checks. Suites were also checked through the promoted import.

Reproduce:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --output-dir work/decomp_round3_20261108/recheck
tools/decomp/venv/bin/python work/decomp_round3_20261108/run_suites.py
python3 work/decomp_round3_20261108/validation.py
python3 work/decomp_round3_20261108/audit.py
```

## Ten-line summary

1. v1.07 is live: 157 PASS / 774 PASS-void / 0 FAIL / 168 SKIP.
2. All 848 original passing statuses are preserved.
3. 83 starting SKIPs now pass under explicit contracts/fixtures.
4. 19 installed C corrections pass 64 requested vectors each.
5. 21 formerly effectless bodies now show matching effects.
6. Four exact rts/nop leaves remain correctly effectless.
7. 1,099 objects compile and the complete tree links.
8. 111 tests pass, including 18 new contract/mutation checks.
9. Remaining ABI/device/pointer/timeout boundaries are registered as floors.
10. v1.06 rollback and prior work are preserved; no commit or out-of-scope edits.
