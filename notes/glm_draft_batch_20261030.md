# GLM drafting batch, 2026-10-30 queue

Executed on 2026-10-31 UTC. Driver: `tools/glm/draft_batch.py` v1.00. No commit.

## Results

The queue contains 52 entries (50 hidden candidates, 02204258, and the confirmed sibling 02204308). Accepted 51 new C files: 1 difftest PASS/PASS-void and 50 compiling SKIPs, explicitly **unverified**. 0 failed entries remain. 022137a0 was analysed and deferred as requested. 0 entries were rejected as not-real. All 52 queue entries are real; 02204308 was already tracked before this session, so it was retained and checked in place (compile + SKIP/unverified), with zero GLM calls.

“Accepted” follows the requested draft gate; it does not mean whole-tree link success or proof of equivalence. Every GLM draft/refinement and boundary deferral has a `glm-draft` attempt record. Difftest also writes its own oracle records. Existing C files were not overwritten.

## Driver and validation

Boundary checks use SH-2 reachability, delay slots, literal-pool annotations, entry prologues, and preceding rts/bra/braf/jmp boundaries. Computed braf tails terminate the body, preventing the existing fn.extent heuristic from swallowing siblings. Pool and mova+delta call targets are resolved for the prompt. Demonstrable overlap with an existing valid prologue is rejected; a missing clean predecessor alone is not enough to reject an entry. `--inspect` produces boundary evidence without model calls.

Prompts contain reachable disassembly, m2c_fn.py output from the local decomp venv, three nearby C style examples, and available callee definitions reduced to prototypes. Drafting invokes glm_call.py without --think. The driver caps jobs at three and uses three process-shared call-slot locks; glm_call.py supplies the shared >=2s pacing and 429 backoff. It compiles with gcc -O1 -c -I project/src_functional, then invokes the existing difftest.py address/--candidate interface. A FAIL or compile failure receives feedback, with at most three refinements after the initial draft per run. Transient oracle tracebacks are retried rather than editing difftest. Per-function transaction locks, pass-log checks, and exclusive destination creation prevent overwrites and support resuming.

The pilot was 02201020, 0220c3e0, and 02204258. It exposed a Python-environment issue and a braf target annotation error; neither faulty draft was accepted. The environment and target handling were corrected, and the corrected pilot finished before the full run. Its final results were SKIP/unverified, SKIP/unverified, and PASS-void respectively. The first pilot made five discarded GLM calls; those calls and the transient traceback retries are included in the cost and attempt history.

Checks in work/glm_draft/check_driver.log exercise refinement feedback, the four-call limit, attempt logging, pass resumption, exclusive destination handling, and SKIP/unverified acceptance. ROM checks cover queue cardinality, data rejection, braf extent/target resolution, and the misnamed 02228ca2 predecessor. The final audit in work/glm_draft/final-audit.txt confirms all 52 classifications, 51 new-file copies, pass logs, no more than four GLM calls per function, and at least two style examples in every prompt. A real resume check skipped the three pilot functions with zero new model calls.

After the external interruption, on-disk reports showed that the batch subprocess had continued to completion at 18:40 UTC. Its only failed entry was 02210b58 after four calls: the final draft used f64 without declaring that alias. Recovery added only typedef double f64 to the scratch C, then compiled and ran difftest before exclusive creation of its new destination. It received SKIP/unverified; no fifth GLM call was made. The driver prompt now explicitly documents accessor arity and float aliases.

## Per-function table

GLM rounds counts every returned drafting call, including discarded pilot calls. The parenthesized number is the final run when different. PASS and PASS-void use the default 16-vector oracle run; SKIP has no equivalence claim.

| Function | Real/not-real | Result | Difftest | GLM rounds | Finding |
|---|---|---|---|---:|---|
| 02201020 | real | accepted, unverified | SKIP | 3 (1 final) | callee has fewer/more than four explicit register arguments |
| 0220c3e0 | real | accepted, unverified | SKIP | 2 (1 final) | raw pointer/address/sizeof operation |
| 0220c858 | real | accepted, unverified | SKIP | 1 | reference cannot run: unmapped 2-byte access at 6A6401D3 |
| 0220cdf6 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 0220d424 | real | accepted, unverified | SKIP | 2 | local array/function declaration |
| 0220e5bc | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 02210b58 | real | accepted, unverified | SKIP | 4 | Four GLM calls; manual missing-f64 typedef repair, then compile + SKIP. |
| 02218eac | real | accepted, unverified | SKIP | 1 | local array/function declaration |
| 0221a9e0 | real | accepted, unverified | SKIP | 3 | compound memory assignment |
| 0221ab1c | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 0221b5a0 | real | accepted, unverified | SKIP | 3 | unresolved callee: func_0221a5d0 |
| 0221b730 | real | accepted, unverified | SKIP | 2 | raw pointer/address/sizeof operation |
| 0221b8e4 | real | accepted, unverified | SKIP | 1 | compound memory assignment |
| 0221bb40 | real | accepted, unverified | SKIP | 2 | callee has fewer/more than four explicit register arguments |
| 0221bd08 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 0221bdce | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 0221cad4 | real | accepted, unverified | SKIP | 2 | reference cannot run: unmapped 4-byte access at 80000008 |
| 0221cd58 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 0221cfa4 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 0221d59c | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 0221d910 | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 0221da60 | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 0221debc | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 0221df80 | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 0221ec08 | real | accepted, unverified | SKIP | 2 | raw pointer/address/sizeof operation |
| 0221edec | real | accepted, unverified | SKIP | 3 | raw pointer/address/sizeof operation |
| 0221f9c4 | real | accepted, unverified | SKIP | 3 | callee has fewer/more than four explicit register arguments |
| 02222efc | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02228874 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02228aa6 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02228ca4 | real | accepted, unverified | SKIP | 1 | True prologue; existing 02228ca2.c starts at the previous tail delay slot. |
| 02229054 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 022294b4 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 022383f8 | real | accepted, unverified | SKIP | 2 | callee has fewer/more than four explicit register arguments |
| 022384e4 | real | accepted, unverified | SKIP | 2 | callee has fewer/more than four explicit register arguments |
| 0223875c | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 022387e0 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 022387fc | real | accepted, unverified | SKIP | 2 | callee has fewer/more than four explicit register arguments |
| 022388b8 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02238960 | real | accepted, unverified | SKIP | 1 | local array/function declaration |
| 02238a9c | real | accepted, unverified | SKIP | 2 | local array/function declaration |
| 0223f95c | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 02241b9c | real | accepted, unverified | SKIP | 2 | raw pointer/address/sizeof operation |
| 02241bee | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02241c40 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 02241d18 | real | accepted, unverified | SKIP | 2 | memory increment or unary operation |
| 02247530 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 02247580 | real | accepted, unverified | SKIP | 1 | raw pointer/address/sizeof operation |
| 02248318 | real | accepted, unverified | SKIP | 1 | callee has fewer/more than four explicit register arguments |
| 02248f86 | real | accepted, unverified | SKIP | 2 | callee has fewer/more than four explicit register arguments |
| 02204258 | real | accepted | PASS-void | 3 (1 final) |  |
| 02204308 | real | existing, unverified (skip write) | SKIP | 0 | Already tracked; retained. Tail at 20435a resolves to 022154fc. |
| 022137a0 | real | skip | not run | 0 | Deferred boundary pass; cold block at 21377c belongs to this entry. |

## Boundary finding: 022137a0

022137a0 is a real callable entry at cart 0x2137a0, where it saves r10/r9/r8 and establishes the matrix, source, and vertex-count registers. Its main body returns at rts 0x213892 with the delay slot ending at 0x213896; trailing pool/padding extends to 0x2138b8. The preceding 0x21377c is an out-of-line block of this function, not a separate ABI entry. The bt/s at 0x2137e6 jumps backward to 0x21377c after preparing a three-word stack coordinate vector and clearing MAC. That block uses live r10 (matrix), r1 (coordinate vector), and r15 (this function's frame), adds matrix+0x60 offsets, selects matrix+0x30, and bra at 0x213794 rejoins the multiply body at 0x2137ec. Its two literals occupy 0x213798..0x2137a0. The physical layout is cold block + its pool, entry/prologue, main body with an embedded pool at 0x213808..0x213816, epilogue, trailing pool.

Existing project/src_functional/0221377c.c already describes this complete algorithm as vertex_project_dualmtx_m(count, verts, M), but its filename names the cold block and it supplies no func_022137a0 definition. A future boundary/name pass should move/canonicalize that implementation and validate it, rather than create another duplicate body. No existing file was edited. Evidence: work/glm_draft/022137a0/boundary-disasm.txt and work/attempts/022137a0.jsonl.

## Other boundary finding

At cart 0x228ca0, braf r3 tail-calls 0x22dc2c; 0x228ca2 is its pop-r14 delay slot. The next independent prologue starts at 0x228ca4 (push r14, then save PR), and returns at 0x228cd2 with its delay slot through 0x228cd6. Existing 02228ca2.c describes the handler at 02228ca4 under the wrong address. The new canonical entry is allowed by the new-files-only scope; the old misnamed implementation remains for a separate cleanup.

## Cost and elapsed time

Actual GLM usage: **89 returned calls**, **721,763 input tokens + 179,666 output tokens = 901,429 tokens**. This includes the discarded first pilot and all refinements. The client exposes token counts, not billed currency; no dollar charge is available from its output. Original drafting window, from first pilot prompt to batch completion: **19.05 minutes** (2026-10-31T18:21:09.968393+00:00 to 2026-10-31T18:40:12.718210+00:00). Full-run timing: {"start": "2026-10-31T18:25:17.061736+00:00", "end": "2026-10-31T18:40:12.718210+00:00", "seconds": 895.656474, "recovery_started": "2026-10-31T19:02:39+00:00", "recovery_finished": "2026-10-31T19:06:20.751951+00:00"}. Raw responses, prompts, usage lines, compiler output, oracle JSON, archived run reports, and final reports remain in work/glm_draft/<fn>/.

## Tree checks and coverage

Final compilation: Tree compile: 1096 files; 0 failures. Whole-tree relocatable linking with gcc -r succeeded for all 1,096 objects (work/glm_draft/tree-link-status.json). An executable link still fails: the aggregate has 631 unresolved symbols, including main, guest globals, descriptive-name aliases and uncovered callees. The C tree has no standalone host runtime entry. No executable-link success or fully verified completion under STATE.md is claimed.

Executable link excerpt (work/glm_draft/tree-executable-link.txt):

```
/usr/bin/x86_64-linux-gnu-ld.bfd: work/glm_draft/functional-tree.o: warning: relocation against `g_06090118' in read-only section `.text'
/usr/bin/x86_64-linux-gnu-ld.bfd: /usr/lib/gcc/x86_64-linux-gnu/15/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
(.text+0x1b): undefined reference to `main'
/usr/bin/x86_64-linux-gnu-ld.bfd: work/glm_draft/functional-tree.o: in function `func_02201020':
(.text+0x24d): undefined reference to `func_02201102'
/usr/bin/x86_64-linux-gnu-ld.bfd: (.text+0x417): undefined reference to `func_022386c6'
/usr/bin/x86_64-linux-gnu-ld.bfd: work/glm_draft/functional-tree.o: in function `func_0220121e':
(.text+0x464): undefined reference to `g_060E0DD0'
/usr/bin/x86_64-linux-gnu-ld.bfd: (.text+0x46d): undefined reference to `g_060E0DD0'
/usr/bin/x86_64-linux-gnu-ld.bfd: (.text+0x47a): undefined reference to `g_06098814'
/usr/bin/x86_64-linux-gnu-ld.bfd: work/glm_draft/functional-tree.o: in function `func_0220123c':
(.text+0x496): undefined reference to `g_060E0DD0'
```

Baseline: 1,045 defined files, 935 distinct canonical references, universe 1,047, 2 referenced-but-undefined, 99.8% reported coverage. Final output from python3 tools/decomp/coverage_callgraph.py:

```
defined (files):                        1096
distinct referenced (canonicalized):    996
universe:                               1106
TRUE unique referenced-but-undefined:   10
TRUE CALL-GRAPH COVERAGE:               99.1%

non-canonical call-site references (naming cleanup targets): 0 distinct

--- the 10 TRUE unique undone (sorted) ---
02201102 0220ff2e 02210068 022137a0 0221a5d0 0221dfaa 0221fdca 0223845c 022386c6 0224a928

WARNING: func_ tokens without exactly 8 hex digits: 1 occurrences
  0223f10e.c:3: func_0223f2xx

files defining more than one func_: 0
```

The metric changes from 99.8% to 99.1% because these previously hidden bodies expose additional references. The remaining ten entries are outside this fixed queue (022137a0 is explicitly deferred). The file-based coverage metric does not prove reachability completeness or semantic verification. The single malformed-token diagnostic is in an existing comment in 0223f10e.c; this batch introduces none.
