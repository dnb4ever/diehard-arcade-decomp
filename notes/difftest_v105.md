# difftest v1.05: vector hints, pointer retry, report-only coverage (2026-11-08)

Tool: `tools/decomp/difftest_v105.py` (new file) + `tools/decomp/difftest_hints.json`.
`tools/decomp/difftest.py` (v1.04) was not touched. Rollbacks: `difftest.py` (v1.04), `difftest_v103.py`, `difftest_v102.py`.
Evidence lives in `work/difftest105/`. Nothing under `project/` was edited and nothing was committed.

Installation follow-up (2026-11-08): the nine candidates and `0221dc04` callbacks are installed and verified; the bit-5 inversion and three retry-exposed C bugs are corrected. The live tester is now v1.05 with varied stubs by default. Two spawn functions still have unconstrained pointer-setup SKIPs; focused mapped-chain vectors pass. Full evidence, boundary review and status tables: `notes/decomp_round_20261108.md`. The original measurements below describe the pre-install snapshot.

## Bottom line

* **v1.05 with default flags gives v1.04's statuses on every function but one**, across all 1,099 sources. That one is `0221dc04`, which moves SKIP -> FAIL because of a new hint. The FAIL is a real C bug.
* **All 5 oracle-limit floors close.** Each one's attempt C (`work/difftest105/candidates/<fn>.c`) gets PASS-void under v1.05, at 16 vectors and again at 64 vectors with `--vary-stubs`. Without hints the same files still SKIP, and the committed C FAILs under v1.05. So the hints unblock the oracle; they don't hide bugs.
* **The 3 FAILs are real C bugs**, each fixed by a one-line or one-block candidate that PASSes. **The 2 "SKIPs" are not SKIPs on the current tree.** `02202a28` already PASS-voids under v1.04 and v1.05. `02245632` is a real FAIL: the C drops a path the ROM takes. Its candidate PASSes.

## What changed (v1.04 -> v1.05)

| Change | Default | Effect on v1.04 behaviour |
| --- | --- | --- |
| **(a) Per-function hints** in `difftest_hints.json`. A hint can do any of these. `args.<i>.range`: pin an argument. `args.<i>.fields`: seed a struct at the argument, with each field `range`, `value` or `pointer` (to a fresh scratch block). `pointer_returns`: extra callees whose stub result is an object pointer, applied identically in the reference and the C harness. `sr_s`: pin SR.S on both reference runs. `max_steps`: reference step cap. `retry_pointers`: per-function opt-in. Hint values come from a separate per-vector RNG, `(0x4a1e0000 ^ entry)*4096 + vector`, so the v1.04 RNG stream is never perturbed. Hints are applied after v1.04's `structure_inputs`, before either side runs, and never depend on the candidate. | on (`--no-hints` turns them off) | only the hinted functions |
| **(b) `--retry-pointers`**. When the reference faults on an unmapped access, the vector is retried, deterministically, at most 16 times. Case 1: the fault is within 64 KiB of an argument value. That argument is remapped to `0x00230000 + 0x1000*i + 0x800`. Case 2: otherwise, the fault is within 64 KiB of a value the reference loaded (4-byte) from seedable initial RAM before writing it. That RAM word is repointed to `0x00238000 + 0x400*k`. Changes are recorded in `input_setup`. If a fault is near a stub return value, the SKIP reason names the callee and suggests a `pointer_returns` hint. | off | none unless the flag or a hint enables it |
| **(c) SR.S / MAC**. The emulator already models 48-bit `mac.l` saturation. What was missing is the game's SR.S state: the two oracle runs seed S=1 and S=0 from noise. The `sr_s` hint pins it. Evidence for S=0: all six real `ldc Rm,SR` sites in the program image (0600418e, 060045f4, 060047f6, 06004c5c, 06005036, 060050d4) are `stc sr` / `and` with the mask `0xff0f` / `ldc`, which preserves S, and every `0x0058` (`sets`) word is in data (pools/tables). The BIOS hand-off state is assumed to be the reset value, S=0. | hint only | `02213ca4` only |
| `--vary-stubs`: a per-vector salt XORed into every stub return value (Python `stub()` and C `dt_callv` alike, passed as `argv[3]`). Without it, a zero-argument callee returns the same value on every vector. | off | none |
| Report-only JSON fields: `effects` (vectors with writes / with calls), `branch_coverage` (`hit` = distinct (pc, taken) conditional-branch directions on the main reference run; `total` = 2 x the conditional branches reachable statically from the entry, plus any executed beyond a switch), and `segmentation` (static walk: rts count, unreached code words, unreached rts, switch flag). | on | statuses and reasons unchanged |
| `test_one` accepts v1.04's 4-tuples (= no hints, no retry). FAIL vectors now fall through to the report-only fields. | | |

Regression tests: `DIFFTEST_MODULE=difftest_v105 venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v104.py` gives 51/54. Run against `difftest` (v1.04), the result is identical: the same 3 already fail there because the source tree drifted (renamed `MEMU32(0x06098878)` text, `0222c322` mutation, and `coverage_callgraph.py` division by zero). No `tools/decomp/tests/` directory exists, so no new tests were added.

## v1.04 vs v1.05 status diff (`--all`, frozen snapshot, `--jobs 4`)

Method: `work/difftest105/snap/` is a frozen copy of `project/src_functional` (manifest `work/difftest105/snap_manifest.sha256`), `notes/symbols.txt`, and both harnesses. That way the concurrent Codex/source jobs could not change inputs between runs. Runner: `work/difftest105/run_all.sh`. Diff: `work/difftest105/compare.py`. Final v1.05 sha256 is `e9a17371...`, hints `893c1ddf...`.

| Run | PASS | PASS-void | FAIL | SKIP | Status moves vs v1.04 |
| --- | ---: | ---: | ---: | ---: | --- |
| v1.04 (`runs/v104`) | 110 | 675 | 16 | 298 | — |
| v1.05 default (`runs/v105_final`) | 110 | 675 | 17 | 297 | **`0221dc04` SKIP -> FAIL** (hint) |
| v1.05 `--no-hints` (`runs/v105_nohints`) | 110 | 675 | 16 | 298 | none, reasons identical |
| v1.05 `--vary-stubs` (`runs/v105_vary`) | 110 | 675 | 17 | 297 | `0221dc04` only; **no PASS became FAIL** |
| v1.05 `--retry-pointers` (`runs/v105_retry`) | 111 | 693 | 20 | 275 | 23 SKIP -> out (below) |

Explanations:
* **`0221dc04`, SKIP -> FAIL (default).** This is a sibling FSM of `0221b5a0`/`0221cad4` with the same `0602e878` descriptor-pointer return. The `--retry-pointers` diagnostic pointed at it, so it got a `pointer_returns` hint. With valid descriptors, vector 0 shows a real bug: the handler argument is cart `0220bbcc` where the ROM passes HWRAM `0600bbcc` (call 0 to `06013f54`). This is the same class as the Codex mechfails.
* Under the default run, only the FAIL detail of the hinted `02223862`/`02223968` changes. The hinted vector is used, and the first difference becomes the cursor-order bug at trace index 8.
* `--retry-pointers` moves 23 SKIPs out. 19 go to PASS/PASS-void: 02203576 0220376c 02203afc 02203c28 022094c4 022095cc 0220a7dc 0220b354 0220b85c 0220bdb4 0220e000 0221ffb0 02227906 02235dc0 0223f494(PASS) 02241324 02246308 022463b8 022482dc. 4 go to FAIL: `0221dc04` (above), and `02227a84`, `02238960` and `02238a9c` (open issues). Each had faulted through a random pointer field, or argument, that v1.04's static inference did not seed. 24 other SKIPs keep SKIP with a different reason, e.g. a deeper pointer, a step cap or a private stack address. All runs are in `runs/v105_retry/difftest_all.json`.

## Per-function results (v1.05, live tree, 16 vectors unless noted)

| Fn | Committed C | Candidate / attempt | Class | Evidence |
| --- | --- | --- | --- | --- |
| `0221b5a0` | FAIL v0 (mem `00280410`) | **PASS-void** 16 and 64+vary; SKIP without hint | oracle limit, closed by `pointer_returns:[0602e878]`. Committed C is buggy, attempt correct | `work/difftest105/oracle_candidates/`, `final10/` |
| `0221cad4` | FAIL v0 (mem `002a0418`) | **PASS-void** 16 and 64+vary; SKIP without hint | same | same |
| `02213ca4` | FAIL v0 (trace order: x,y,z vs ROM z,y,x) | **PASS-void** 16 and 64+vary; SKIP without hint | oracle limit (SR.S), closed by `sr_s:0`. Store-order bug in the committed C | same |
| `02223862` | FAIL v0 trace #8 (char stored before cursor advance) | **PASS-void** 16 and 64+vary, coverage 20/22; SKIP (step cap) without hint | oracle limit (ctx width), closed by ctx-struct hint + `max_steps`. Store-order bug in C | same |
| `02223968` | FAIL v0 trace #8 | **PASS-void** 16 and 64+vary, coverage 16/18 | same | same |
| `0222d0e8` | FAIL calls #59: ROM makes 63 calls / 15 `submit_model`, C makes 59 / 14 | **PASS-void** 16 and 32+vary | **real C bug**: the second limb chain (after the first `MTX_TOP -= 0x90`) has 6 push/apply/rotate/submit segments in ROM, the C had 5 | interleaved event order: `work/difftest105/events.py 0222d0e8` |
| `022469f2` | FAIL trace #18: ROM stores `+0xD8` twice (`&= 0x20320`, then `|= 1`; 02246a90..a9c), C once | **PASS-void** 16 and 32+vary | **real C bug** (missing intermediate store; final footprint was equal) | `work/difftest105/022469f2.asm` |
| `02246254` | FAIL mem `00280000`: ref `06`, C `02` | **PASS-void** 16 and 32+vary | **real C bug**: next-frame handler stored as cart `0224627E`. ROM `mova` gives HWRAM `0604A27E` | reference events: `W 00280000/4=604a27e` |
| `02245632` | FAIL calls #0: ROM calls `func_02245fc4(actor, 06099584)`, C returns | **PASS-void** 16 and 32+vary | **real C bug** (deliberate simplification). For combat mode not in 0..3, ROM 0224566a branches to 022456fa with the cluster base `06099584` as peer and skips the z window. The C turned it into `return`, claiming it was dead. Not a SKIP on the current tree, under v1.04 or v1.05. | listing `fn.py 0x245632` |
| `02202a28` | **PASS-void** (v1.04 and v1.05), coverage 7/8, writes+calls on 16/16 vectors | — | not a SKIP on the current tree; nothing to do (the SKIP report is stale: `git status` shows the source modified since commit `be93c6e`) | `final10/02202a28.cur.json` |

## Recommended installs (owner of `project/src_functional` to apply)

All candidates compile with the whole-tree gate (`gcc -O1 -c -I project/src_functional <file>`) and PASS-void under v1.05.
1. `0221b5a0.c`, `0221cad4.c`: the Codex attempt rewrites, verbatim. They are based on the pre-readability source and use raw offsets (`M8(task,0x10)`) instead of `TASK_S8(...)`/`dh_work.h`. Install as-is, or re-apply the readability renames afterwards and re-run `difftest_v105.py <fn>`.
2. `02213ca4.c`: the attempt's z,y,x scratch-copy order, rebased on the current `g_matrix_stack_cursor` names.
3. `02223862.c`, `02223968.c`: the attempts verbatim; the current source equals the pre-attempt original.
4. `0222d0e8.c` (add the missing 6th segment), `022469f2.c` (two stores), `02246254.c` (HWRAM handler), `02245632.c` (faithful default path). These are new fixes from this session.

Verify after install: `tools/decomp/venv/bin/python tools/decomp/difftest_v105.py 0221b5a0 0221cad4 02213ca4 02223862 02223968 0222d0e8 022469f2 02246254 02245632 --jobs 4`. Expect 9 PASS-void. Under v1.04 the first five will still SKIP; that is expected.

## Proposed `notes/floors.tsv` row changes (not applied)

```
0221b5a0,0221cad4	oracle-limit	closed-by-difftest	difftest v1.05 hint pointer_returns 0602e878; install work/difftest105/candidates/<fn>.c (PASS-void)	notes/difftest_v105.md	claude
02213ca4	oracle-limit	closed-by-difftest	difftest v1.05 hint sr_s=0 (game never sets S); install candidate (PASS-void)	notes/difftest_v105.md	claude
02223862,02223968	oracle-limit	closed-by-difftest	difftest v1.05 printf-ctx hint + max_steps; install candidates (PASS-void)	notes/difftest_v105.md	claude
0221dc04	c-bug	open	unblocked by v1.05 hint; handler arg cart 0220bbcc vs HWRAM 0600bbcc (mechfail class)	notes/difftest_v105.md	claude
```
The "closed" status should be applied only once the candidates are installed and verified. Until then, `lever-ready` is accurate.

## Peer-review scope items

1. **PASS-void meaning.** v1.04 already compares the full byte-normalised write footprint, the chronological byte-level write trace (address, value, order) and the callee trace (target, args, order). So PASS-void means effects matched, not just "no return value". Widths are normalised to bytes on purpose, because optimised C may split or merge stores. The statuses were not split, since that would break the identical-status requirement. Instead, `effects` shows that **88 of 675 PASS-voids had no writes and no calls on any vector** (unverified in practice; usually an early exit on every random vector). The list is in `work/difftest105/runs/v105_final_analysis.txt` §1. A future `PASS-void-unverified` label could key on `effects`.
2. **Stub returns.** v1.04 stub values are a hash of the callee and its args, not 0. They do vary per vector whenever the args vary, but a zero-argument callee repeats its value every vector. `--vary-stubs` adds a deterministic per-vector salt. The full run moved **no** PASS to FAIL; only `0221dc04` (from the hint) changed. It is safe to make it the default.
3. **Path coverage** (report only). Of 785 PASS/PASS-void: 185 have no conditional branches, 93 are at 100%, 83 at 75% or more, 171 at 50% or more, and **253 below 50% (117 below 25%)**. The worst include `02247a8c` 1/66, `02247106` 1/50, `02229074` 1/48 and `02246ab8` 1/44. The full list is in §3 of the analysis file. Caveats: coverage comes only from the main reference run, and `total` includes branches no input reaches.
4. **SKIP histogram** (default, 297). The biggest tester limitation, by far, is random pointer fields: 78 SKIPs fault on unmapped addresses that came from random RAM. `--retry-pointers` already clears 23 of them, and making it the default after triaging the 3 new FAILs is cheap. The next classes, with the cheap fix where there is one:

   | Count | Class | Cheap to fix? |
   | ---: | --- | --- |
   | 36 | compiler-runtime helper ABI (r0/r1 helpers, `__divdf3` etc.) | no; asm floor |
   | 25 | other: 12 unknown/ambiguous indirect arg count, 3 unresolved callee, misc | partly; per-callee prototypes |
   | 24 | step cap on random input | yes, per-function `max_steps` / range hints, as for the printf pair |
   | 24 | C constructs the generator doesn't model (shadowed locals, struct return, address-taking) | medium |
   | 24 | hardware / bus (FFFFFF00 on-chip, 25Dxxxxx SCU, 20100063 SMPC) | no; unmodelled hardware |
   | 23 | private stack address passed to callee | no; by design |
   | 22 | unresolved identifier (`reg_*`, `g_curActorObj`) | yes; header/symbol fix |
   | 17 of the 78 | HWRAM beyond 1 MiB (`0628xxxx`, `063xxxxx`) | yes; model the HWRAM mirror (fold 0x06000000-0x07FFFFFF mod 1 MiB, both sides) |
   | 14 | variadic callee ABI | medium |
   | 11 | reads of uninitialised registers (noise `13579xxx`) | no; usually a C-side argument-count issue, worth a look |
5. **Segmentation suspects** among FAIL/SKIP (static walk finds an rts no path from the entry reaches, outside switches): `022005fa` (3 unreached rts from 0600464a), `02200c62` (6 from 06004cb2). These look like unnamed glued functions in early init code. Also `02223c74` (lone rts at 06027cec, likely a shared epilogue), `022452be` (one unreached rts at 0604931c), and the runtime `0224a830`/`0224abb6`. Recommendation: check the split with `tools/decomp/fn.py`; do not change the C. No FAIL was flagged.

## Open issues

* `--retry-pointers` exposed 3 new FAILs to triage: `02227a84` (write order `0609a092` vs `0609a090`), `02238960` (write to the stub object region `0021e09a`) and `02238a9c` (call 0 arg 2: C passes `0238d9dc`, ROM passes HWRAM `060419dc`). Triage these before turning retry on by default.
* `0220fd52` and `02215228`: given a `pointer_returns`/retry hint, the reference then hits the step cap (`06013D84`, `0601936C`). They need range hints on the loop bounds.
* `0221dc04`: the C fix (HWRAM handler) has not been written yet.
* The `options.hinted` field in the report lists canonical HWRAM addresses (e.g. `0601f5a0` for `0221b5a0`).
* Housekeeping: during the comparison, a duplicate copy of my runner briefly ran a second `--jobs 4` run in parallel (about 7 minutes, my processes only; since stopped). All reported runs are complete single runs. The `runs_old/` directory holds superseded runs that used an earlier v1.05 file; their statuses are identical to the final ones.
