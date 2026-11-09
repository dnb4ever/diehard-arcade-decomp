# Functional decomp round 2 — 2026-11-08

v1.06 is live. The unverified tail shrinks from **304 FAIL/SKIP to 251 SKIP**: **118 PASS / 730 PASS-void / 0 FAIL / 251 SKIP**. All **795** initial PASS/PASS-void functions retain their exact status. The seven original FAILs become four passes and three explicit, ROM-proven register-ABI SKIPs; zero FAIL does not mean those three C interfaces were fully verified.

Prior uncommitted verified work was preserved. This round changes 38 C files, adds the separate v1.06 tester/hints/tests, then promotes the exact gated tester. No commit, port edit, GPU use or unrelated process termination was performed. Scans use at most four jobs. Evidence and reproduction scripts are under `work/decomp_round2_20261108/`.

## Tester and gate

| Full scan (16 original vectors; additive hints add 16 where applicable) | PASS | PASS-void | FAIL | SKIP |
|---|---:|---:|---:|---:|
| v1.05 baseline | 110 | 685 | 7 | 297 |
| v1.06 first gate | 118 | 730 | 0 | 251 |
| v1.06 final frozen gate | 118 | 730 | 0 | 251 |

Transitions: **42 SKIP→PASS-void, 7 SKIP→PASS, 3 FAIL→PASS-void, 1 FAIL→PASS, 3 FAIL→SKIP**. `audit.json` checks all 1,099 addresses and the frozen C/header/tester/hint hashes; no input drift and no passing-status regression. `final_gate/difftest_all.json` is the promotion gate; `promotion.json` records hashes. Both scans have retry and varied stubs enabled.

`tools/decomp/difftest_v106.py` was developed independently. Changes:

- Fold physical HWRAM `06000000..07ffffff` modulo 1 MiB on reference and C memory accesses, including cache-through aliases; normalize effect addresses identically. Image-write protection also checks aliases. Other buses remain unmapped.
- Resolve declared `reg_XXXXXXXX`, uniquely named globals and declared address arrays using their translation-unit widths; add the explicit proven alternate callee spellings. Static initialized C tables such as `atan2_octant` still require generator support.
- Enable deterministic initial-state pointer retry by default, with `--no-retry-pointers` available. Retry never patches a running execution or immutable program bytes. The full scan preserves all 795 previous passes.
- Extend hints with nested pointer fields, global words/bit masks, value choices, callback signatures, and ROM-proven stub result masks/domains. Python and C apply identical contracts. Stub result choices make previously almost-impossible zero-return paths reachable.
- New coverage hints use `additional_vectors`: run the original RNG/input vectors first, then repeat that RNG with the hinted setup. Existing v1.05 hints keep their prior behavior. Four-tuple callers remain unsalted/unhinted without retries.
- Keep explicit register-ABI floors for the quad dispatcher, sin/cos pair, and live-r5 caller; do not report false C mismatches at these boundaries.

`difftest_hints_v106.json` contains 171 function profiles. The original `difftest_hints.json` and `difftest_v105.py` remain intact. The exact former live tester is also retained in `work/decomp_round2_20261108/difftest_v105_live.py` for rollback. The v1.05 standalone rollback needs `--vary-stubs` to match the former live default.

| Initial SKIP cohort | Functions | Verified after | Still SKIP |
|---|---:|---:|---:|
| Step cap | 24 | 20 | 4 |
| Unresolved identifier | 22 | 8 | 14 |
| HWRAM mirror address | 17 | 8 | 9 |

These are starting-reason cohorts, not causal ablations. Overall 49 former SKIPs pass. Of 22 identifier gaps, 21 identifiers now resolve; eight functions pass and thirteen reach a deeper ABI/hardware limit. The remaining unresolved identifier is the initialized static `atan2_octant` table. Mirror modelling correctly leaves malformed boot-pool reads and image-write aliases unsupported. The four original step-cap survivors are the two polling/DMA waits `02214e2c`/`02214e2e`, asynchronous `02223c74`, and `022332d4` passing a private stack vector to a callee.

## ROM triage and corrections

All **six SKIP→FAIL** cases from the exploratory full run are real C bugs: `022016ac`, `022058a8`, `02220828`, `0223e3d0`, `0223fec4`, `022434ae`. All six now PASS-void. No SKIP→FAIL was hidden by reclassifying it as an ABI limit.

| Original FAIL | Final | Finding |
|---|---|---|
| `02202d7e` | PASS-void | HWRAM callback |
| `02213e0c` | SKIP | callee reads r8/r9 and returns r1; ordinary r4/r5/r0 stub ABI is invalid |
| `02213f90` | SKIP | ROM returns sine/cosine in r4/r5; C uses output pointers |
| `0221a452` | PASS-void | cancellation ran in inactive states |
| `0222bdf2` | PASS-void | missing exts.b of input-record displacement |
| `02244820` | PASS | stub hash was outside the proven 0..3 callee result domain; constrain both sides, no C edit |
| `02249240` | SKIP | intermediate store fixed; caller consumes callee live r5, unsupported by r0-only stub |

The three ABI conversions are supported by `abi_evidence.asm`: `06017d90..17dce` uses r8/r9/r1; `06017f90` writes its results to r4/r5; `0604d274..4d276` consumes r5 from `02245e58`. `06047fba..47fd4` proves the classifier returns only the two-bit OR mask. The `02249240` store correction separately FAILs before and PASS-voids after on an r5-independent fixture; it remains a default SKIP.

| C function | ROM evidence / corrected behavior | Evidence file |
|---|---|---|
| `022016ac` | `06005766..5776; 060057d2..57de` — Store BIOS timer result to 060987f4 before calling 0220acf6; store that second result to 06098800. BIOS timer has zero arguments. | `newfails2.asm` |
| `02202d7e` | `06006dc8..6dcc` — PIC callback anchor 06006dcc + 0xe = 06006dda. | `fail7.asm` |
| `02203ed4` | `06008192..819a` — Compare full r13 against 0xffff; u16 truncation made C loop infinite. | `ring.asm` |
| `022058a8` | `cart 2058a8 PIC anchors (more_callbacks.txt)` — Store runtime HWRAM scene steppers 06008370/060084cc and attract handler 06008868. | `more_callbacks.txt` |
| `02208c44` | `0600cf1a..cf3e; 0600cf58..cf7a` — Write high half of each row command, advance cursor, then OR/store low half. C had merged the two writes and reordered the cursor. | `newfails.asm` |
| `0220f26a` | `06013274..13286` — Callback is 06013280 + 0xa704 = 0601d984, not 022199da. | `newfails.asm` |
| `02212350` | `060163dc..163f2` — Zero actor fields +80, +7c, +78 in ROM order. | `newfails.asm` |
| `02214328` | `06018336..18350` — shlr exposes the old bit zero in T; sample control bits before shifting. C sampled the shifted word twice. | `lz.asm` |
| `02219f26` | `0601e026 / 0601e03a (cosine/sine PIC call deltas)` — ROM calls cosine before sine before billboard submission. Explicit temporaries preserve this call order. | `orbit.asm` |
| `0221a452` | `cart 21a46e; 21a5dc..21a5ea` — Inactive states branch past the cancellation block; cancellation applies only to states 0/1. | `fail7b.asm` |
| `0221ab1c` | `0601eb9c..1ebda` — Variable shift count is saved rate low nibble (r1), not task pointer. | `newfails.asm` |
| `0221b0a2` | `0601f0c2..1f0de` — Set default velocity, position Z and lifetime, then write selected velocity in RNG call delay slot. | `newfails.asm` |
| `0221b730` | `cart 21b76a..21b794` — Use returned spawned task +8 as work; pass recovered callback 0601f14a. C used descriptor +8 and omitted callback. | `b730.asm` |
| `0221bb40` | `0601fbbc..1fbc4` — Callback 0601fbc0 + 0x5c = 0601fc1c; C used 0021bc1c. | `newfails.asm` |
| `0221cd58` | `06020e04..20e0a; 02216c88 definition` — r4=17/r5=8 are set, r6 is stale and ignored by callee. Drop fabricated third C argument; this is an ABI modelling correction. | `newfails3.asm` |
| `0221e256` | `0602240a..2241c` — Peer task state is signed mov.b; cast before testing >=2. Unsigned C state interpreted negative state as live. | `e256.asm` |
| `0221e844` | `06022892..2289c` — Callback 06022898 + 0x30 = 060228c8. | `newfails.asm` |
| `0221f2bc` | `060233b8..233bc; f2bc_callbacks.txt` — Restore missing 02217f04 call at state 1; use five PIC-recovered HWRAM callbacks. Hardware paths remain a coverage floor. | `newfails3.asm` |
| `02220828` | `06024956..24960` — Command 10 shifts by 2+2+1=5, not 11; literal mask FF1F is retained. | `newfails2.asm` |
| `0222378c` | `06027792..277a8; output loop` — Output context arrives in r6, with unused r5 slot; advance cursor before writing character. | `latefails_full.asm` |
| `0222bdf2` | `cart 22be04..22be14` — Input-record displacement is truncated by exts.b after multiplication by 24. | `fail7b.asm` |
| `022306c4` | `060347d6..347de` — Load full parameter long then truncate low halfword into +22; C loaded the high halfword. | `latefails.asm` |
| `02230ae0` | `same parameter load/store sequence in sibling` — Load full parameter long then truncate low halfword into +22. | `latefails_full.asm` |
| `0223101c` | `cart 2310a6` — PIC callback 02230f74 -> runtime 06034f74. | `callbacks.txt` |
| `02231c30` | `cart 231cfa` — PIC callback 02231d72 -> runtime 06035d72. | `callbacks.txt` |
| `02231db8` | `cart 231eb6 / 231f5a` — Handler 06035fd4 and child template 06035336. | `callbacks.txt` |
| `02231fd4` | `cart 231ff2 / 23201a / 232052` — Three callbacks relocated to 0603ce94 / 0603515c / 060360d4. | `callbacks.txt` |
| `02232610` | `cart 23286e..23298a` — Five callback/comparison addresses use HWRAM; final child is 060141ce (C had 0220b1ce). Hardware paths remain a coverage floor. | `callbacks.txt` |
| `02232bc4` | `cart 232e12..23301a` — Five PIC callback/comparison addresses use HWRAM. The stack-only glyph ABI remains excluded from accepted hints. | `callbacks.txt` |
| `02237ea6` | `0603bed2..3bed8` — Reset slot2 halfword, slot1 halfword, slot2 count, slot1 count. Active-slot pointer setup remains a floor. | `newfails2.asm` |
| `0223d558` | `cart 23d662 / 23d6c6` — Correct child callbacks 0603501c and 0603ce94; second C address also had a wrong offset. | `callbacks.txt` |
| `0223d9dc` | `cart 23da4a` — PIC callback 0603d9bc. | `callbacks.txt` |
| `0223e3d0` | `cart 23e442` — PIC handler 06041b2a. | `callbacks.txt` |
| `0223fec4` | `cart 23fef2` — PIC callback 0603636c; C had 0223636c (wrong cart offset and address space). | `callbacks.txt` |
| `022434ae` | `cart 2434d4` — PIC callback 060364da. | `callbacks.txt` |
| `02247106` | `case 2 AND/OR sequence in newfails3.asm` — Retain intermediate state_flags &= 0x300 before |= 0x2000. | `newfails3.asm` |
| `02248436` | `0604c4f2..4c4fa` — Load animation request from signed halfword +150, not truncated long +150. | `newfails2.asm` |
| `02249240` | `0604d2a6..4d2b2; 0604d274..4d276` — Retain intermediate state_flags AND before OR. Live r5 from 02245e58 remains unsupported by the stub ABI. | `fail7.asm` |

Five stale source comments were corrected after promotion; `comment_updates.json` records identical GCC-preprocessed text before/after and saved gate-source hashes. The frozen executable gate remains applicable. `source_fixes.diff` compares this round’s saved originals with the final C. `before_after.json` replays original sources under the final tester: 29 FAIL, five SKIP and four ordinary/vacuous PASS-void results. Earlier exploratory reports retain their initial FAILs where changed ABI annotations make the final old-source probe SKIP. `focused_fixes.json` restores safe state-specific vectors for `0221f2bc`, `02232610`, `02232bc4`, `02237ea6`: all four original sources FAIL and all four corrections PASS-void, while broader hardware/stack profiles remain floors. The same file isolates the `02249240` intermediate-store fix. Of 38 edited C files, `0221cd58` is an ignored-argument ABI annotation correction, and `02249240` remains only partially verified; no claim of complete path equivalence is made for all 38.

Known actor fields use the pass-2 accessors/names in the touched actor code. Scene/effect/door payloads retain unknown offsets rather than being assigned an unproven actor tail layout. The base-2 formatter callers already pass three arguments, so correcting its missing r5 slot agrees with both callers.

## Previously inactive PASS-voids

Of the initial **88** with no writes/calls, **71** now have matching effects in the default scan. Another **3** (`0221f2bc`, `02232610`, `02237ea6`) have matching effects in retained focused profiles: **74/88** are now meaningfully exercised across the retained tests. The other 14 consist of four exact no-ops and ten remaining setup/model/coverage limits. These are partial path checks, not proof of every state.

| Still inactive in default scan | Cause / retained evidence |
|---|---|
| `02201b8e` | exact rts/nop |
| `022143b8` | exact rts/nop |
| `022191ec` | body passes private stack data to a callee |
| `0221a1a0` | nested input pointers remain unmapped under stronger state vectors |
| `0221a6a8` | latched-input pointer chain needs a structured scenario |
| `0221d302` | unknown callee ABI at 06019fac |
| `0221f2bc` | focused state 1 verifies effects; broad profile reaches VDP2 hardware |
| `02223cf0` | VDP1 hardware access |
| `02226144` | exact rts/nop |
| `02229640` | exact rts/nop |
| `0222a120` | nested player/task chain needs structured setup |
| `0222bfc2` | range/target eligibility gates all reject; later angle out-param also needs a richer stub |
| `02231b14` | private stack argument |
| `02232610` | focused state 1 verifies effects; broad profile reaches VDP2 hardware |
| `02236246` | private stack argument |
| `02237ea6` | focused state 0 verifies reset stores; active-slot countdown/pointer chain still faults |
| `022384e4` | state assigned after declarations escaped automatic seed discovery; linked slot/animation gates remain inactive |

The final default scan still has **25 effectless PASS-voids**, including those 17 original-cohort entries, seven newly enabled symbol-gap functions (`02201f20`, `02223d98`, `0223358c`, `02234984`, `022363e4`, `0223673a`, `02236e66`) and corrected `0221a452`. These eight additional effectless outcomes are recorded as coverage floors; the 49 SKIP-to-pass status gains must not all be read as newly exercised bodies. Four of the 25 are the exact no-ops listed above.

`coverage_inventory.json` records the entry gates for all 166 functions in the union of the inactive and low-coverage sets. `coverage_hints_proposals.json`, `coverage_attempts.json`, `rejected_hints.json` and the named probe reports retain accepted, rejected and non-productive attempts. Rejected broad profiles were not silently marked verified; passing original vectors remain in the default scan.

## Branch-direction coverage

The frozen live baseline has **118**, rather than the older quoted 117, passing functions below 25%. Default hints increase distinct direction hits for **89/118**; **81** reach at least 25%. The same 795-function cohort falls from 118 to 37 below 25%. The separate `0220c782` phase fixture passes twice at 3/8 directions; it is outside the default-distribution table. `0221482e` descriptor hints hit the unmodelled SCU status register in both attempts.

| Coverage bucket | Baseline 795 | Same 795 after | All 848 after |
|---|---:|---:|---:|
| reported total = 0 | 187 | 187 | 192 |
| <25% | 118 | 37 | 46 |
| 25%–<50% | 135 | 120 | 130 |
| 50%–<75% | 169 | 197 | 203 |
| 75%–<100% | 87 | 133 | 150 |
| 100% | 99 | 121 | 127 |

Examples: `02247106` improves from 1/50 to 19/50; `02247a8c` reaches 33/66. Coverage is report-only and uses the existing static walker: switches can expand denominators, and interior aliases such as `02248f86` may report zero directions despite matched effects. “Reported total = 0” is therefore not a proof that the body contains no branches. Full per-function before/after counts are in `coverage_metrics.json` / `coverage_attempts.json`.

## Floors, validation and remaining work

Added **316** floor rows (class counts in `floors_added.json`), covering repeated remaining full-scan SKIPs, the rejected stronger profiles, four exact no-ops, and closed or partial C corrections. Existing unrelated entries remain. The previous grouped owner/actor pointer floor for `02238960`/`02238a9c` is closed: the nested chain is now a production hint. Shared per-function attempt ledgers include the full scans and targeted probes; no stuck profile needs to be rediscovered.

The 251 remaining SKIPs include compiler/runtime/custom-register ABIs, hardware/polling, private stack arguments, unsupported generator constructs, remaining pointer provenance, and invalid/missing entry bytes. The two raw boot-pool functions still fault after mirror folding; no executable bytes were repaired. Static initialized arrays and variadic/stack-only callbacks remain unsupported. Some accepted profiles deliberately cover safe states while separately recorded broader profiles remain floors.

Validation: **1,099/1,099** GCC `-O1` objects compile, with **16** diagnostic-bearing translation units; `ld -r` succeeds. The inherited suite is **54/54** against both v1.06 and the promoted live tester; **8/8** new tests cover reference/native mirror sharing, effect normalization, unmapped buses, immutable-image setup protection, nested deterministic hints, named-global width, and identical stub domains. Initial regression/probe scripting errors were corrected before the frozen gate; final logs are `build.log`, `regressions_final.log`, `regressions_live.log`, `regressions_new_final.log`.

Reproduce from the repository root:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --output-dir work/decomp_round2_20261108/recheck
DIFFTEST_MODULE=difftest tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v104.py
DIFFTEST_MODULE=difftest_v106 tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v106.py
tools/decomp/venv/bin/python work/decomp_round2_20261108/build_check.py
tools/decomp/venv/bin/python work/decomp_round2_20261108/focused_fixes.py
tools/decomp/venv/bin/python work/decomp_round2_20261108/last_coverage.py
```

1. v1.06 was built separately and promoted only after a frozen full gate.
2. Final: 118 PASS, 730 PASS-void, 0 FAIL, 251 SKIP.
3. All 795 original passes retain their exact status; 49 SKIPs now pass.
4. Twenty of 24 step-cap SKIPs and eight functions in each symbol/mirror cohort now pass.
5. All six exploratory SKIP-to-FAIL cases were real C bugs and now pass.
6. Four original FAILs now pass; three proven register-ABI limits remain explicit SKIPs.
7. Thirty-eight C files carry ROM-supported corrections or ABI annotations.
8. Seventy-one inactive PASS-voids gain default effects; three more pass focused body profiles.
9. Low coverage drops from 118 to 37 in the original 795-function cohort.
10. All 1,099 files compile/link, suites pass 54/54 and 8/8, floors are recorded, and no commit was made.
