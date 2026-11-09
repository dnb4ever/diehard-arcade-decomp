# Decomp tail status (2026-10-22)

## Local easy-leaf pipeline: EXHAUSTED
census leafC+code, undone, ninsn<=20, minus skip-list = **0 candidates**. The local
122B (easy-leaf-only by design) has no road left. Last local batch (29 staged) yielded
0 integrable: 15 already done, 11 mis-segmented DATA, 1 logically-wrong real fn
(0222812e), 3 epilogue/padding fragments. All 14 blacklisted in tools/decomp/llm_skip.txt.

## Honest coverage picture
- members: 1272 total, 918 done, 599 undone.
- undone breakdown: **misseg 418, data 299-class, callC 140, leafC 41**.
- The 418 misseg + most of the 299 data are NOT real functions -- they are the fn.py
  over-swallow / data-as-code mirage. So the 72% member-level figure is UNDERSTATED;
  true function-level coverage is materially higher. The denominator is polluted.

## The real remaining decomp work (the fork)
1. ~140 callC (non-leaf, control flow) -- the genuinely hard tail. Local 122B quality
   here is unproven + risky (it botched a 29-insn leaf). Cloud-Claude is the right tool
   but costs cloud tokens -- which cuts against the stated reason bulk went local.
2. ~13 larger leaf fns (29..312 insn) -- mid difficulty. Could try local with a raised
   ninsn ceiling + strict gate-2 review, or cloud.
3. Denominator cleanup: characterize the 418 misseg -- how many hide real small fns
   behind the over-swallow (the recurring mirage). Recovering those both raises true
   coverage honestly AND surfaces real functions. Same gen cost applies to whatever is found.

## Recommendation
Highest value / lowest risk is NOT more blind generation. It is (3) denominator cleanup
to get an honest number + recover buried real fns, THEN (1) deliberate cloud passes on the
callC tail in small reviewed batches. (2) local-with-raised-ceiling is a cheap trickle in
parallel. DECISION for the owner: how much cloud-token budget to spend on the hard tail vs.
keeping the decomp where it is and pushing the PORT track (which needs no such tradeoff).

## RESOLVED (2026-10-22): hard tail processed + TRUE coverage established

Ran 5 cloud-Claude agents over the 90 "undone" callC/leafC census targets. Outcome:
the census undone-list is ~90% ARTIFACTS (literal-pool aliases resolving forward to
already-decompiled real entries, plus mis-segmented data). Only 8 were genuinely-new
real functions; all 8 decompiled + committed (0220e1e2, 0221dc04, 02222c08, 02223ac0,
0222bfc2, 022305e2, 02231382, 02239ce8). Also fixed a real inverted-condition bug in
the executive 022016ac.c (tst/bf polarity).

### TRUE coverage = call-graph coverage (tools/decomp/coverage_callgraph.py)
The member-level 72% was deflated by data/aliases polluting the denominator. The honest
metric is call-graph coverage: of the real functions the code actually references
(func_<8hex> call targets, canonicalized across cart-offset/sh2/runtime address forms),
how many are defined.
  defined files 926 | referenced (canon) 875 | universe ~1017
  => ~91% CALL-GRAPH COVERAGE  (vs the misleading 72% member figure)
  ~88-91 genuinely-undone real functions remain (referenced but not yet decompiled;
  a handful are defined inside multi-function files so the true figure is marginally better).

### Naming normalization (tools/decomp/normalize_refs.py)
Found + fixed 71 non-canonical call-site references across 21 files (func_060xxxxx runtime
form and func_00xxxxxx cart-offset form -> canonical func_022xxxxx sh2 form). 8 of these
were dangling refs to already-defined functions (latent link-breakers); now resolved.

### Genuinely-hard remainder (deferred, documented - NOT artifacts)
- 0222d450: 43-branch bsrf jump-table giant (needs jumptable treatment).
- 02227ca8: ~660B, nonstandard r0/r1/r2-ABI copy pocket, too ambiguous to do faithfully yet.
- 02225ee0: mid-function misseg, needs a boundary pass.
- Soft-float / divider math pocket: 0224a518=__divdf3, 0224a830=__floatsidf,
  0224a7a8=__fixdfsi, 0224a884, 0224a280 block-copy. ASM-ORIGIN - belongs in an asm path,
  NOT equivalence-C.
- Six 0228xxxx data blocks (0x50-strided mov.b store pattern) = data, not functions.
