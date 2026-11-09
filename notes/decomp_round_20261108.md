# Functional decomp round — 2026-11-08

Starting commit: `98ca3ae`. Changes are confined to `project/src_functional/`, `tools/decomp/`, `notes/` and `work/`. No commit was made. The pre-existing `notes/pm_board.md` modification and untracked, byte-identical `tools/decomp/difftest_v104.py` were preserved. Concurrent additions under `port/` appeared during the run and were left untouched. Python tools use `tools/decomp/venv/bin/python`; full scans use at most four jobs. Evidence below is under `work/decomp_round_20261108/` unless another path is specified.

The nine staged candidates and `0221dc04` are installed. All ten reproduce FAIL before correction under v1.05 with hints, and all ten pass after correction: PASS-void at 16 ordinary vectors and 64 vectors with `--vary-stubs`. The ordinary results for the nine staged files are in `installed.json`; the final `0221dc04` ordinary result is in `triage_plain.json`. All ten varied results are in `installed_vary.json`. Original sources are retained in `originals/`, final sources in `candidates/`; the first installation's remaining `0221dc04` failure and source are retained in `installed.json` and `intermediate/0221dc04_first_callback.c` rather than overwritten.

| Function | v1.05 before | Installed correction | After, 16 | After, 64 + vary |
| --- | --- | --- | --- | --- |
| `0221b5a0` | FAIL | Restore ten-state spawn/transfer sequence, descriptor widths, timers and effects | PASS-void | PASS-void |
| `0221cad4` | FAIL | Restore eleven-state sequence, spawn callbacks, waits and branch/list selection | PASS-void | PASS-void |
| `02213ca4` | FAIL | Copy scratch vector in ROM order: z, y, x; retain SR.S=0 hint | PASS-void | PASS-void |
| `02223862` | FAIL | Advance context cursor before storing output character | PASS-void | PASS-void |
| `02223968` | FAIL | Same cursor-first output ordering | PASS-void | PASS-void |
| `0222d0e8` | FAIL | Add sixth segment in second limb chain; restore 15 submissions / 63 calls | PASS-void | PASS-void |
| `022469f2` | FAIL | Separate AND and OR stores to actor `state_flags` (+0xE0) | PASS-void | PASS-void |
| `02246254` | FAIL | Store next handler as HWRAM `0604a27e` | PASS-void | PASS-void |
| `02245632` | FAIL | Restore non-0..3 mode path: cluster-base peer, skip z window; mode 0 still returns | PASS-void | PASS-void |
| `0221dc04` | FAIL | Use HWRAM callbacks `0600bbcc`, `0603b828`, `0601e1a0` | PASS-void | PASS-void |

The staged rewrites use the existing semantic function/global aliases, task field accessors and address constants from `dh_names.h` / `dh_work.h`. Actor `state_flags` stays a named field. The scene-sequence payloads are not actor work: their +0x18 timers and +0x30 descriptors retain raw accesses. Existing `MEM*` accesses in `0221dc04` retain their volatile widths. No layout was imposed on an unproven payload.

`0221dc04` required more than its first callback correction. Fixing `0600bbcc` exposed the next incorrect cart-form callback at call index 1. The ROM listing `0221dc04.asm` and `ptr_resolve.py 21dc04` confirm all three PIC anchors/deltas; adding the runtime HWRAM relocation gives the values above. Its intermediate FAIL and final PASS records make this extension reviewable.

`02201590` now takes its BIOS-enable path when bit 5 is **clear**. In `listings.asm`, cart `2015f2..201612` (HWRAM `060055f2..06005612`) loads settings, shifts right by five bits, executes `tst r13,r2` with r13=1, then `bf 201610`. Thus a set bit branches to the zero store; a clear bit stores 1 at `060987d0` and calls the vector at `06000680` with r4=1. The source condition is changed only by adding `== 0`.

The ordinary 16-vector PASS-void was insufficient here: it exercised no writes or calls. `bit5_check.py` uses the same v1.05 comparison with identical initial-state edits on both sides: task state 0, boot mode 1, alternating bit 5 clear/set, and a seeded BIOS vector/signature. The old source FAILs on the missing BIOS call; the fixed source PASS-voids all 16 varied vectors, eight per bit value, with writes and calls in every vector. Ordinary varied vectors still PASS-void. Evidence: `bit5_before.json`, `bit5_after.json`, `bit5_default_after.json`. This focused fixture does not change the production tester or its hints.

All three retry-pointer FAILs are real C bugs. ROM evidence and verification are as follows.

| Function | ROM evidence / bug | Result after correction |
| --- | --- | --- |
| `02227a84` | `227b32` stores pair halfword +2, then `227b34` stores +0; C reversed them. The row/column matrix order already matches. | PASS-void, 16 retry vectors; PASS-void, 64 retry + varied vectors |
| `02238960` | `2389b8..2389cc` copies two longs and one halfword: five contiguous s16 values. C initialized only elements 0..2 from offsets 0/4/8, leaving two uninitialized. `2389da..2389e6` also passes callback `060419dc`; C omitted that argument and declared a one-argument prototype, which also hid the argument from the old call comparison. | PASS-void, 64 varied vectors with mapped owner/actor fixture; original C FAILs on the same fixture |
| `02238a9c` | `mova` HWRAM `0603cb04` + `4ed8` gives `060419dc`, not `0238d9dc`. The halfword stores use hex offsets 0x26/0x28 (decimal 38/40); C used decimal 26/28, including mirror-path updates. | PASS-void, 64 varied vectors with mapped owner/actor fixture; original C FAILs on the same fixture |

The two spawn functions retain an independent tester limit. Unconstrained `--retry-pointers` completes 14 vectors for `02238960`, then SKIPs on `00000008`; it completes eight for `02238a9c`, then SKIPs on `E39037AD`. The same limits occur with and without varied stubs. Static provenance describes their stack-spilled owner arguments as loads from private stack addresses, so it cannot reliably seed the nested owner/actor chain. Retry also cannot repair pointers loaded from immutable program bytes when a random argument aliases the mapped program image (e.g. argument `2605e79e`). These are reference setup failures, not C mismatches, and are retained in `triage_plain.json` / `triage_vary.json`.

`spawn_check.py` seeds the ROM-proven chain `arg0+20 -> parent`, `parent+8 -> actor`, a valid slot 0..7 and both facing values, before either side runs. It leaves data tables and returned stub objects unchanged and uses the ordinary v1.05 effect/call comparisons. `spawn_valid.json` records both original-source FAILs and both final 64-vector PASS-voids. This separates proven C corrections from the remaining input-generator limitation; it does not label the unconstrained runs PASS or enable retry by default.

The four segmentation suspects do not require splitting the existing C. `fn.py` already computes the correct code ends. The v1.05 report scans up to the next symbol (or its fallback cap), which can extend beyond the actual code and its literal pool. `bounded_shapes.json` repeats the report-only walk with the existing `fn.py` code end; every suspect then has **zero unreached returns**. No persistent boundary override exists in `fn.py` at this HEAD; `decomp_bounded.py` supplies the existing transient clamp mechanism, which these already-correct extents do not need.

| Suspect | Confirmed cart code end / pool end | Verdict and remaining work |
| --- | --- | --- |
| `022005fa` | `200624` / `200648` | Own `rts` at `200620`, delay store at `200622`. Later reset sibling starts `20064a`; it is outside the current function. Reference faults on raw literal `06280602`. |
| `02200c62` | `200c8c` / `200cb0` | Same sequence, `rts` at `200c88`, delay store at `200c8a`. Later sibling starts `200cb2`; outside current extent. Same raw-literal fault. |
| `02223c74` | `223cce` / `223cec` | Own restore/return ends at `223ccc`. The following `223cec: rts; nop` has no direct, absolute or local PIC reference found; no callable boundary is established. Current SKIP is its asynchronous polling loop hitting the step cap. |
| `022452be` | `24537a` / `245400` | All three reachable returns belong to this function. Literal bytes beginning `24531a` are not code. `24537a..245380` is an unreferenced epilogue fragment, already rejected in `notes/functional_band240.md`. Current SKIP is a private stack pointer passed to `dispatch_cmd`. |

`tails.asm`, `tail_refs.json` and `tail_pic_refs.json` retain the neighboring disassembly and reference scans. Absence of a reference in these scans is not proof of global unreachability. The early boot siblings and additional task-shaped code are drafting gaps, not glued bodies in the existing C. Their PC-relative longword literals straddle the suspicious boot pools; `pool_read.py` proposes halfword-swapped constants, but the raw executable oracle does not implement that interpretation. A split/data override cannot resolve this image/boot-mapping discrepancy. No speculative sibling C, ROM-byte repair, or emulator change was installed. This follows the requested document-instead exception for fixes needing more than a data override.

Tester promotion and whole-tree results are recorded below. `before.json` captures the initial tree; `pre_promotion.json` and `after.json` scan identical corrected C with the old and promoted testers respectively. Exit status 1 reflects the seven retained FAILs, not an interrupted scan.

| Full scan, 16 vectors | PASS | PASS-void | FAIL | SKIP |
| --- | ---: | ---: | ---: | ---: |
| Initial v1.04, original source | 110 | 675 | 16 | 298 |
| v1.04, corrected source, immediately before promotion | 110 | 679 | 7 | 303 |
| Promoted v1.05, corrected source, varied stubs by default | 110 | 685 | 7 | 297 |

`audit.json` and `status_by_file.tsv` compare every one of the 1,099 addresses. All **785 initially passing functions** and all **789 passing immediately before promotion** retain their exact PASS/PASS-void status: **zero regressions**, including zero PASS-to-SKIP moves. Against the initial tree, nine FAILs become PASS-void and `0221dc04` moves SKIP to PASS-void. Across promotion alone, the five oracle-limit functions plus `0221dc04` move SKIP to PASS-void; every other status is unchanged. All pre/post-promotion source hashes match, the final source manifest matches, and the final report confirms `vary_stubs: true`, `retry_pointers: false`. The default run still SKIPs the three retry-triaged functions; their stronger verification is recorded separately above.

The v1.04 rollback is retained at `tools/decomp/difftest_v104.py`: it was byte-identical to the original live tester, so `difftest_v104_live.py` is not needed. Promotion copies v1.05 into `difftest.py`, changes its CLI to varied stubs by default with `--no-vary-stubs` available, and updates its explanatory header. `difftest_v105.py` remains the original opt-in version. Four-tuple `test_one` callers retain the compatibility contract. `--retry-pointers` remains off by default. `promotion.json` records hashes and the backup decision.

The existing v1.04 regression suite initially passed **52/54** against v1.05 here. Two mutation fixtures were stale after readability renames: the matrix-cursor accessor text and the actor pose-handler call. The latter matched an old comment and therefore changed no executable C. Both fixtures now mutate the current executable statements and still require FAIL. The third failure mentioned in `notes/difftest_v105.md` (coverage division by zero) did not reproduce from the repository root; its test passes without changing the coverage tool. After the fixture corrections, both v1.04 and v1.05 pass **54/54**. The promoted live tester also passes **54/54**. Logs: `regressions_before.log`, `regressions_v104.log`, `regressions_v105.log`, `regressions_live.log`.

The whole-tree compile gate passes: **1,099/1,099** GCC `-O1` objects; **zero failures**; **16 translation units with diagnostics**; `ld -r` succeeds. `build_check.py`, `build.log` and `build/report.json` retain commands and diagnostics. The diagnostic-bearing TU count is 16, compared with 17 in the recorded readability-pass-2 build; `0221b5a0` is the sole TU removed from that diagnostic set. A source hash manifest is retained in `source_manifest.json`.

`notes/floors.tsv` closes the five oracle-limit functions in their existing three grouped rows, with status `closed-by-difftest` and evidence `notes/difftest_v105.md`. New `lever-exists` closed rows cover the callback, bit-5 and retry C corrections. New `tool-limit` rows retain the two spawn input limits, boot-pool mismatch, polling loop and private-stack arguments. A `drafting-gap` row covers the two early reset siblings; `noise` reviewed rows cover the unreferenced return/epilogue fragments. A final `lever-exists` / `needs-triage` row records the four remaining pre-existing FAILs without prior floor rows; existing unrelated floors remain unchanged. Per-function attempt records were appended in `work/attempts/`; raw scan attempts also live beside each report.

Open issues: the reference setup and boundary-related limits listed above remain. These seven default FAILs are unchanged from the initial scan and were outside the requested correction set:

| Function | Final first difference | Floor treatment |
| --- | --- | --- |
| `02202d7e` | Memory `002800c0`: reference 06, C 02 | Existing historical artefact row retained; not re-triaged here |
| `02213e0c` | Call 0 arguments differ | New `lever-exists` / `needs-triage` row |
| `02213f90` | Extra C write at `002810f4` | Same new triage row |
| `0221a452` | Extra C write at `00280400` | Existing historical artefact row retained; not re-triaged here |
| `0222bdf2` | Memory `002800e1`: reference e8, C f4 | Existing historical artefact row retained; not re-triaged here |
| `02244820` | r0: reference `00280068`, C 0 | New triage row |
| `02249240` | Memory `002800d8`: reference 7f, C ff | New triage row |

Historical artefact classifications are not a claim that the current FAILs were re-proven harmless. No semantic change was made to these sources. Passing the tested vectors establishes equivalence within difftest's ABI/stub/input model, not complete path coverage.

Reproduce the main checks from the repository root:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --json work/decomp_round_20261108/recheck.json --output-dir work/decomp_round_20261108/recheck
tools/decomp/venv/bin/python work/decomp_round_20261108/bit5_check.py after
tools/decomp/venv/bin/python work/decomp_round_20261108/spawn_check.py
DIFFTEST_MODULE=difftest tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v104.py
tools/decomp/venv/bin/python work/decomp_round_20261108/build_check.py
tools/decomp/venv/bin/python work/decomp_round_20261108/audit.py
```

1. Nine staged fixes installed with readability-pass-2 names.
2. All ten requested installs pass 16 ordinary and 64 varied vectors.
3. `0221dc04` now uses the three proven HWRAM callbacks.
4. The bit-5 inversion is fixed and its focused vectors pass.
5. Three retry-exposed C bugs are fixed; residual pointer limits are documented.
6. Four segmentation suspects were reviewed; no existing C boundary needs splitting.
7. v1.05 is live with varied stubs by default and a preserved v1.04 rollback.
8. Final --all: 110 PASS, 685 PASS-void, 7 FAIL, 297 SKIP; zero PASS regressions.
9. All 1,099 C files compile and link; v1.04, v1.05 and live suites each pass 54/54.
10. Five oracle-limit floors are closed, remaining blocks are registered, and nothing was committed.
