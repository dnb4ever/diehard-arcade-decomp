# Differential tester v1.04 report

Final `--all` scan: **1099 C files**, **16 vectors per candidate**. Implementation: `tools/decomp/difftest_v104.py` (developed beside the live `tools/decomp/difftest.py`, which was not modified; the PM installs v1.04). The v1.03 column is a fresh `tools/decomp/difftest_v103.py` scan run concurrently on the same tree, so source edits made by the repair job during this session affect both columns alike.

```sh
tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v104.py
DIFFTEST_MODULE=difftest_v104 tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest.py
tools/decomp/venv/bin/python tools/decomp/difftest_v104.py --all --vectors 16 --jobs 16 --output-dir work/difftest_v104/final
tools/decomp/venv/bin/python tools/decomp/difftest_v103.py --all --vectors 16 --jobs 12 --output-dir work/difftest_v104/baseline_v103
```

| Result | v1.03 (same tree) | v1.04 | Change |
| --- | ---: | ---: | ---: |
| PASS | 70 | 106 | +36 |
| PASS-void | 467 | 642 | +175 |
| FAIL | 13 | 53 | +40 |
| SKIP | 549 | 298 | -251 |

Source files with identical hashes in both scans: **1099 of 1099** (the rest were edited by the repair job between the two reads; their transitions are still listed, but they are not attributable to the harness alone).

Harness SHA-256: `55981ae8e7e85cbb6aabfcf9e3b63959b0880539d563fb2aae093ba92417ac25`. Cart SHA-256: `309603533747f9e235c57576ec04b4c6274338b4a46efcf1a834fdd4f7d88ea9`. SH-2 interpreter SHA-256: `c43458498cde2f4b727aab6298cfba84487b03fcecd544161ab0c7007e13c253`.

Full JSON and attempt logs: `work/difftest_v104/final/` (v1.04), `work/difftest_v104/baseline_v103/` (v1.03); consoles `work/difftest_v104/final.log`, `work/difftest_v104/baseline_v103.log`; regression output `work/difftest_v104/tests_v104.log`. Nothing was written to `work/difftest_all.json`.

## What v1.04 adds

Biggest SKIP buckets first. Every change is static (decided from the C source or the original SH-2 bytes before
any vector runs) and applies identically to the reference and the candidate.

1. **Reference cannot run** (the largest bucket). The static SH-2 provenance analysis now also
   * tags scaled/extended values (`shll*`, `add Rn,Rn`, `ext*`, `and #imm`, `mul*`/`sts macl`) and narrow loads that are used
     as an index next to a pointer or constant base, and gives that source word a small value (its own random bits masked
     to 0..7);
   * bounds loop counters: a `dt Rn` operand of a backward branch gets 1..8, an unsigned/signed compare operand of a backward
     branch 0..7, so bounded loops finish inside the 20,000-step budget;
   * seeds all eight slots of a constant pointer table indexed that way (`tbl[idx]->field`);
   * follows SHC switch tables (`mova`; `mov.w/mov.l @(r0,Rm)`; `braf/jmp`) into this function's own case bodies, so their
     pointer uses are inferred too, and the Oracle allows a computed jump whose target stays inside the function (up to
     the next known entry). Jumps elsewhere stay SKIP;
   * tracks `bsr`/`jsr`/`bsrf` callee results. When a callee result is used as an address, that target's stub returns a word in a
     64 KiB low-RAM region (`0x00210000`) on both sides, and every word of that region points back into it, so fields and
     nested links of returned objects stay mapped;
   * maps the complete cart image **read-only** at `0x02000000` (A-bus CS0, cache-through mirror included) on both sides.
     Loads from ROM are constants for the analysis. A write to ROM is a fault (reference) or an unmapped access (candidate),
     so SKIP.
2. **Local arrays, local aggregates, address-taken scalars** live in instrumented guest memory: a fixed frame inside the
   ABI-private stack window (`0x060f7100..0x060fef00`), never host storage. Initializers are instrumented stores
   (C zero-fill included). Frame stores are private, so they are never compared, the same as SH-2 stack spills. If a
   private-stack address reaches a callee on either side, the result is SKIP (`private stack address passed to callee`): a stub
   cannot model a callee filling a caller frame. If a frame address is stored to non-frame guest memory, the candidate
   stops with SKIP.
3. **Static helper functions** without a known address (`static void hud_print(...)` beside the entry) are generated with the
   same instrumentation and run natively. Named or address-known functions are still stubbed.
4. **Callee prototypes with narrow parameters** (`s16`, `u8`, ...) are accepted, and the C value is converted to the declared type.
   SHC callees extend their own narrow parameters (verified in the bytes, e.g. `extu.b r4` at entry). A first call mismatch
   confined to the upper bits of narrow declared parameters is SKIP, never PASS. **Narrow entry parameters** get
   ABI-valid extended input words.
5. **Unnamed prototype parameters** (`void f(void *)`) no longer crash the void check (the whole former "non-integer or
   aggregate type" bucket).
6. **Callee arity.** A known direct target keeps its declared arity even when the C also has indirect calls. If an indirect call of
   another arity reaches it, the length mismatch is SKIP. A function-pointer cell in the immutable image (`g_0609029C`) gets a direct
   arity for its known target.
7. **Compiler runtime helpers** `sdiv32`, `udiv32`, `umod32`, `ashl_var` and `ashr_var` (`0x0224a06c/120/1c8/360/420`) run as original
   bytes inline in the reference (no stub event). An explicit C call uses a host model whose results, including divide-by-zero's
   `0x44E` store to `0x060E132C` and `INT_MIN/-1`, a regression test checks against those bytes (divide: dividend r1,
   divisor r0; shift: value r0, count r1). Other runtime entries (`shr15` sled tails, `dtoi`, ...) stay SKIP.
8. `long long`/`s64` host values and `LL` constants are allowed (64-bit on host and SHC; they never reach guest memory or
   the ABI). Plain `L` stays rejected. `__attribute__((...))` is dropped by the preprocessor. Block-scope `extern` declarations
   refer to the global object.
9. **Harness bug fixed:** `(*(void (**)(T))CELL)(...)` used to call the *cell address*. It now loads the pointer from the cell,
   and only `(*fp)(...)` with `fp` a function pointer drops the dereference. Under v1.03 this made calls through BIOS/thunk
   cells compare the wrong target.

Retained without change: no reference retries, no fault-driven repairs or remapping, no discarded vectors, no
candidate-dependent comparison filters. A reference exception at any vector is SKIP with the exact text. Unsupported stays SKIP
with a precise reason. r0, final footprint, chronological store trace and ordered call events are compared exactly as in v1.03.
The new SKIP-not-FAIL rules (ambiguous arity length, narrow upper bits, private stack escapes) only ever turn a mismatch into SKIP.

**Input-domain caveat.** Index/loop-bound constraints narrow the tested values of the affected words (for example, a
state byte used as a switch index is now 0..7 instead of random). This is a bounded test domain, not a proof. It also changes
which paths the existing vectors take, which is why a few v1.03 PASS-void results now reach code that still cannot run
(listed below), and why several former vacuous PASS-voids now FAIL: their random state value used to take the early-return
path, and now reaches the case bodies.

## Transitions (v1.03 -> v1.04)

| v1.03 \ v1.04 | PASS | PASS-void | FAIL | SKIP |
| --- | ---: | ---: | ---: | ---: |
| PASS | 70 | 0 | 0 | 0 |
| PASS-void | 0 | 464 | 2 | 1 |
| FAIL | 0 | 1 | 11 | 1 |
| SKIP | 36 | 177 | 40 | 296 |

v1.03 SKIP buckets and where they went:

| v1.03 SKIP category | files | PASS | PASS-void | FAIL | still SKIP |
| --- | ---: | ---: | ---: | ---: | ---: |
| reference cannot run: unmapped 4-byte access | 143 | 19 | 61 | 16 | 47 |
| local array/function declaration | 53 | 1 | 8 | 3 | 41 |
| unresolved callee | 40 | 0 | 18 | 3 | 19 |
| compiler runtime/nonstandard callee ABI | 38 | 3 | 23 | 1 | 11 |
| non-integer or aggregate type | 32 | 3 | 16 | 1 | 12 |
| compiler runtime/nonstandard register ABI | 32 | 0 | 0 | 0 | 32 |
| unknown/ambiguous callee argument count | 31 | 0 | 15 | 4 | 12 |
| reference cannot run: no return after 20000 steps | 27 | 0 | 7 | 2 | 18 |
| callee prototype has unsupported parameter type | 22 | 5 | 11 | 5 | 1 |
| reference cannot run: unmapped 2-byte access | 20 | 0 | 6 | 0 | 14 |
| unresolved identifier | 18 | 0 | 0 | 0 | 18 |
| reference cannot run: unmapped 1-byte access | 15 | 0 | 3 | 1 | 11 |
| variadic/non-word callee ABI | 11 | 0 | 0 | 0 | 11 |
| local/unsupported address taking | 11 | 2 | 1 | 0 | 8 |
| unsupported host type: long long | 11 | 3 | 0 | 1 | 7 |
| computed tail/switch target not a known callee | 9 | 0 | 1 | 2 | 6 |
| non-word parameter ABI | 8 | 0 | 6 | 1 | 1 |
| shadowed local has conflicting types | 5 | 0 | 0 | 0 | 5 |
| reference cannot run: cannot execute '.word 0xFFFF' | 3 | 0 | 0 | 0 | 3 |
| writes to original code/rodata image | 3 | 0 | 1 | 0 | 2 |
| no original bytes in main program image | 2 | 0 | 0 | 0 | 2 |
| candidate cannot run: unmapped C memory | 2 | 0 | 0 | 0 | 2 |
| local aggregate access | 2 | 0 | 0 | 0 | 2 |
| callee prototype has unsupported return type | 2 | 0 | 0 | 0 | 2 |
| preprocessor: <stdin>:2:10: fatal error: stdarg.h: No such file or directory | 2 | 0 | 0 | 0 | 2 |
| reference cannot run: cannot execute '.word 0x0609' | 1 | 0 | 0 | 0 | 1 |
| reference cannot run: cannot execute '.word 0x0000' | 1 | 0 | 0 | 0 | 1 |
| depends on non-argument registers / undefined r0 | 1 | 0 | 0 | 0 | 1 |
| unsupported host type: unsigned long long | 1 | 0 | 0 | 0 | 1 |
| reference cannot run: cannot execute '.word 0x8668' | 1 | 0 | 0 | 0 | 1 |
| unsupported host type: double | 1 | 0 | 0 | 0 | 1 |
| floating/string/wide constant | 1 | 0 | 0 | 0 | 1 |

Former PASS/PASS-void/FAIL that are now SKIP (2), all with their exact reason:

| Address | v1.03 | v1.04 reason | same source |
| --- | --- | --- | --- |
| `02216714` | FAIL | reference cannot run: unmapped 2-byte access at 3A40C83A | yes |
| `02242cb8` | PASS-void | reference cannot run: unmapped 4-byte access at 66D5F114 | yes |

Former PASS/PASS-void that now FAIL (2), expected when wider inputs reach case bodies the C does not implement: `0221b8e4`, `0221cad4`

## NEW FAILs (42)

Not FAIL in the same-tree v1.03 scan. These are the most likely real bugs in the C. The class column is a mechanical reading of the first difference. "address form only" means that the C passes the cart-form address of the same code/data (`0x022...`), where the original passes the HWRAM address (`0x060...`). v1.03 already counts this as a difference, and the fix is usually mechanical. Each entry was not hand-verified unless noted.

| Address | First difference | Class | v1.03 result |
| --- | --- | --- | --- |
| `0220074c` | r0 | return value | SKIP: reference cannot run: no return after 20000 steps (pc 0600475A) |
| `02205b20` | memory write | final memory footprint | SKIP: callee prototype has unsupported parameter type |
| `02207360` | calls | call target/argument/count | SKIP: reference cannot run: no return after 20000 steps (pc 0600B494) |
| `022078aa` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 226AEB00 |
| `022079d0` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: unresolved callee: push_coef |
| `022080ac` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: unresolved callee: push_line |
| `022082be` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 22676078 |
| `0220b1c0` | memory write | final memory footprint | SKIP: reference cannot run: unmapped 4-byte access at 9DBDDA00 |
| `0220e6d4` | calls | call target/argument/count | SKIP: compiler runtime/nonstandard callee ABI |
| `02213ca4` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: local array/function declaration |
| `0221b5a0` | memory write | final memory footprint | SKIP: computed tail/switch target not a known callee |
| `0221b8e4` | memory write | final memory footprint | PASS-void |
| `0221cad4` | memory write | final memory footprint | PASS-void |
| `0221df80` | memory write | final memory footprint | SKIP: computed tail/switch target not a known callee |
| `02223862` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: local array/function declaration |
| `02223968` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: local array/function declaration |
| `02223ee0` | memory write | final memory footprint | SKIP: unresolved callee: __attribute__ |
| `0222863e` | memory write | final memory footprint | SKIP: unknown/ambiguous callee argument count |
| `0222934a` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 03213924 |
| `02229c00` | calls | upper bits only (sign/zero extension) | SKIP: unknown/ambiguous callee argument count |
| `0222ca98` | memory write | final memory footprint | SKIP: callee prototype has unsupported parameter type |
| `022304b4` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 1-byte access at 99BCB7DD |
| `022305e2` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 99F4C79E |
| `022387fc` | calls | call target/argument/count | SKIP: reference cannot run: unmapped 4-byte access at 055890D5 |
| `0223a888` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: callee prototype has unsupported parameter type |
| `0223ac9c` | memory write | final memory footprint | SKIP: reference cannot run: unmapped 4-byte access at 0D4D7D7B |
| `0223b01e` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: callee prototype has unsupported parameter type |
| `0223c7ba` | r0 | return value | SKIP: reference cannot run: unmapped 4-byte access at 5D585431 |
| `0223d3d0` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 98AFEE30 |
| `0223df30` | calls | call target/argument/count | SKIP: reference cannot run: unmapped 4-byte access at 27A7215D |
| `0223eb3c` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 9A09B0C6 |
| `0223ec44` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 9A08CC9A |
| `0223ed94` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at 9A2196BA |
| `0223edce` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at E5DEC175 |
| `02241a10` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: callee prototype has unsupported parameter type |
| `02241b9c` | calls | call target/argument/count | SKIP: unknown/ambiguous callee argument count |
| `02241bee` | calls | call target/argument/count | SKIP: non-integer or aggregate type |
| `02241c40` | calls | call target/argument/count | SKIP: unknown/ambiguous callee argument count |
| `02242fb0` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: unsupported host type: long long |
| `02243590` | calls | address form only (cart 0x022... vs HWRAM 0x060...) | SKIP: reference cannot run: unmapped 4-byte access at E67B2A75 |
| `022447d4` | r0 | return value | SKIP: non-word parameter ABI |
| `02246052` | memory write trace | store order / intermediate value (final footprint equal) | SKIP: reference cannot run: unmapped 4-byte access at D65548B8 |

Hand-checked examples: `0220074c` (crc16_buffer) tests the MSB *after* `crc <<= 1`; the original `shll` tests the bit
shifted out, so this is a real logic bug. `0221b8e4`, `0221cad4`, `0221df80` and `0221b5a0` implement only cases 0-1 of a
10/12-way state switch. The original has bodies for the other cases (v1.03 passed them vacuously because the random state
byte took the early return).

## Complete FAIL list with first differences (53)

Vector/event indices are zero-based. Entry words are r4-r7 followed by initial stack words. Call events begin with the raw target. A `null` memory value means that side did not write the byte.

### `0220074c` — r0 — NEW

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `00280010 00000001 80000000 ffffffff`. Class: return value.

```json
{
  "kind": "r0",
  "reference": "0000ab12",
  "candidate": "0000f877"
}
```

Source SHA-256: `11b134ff80bbcf0d9ed21de435fc8b953da41539c17bc8da5ae493c81ce1917d`.

### `02202d7e` — memory write — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c0 00000020 80000000 80000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002800c0",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `4967a1a67eb0678f6a3f8dbf0d8d350c7b7e351baa59735f17d680850b60a374`.

### `02205b20` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 00000001 0000001f 00000007`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002a043f",
  "reference": 76,
  "candidate": 77
}
```

Source SHA-256: `eb8a65ebb4e7d5c0c8c8f4687e7bc6a0b135d71e9718c4fbfca4d7c3d16d4347`.

### `02207360` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800e0 00281090 00000007 00000002`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100812840,
      100993296
    ]
  ],
  "candidate": []
}
```

Source SHA-256: `3639346a640333d15469717b8665683ef2ce13f4e08306897ab59a4b85f5107a`.

### `022078aa` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280040 80000000 80000000 00000002`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 60,
  "reference": [
    [
      100745044,
      2621504,
      100710864
    ]
  ],
  "candidate": [
    [
      100745044,
      2621504,
      35682768
    ]
  ]
}
```

Source SHA-256: `d6461b868585866ee777d1b21464fc890369f8a4d6213fac3547f8a22717086d`.

### `022079d0` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0028002c 80000000 00000020 0000000f`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 15,
  "reference": [
    [
      101291280,
      0
    ]
  ],
  "candidate": [
    [
      2765824,
      0
    ]
  ]
}
```

Source SHA-256: `e5396a09cea7ba178bd78d59360cbaca6dfb1108abb63841e96c435b3060c710`.

### `022080ac` — memory write trace — NEW

Vector 6; r0 checked: False; completed comparisons: 7. Entry words: `00280080 ffffffff 00000001 ffffffff`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 0,
  "reference": [
    [
      101291280,
      0
    ]
  ],
  "candidate": [
    [
      2757632,
      22
    ]
  ]
}
```

Source SHA-256: `bdfe17e9d649ca29fcfc3974acb75aa2a915ff6c0ad3df5f4a10f09bb3c180c8`.

### `022082be` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280058 0000001f 00000001 ffffffff`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 6,
  "reference": [
    [
      100745044,
      2621528,
      100713376
    ]
  ],
  "candidate": [
    [
      100745044,
      2621528,
      35685280
    ]
  ]
}
```

Source SHA-256: `09891204d39c6cd70baf2d855329d4c3591259d29a3d8c960f5ba0f795986729`.

### `0220b1c0` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280054 00000002 0000000f 00000020`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "00280054",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `d6c9238d940301fa2e9d9ecdf0429e2dbd139a524f3e30512ee908e4cf1173cc`.

### `0220e6d4` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280008 00281034 00282028 80000000`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100746588,
      565095549,
      1953479239,
      3718624618,
      911635410,
      466028791,
      1532791932,
      2621484,
      2621486
    ]
  ],
  "candidate": [
    [
      100746588,
      565095549,
      1953479239,
      3718624618,
      1019443700,
      2457034486,
      3067551391,
      2621484,
      2621486
    ]
  ]
}
```

Source SHA-256: `b0bbb3f7821214e6fcf853c76f2d278f6d2d654983a8ff645b7322464c7f7264`.

### `02213ca4` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `ffffffff 0000000f 0000001f ffffffff`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 0,
  "reference": [
    [
      101288868,
      0
    ]
  ],
  "candidate": [
    [
      101288860,
      255
    ]
  ]
}
```

Source SHA-256: `b7f43f9fa57f833a37d9ce0e93c02c631ead92cb9ce97ecc237ce5a1a75f580f`.

### `02213e0c` — calls — retained v1.03 FAIL

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `7fffffff 00000007 00282024 ffffffff`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100760976,
      2147483647,
      7
    ]
  ],
  "candidate": [
    [
      100760976,
      7,
      2629668
    ]
  ]
}
```

Source SHA-256: `18e093132b3a4a2640328de662834a38356c72d1e7e5a2739d9903409e8e6f68`.

### `02213f90` — memory write — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0000000f 002810f4 0028207c 80000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002810f4",
  "reference": null,
  "candidate": 0
}
```

Source SHA-256: `fca4f4fd5da578f858025b5eee6877ffb82cacf052fc75daf147fa1d8b299829`.

### `0221a452` — memory write — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 00000007 0000001f 00000020`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "00280400",
  "reference": null,
  "candidate": 0
}
```

Source SHA-256: `a5c035d2ebcbce76151e9e9937efb03dd3e3bbc9a854e5aeab81b43e459033bf`.

### `0221b5a0` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 7fffffff ffffffff ffffffff`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "00280410",
  "reference": null,
  "candidate": 8
}
```

Source SHA-256: `56afa7901317109bf44fc374ea04d1882bbe4094bb9277c00f66198c2dd9c401`.

### `0221b8e4` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 0000001f 7fffffff 80000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002a0418",
  "reference": 204,
  "candidate": null
}
```

Source SHA-256: `a1cbabae0e09d61e3dd420edb8b9eadd5d99b527ceb247cc960ad1605c26fb4a`.

### `0221cad4` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 00000001 0000001f 00000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002a0418",
  "reference": 193,
  "candidate": null
}
```

Source SHA-256: `47110a6bd65218f1ecb6d9f6e046975cd0f0f670a95ad318b83d228733907fa8`.

### `0221df80` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 80000000 80000000 00000002`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002a0418",
  "reference": 216,
  "candidate": null
}
```

Source SHA-256: `323250f1bc54662e5e697c545e3724569a0dd9f6ac762dda8fe14257f1a95ae3`.

### `02223862` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0000001f 00000020 00282094 00000020`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 4,
  "reference": [
    [
      2629792,
      0
    ]
  ],
  "candidate": [
    [
      2753536,
      51
    ]
  ]
}
```

Source SHA-256: `5b81bc0fc1c906340e2f49677cd28e8a523b11aa2e588223c6c5e26d2c89ed8e`.

### `02223968` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00000007 80000000 00282020 80000000`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 4,
  "reference": [
    [
      2629676,
      0
    ]
  ],
  "candidate": [
    [
      2753536,
      55
    ]
  ]
}
```

Source SHA-256: `6bec3f5d579425faf8603ec51d8c4da2c2aba9708e71a925f2364c02c321d598`.

### `02223ee0` — memory write — NEW

Vector 2; r0 checked: False; completed comparisons: 3. Entry words: `00280048 ffffffff 0000000f 00000001`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "0028008e",
  "reference": null,
  "candidate": 0
}
```

Source SHA-256: `e7cf0289974edd66ccb4237cf9f8c7d9aaf5b83a15e1dcc06861ca9cd8b9ffaf`.

### `0222863e` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280058 00000020 7fffffff 0000001f`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "060997a8",
  "reference": 81,
  "candidate": 169
}
```

Source SHA-256: `c851e61b8924f52211b207de956844607a7814ad1e343a3abc549b07a18cda6f`.

### `0222934a` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800a4 00281078 80000000 00000007`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745074,
      2587894740,
      100839096
    ]
  ],
  "candidate": [
    [
      100745074,
      2587894740,
      35811000
    ]
  ]
}
```

Source SHA-256: `9785ce48c37eeca58b1e4556006b71dcd704de43ac58d0bca0c665a3bf398980`.

### `02229c00` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c0 00281000 0000001f 00000002`. Class: upper bits only (sign/zero extension).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100723724,
      4294957374
    ]
  ],
  "candidate": [
    [
      100723724,
      55614
    ]
  ]
}
```

Source SHA-256: `773ef98d8e5e78d1a6ab6b03b3d2722b6bd252b58f42f62ef79835d0597876d0`.

### `0222bdf2` — memory write — retained v1.03 FAIL

Vector 1; r0 checked: False; completed comparisons: 2. Entry words: `00280084 0000001f 00000001 80000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002800e1",
  "reference": 232,
  "candidate": 244
}
```

Source SHA-256: `d55c0d172baca7c20cac9f8bed68b5f3c761e3933c266054eaebeec4295bf1e3`.

### `0222ca98` — memory write — NEW

Vector 3; r0 checked: False; completed comparisons: 4. Entry words: `00280014 0000001f 80000000 ffffffff`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "00280044",
  "reference": 19,
  "candidate": 63
}
```

Source SHA-256: `b89c9971754bf2c1a2df3db125da6277fff2636f628e9ac0e0dd19c902d84611`.

### `0222d0e8` — calls — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280014 7fffffff 00000020 0000001f`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 59,
  "reference": [
    [
      100764206
    ]
  ],
  "candidate": []
}
```

Source SHA-256: `053ac6525569d6113c71ef5f9747fd332e35b301e4530fafa193355ce76f4c95`.

### `022304b4` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0028005c 80000000 00000000 0000001f`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745000,
      2621532,
      100877794
    ]
  ],
  "candidate": [
    [
      100745000,
      2621532,
      35849698
    ]
  ]
}
```

Source SHA-256: `ccba051360e632f4bae9a8e37171c7a9f0f8d86f44fe8de516a028dbdda7d42e`.

### `022305e2` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c0 0000000f 0000001f 00000007`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100744940,
      2621632,
      100914432
    ]
  ],
  "candidate": [
    [
      100744940,
      2621632,
      35886336
    ]
  ]
}
```

Source SHA-256: `eff3d6912456d87fce77cc83f3a87ef461dd3b8a4fcbc00e03f322473654c8eb`.

### `022387fc` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 7fffffff 00000001 80000000`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745000,
      2674162271,
      100930904
    ]
  ],
  "candidate": [
    [
      100745000,
      2674162271,
      37279064
    ]
  ]
}
```

Source SHA-256: `5faf2a9a62788442df62e051ca27f0490144d9555fd022c4eb6c71a5400a5767`.

### `0223a888` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800cc 0028107c 00000002 0000000f`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 14,
  "reference": [
    [
      2621846,
      223
    ]
  ],
  "candidate": [
    [
      2621846,
      239
    ]
  ]
}
```

Source SHA-256: `f03ed1a02441937b1807599c735eabe237ba90c280ac4d08a35f7d65af39426d`.

### `0223ac9c` — memory write — NEW

Vector 5; r0 checked: False; completed comparisons: 6. Entry words: `0028007c 00000001 0000001f 00000000`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "06096c7b",
  "reference": 1,
  "candidate": 0
}
```

Source SHA-256: `c1275c05952c101fcbf2f7567b5166934010284fab7d49fb8c8433617cc218d4`.

### `0223b01e` — calls — NEW

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `00280020 80000000 00000007 00000000`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100744940,
      4283766865,
      100929974
    ]
  ],
  "candidate": [
    [
      100744940,
      4283766865,
      35901878
    ]
  ]
}
```

Source SHA-256: `9bb4c17de4b29fd54569c98d9ff3f43d56866862dbdbbed5e2442a8dbf980c52`.

### `0223c7ba` — r0 — NEW

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `002800e0 00000001 80000000 00000001`. Class: return value.

```json
{
  "kind": "r0",
  "reference": "00000000",
  "candidate": "ffffffff"
}
```

Source SHA-256: `e2cf3271a17abe032cd49285ee117a20682eddd999f20f1762e01450d5d305fc`.

### `0223d3d0` — calls — NEW

Vector 5; r0 checked: False; completed comparisons: 6. Entry words: `00280098 80000000 00000020 80000000`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100744970,
      2621592,
      100754256
    ]
  ],
  "candidate": [
    [
      100744970,
      2621592,
      35726160
    ]
  ]
}
```

Source SHA-256: `133516d2227e032cf4ad1695899956cb85f960149703c48f2aa9e9dfa05d8d8b`.

### `0223df30` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280004 00000007 ffffffff ffffffff`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100909248,
      15,
      4495,
      13,
      4294967252
    ]
  ],
  "candidate": [
    [
      100909248,
      15,
      4495,
      13,
      212
    ]
  ]
}
```

Source SHA-256: `61f030e10a36e240664f9fe072fd4e2be07bd4b1b40b099bbaf34bc93f97200e`.

### `0223eb3c` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0028002c 002810d8 00000002 0000000f`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100745000,
      2621484,
      100937708
    ]
  ],
  "candidate": [
    [
      100745000,
      2621484,
      35909612
    ]
  ]
}
```

Source SHA-256: `c15a8a645deeb6cf0346530bae041884c88f2b81414ee4e78f3b6b427096fbb8`.

### `0223ec44` — calls — NEW

Vector 1; r0 checked: False; completed comparisons: 2. Entry words: `00280000 00281030 00000007 00000001`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100745000,
      2621440,
      100936980
    ]
  ],
  "candidate": [
    [
      100745000,
      2621440,
      35908884
    ]
  ]
}
```

Source SHA-256: `480c3066dfb51de015429b715c4493b19e91bfcea2eab89fd9d9c1c125e064d7`.

### `0223ed94` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00000020 00281098 00000001 0000001f`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745000,
      32,
      100937632
    ]
  ],
  "candidate": [
    [
      100745000,
      32,
      35909536
    ]
  ]
}
```

Source SHA-256: `3c76148f9c6984314c5eeec6edfa827da949b2815a5772a4c341ed1a652d76eb`.

### `0223edce` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `7fffffff 002810a8 00000001 ffffffff`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745000,
      2147483647,
      100937456
    ]
  ],
  "candidate": [
    [
      100745000,
      2147483647,
      35909360
    ]
  ]
}
```

Source SHA-256: `1762311b0de9c1b28c9071c2eb84b97b618a75d91143806a20e8de2d533b8118`.

### `02241a10` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `7fffffff 0028100c 00000020 ffffffff`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100744940,
      2147483647,
      100755356
    ]
  ],
  "candidate": [
    [
      100744940,
      2147483647,
      35727260
    ]
  ]
}
```

Source SHA-256: `8354f1d74769798d95d5d2d63f0926ceadb1f5b5ed05199f3bba567f9ff1bba0`.

### `02241b9c` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800d0 7fffffff 80000000 0000000f`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 4,
  "reference": [
    [
      100871908,
      2753536
    ]
  ],
  "candidate": [
    [
      100871908,
      2621648
    ]
  ]
}
```

Source SHA-256: `36f3b5692b4bf72cde674c831431ff8e1076a38d9eb91124a23053bc861f34ef`.

### `02241bee` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280038 00000020 80000000 00000007`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 2,
  "reference": [
    [
      100979448,
      2621496
    ]
  ],
  "candidate": [
    [
      100987164,
      2621496
    ]
  ]
}
```

Source SHA-256: `cd89bd81854fc8a432dcd6ce57cbbf9797fe3f71fc2a16a161ef7671239edf23`.

### `02241c40` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0028000c 00000020 80000000 00000020`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 4,
  "reference": [
    [
      100871908,
      2753536
    ]
  ],
  "candidate": [
    [
      100871908,
      2621452
    ]
  ]
}
```

Source SHA-256: `9ae72b5db63741447c5f181f7a64df9794b12ee299d9b31463b91f5671d15514`.

### `02242fb0` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `7fffffff 00000001 80000000 7fffffff`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 4,
  "reference": [
    [
      2753552,
      0
    ]
  ],
  "candidate": [
    [
      2753551,
      0
    ]
  ]
}
```

Source SHA-256: `8b71b3ff6599070aa95895c558db36421c309cd1a162dcca286299c097c77159`.

### `02243590` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `7fffffff 002810cc 00000020 80000000`. Class: address form only (cart 0x022... vs HWRAM 0x060...).

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100745000,
      2147483647,
      100885798
    ]
  ],
  "candidate": [
    [
      100745000,
      2147483647,
      35857702
    ]
  ]
}
```

Source SHA-256: `af74d9e69e31802e76a61362501742d402fb089ad6db9a0757efa89b22bc66c3`.

### `022447d4` — r0 — NEW

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `00280034 002810a8 ffffffff 00000020`. Class: return value.

```json
{
  "kind": "r0",
  "reference": "00280035",
  "candidate": "00000001"
}
```

Source SHA-256: `c65d235a5c4580830ce5633de01b033a5510d2c0693cae0839d423c34c7eb0c7`.

### `02244820` — r0 — retained v1.03 FAIL

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `00280068 00281028 7fffffff 00000002`. Class: return value.

```json
{
  "kind": "r0",
  "reference": "00280068",
  "candidate": "00000000"
}
```

Source SHA-256: `8151db52ae5f959ec361d3f2359bb60f7b080743d3a88b942142255ad78a971a`.

### `02245632` — calls — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 00281010 002820e8 00000007`. Class: call target/argument/count.

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [
    [
      100966340,
      2621440,
      101291396
    ]
  ],
  "candidate": []
}
```

Source SHA-256: `6e10fe4d0af488b6717c582ae699dcb68a27c19557b857798e8981535d2e4bfe`.

### `02246052` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0000000f ffffffff 0000000f 80000000`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 42,
  "reference": [
    [
      101291908,
      0
    ]
  ],
  "candidate": [
    [
      101291904,
      0
    ]
  ]
}
```

Source SHA-256: `9662f6b98f393c62fb4339f8b7c1a77e7404573239cdda715958f134a3fa0856`.

### `02246254` — memory write — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 ffffffff 0000001f 00000007`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "00280000",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `3a286566911ca189c693701062eea9a3813baf60ef4dbca0dd75596f8b6a7389`.

### `022469f2` — memory write trace — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800f8 00000002 7fffffff 00000000`. Class: store order / intermediate value (final footprint equal).

```json
{
  "kind": "memory write trace",
  "index": 18,
  "reference": [
    [
      2621915,
      0
    ]
  ],
  "candidate": [
    [
      2621915,
      1
    ]
  ]
}
```

Source SHA-256: `3ba581a83064986b49eb87f97cdf993629f7a16895a6da192b1a5bdf0fc2bc4c`.

### `02249240` — memory write — retained v1.03 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280014 7fffffff 00000020 0000000f`. Class: final memory footprint.

```json
{
  "kind": "memory write",
  "address": "002800d8",
  "reference": 127,
  "candidate": 255
}
```

Source SHA-256: `24886ded7ded86a2f31613caf729dc93e1fa9ec4a4c90768c18c9b29caed8902`.

## SKIP histogram (298)

Grouped first reasons (addresses/PCs folded):

| Category | Files |
| --- | ---: |
| reference cannot run: unmapped 4-byte access | 71 |
| compiler runtime/nonstandard register ABI | 32 |
| reference cannot run: no return after 20000 steps | 24 |
| private stack address passed to callee | 23 |
| unresolved identifier | 22 |
| reference cannot run: unmapped 1-byte access | 20 |
| reference cannot run: unmapped 2-byte access | 17 |
| variadic/non-word callee ABI | 14 |
| unknown/ambiguous callee argument count | 12 |
| computed tail/switch target not a known callee | 6 |
| candidate cannot run: unmapped C memory | 6 |
| shadowed local has conflicting types | 5 |
| callee prototype has unsupported return type | 5 |
| depends on non-argument registers / undefined r0 | 5 |
| local/unsupported address taking | 4 |
| reference cannot run: cannot execute '.word 0xFFFF' | 4 |
| static/multi-declarator stack-backed local | 4 |
| compiler runtime/nonstandard callee ABI | 4 |
| unresolved callee | 3 |
| writes to original code/rodata image | 3 |
| no original bytes in main program image | 2 |
| local aggregate access | 2 |
| unsupported guest type: double | 2 |
| preprocessor: <stdin>:2:10: fatal error: stdarg.h: No such file or directory | 2 |
| non-scalar guest load | 1 |
| reference cannot run: cannot execute '.word 0x0609' | 1 |
| reference cannot run: cannot execute '.word 0x0000' | 1 |
| reference cannot run: cannot execute '.word 0x8668' | 1 |
| variable/incomplete guest array layout | 1 |
| floating/string/wide constant | 1 |
| **Total** | **298** |

Exact reasons:

| Exact SKIP reason | Files |
| --- | ---: |
| compiler runtime/nonstandard register ABI | 32 |
| private stack address passed to callee | 23 |
| variadic/non-word callee ABI | 14 |
| computed tail/switch target not a known callee | 6 |
| reference cannot run: unmapped 4-byte access at FFFFFF00 | 6 |
| shadowed local has conflicting types | 5 |
| reference cannot run: unmapped 4-byte access at 13579BE7 | 5 |
| callee prototype has unsupported return type | 5 |
| depends on non-argument registers / undefined r0 | 5 |
| local/unsupported address taking | 4 |
| unresolved identifier: reg_ffffffb0 | 4 |
| static/multi-declarator stack-backed local | 4 |
| compiler runtime/nonstandard callee ABI | 4 |
| unresolved identifier: g_curActorObj | 4 |
| unresolved identifier: reg_20100063 | 3 |
| writes to original code/rodata image | 3 |
| no original bytes in main program image | 2 |
| reference cannot run: unmapped 4-byte access at 06280602 | 2 |
| unresolved identifier: reg_25F80000 | 2 |
| reference cannot run: unmapped 1-byte access at 0631DBB0 | 2 |
| local aggregate access | 2 |
| unsupported guest type: double | 2 |
| reference cannot run: unmapped 4-byte access at 13579BDF | 2 |
| reference cannot run: unmapped 4-byte access at 13579BF3 | 2 |
| reference cannot run: unmapped 1-byte access at 20100077 | 2 |
| reference cannot run: unmapped 2-byte access at 25D00004 | 2 |
| preprocessor: <stdin>:2:10: fatal error: stdarg.h: No such file or directory | 2 |
| reference cannot run: unmapped 1-byte access at 0634BBED | 2 |
| unresolved identifier: g_glyph_param0 | 2 |
| reference cannot run: unmapped 1-byte access at 063192C0 | 2 |
| unknown/ambiguous callee argument count: 62473fe7 | 1 |
| reference cannot run: unmapped 1-byte access at 06339BED | 1 |
| unknown/ambiguous callee argument count: abdc8b7f | 1 |
| reference cannot run: unmapped 1-byte access at 06338BEC | 1 |
| unknown/ambiguous callee argument count: 06026efc | 1 |
| reference cannot run: unmapped 4-byte access at BFA81A5B | 1 |
| reference cannot run: unmapped 4-byte access at 361CDBEE | 1 |
| reference cannot run: unmapped 4-byte access at 48603AAF | 1 |
| reference cannot run: unmapped 4-byte access at 48D4A02E | 1 |
| unknown/ambiguous callee argument count: a9e547b9 | 1 |
| unknown/ambiguous callee argument count: c1c2ef80 | 1 |
| unknown/ambiguous callee argument count: 2c8dfae8 | 1 |
| reference cannot run: unmapped 1-byte access at 06318A88 | 1 |
| reference cannot run: unmapped 1-byte access at 0633ABEC | 1 |
| unknown/ambiguous callee argument count: 06027460 | 1 |
| candidate cannot run: unmapped C memory 329e4264/4 | 1 |
| unresolved identifier: g_vdp2Shadow | 1 |
| reference cannot run: unmapped 1-byte access at CDEF05A3 | 1 |
| reference cannot run: unmapped 4-byte access at DBC75223 | 1 |
| reference cannot run: unmapped 4-byte access at D92E59D9 | 1 |
| reference cannot run: unmapped 4-byte access at 823313B3 | 1 |
| reference cannot run: unmapped 4-byte access at 5BA9B5A0 | 1 |
| reference cannot run: unmapped 4-byte access at 1FCEEF96 | 1 |
| reference cannot run: unmapped 4-byte access at 98D2CA42 | 1 |
| reference cannot run: unmapped 4-byte access at 97E3DA3E | 1 |
| reference cannot run: unmapped 4-byte access at 32F1049C | 1 |
| reference cannot run: unmapped 4-byte access at CDB1D966 | 1 |
| unresolved callee: trig_lookup | 1 |
| candidate cannot run: unmapped C memory 00000008/4 | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 06013C16 | 1 |
| reference cannot run: unmapped 1-byte access at 8C166142 | 1 |
| reference cannot run: no return after 20000 steps (pc 06013DA4) | 1 |
| reference cannot run: unmapped 1-byte access at B570312F | 1 |
| reference cannot run: unmapped 4-byte access at 5A7C0680 | 1 |
| reference cannot run: unmapped 2-byte access at F49C10DA | 1 |
| unresolved identifier: atan2_octant | 1 |
| reference cannot run: no return after 20000 steps (pc 060172A2) | 1 |
| reference cannot run: no return after 20000 steps (pc 060172B4) | 1 |
| reference cannot run: unmapped 2-byte access at 402820F8 | 1 |
| non-scalar guest load | 1 |
| reference cannot run: cannot execute '.word 0x0609' at 06017D18 | 1 |
| reference cannot run: unmapped 4-byte access at 535FED18 | 1 |
| reference cannot run: unmapped 4-byte access at 13579BCF | 1 |
| reference cannot run: no return after 20000 steps (pc 06018336) | 1 |
| reference cannot run: unmapped 4-byte access at 25FE007C | 1 |
| reference cannot run: unmapped 4-byte access at 40000020 | 1 |
| reference cannot run: unmapped 4-byte access at 46099584 | 1 |
| reference cannot run: unmapped 4-byte access at 802A3410 | 1 |
| reference cannot run: unmapped 4-byte access at D8E86053 | 1 |
| unresolved identifier: reg_25C00000 | 1 |
| reference cannot run: no return after 20000 steps (pc 06018ED2) | 1 |
| reference cannot run: no return after 20000 steps (pc 06018ED4) | 1 |
| reference cannot run: unmapped 2-byte access at 25D00010 | 1 |
| unresolved callee: dma_ch0_transfer | 1 |
| reference cannot run: unmapped 2-byte access at 00000007 | 1 |
| reference cannot run: cannot execute '.word 0x0000' at 060194FA | 1 |
| reference cannot run: unmapped 2-byte access at 06137C84 | 1 |
| reference cannot run: unmapped 4-byte access at 12B11480 | 1 |
| reference cannot run: unmapped 2-byte access at 328E5B97 | 1 |
| reference cannot run: unmapped 4-byte access at 08189024 | 1 |
| reference cannot run: unmapped 2-byte access at 53CD4BC8 | 1 |
| reference cannot run: unmapped 2-byte access at A388BBA2 | 1 |
| reference cannot run: unmapped 2-byte access at 25B00400 | 1 |
| reference cannot run: unmapped 4-byte access at 25A03C20 | 1 |
| unknown/ambiguous callee argument count: 0601a258 | 1 |
| reference cannot run: unmapped 2-byte access at 3A40C83A | 1 |
| reference cannot run: no return after 20000 steps (pc 0601AB4E) | 1 |
| reference cannot run: unmapped 4-byte access at A1BFC798 | 1 |
| reference cannot run: unmapped 4-byte access at F88B4886 | 1 |
| reference cannot run: unmapped 4-byte access at 18282707 | 1 |
| reference cannot run: unmapped 4-byte access at AF3D991E | 1 |
| reference cannot run: unmapped 4-byte access at 4DC0DB91 | 1 |
| reference cannot run: unmapped 4-byte access at C5133AC4 | 1 |
| candidate cannot run: unmapped C memory bad7b1c4/4 | 1 |
| reference cannot run: unmapped 1-byte access at 137F9FDF | 1 |
| candidate cannot run: unmapped C memory db9c7de1/4 | 1 |
| candidate cannot run: unmapped C memory 0d3e44df/4 | 1 |
| reference cannot run: unmapped 4-byte access at 983591C5 | 1 |
| unknown/ambiguous callee argument count: 06013ece | 1 |
| reference cannot run: unmapped 2-byte access at FFFFE8FC | 1 |
| reference cannot run: unmapped 4-byte access at 39FDB527 | 1 |
| reference cannot run: cannot execute '.word 0x8668' at 0602477E | 1 |
| reference cannot run: no return after 20000 steps (pc 06024882) | 1 |
| reference cannot run: no return after 20000 steps (pc 06024B5A) | 1 |
| reference cannot run: no return after 20000 steps (pc 0602533A) | 1 |
| reference cannot run: no return after 20000 steps (pc 06026146) | 1 |
| reference cannot run: no return after 20000 steps (pc 0602641A) | 1 |
| reference cannot run: no return after 20000 steps (pc 060264E4) | 1 |
| unresolved identifier: reg_25E5FFFE | 1 |
| reference cannot run: no return after 20000 steps (pc 0602704C) | 1 |
| reference cannot run: no return after 20000 steps (pc 060277F2) | 1 |
| reference cannot run: no return after 20000 steps (pc 06027A86) | 1 |
| reference cannot run: unmapped 4-byte access at FE13E81C | 1 |
| reference cannot run: no return after 20000 steps (pc 06027C9E) | 1 |
| unresolved identifier: reg_25D00000 | 1 |
| unresolved identifier: reg_25FE0090 | 1 |
| reference cannot run: unmapped 4-byte access at 04FD2D5B | 1 |
| reference cannot run: unmapped 4-byte access at F5447BE6 | 1 |
| reference cannot run: no return after 20000 steps (pc 0602BB7C) | 1 |
| variable/incomplete guest array layout | 1 |
| reference cannot run: unmapped 1-byte access at 06318A74 | 1 |
| unresolved identifier: D_02226148 | 1 |
| unknown/ambiguous callee argument count: 0602d4b4 | 1 |
| reference cannot run: unmapped 2-byte access at 137FACCB | 1 |
| unresolved callee: actor_state_select | 1 |
| reference cannot run: unmapped 4-byte access at 54720277 | 1 |
| reference cannot run: unmapped 1-byte access at 06348BED | 1 |
| unknown/ambiguous callee argument count: 060194fc | 1 |
| reference cannot run: no return after 20000 steps (pc 0603735C) | 1 |
| reference cannot run: no return after 20000 steps (pc 06037444) | 1 |
| reference cannot run: unmapped 2-byte access at 28AC5946 | 1 |
| reference cannot run: unmapped 4-byte access at 00000034 | 1 |
| reference cannot run: unmapped 4-byte access at 00000033 | 1 |
| reference cannot run: unmapped 4-byte access at 5CE8995C | 1 |
| reference cannot run: unmapped 1-byte access at 19606FC7 | 1 |
| reference cannot run: unmapped 4-byte access at 36112BF0 | 1 |
| reference cannot run: no return after 20000 steps (pc 06040C38) | 1 |
| reference cannot run: unmapped 2-byte access at 19B20688 | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 06041E0E | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 060423BA | 1 |
| reference cannot run: no return after 20000 steps (pc 06042430) | 1 |
| candidate cannot run: unmapped C memory 06100004/4 | 1 |
| reference cannot run: unmapped 4-byte access at 25FE0090 | 1 |
| unknown/ambiguous callee argument count: 060193f6 | 1 |
| reference cannot run: unmapped 4-byte access at 6D671ED7 | 1 |
| reference cannot run: no return after 20000 steps (pc 06043FAE) | 1 |
| reference cannot run: unmapped 4-byte access at 67323215 | 1 |
| reference cannot run: unmapped 4-byte access at 637D6AD8 | 1 |
| reference cannot run: unmapped 4-byte access at 30F37C20 | 1 |
| reference cannot run: unmapped 4-byte access at 66D5F114 | 1 |
| reference cannot run: no return after 20000 steps (pc 06047524) | 1 |
| reference cannot run: unmapped 1-byte access at 063572C0 | 1 |
| reference cannot run: unmapped 4-byte access at 06A9E5B0 | 1 |
| reference cannot run: unmapped 2-byte access at 137E1099 | 1 |
| reference cannot run: unmapped 4-byte access at 13579BEF | 1 |
| reference cannot run: unmapped 4-byte access at 79652B6C | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 06049C76 | 1 |
| reference cannot run: unmapped 4-byte access at 4F7F22C3 | 1 |
| reference cannot run: unmapped 4-byte access at C6EB8EF0 | 1 |
| floating/string/wide constant | 1 |
| reference cannot run: unmapped 4-byte access at 9FCDF598 | 1 |
| reference cannot run: unmapped 4-byte access at 3CEA9B02 | 1 |
| reference cannot run: unmapped 4-byte access at 05ED3E93 | 1 |
| **Total** | **298** |

## Validation

**54 regression tests pass** in `tools/decomp/test_difftest_v104.py`: the 35 retained v1.03 contract tests, re-run against v1.04, plus 19 new ones. `tools/decomp/test_difftest.py` still selects its module with `DIFFTEST_MODULE` and passes for both v1.03 and v1.04. One retained test changed: it now uses `dtoi` (unmodelled in every version) instead of `udiv32` for "an unmodelled runtime helper stays SKIP". The new tests cover runtime models against the original helper bytes, plus inline execution with an argument-swap mutation that must FAIL. They also cover native static helpers (a mutated helper must FAIL), unnamed prototype parameters, direct-arity precedence and its length-mismatch SKIP, image function-pointer arity, the pointer-to-function-pointer fix against SH-2, narrow callee upper-bit SKIP versus a low-bit FAIL, `long long` with a shift mutation, narrow entry extension against SH-2, and frame arrays, structs and address-taken scalars (an initializer mutation must FAIL). Stack-escape SKIPs, the real-cart `sound_queue_push` index bound (an increment mutation must FAIL), a `dt` loop bound against SH-2 (a mutation must FAIL), real-cart switch-table analysis, the intra-function computed jump versus external SKIP, the pointer-returning stub on both sides (a mutation must FAIL) and the read-only cart ROM on both sides are covered too.

## Remaining limits

Hardware registers (`0x25xxxxxx`, on-chip `0xFFFFFFxx`), HWRAM mirrors (`0x0628xxxx`), mid-function fragments that
inherit caller registers, pointers created at run time by arithmetic the analysis does not follow, deeper chains under
seeded objects, buffers passed to callees, varargs/non-word ABIs, floating point, unmodelled runtime helpers and the
runtime helper entries themselves (r0/r1 register ABI, 32 files) stay SKIP. Hardware-name identifiers (`reg_25F80000`,
...) remain unresolved by design. PASS is bounded evidence in this isolated integer/stub model, not a proof or gameplay validation.

