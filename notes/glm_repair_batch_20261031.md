# GLM repair and draft batch, 2026-10-31

Driver: `tools/glm/draft_batch.py` v1.01. GLM-5.3 supplied corrections and diagnoses; the driver compiled, checked, and installed accepted C. No commit.

Repair totals: **13 fixed, 4 artefact, 0 unfixed**, across all 17 requested FAILs. Artefact sources are byte-for-byte identical to their originals. Draft totals: **3 new files**, finally one PASS and two PASS-void; five interior labels and one special-ABI runtime entry were retained as boundary rejections. `022137a0` remains deferred and untouched.

## Repair results

Rounds count every returned per-function GLM call, including review diagnoses and refinements. The limit is one initial call plus at most three refinements, persisted across repair resumes. Difftest statuses below describe the final project files, not rejected scratch candidates. All fixed entries completed 16 vectors.

| Function | Result | GLM rounds | Final difftest | Correction / diagnosis |
| --- | --- | ---: | --- | --- |
| 02202c5c | fixed | 1 | PASS-void | Runtime HWRAM template and installed-handler literals. |
| 0220f990 | fixed | 1 | PASS-void | Descending record stores and exact byte/short reset order. |
| 022104f6 | fixed | 1 | PASS-void | Explicit cosine-before-sine call order; no nested evaluation ambiguity. |
| 02210622 | fixed | 1 | PASS-void | Square q before p; explicit fixmul calls retain the eight-word entry ABI. |
| 02213e0c | artefact | 2 | FAIL | Shared helper takes selector/object in r9/r8; oracle models ordinary r4/r5. |
| 02213f90 | artefact | 1 | FAIL | r4/r5 register results are exposed by an intentional C output-pointer wrapper. |
| 0221ad58 | fixed | 2 | PASS-void | Both spawn literals: matched 0601edd4, fallback 0603bea6; second audited after initial PASS. |
| 02227602 | fixed | 1 | PASS-void | Signed 16-bit dwell-counter increment/clamp. |
| 02227790 | fixed | 2 | PASS-void | Installed handler runtime literal 0602b830; first proposed mapping was rejected. |
| 0222ae40 | fixed | 1 | PASS-void | Walking-facing extraction shifts by 13, not 15. |
| 02233122 | fixed | 4 | PASS-void | Stack-only formatter ABI: preserve four register words and place format pointer in slot five. |
| 0223b254 | fixed | 1 | PASS-void | Sign extension after scaled source/destination indexing. |
| 022418a0 | fixed | 2 | PASS-void | Clear actor+0x45, preserving the original low-18-bit gate; canonical entry name. |
| 02244820 | artefact | 1 | FAIL | Real callee returns only 0..3; hash-stub default is outside that domain. |
| 022462bc | fixed | 2 | PASS | Correct bit-test branch polarity and retain forwarded r0. |
| 02246cac | fixed | 2 | PASS-void | Separate explicit AND store, re-read, then OR store; compound form rejected as SKIP. |
| 02249240 | artefact | 1 | FAIL | Real callee writes field+0x4d and returns extra r5; isolated stub does neither. |

## Model and review findings

`02213e0c` calls `02213d90` without ordinary register-argument setup. The callee C explicitly documents selector in r9 and object in r8. Forwarding r4/r5 to imitate the event trace produced SKIP rather than a valid repair; the original caller remains intact.

`02213f90` returns two values in r4/r5. Its existing output-pointer C wrapper is an explicit portable adaptation. GLM initially removed the outputs and obtained PASS-void; review rejected that loss of behavior and restored the original.

`02244820` calls `02243fba`, whose implementation produces only a two-bit mask. GLM initially accommodated the arbitrary hash-stub default and obtained PASS; review rejected that unreachable-domain change and restored the original.

`02249240` consumes the extra r5 result of `02245e58`, modeled portably through the field that the real callee writes. The stub preserves incoming r5 and performs no field write. On the failing vector, r5=7fffffff minus four yields 7ffffffb, explaining the reference high byte 7f. The field-based C uses stale stub memory. This establishes a model dependency for the reported first difference; it does not prove every later path equivalent.

`02233122` required additional review after an apparent PASS reduced the indirect callback arity from three to two. Target 06027460 maps to `02223460`: its first four register words are unused and its format pointer is the first stack word. ROM 23316a pushes 06088824 in the call delay slot. The reduced-arity candidate was rejected and the original restored before the remaining repair rounds. The final table records the outcome of the subsequent ABI repair, not that rejected PASS.

All original versions are retained as `work/glm_repair/<fn>.orig.c`. Prompts, answers, disassembly, compile output, oracle JSON and reports are under `work/glm_repair/<fn>/`. Rejected answers and historical reports remain available. Every model attempt and review outcome is appended to `work/attempts/<fn>.jsonl`, using `glm-repair` or `glm-draft`. Oracle logs are isolated below the relevant scratch directories.

## Draft results

The initial coverage scan listed ten undone entries. Normal draft mode processed the nine requested entries other than `022137a0`. The six boundary rejections also received one shared GLM diagnosis call; that call is charged once in the cost table.

| Function | Result | Draft calls | Boundary diagnosis | Difftest / evidence |
| --- | --- | ---: | --- | --- |
| 02201102 | new, accepted | 1 | ordinary function | PASS-void, 16 vectors |
| 0220ff2e | skipped, interior | 0 | shared call | Inside 0220ff28 prologue/body |
| 02210068 | skipped, interior | 0 | shared call | Mid-arithmetic in 02210020; live registers and stack frame |
| 0221a5d0 | skipped, interior | 0 | shared call | Uses live r1/r12/r13 and pops an enclosing saved frame |
| 0221dfaa | skipped, interior | 0 | shared call | Jump-table dispatch within 0221df80 |
| 0221fdca | skipped, interior | 0 | shared call | Loop body in 0221fd7a, back-edge before this address |
| 0223845c | new, verified in final run | 1 | ordinary function | PASS, 16 vectors under v1.03; initial v1.02 gate was SKIP |
| 022386c6 | new, verified in final run | 1 | clean leaf | PASS-void, 16 vectors under v1.03; initial v1.02 gate was SKIP |
| 0224a928 | deferred runtime entry | 0 | shared call | Soft-float multiply, stack ABI and shared cold paths before entry |

During final verification the other agent installed difftest v1.03. The initial v1.02 SKIPs for 0223845c (unmapped 00000066) and 022386c6 (unmapped 86A67EB6) became PASS and PASS-void respectively in the v1.03 run, with no further C edits or GLM calls. All three drafts now have 16-vector passing evidence. No placeholder files were created for interior labels. `0224a928` is a real compiler-runtime entry with a special calling convention; its backward branches reach shared exception paths in the existing soft-float region. GLM identified the runtime family; the report does not adopt its speculative claim that every backward shared block must imply an earlier callable entry. A separate runtime/boundary treatment is needed. Existing call sites producing interior-label references are outside this batch scope.

## Driver behavior and verification

Without `--repair`, the v1.00 default queue, existing-file protection, one-plus-three call limit, and compiling-SKIP/unverified acceptance remain. `--allow-leaf` is an explicit draft option for clean, call-free, stack-free returning bodies; it admitted 022386c6 without weakening the default prologue check.

Repair mode includes current C, reachable SH-2 extent disassembly with literals, report first-difference evidence plus a fresh baseline, three neighbouring style examples, and available callee bodies. Observed indirect targets from call differences are included too. Replies carry either corrected C or an evidence-only diagnosis. Compilation and PASS/PASS-void are mandatory before atomic replacement. SKIP, compile failure, oracle error, or exhausted rounds retain the source. Source hashes protect backups, concurrent edits and repair resumption.

Three process-shared slot locks cap GLM calls across draft and repair processes. `glm_call.py` supplies quota pacing and 429 backoff. Drafting uses thinking off. Repair thinking is optional after two failed rounds via `--think-after-two`; this batch enabled it only for round three of 02233122. That call consumed its output budget without returning C; the final permitted round used thinking off. The final driver refines empty replies within the persisted call cap and reserves round four for nonthinking output. Oracle calls use `tools/decomp/venv/bin/python tools/decomp/difftest.py`; traceback retries wait and retry that same path, without editing the oracle.

Additional guards reject PASS obtained by dropping a previously compared return value or reducing declared callee/callback argument counts. Source-hash-matched report caveats protect documented ABI/domain adaptations from replacement. These guards were added after review caught the initial misleading proposals.

`work/glm_repair/check_driver.log` verifies strict SKIP rejection, backups, persisted four-call limit, thinking threshold, model-contract and return/call-ABI guards, floor deferral, mode-specific logs, repair resumption, and default draft SKIP acceptance. Real repair and draft resume checks made zero GLM calls. `git diff --check` passed.

Final verification called the installed oracle path, reporting v1.03. The 20-function run has 2 PASS, 14 PASS-void, 4 FAIL and 0 SKIP. Results are in `work/glm_repair/final-difftest.json`; all remaining FAILs are identified in the repair table.

Fresh whole-tree compilation covered **1,099 files with zero failures**. A combined relocatable link succeeded. The executable link still fails with **629 unresolved symbols**, including the missing standalone runtime entry and project externs. Artifacts: `work/glm_repair/tree-status.json`, `work/glm_repair/tree-link.txt`, `work/glm_repair/tree-executable-link.txt`. This is not executable-link success or a proof of semantic completion under STATE.md. Passing difftest remains bounded model evidence.

`work/glm_repair/final-audit.json` verifies every backup against the batch-start source hash, every retained artefact against its original, the per-function call limit, and that source changes comprise only accepted listed repairs plus the three new files. Other-agent oracle/test edits were left alone.

## GLM cost and time

| Work | Returned calls | Input tokens | Output tokens |
| --- | ---: | ---: | ---: |
| Repairs / review refinements | 26 | 178,874 | 63,914 |
| Drafting | 3 | 11,295 | 2,329 |
| Shared boundary diagnosis | 1 | 12,911 | 2,818 |
| Total | 30 | 203,080 | 69,061 |

Total usage: **272,141 tokens**. Batch execution/verification window: `2026-10-31T19:20:01.583390+00:00` to `2026-10-31T19:37:21.604836+00:00`, **17.33 minutes**. Sum of per-call elapsed times is **15.63 minutes**, including slot waits and quota pacing; concurrent calls overlap. These are client token/time measurements. The client does not report a currency charge. Raw usage files are retained with the prompts and answers. The recovered boundary-review parser accepted an extra deferral key without making a second model call.

## Coverage

Coverage rose from **99.1% to 99.4%**. The file count rose from 1,096 to 1,099; the reference universe stayed 1,106. This file-based callgraph metric does not prove equivalence or complete reachability.

```text
defined (files):                        1099
distinct referenced (canonicalized):    997
universe:                               1106
TRUE unique referenced-but-undefined:   7
TRUE CALL-GRAPH COVERAGE:               99.4%

non-canonical call-site references (naming cleanup targets): 0 distinct

--- the 7 TRUE unique undone (sorted) ---
0220ff2e 02210068 022137a0 0221a5d0 0221dfaa 0221fdca 0224a928

WARNING: func_ tokens without exactly 8 hex digits: 1 occurrences
  0223f10e.c:3: func_0223f2xx

files defining more than one func_: 0
```

The seven remaining entries are five interior-label references needing later call-site cleanup, the deferred 022137a0 boundary/name decision, and the 0224a928 runtime helper. The malformed-token warning is pre-existing.
