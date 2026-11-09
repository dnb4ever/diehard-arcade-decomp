# Honest function-count denominator — Die Hard Arcade (ST-V) decomp

**Run date:** 2026-10-19 · **Tool:** `tools/decomp/census.py` (re-run anytime)

## TL;DR

| metric | reported | HONEST |
|---|---|---|
| denominator (functions) | **1272** | **~835** (range 823–867) |
| phantom data-as-code "functions" | — | **~437** (range 405–449) |
| functional coverage | **32.9%** (419/1272) | **~50%** (419/835, range 48–51%) |

The decomp is **~50% of the way through the genuine code**, not ~33%. The 1272
denominator is inflated by ~437 members that are DATA the forced-disassembly
sweep carved into pseudo-functions — overwhelmingly in the upper region
(0x24c000–0x290000).

Numbers are live (another agent is actively adding functions; functional count
moved 418→419 during this measurement). Re-run `census.py` for current values.

## What the 1272 members are

`progress_functional.py`'s denominator = `member_set()` = every function-start
`fn.units()` returns over 0x200400–0x290000. `fn.units()` runs `fn.extent()`
(an SH-2 forced-disassembly walk) from each point and records every start it
finds. In a data region the walk still "finds" starts, so data becomes members.
(Note: this is `fn.units()`'s **members**, 1272 — not its 258 compilation-unit
groups.)

## Method — two independent signals

**Signal 1 — byte-fraction disasm screen** (the wave-6 agent's method). For each
member `[start, code_end)` (code_end from `fn.extent`), disassemble with capstone
(SH2, big-endian, `skipdata=on`) and compute the fraction of slots that fail to
decode (`.byte`/invalid). Thresholds, calibrated below:

- `frac <= 0.10` → **code**
- `0.10 < frac < 0.35` → **ambiguous**
- `frac >= 0.35` → **data**

**Signal 2 — triage category** (`notes/function_triage.tsv`, built independently
from call-graph / segmentation analysis): `callC` / `leafC` / `asm` = genuine
reachable code; `misseg` = mis-segmented data-as-code suspect.

## Calibration evidence

Known **code** (hand-decompiled, compile-clean functions): 0x200400 frac 0.00,
0x201b92 0.00, 0x24a06c (`sdiv32`) 0.00, 0x214e2e 0.04.
Known **data**: 0x24c81a 0.71, 0x24cc9a 0.80, 0x250000-region 0.53, 0x280000 0.66.

**Where it gets fuzzy (reported honestly):**
- *Real code can score high.* 27 of the 419 hand-decompiled functions score
  >0.10 (up to 0.40, plus one 8-byte noise case at 0.75) — they contain embedded
  switch/jump tables, or are tiny and statistics-dominated. So >0.10 ≠ data.
- *Data can score low.* Short sequential-value tables disassemble as valid-looking
  SH2 (the confirmed-data family 0x267e92–0x26dcbe reads 0.29–0.42; trivial 4-byte
  members decode to 2 valid slots → frac 0.00). So frac≈0 in the data band ≠ code.

Byte-fraction is therefore **necessary but not sufficient**. Member **size** and
**address band** are needed to disambiguate the 0.10–0.35 zone — which is exactly
why the two-signal cross-check matters.

## The two signals agree

Agreement matrix (triage category × byte-fraction class):

| triage | code | amb | data | total |
|---|---|---|---|---|
| callC | 434 | 110 | 3 | 547 |
| leafC | 270 | 16 | 9 | 295 |
| asm | 9 | 0 | 0 | 9 |
| **genuine** | **713** | **126** | **12** | **851** |
| misseg | 65 | 66 | 290 | 421 |

- **98.6%** of the triage-genuine set (839/851) is byte-fraction code/ambiguous.
- Of 421 misseg, **290** are byte-fraction data and **~90** are trivial 4-byte
  fragments — i.e. the misseg set is overwhelmingly real phantoms.
- **Decisive check:** 418 of 419 hand-decompiled functions fall inside the
  genuine set. If the denominator were contaminated, decompiled work would scatter
  into the phantom set — it does not.

## Per-band breakdown (code / ambiguous / data / total, byte-fraction)

```
0x200400-0x210000  134 /  40 /  11 / 185   lower  — genuine code
0x210000-0x220000  190 /  32 /  11 / 233   lower  — genuine code
0x220000-0x230000  145 /  27 /   2 / 174   lower  — genuine code
0x230000-0x240000  120 /  31 /   2 / 153   lower  — genuine code
0x240000-0x248000   83 /  13 /   0 /  96   lower  — genuine code
0x248000-0x24c000   44 /   0 /   2 /  46   upper code pocket (wave-confirmed)
0x24c000-0x260000    0 /   2 /  95 /  97   UPPER — data
0x260000-0x270000    2 /   5 /  94 / 101   UPPER — data
0x270000-0x280000   25 /   2 /  14 /  41   UPPER — data (the 25 "code" are misseg)
0x280000-0x290000   35 /  40 /  71 / 146   UPPER — data (0x2896xx table families)
```

Genuine code is essentially all **below 0x24c000** (835 of the 851 triage-genuine
members). The 16 triage-genuine members **above** 0x24c000 are all in the
0x2896xx / 0x228c1xx blocks — confirmed data tables (the wave agent's "banned"
region) that decode as valid-looking table entries; they are **excluded** from
the honest denominator. The upper-region "code" byte-fraction hits (0x270000+,
e.g. 27 consecutive 4-byte members at 0x272a94) are misseg fragments, already
excluded.

## Honest denominator

```
callC + leafC + asm (triage genuine)   851
  − 16 confirmed upper data tables      -16
central honest denominator             835
  range                                823 .. 867
    low  = 835 − 12 lower byte-data members (genuine set that still screens data)
    high = 835 + 32 plausibly-real misseg (lower-region, non-trivial, decode as
           code — large clean functions the misseg heuristic over-flagged, e.g.
           0x220917a, 0x223ee0, 0x226148, 0x224852c, 0x224ab00)
phantom (data-as-code)                 ~437   (range 405 .. 449)
```

**Confidence.** High on the headline: the two independent signals agree to ~99%,
and 418/419 decompiled functions land in the genuine set. The ±~25 range comes
from the genuinely fuzzy 0.10–0.35 members and a handful of misseg the triage may
have over-flagged. The data region 0x24c000–0x290000 being almost entirely
phantom is the single most robust finding (multiple methods + the wave agent's
spot-reads concur).

## Honest coverage

```
REPORTED : 419 / 1272 = 32.9%
HONEST   : 419 / 835  = 50.2%   (range 48.3% .. 50.9%)
```

The project is **about halfway** through the real code, not a third.

## Recommendation for progress_functional.py (do NOT modify blindly)

`member_set()` is the inflated denominator. To report honestly, filter members to
the genuine-code set. Lowest-risk option:

1. **Add a genuine-member filter keyed on `function_triage.tsv`**: keep members
   whose category is `callC`/`leafC`/`asm`, drop `misseg`. One caveat to encode:
   also drop the ~16 genuine-labelled members at ≥0x24c000 (confirmed data), and
   consider re-including the ~32 plausibly-real misseg below 0x24c000. `census.py`
   already computes exactly this set — simplest is to have `census.py` emit the
   genuine member list and have `progress_functional.py` import it.
2. **Keep reporting both numbers** (reported vs honest) rather than silently
   switching — the 1272 is still the full sweep universe and is useful context.
3. Do **not** rely on byte-fraction alone inside `progress_functional.py`: it
   mis-handles trivial fragments and switch-heavy real code (see calibration).

The triage TSV is the cheapest honest denominator source today; if it is
regenerated after the upstream mis-segmentation is fixed, re-run `census.py` to
re-measure (the byte-fraction screen is the independent check on the new triage).

## Usage

```
tools/decomp/venv/bin/python tools/decomp/census.py          # full report
tools/decomp/venv/bin/python tools/decomp/census.py --tsv    # per-member rows
```
`--tsv` columns: `sh2_addr cart_addr code_end frac ninsn bytefrac_class triage_cat band`.
Reads `cart.bin` via `fn.py`, `notes/function_triage.tsv`, and
`project/src_functional/`. Read-only.
