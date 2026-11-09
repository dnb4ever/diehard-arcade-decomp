# Differential tester v1.03 report

Run date: 2026-10-31. Final `--all` scan: **1,099 C files**, **16 vectors per candidate**. Implementation: `tools/decomp/difftest_v103.py`, installed atomically as `tools/decomp/difftest.py` only after validation. The identical v1.02 entry point is preserved as `tools/decomp/difftest_v102.py`.

```sh
DIFFTEST_MODULE=difftest_v103 tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest.py
tools/decomp/venv/bin/python tools/decomp/difftest_v103.py --all --vectors 16 --jobs 4 --output-dir /tmp/difftest-v103-final
```

| Result | v1.02 historical (1,095 files) | v1.03 final |
| --- | ---: | ---: |
| PASS | 36 | 68 |
| PASS-void | 176 | 453 |
| FAIL | 17 | 29 |
| SKIP | 866 | 549 |

The 521 passing candidates completed **8,336 complete passing vector comparisons**. The 29 FAILs are observable mismatches under the isolated stub model, not failed harness regressions. `--all` correctly exits **1** when it finds FAILs. All **35 harness regression tests pass**.

Cart SHA-256: `309603533747f9e235c57576ec04b4c6274338b4a46efcf1a834fdd4f7d88ea9`.

v1.03 harness SHA-256: `b0b8237e5ca1b6b7a0a733cfd6c60f2c4e33d35f2c26aeabb6df1ac4ed5fe8e8`.

SH-2 interpreter SHA-256: `c43458498cde2f4b727aab6298cfba84487b03fcecd544161ab0c7007e13c253`.

Preserved v1.02 harness SHA-256: `7c4888b1ab4db8ee395f352b2ecd04febb8f636c30d30cba2d626659bcaa4f61`.

Full JSON and isolated attempt logs: `/tmp/difftest-v103-final/`; console: `/tmp/difftest-v103-final.log`; regression output: `/tmp/difftest-v103-tests.log`. These temporary artifacts do not overwrite the other agent's `work/difftest_all.json`. This report preserves all counts, exact SKIP reasons and full first differences with source hashes.

## Source revisions and coverage attribution

Another agent added 4 source files and changed 12 existing sources during this session. This task did not edit any project source. Historical and final totals therefore include both harness improvements and those independent source changes. There are **1083 shared functions with identical source hashes**; only that cohort is used for the feature transitions below.

Added source addresses: `02201102`, `02210b58`, `0223845c`, `022386c6`.

| Independently changed source | v1.02 result | v1.03 result | Final tested source SHA-256 |
| --- | --- | --- | --- |
| `02202c5c` | FAIL | PASS-void | `30adafce6f2d804659b67e944125f953e65874519f714ec7eaf4121be52de2a0` |
| `0220f990` | FAIL | PASS-void | `09bbf3b462d9478d3e7a6711fa54d079699c700f9885565d3cda6b2318b6b4d3` |
| `022104f6` | FAIL | PASS-void | `b883aa0ffa599d0b270980dc9cc171efd5813782b8c973b23a1f96124960fcdf` |
| `02210622` | FAIL | PASS-void | `0286e7977fb35e90bc534f3c2d8e1ce485c30ace6799132aef0331e9f82ea9b6` |
| `0221ad58` | FAIL | PASS-void | `51dd9e2319785eeb22a31f495698719a5317f7af911320790700b8467c7bfdd9` |
| `02227602` | FAIL | PASS-void | `f63d03ba094c6ad292f40076e22781890abe34b3e248d4feb7980d58d6389fb7` |
| `02227790` | FAIL | PASS-void | `5cb384d9f02e212ae0f5857b886a2f8d87b1a7caa8f10afd2b5275765aab8da7` |
| `0222ae40` | FAIL | PASS-void | `46279e042e3a23ffd303879665fe59273a4d7b3ac1ad2bec1886dbcae3b2b14b` |
| `02233122` | FAIL | PASS-void | `273acf2577f6c750f5f40783b5a6a905e6bdfc0337006d4850b4f68840a3e7ea` |
| `0223b254` | FAIL | PASS-void | `42bcfe1e5883e5cb2f0e1a7d6673dc1dc78c2a2638d3de6453a7cd12a60fca7d` |
| `022418a0` | FAIL | PASS-void | `be705f07355a7fea7de462cb43e3c8fe148d3a7e37b9a66d666cff658cbfa1c8` |
| `02246cac` | FAIL | PASS-void | `8038338966a65ecf811010e41f88dddc4a16b2adce43fc87d267654cfa881ff5` |

All **212 unchanged-source v1.02 PASS/PASS-void entries** retain their result; regressions: **0**. Historical FAILs that now pass have independently edited sources, as recorded above. No reference fault is suppressed to obtain a passing result.

The saved v1.02 report/JSON has 264 compound-assignment first rejections and 213 `reference cannot run` rejections (the task quoted 259/200). The table uses the saved results and excludes independently changed sources. A remaining SKIP may have advanced to a different blocker.

| v1.02 first blocker | Historical files | Identical-source cohort | Now PASS | Now PASS-void | Now FAIL | Still SKIP |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| compound memory assignment | 264 | 264 | 4 | 169 | 14 | 77 |
| reference cannot run | 213 | 213 | 7 | 56 | 6 | 144 |
| local array/function declaration | 47 | 47 | 0 | 0 | 0 | 47 |
| non-integer or aggregate type | 60 | 60 | 18 | 2 | 0 | 40 |
| unresolved callee | 52 | 52 | 0 | 12 | 2 | 38 |

## Implementation and retained honesty rules

Compound memory assignments (`+=`, `-=`, `*=`, `/=`, `%=`, shifts and bitwise operators) lower to a single address evaluation, an instrumented typed read, the C operation and destination conversion, and an instrumented write. GNU statement expressions provide unique local temporaries for nested assignments. Signed/unsigned 8-, 16- and 32-bit types retain integer promotion, signed loads, destination narrowing and expression-result semantics. Pointer-valued memory updates scale by the guest pointee size. Prefix/postfix memory increments are instrumented with the correct old/new result. Unchanged-value writes and intermediate stores remain observable.

Scalar struct/union members use explicit SH-2 layouts: pointers are four bytes, member alignment is at most four, arrays retain their guest element stride, and nested members use guest addresses. This covers patterns such as `M(a)->flags |= bits` without native pointer dereferences. Natural-layout aggregates containing supported scalar/pointer members are supported through pointers; packed layouts, bitfields, anonymous members, aggregate values/ABI arguments and local aggregate storage remain unsupported. Guest `sizeof` is evaluated from retained types. Pointer return declarations remain word-valued functions rather than losing their parameter lists. Plain `char` uses signed-byte semantics.

Structured input generation combines C pointer parameter types and used pointer globals with a **static SH-2 provenance analysis**. The analysis follows entry r4-r7 and initial stack words, register copies, PC-relative literal pools, constant pointer offsets, loads subsequently used as addresses, both branch successors and delay slots. It does not execute a trial reference. It is bounded at 12,000 analysis states, eight states per PC and four nested pointer loads. It can miss shapes involving complex arithmetic, arbitrary table indices, deep/cyclic lists, runtime-created pointers or unsupported instructions; those executions remain SKIP when they cannot complete.

Each vector first randomizes both 1 MiB RAMs using `0xd1ff0000 XOR canonical entry`, restores the original high-RAM code/rodata image, and chooses scalar words from the unchanged edge/random schedule. Declared pointer parameters retain the v1.02 mapped scratch schedule. Statically inferred pointer arguments acquire mapped scratch addresses; inferred pointer-valued global/field words are seeded with distinct mapped scratch objects. Up to 80 such objects are placed from `0x002a0400` in `0x1000` steps. Other scratch bytes remain randomized. Input generation does not seed hardware or any word overlapping code/rodata or private stack RAM. The complete initial state is then frozen and shared by the reference, alternate-register reference, and native candidate.

There are **no reference retries, fault-driven repairs, runtime remappings, discarded vectors or candidate-dependent comparison filters**. A reference exception immediately makes the function SKIP with the precise exception text, including after earlier vectors completed. JSON retains static `pointer_constraints`, `typed_pointer_globals` and the last/failing vector's `input_setup` edits, plus entry words for FAIL/SKIP and per-source hashes. Static input constraints are bounded test domains, not proof of gameplay invariants.

Known names in `notes/symbols.txt` now supplement address-named and source-derived callees. Targets are canonicalized only for execution/stub lookup, and conflicting name-to-address definitions remain unresolved. Block-scope function declarations retain their signatures and are omitted from native generated code because calls use the instrumentation stub. Missing/old-style prototypes still require consistent call-site arities and are reported as inferred. Unknown addresses, conflicting signatures, variadic/non-word ABIs and compiler-runtime helper ABIs stay SKIP. Local arrays remain unsupported rather than being silently converted into host storage.

The reference still executes original SH-2 bytes from `cart.bin[0x200000:0x28de20]` at `0x06004000`. Cart-form entry addresses map to HWRAM. A completed vector compares unsigned r0 bit patterns, the final externally visible written-byte footprint (including unchanged-value writes), the chronological byte-normalized store trace (including intermediate values), and ordered callee events. Void functions omit only r0 and remain labelled PASS-void. Only private stack stores in `[SP-0x8000, SP+0x100)` are excluded. No memory, call or return difference is excused because the final state later converges.

Callee events retain **raw target values and precisely the declared word arguments**. r4-r7 hold the first four; later arguments are read from call-time SP after the SH-2 delay slot. Direct/indirect calls and known tail calls use the same deterministic target/argument hash; stubs have no side effects and preserve all registers except r0. Callback targets are never followed. Raw cart/HWRAM argument differences remain differences. Unknown/computed tail boundaries and ambiguous indirect arities remain SKIP, and compiler-runtime helper exclusions take precedence over inferred arity.

Signed memory macro semantics still come from the actual header (`project/src_functional/dh_band21.h`): MEM32/MEM16 signed, MEMU32/MEMU16 unsigned, MEM8 unsigned. Guest pointers, casts and arithmetic remain 32-bit; instrumented accesses remain big-endian and never dereference guest values as native pointers. Two reference runs vary non-argument registers/CPU flags; observable dependence or undefined r0 remains SKIP. GCC retains wrapping signed arithmetic, uninitialized diagnostics and shift-exponent/division-by-zero/bounds sanitizers. Limits remain 20,000 SH-2 instructions, two seconds per native vector and 100,000 candidate events. Faults, hardware accesses, invalid pointers, code/rodata writes, unsupported execution and timeouts remain SKIP. PASS is bounded evidence in this isolated integer/stub model, not a proof or gameplay validation.

## Validation

**35 regression tests pass (25 retained, 10 new).** New checks cover every compound operator, signed/unsigned byte/halfword/word conversions, wrapping and narrowing, single lvalue evaluation, nested stores, pointer stride, postfix increments, scalar struct fields/padding/array fields, explicit unsupported aggregate cases, pointer-return declarations and block-scope prototypes. Static inference checks register-copied integer pointer arguments, nested pointer loads and global pointer chains against original SH-2 execution. Mutations change observable results/stores/calls and must FAIL. Fault regressions verify immediate SKIP without retries both at vector 0 and after a completed passing vector; setup cannot modify code/rodata, private stack or hardware.

Real-cart `0220121e` passes with a seeded global stream pointer; changing the cursor increment from 4 to 8 must FAIL. Real-cart `0221080a` passes with compound stores; changing its matrix-stack increment from `0x30` to `0x31` must FAIL. The retained `02201b8e` void-store and `0222c322` callback-argument mutation checks still require FAIL. All candidate mutations use temporary files; no project source is edited.

The six established known-good functions pass **64 vectors each**, 384 comparisons: `02210020`, `02213164`, `0222f3ea`, `0222f40e`, `0223e5bc`, `02243fba`. Results: `/tmp/difftest-v103-validation/results.json`.

```sh
tools/decomp/venv/bin/python tools/decomp/difftest_v103.py 02210020 02213164 0222f3ea 0222f40e 0223e5bc 02243fba --vectors 64 --jobs 4 --output-dir /tmp/difftest-v103-validation --json /tmp/difftest-v103-validation/results.json
```

## New FAILs

**24 newly exposed FAILs** relative to the saved v1.02 results. They are listed separately here and marked NEW in the complete first-difference list below.

| Address | First difference | v1.02 blocker |
| --- | --- | --- |
| `02202a28` | memory write | compound memory assignment |
| `02202d7e` | memory write | unresolved callee: spawn_obj_0x6c |
| `0220903e` | memory write trace | unsupported guest type: char |
| `02213d24` | memory write trace | reference cannot run: unmapped 1-byte access at 00000001 |
| `02215000` | memory write trace | compound memory assignment |
| `02216714` | memory write | reference cannot run: unmapped 2-byte access at 78827D6D |
| `022173f4` | memory write | reference cannot run: unmapped 4-byte access at 00000007 |
| `0221885c` | calls | compound memory assignment |
| `0221a452` | memory write | compound memory assignment |
| `0221c4ac` | calls | compound memory assignment |
| `0222baf8` | memory write | compound memory assignment |
| `0222bdf2` | calls | compound memory assignment |
| `0222d0e8` | calls | compound memory assignment |
| `0222f84e` | memory write | unsupported guest type: char |
| `02233250` | calls | reference cannot run: unmapped 2-byte access at 900B404D |
| `02240e54` | memory write trace | compound memory assignment |
| `02240efe` | memory write trace | compound memory assignment |
| `022415ee` | calls | unresolved callee: actor_facing_clamp |
| `02241b2c` | memory write | compound memory assignment |
| `02245632` | calls | compound memory assignment |
| `02246180` | memory write | compound memory assignment |
| `02246254` | memory write | reference cannot run: unmapped 2-byte access at 33598A87 |
| `022469f2` | memory write trace | compound memory assignment |
| `022470a2` | memory write trace | reference cannot run: unmapped 4-byte access at 604EE44F |

## Complete FAIL list with first differences

All **29 FAILs** follow. Vector/event indices are zero-based. Entry words are r4-r7 followed by initial stack words. Call events begin with the raw target, followed by arguments. A memory difference containing `null` means that side did not write the byte. Every mismatch is evidence under the isolated stub model; the report does not assert that all are proven gameplay bugs.

### `02202a28` — memory write — NEW

Vector 1; r0 checked: False; completed comparisons: 2. Entry words: `002800f4 00000001 80000000 ffffffff`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002a0408",
  "reference": 0,
  "candidate": 32
}
```

Source SHA-256: `e28a06cb0374c718f379168605bae0b7a569e8440b0562b68f4be1b304c107b2`.

### `02202d7e` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c0 00000020 80000000 80000000`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800c0",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `4967a1a67eb0678f6a3f8dbf0d8d350c7b7e351baa59735f17d680850b60a374`.

### `0220903e` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280098 00000002 0000001f 80000000`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 4,
  "reference": [
    [
      2757634,
      0
    ]
  ],
  "candidate": [
    [
      2757634,
      32
    ]
  ]
}
```

Source SHA-256: `2f8cf3e38ad7cb31a3ffb6ddb7aaecd3b9318b3eb700fbceff8cf0e133bca98e`.

### `02213d24` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 00281400 00282400 0000001f`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 2,
  "reference": [
    [
      2626576,
      128
    ]
  ],
  "candidate": [
    [
      2626562,
      248
    ]
  ]
}
```

Source SHA-256: `fb40c0e707bae536aa2200153c5610880a50e342259da9d084d1a09953ae3e18`.

### `02213e0c` — calls — retained v1.02 FAIL

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `7fffffff 00000007 00282024 ffffffff`.

First difference:

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

### `02213f90` — memory write — retained v1.02 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0000000f 002810f4 0028207c 80000000`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002810f4",
  "reference": null,
  "candidate": 0
}
```

Source SHA-256: `fca4f4fd5da578f858025b5eee6877ffb82cacf052fc75daf147fa1d8b299829`.

### `02215000` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0000001f 00000002 00000007 0000000f`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 54,
  "reference": [
    [
      2753648,
      0
    ]
  ],
  "candidate": [
    [
      2753644,
      0
    ]
  ]
}
```

Source SHA-256: `ff1d52a2746f87ccf1065d5b1ea2f2fc34812998489a69c0265c9c1617c782f7`.

### `02216714` — memory write — NEW

Vector 3; r0 checked: False; completed comparisons: 4. Entry words: `00000000 00281400 002820ec 0000001f 00000001`.

First difference:

```json
{
  "kind": "memory write",
  "address": "00282132",
  "reference": 89,
  "candidate": null
}
```

Source SHA-256: `2f80dc7892e6b2ce3c281415a74cebcfa3ed3d4a252170727d6a102b92506e35`.

### `022173f4` — memory write — NEW

Vector 8; r0 checked: True; completed comparisons: 9. Entry words: `ffffffff 00281400 00000002 00000000`.

First difference:

```json
{
  "kind": "memory write",
  "address": "060b889c",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `5ea1ef8cae0b8f040cccc71119f25c8998930881c88d173245038cd5d674c650`.

### `0221885c` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 00000020 0000000f 7fffffff`.

First difference:

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100764240,
      101009184
    ]
  ],
  "candidate": [
    [
      100764240,
      101009696
    ]
  ]
}
```

Source SHA-256: `67be8dd77c6ec779120ba4fd202e70b6c3ca2ce05a0624b2994d5cd938717cc9`.

### `0221a452` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280400 00000007 0000001f 00000020`.

First difference:

```json
{
  "kind": "memory write",
  "address": "00280400",
  "reference": null,
  "candidate": 0
}
```

Source SHA-256: `a5c035d2ebcbce76151e9e9937efb03dd3e3bbc9a854e5aeab81b43e459033bf`.

### `0221c4ac` — calls — NEW

Vector 2; r0 checked: False; completed comparisons: 3. Entry words: `00280400 ffffffff 00000001 7fffffff`.

First difference:

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100764240,
      101138620
    ]
  ],
  "candidate": [
    [
      100764240,
      101142716
    ]
  ]
}
```

Source SHA-256: `8dcc2b88f0e3b26cea0b3067b42abf1bed639f6641e571d5a91f20ee46d46b11`.

### `0222baf8` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `0028004c 00000001 0000001f 0000000f`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800a9",
  "reference": 48,
  "candidate": 51
}
```

Source SHA-256: `8a5640ab0a4e11c9499d6a7c2270cdf7a33631ffa3471edf8c81e8c9c3533110`.

### `0222bdf2` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c4 00000002 0000001f 00000000`.

First difference:

```json
{
  "kind": "calls",
  "index": 0,
  "reference": [],
  "candidate": [
    [
      100867116,
      1470159862,
      2621636
    ]
  ]
}
```

Source SHA-256: `d55c0d172baca7c20cac9f8bed68b5f3c761e3933c266054eaebeec4295bf1e3`.

### `0222d0e8` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280014 7fffffff 00000020 0000001f`.

First difference:

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

### `0222f84e` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c4 ffffffff 00000007 00000007`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800c4",
  "reference": 6,
  "candidate": 0
}
```

Source SHA-256: `bc86a9b0fd9274cbe38f85e17d58102343bc02fd6341852bbaa8e24555216a3e`.

### `02233250` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800f4 80000000 0000001f 00000001`.

First difference:

```json
{
  "kind": "calls",
  "index": 2,
  "reference": [
    [
      100744910,
      2621684,
      100905408
    ]
  ],
  "candidate": [
    [
      100744910,
      2621684,
      35877312
    ]
  ]
}
```

Source SHA-256: `989b012d2ce5990ece1b99048f5eab368c0346bbff9001331ec933f9f4be025a`.

### `02240e54` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800a0 00281080 00000000 00000020`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 2,
  "reference": [
    [
      2621732,
      0
    ]
  ],
  "candidate": [
    [
      2621864,
      0
    ]
  ]
}
```

Source SHA-256: `e2c803bbff3f1642ce13381a4dbf08423f1e34fd0ad066061f71064d433fab61`.

### `02240efe` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800c0 002810c8 00000002 0000001f`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 2,
  "reference": [
    [
      2621764,
      0
    ]
  ],
  "candidate": [
    [
      2621896,
      0
    ]
  ]
}
```

Source SHA-256: `9e4e523622f1dd7293c4235efe86be15482217e5d4607d0e633ceb40b8bf8da6`.

### `022415ee` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280008 00000001 00000001 00000000`.

First difference:

```json
{
  "kind": "calls",
  "index": 1,
  "reference": [
    [
      100757904,
      4294950702
    ]
  ],
  "candidate": [
    [
      100757928,
      4294947997
    ]
  ]
}
```

Source SHA-256: `b2ab32504e2c221669e1f2840eab26fd3115257ba8ceb8b1188a4fab336a4758`.

### `02241b2c` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280030 0000001f 00000002 7fffffff`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800ec",
  "reference": null,
  "candidate": 190
}
```

Source SHA-256: `452e259a22335695613815d9acedc11fa2a412cd446253572289c946261905ea`.

### `02244820` — r0 — retained v1.02 FAIL

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `00280068 00281028 7fffffff 00000002`.

First difference:

```json
{
  "kind": "r0",
  "reference": "00280068",
  "candidate": "00000000"
}
```

Source SHA-256: `8151db52ae5f959ec361d3f2359bb60f7b080743d3a88b942142255ad78a971a`.

### `02245632` — calls — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 00281010 002820e8 00000007`.

First difference:

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

### `02246180` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800a0 ffffffff 00000007 0000000f`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800fc",
  "reference": 154,
  "candidate": 17
}
```

Source SHA-256: `ab8556b9219172c389fafcf0c2d6968f7555eaeb072f3bc76a9a1fbc83b3b379`.

### `02246254` — memory write — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 ffffffff 0000001f 00000007`.

First difference:

```json
{
  "kind": "memory write",
  "address": "00280000",
  "reference": 6,
  "candidate": 2
}
```

Source SHA-256: `3a286566911ca189c693701062eea9a3813baf60ef4dbca0dd75596f8b6a7389`.

### `022462bc` — r0 — retained v1.02 FAIL

Vector 0; r0 checked: True; completed comparisons: 1. Entry words: `002800bc 00000000 00000001 00000007`.

First difference:

```json
{
  "kind": "r0",
  "reference": "981c6d55",
  "candidate": "981c739d"
}
```

Source SHA-256: `ac4cf553c06ee806995e3657a98772466d39140c5d67161d4149e197a80c3b03`.

### `022469f2` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `002800f8 00000002 7fffffff 00000000`.

First difference:

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

### `022470a2` — memory write trace — NEW

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280000 00000007 00000007 00000007`.

First difference:

```json
{
  "kind": "memory write trace",
  "index": 15,
  "reference": [
    [
      2621667,
      0
    ]
  ],
  "candidate": [
    [
      2621667,
      2
    ]
  ]
}
```

Source SHA-256: `f6ec3e86807b3c00321aca67a86fc0a7a2de2a99d68f3f7def5cbf5ead3b530f`.

### `02249240` — memory write — retained v1.02 FAIL

Vector 0; r0 checked: False; completed comparisons: 1. Entry words: `00280014 7fffffff 00000020 0000000f`.

First difference:

```json
{
  "kind": "memory write",
  "address": "002800d8",
  "reference": 127,
  "candidate": 255
}
```

Source SHA-256: `24886ded7ded86a2f31613caf729dc93e1fa9ec4a4c90768c18c9b29caed8902`.

## SKIP histogram

Grouped first-reason categories (preparation and runtime blockers share this table):

| Category | Files |
| --- | ---: |
| reference cannot run | 211 |
| local array/function declaration | 53 |
| unresolved callee | 40 |
| compiler runtime/nonstandard callee ABI | 38 |
| non-integer or aggregate type | 32 |
| compiler runtime/nonstandard register ABI | 32 |
| unknown/ambiguous callee argument count | 31 |
| callee prototype has unsupported parameter type | 22 |
| unresolved identifier | 18 |
| unsupported host type | 13 |
| variadic/non-word callee ABI | 11 |
| local/unsupported address taking | 11 |
| computed tail/switch target not a known callee | 9 |
| non-word parameter ABI | 8 |
| shadowed local has conflicting types | 5 |
| writes to original code/rodata image | 3 |
| no original bytes in main program image | 2 |
| candidate cannot run | 2 |
| local aggregate access | 2 |
| callee prototype has unsupported return type | 2 |
| preprocessor | 2 |
| depends on non-argument registers / undefined r0 | 1 |
| floating/string/wide constant | 1 |
| **Total** | **549** |

Full exact reason histogram, retaining unmapped addresses and execution-limit PCs:

| Exact SKIP reason | Files |
| --- | ---: |
| local array/function declaration | 53 |
| compiler runtime/nonstandard callee ABI | 38 |
| non-integer or aggregate type | 32 |
| compiler runtime/nonstandard register ABI | 32 |
| unknown/ambiguous callee argument count | 31 |
| callee prototype has unsupported parameter type | 22 |
| variadic/non-word callee ABI | 11 |
| local/unsupported address taking | 11 |
| unsupported host type: long long | 11 |
| computed tail/switch target not a known callee | 9 |
| non-word parameter ABI | 8 |
| shadowed local has conflicting types | 5 |
| unresolved callee: hud_print | 4 |
| unresolved identifier: reg_ffffffb0 | 4 |
| unresolved callee: draw_obj | 4 |
| unresolved identifier: reg_20100063 | 3 |
| writes to original code/rodata image | 3 |
| unresolved callee: anim_tbl | 3 |
| unresolved identifier: g_curActorObj | 3 |
| no original bytes in main program image | 2 |
| reference cannot run: unmapped 4-byte access at 06280602 | 2 |
| reference cannot run: unmapped 4-byte access at 22676078 | 2 |
| local aggregate access | 2 |
| callee prototype has unsupported return type | 2 |
| reference cannot run: unmapped 4-byte access at FFFFFF00 | 2 |
| reference cannot run: unmapped 4-byte access at 13579BF3 | 2 |
| reference cannot run: unmapped 1-byte access at 20100077 | 2 |
| reference cannot run: unmapped 2-byte access at 25D00004 | 2 |
| unresolved callee: attr_dispatch | 2 |
| preprocessor: <stdin>:2:10: fatal error: stdarg.h: No such file or directory | 2 |
| reference cannot run: unmapped 4-byte access at 13579BE7 | 2 |
| reference cannot run: unmapped 4-byte access at 98337231 | 2 |
| reference cannot run: unmapped 1-byte access at 063192C0 | 2 |
| reference cannot run: no return after 20000 steps (pc 0600475A) | 1 |
| reference cannot run: unmapped 1-byte access at 06338BED | 1 |
| unresolved callee: fade_scene_ready | 1 |
| unresolved identifier: reg_25F80000 | 1 |
| unresolved callee: hud_status | 1 |
| reference cannot run: unmapped 4-byte access at D9272604 | 1 |
| reference cannot run: unmapped 1-byte access at 0631DBB0 | 1 |
| reference cannot run: unmapped 1-byte access at 06318A88 | 1 |
| unresolved callee: panel_dir | 1 |
| unresolved callee: credit_render | 1 |
| candidate cannot run: unmapped C memory 329e4264/4 | 1 |
| reference cannot run: unmapped 4-byte access at 226D1FDC | 1 |
| reference cannot run: no return after 20000 steps (pc 0600B494) | 1 |
| reference cannot run: no return after 20000 steps (pc 0600B6B2) | 1 |
| reference cannot run: unmapped 4-byte access at 226EDC0C | 1 |
| reference cannot run: unmapped 4-byte access at 226AEB00 | 1 |
| unresolved callee: push_coef | 1 |
| reference cannot run: unmapped 4-byte access at 226B1DE8 | 1 |
| reference cannot run: unmapped 4-byte access at 226AF5F0 | 1 |
| unresolved callee: push_line | 1 |
| reference cannot run: unmapped 4-byte access at 225DCEBC | 1 |
| unresolved callee: rb_push | 1 |
| unresolved identifier: g_vdp2Shadow | 1 |
| reference cannot run: unmapped 4-byte access at 9DBDDA00 | 1 |
| reference cannot run: unmapped 4-byte access at 4032C1C8 | 1 |
| reference cannot run: unmapped 4-byte access at 31E54010 | 1 |
| reference cannot run: unmapped 4-byte access at 705B2734 | 1 |
| reference cannot run: unmapped 4-byte access at DEBFF920 | 1 |
| reference cannot run: unmapped 4-byte access at 06104890 | 1 |
| reference cannot run: unmapped 4-byte access at 003E0047 | 1 |
| reference cannot run: unmapped 4-byte access at 397558E5 | 1 |
| reference cannot run: unmapped 4-byte access at 83284EA8 | 1 |
| reference cannot run: unmapped 4-byte access at 32F1049C | 1 |
| reference cannot run: unmapped 4-byte access at CDB1D966 | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 06013C16 | 1 |
| reference cannot run: unmapped 1-byte access at 8C166142 | 1 |
| reference cannot run: no return after 20000 steps (pc 06013DA4) | 1 |
| reference cannot run: unmapped 4-byte access at 5A7C0680 | 1 |
| reference cannot run: unmapped 2-byte access at A38708DA | 1 |
| reference cannot run: unmapped 4-byte access at A8E65530 | 1 |
| unresolved callee: spawn_aimed_child | 1 |
| unresolved callee: ashl | 1 |
| reference cannot run: unmapped 4-byte access at 98337397 | 1 |
| unresolved identifier: atan2_octant | 1 |
| reference cannot run: no return after 20000 steps (pc 0601726E) | 1 |
| reference cannot run: no return after 20000 steps (pc 0601727E) | 1 |
| reference cannot run: no return after 20000 steps (pc 060172A2) | 1 |
| reference cannot run: no return after 20000 steps (pc 060172B4) | 1 |
| reference cannot run: no return after 20000 steps (pc 060173BC) | 1 |
| reference cannot run: cannot execute '.word 0x0609' at 06017D18 | 1 |
| reference cannot run: unmapped 4-byte access at 535FED18 | 1 |
| reference cannot run: unmapped 4-byte access at 13579BCF | 1 |
| reference cannot run: no return after 20000 steps (pc 06018336) | 1 |
| unresolved callee: lz_getbit | 1 |
| reference cannot run: unmapped 4-byte access at 25FE007C | 1 |
| reference cannot run: unmapped 4-byte access at 40000020 | 1 |
| reference cannot run: unmapped 4-byte access at 46099584 | 1 |
| reference cannot run: unmapped 4-byte access at 802A3410 | 1 |
| reference cannot run: unmapped 4-byte access at D8E86053 | 1 |
| unresolved identifier: reg_25C00000 | 1 |
| reference cannot run: no return after 20000 steps (pc 06018ED2) | 1 |
| unresolved callee: list_push | 1 |
| reference cannot run: unmapped 2-byte access at 25D00010 | 1 |
| unresolved callee: dma_ch0_transfer | 1 |
| reference cannot run: unmapped 2-byte access at 00000007 | 1 |
| reference cannot run: cannot execute '.word 0x0000' at 060194FA | 1 |
| reference cannot run: unmapped 2-byte access at 061299A4 | 1 |
| reference cannot run: unmapped 4-byte access at 12B11480 | 1 |
| reference cannot run: unmapped 2-byte access at 06116E84 | 1 |
| reference cannot run: unmapped 4-byte access at 08189024 | 1 |
| reference cannot run: unmapped 2-byte access at E301C41A | 1 |
| reference cannot run: unmapped 2-byte access at 25B00400 | 1 |
| reference cannot run: unmapped 4-byte access at 25A03C20 | 1 |
| reference cannot run: unmapped 2-byte access at BECE8D88 | 1 |
| reference cannot run: unmapped 4-byte access at 0D1DB1A4 | 1 |
| reference cannot run: unmapped 1-byte access at 0F48F93C | 1 |
| unresolved callee: init_one | 1 |
| reference cannot run: no return after 20000 steps (pc 0601A4D6) | 1 |
| reference cannot run: no return after 20000 steps (pc 0601A8E0) | 1 |
| reference cannot run: no return after 20000 steps (pc 0601AB4E) | 1 |
| reference cannot run: unmapped 4-byte access at A1BFC798 | 1 |
| reference cannot run: unmapped 2-byte access at 000F0012 | 1 |
| reference cannot run: no return after 20000 steps (pc 0601AE3C) | 1 |
| reference cannot run: unmapped 4-byte access at F88B4886 | 1 |
| reference cannot run: unmapped 4-byte access at 0000000C | 1 |
| depends on non-argument registers / undefined r0 | 1 |
| reference cannot run: unmapped 4-byte access at C5133AC4 | 1 |
| reference cannot run: unmapped 2-byte access at 1D05A4BE | 1 |
| reference cannot run: unmapped 2-byte access at AA7B167C | 1 |
| candidate cannot run: unmapped C memory 0d3e44df/4 | 1 |
| unsupported host type: unsigned long long | 1 |
| reference cannot run: unmapped 4-byte access at 98E8C4EC | 1 |
| reference cannot run: unmapped 2-byte access at FFFFE8FC | 1 |
| reference cannot run: unmapped 4-byte access at 39FDB527 | 1 |
| reference cannot run: unmapped 2-byte access at A274AA66 | 1 |
| unresolved callee: fire2 | 1 |
| reference cannot run: cannot execute '.word 0x8668' at 0602477E | 1 |
| reference cannot run: no return after 20000 steps (pc 06024882) | 1 |
| reference cannot run: no return after 20000 steps (pc 06024B5A) | 1 |
| reference cannot run: no return after 20000 steps (pc 0602533A) | 1 |
| reference cannot run: no return after 20000 steps (pc 06026146) | 1 |
| reference cannot run: no return after 20000 steps (pc 0602641A) | 1 |
| reference cannot run: no return after 20000 steps (pc 060264E4) | 1 |
| unresolved identifier: reg_25E5FFFE | 1 |
| unresolved callee: prio_bits | 1 |
| reference cannot run: unmapped 4-byte access at 3E0ADC50 | 1 |
| reference cannot run: unmapped 4-byte access at BE3C3854 | 1 |
| reference cannot run: no return after 20000 steps (pc 06027A86) | 1 |
| reference cannot run: unmapped 1-byte access at 22D6485C | 1 |
| reference cannot run: no return after 20000 steps (pc 06027C9E) | 1 |
| unresolved identifier: reg_25D00000 | 1 |
| unresolved identifier: reg_25FE0090 | 1 |
| unresolved callee: __attribute__ | 1 |
| unresolved callee: register_clash_hit | 1 |
| reference cannot run: unmapped 4-byte access at 069C8798 | 1 |
| reference cannot run: unmapped 4-byte access at F5447BE6 | 1 |
| reference cannot run: no return after 20000 steps (pc 0602BB7C) | 1 |
| reference cannot run: unmapped 4-byte access at 41BDC75C | 1 |
| unresolved callee: js_find_cursor | 1 |
| reference cannot run: unmapped 4-byte access at F1B8DFA7 | 1 |
| reference cannot run: unmapped 4-byte access at 86920663 | 1 |
| unresolved identifier: D_02226148 | 1 |
| reference cannot run: unmapped 4-byte access at 03213924 | 1 |
| reference cannot run: unmapped 2-byte access at 137FACCB | 1 |
| reference cannot run: unmapped 4-byte access at 9835C235 | 1 |
| unresolved callee: combo_gate | 1 |
| unresolved callee: actor_state_select | 1 |
| reference cannot run: unmapped 2-byte access at 516562D4 | 1 |
| reference cannot run: unmapped 4-byte access at 54720277 | 1 |
| reference cannot run: unmapped 4-byte access at CEB55B40 | 1 |
| reference cannot run: unmapped 4-byte access at B52CA108 | 1 |
| reference cannot run: unmapped 4-byte access at 4F53B8BC | 1 |
| reference cannot run: unmapped 1-byte access at 0633ABED | 1 |
| reference cannot run: unmapped 1-byte access at 0633BBED | 1 |
| reference cannot run: unmapped 4-byte access at 1920EDA0 | 1 |
| reference cannot run: unmapped 4-byte access at 99BF73C6 | 1 |
| reference cannot run: unmapped 1-byte access at 99BCB7DD | 1 |
| reference cannot run: unmapped 4-byte access at 99F4C79E | 1 |
| unresolved callee: seed_sub | 1 |
| reference cannot run: unmapped 4-byte access at 0A98C377 | 1 |
| reference cannot run: unmapped 4-byte access at 226FD568 | 1 |
| reference cannot run: unmapped 2-byte access at EB9C3C55 | 1 |
| unresolved callee: scale_target | 1 |
| unresolved callee: slot_init | 1 |
| unresolved callee: fill_bar | 1 |
| unresolved callee: hud_popup | 1 |
| reference cannot run: unmapped 4-byte access at 225B9610 | 1 |
| reference cannot run: unmapped 1-byte access at 0622F3EF | 1 |
| reference cannot run: unmapped 4-byte access at C86BC574 | 1 |
| reference cannot run: unmapped 4-byte access at 055890D5 | 1 |
| reference cannot run: unmapped 4-byte access at 2948D8DE | 1 |
| reference cannot run: unmapped 4-byte access at C22A7B72 | 1 |
| reference cannot run: unmapped 4-byte access at B1B2997C | 1 |
| reference cannot run: unmapped 4-byte access at 130BE0B4 | 1 |
| reference cannot run: unmapped 1-byte access at 19606FC7 | 1 |
| reference cannot run: unmapped 4-byte access at 913A1B08 | 1 |
| reference cannot run: unmapped 4-byte access at D5900AAB | 1 |
| reference cannot run: unmapped 4-byte access at 9833739E | 1 |
| reference cannot run: unmapped 4-byte access at 9833738E | 1 |
| reference cannot run: unmapped 4-byte access at 0D4D7D7B | 1 |
| reference cannot run: unmapped 4-byte access at 2E72AFF1 | 1 |
| reference cannot run: unmapped 4-byte access at 24EA6A60 | 1 |
| reference cannot run: unmapped 4-byte access at 5D585431 | 1 |
| reference cannot run: unmapped 4-byte access at 28C007D4 | 1 |
| reference cannot run: unmapped 4-byte access at CCE941F4 | 1 |
| reference cannot run: no return after 20000 steps (pc 06040C38) | 1 |
| reference cannot run: unmapped 2-byte access at CC6BB9C0 | 1 |
| reference cannot run: unmapped 4-byte access at 98337210 | 1 |
| reference cannot run: unmapped 4-byte access at 98AFEE30 | 1 |
| unsupported host type: double | 1 |
| reference cannot run: unmapped 4-byte access at F2AEE3F5 | 1 |
| reference cannot run: unmapped 4-byte access at 0414BE81 | 1 |
| unresolved callee: de0e_trigger | 1 |
| unresolved callee: player_special_refresh | 1 |
| reference cannot run: unmapped 4-byte access at 27A7215D | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 060423BA | 1 |
| reference cannot run: no return after 20000 steps (pc 06042430) | 1 |
| reference cannot run: no return after 20000 steps (pc 06042542) | 1 |
| reference cannot run: unmapped 4-byte access at 25FE0090 | 1 |
| reference cannot run: unmapped 4-byte access at E1DF5308 | 1 |
| reference cannot run: unmapped 4-byte access at 9110CB50 | 1 |
| reference cannot run: unmapped 4-byte access at 9A09B0C6 | 1 |
| reference cannot run: unmapped 4-byte access at 9A08CC9A | 1 |
| reference cannot run: unmapped 4-byte access at 9A2196BA | 1 |
| reference cannot run: unmapped 4-byte access at E5DEC175 | 1 |
| reference cannot run: unmapped 4-byte access at EFFFA701 | 1 |
| reference cannot run: unmapped 4-byte access at FE6D911A | 1 |
| reference cannot run: unmapped 4-byte access at D7DF47E0 | 1 |
| reference cannot run: unmapped 4-byte access at 99AFF01E | 1 |
| reference cannot run: unmapped 4-byte access at 85B396EC | 1 |
| reference cannot run: unmapped 4-byte access at C5EA51F4 | 1 |
| reference cannot run: unmapped 4-byte access at C366D268 | 1 |
| reference cannot run: unmapped 4-byte access at 6B194F70 | 1 |
| reference cannot run: unmapped 4-byte access at EB59B7F0 | 1 |
| reference cannot run: unmapped 4-byte access at 66F310D8 | 1 |
| reference cannot run: unmapped 4-byte access at 79D70908 | 1 |
| reference cannot run: unmapped 4-byte access at D0ADB9A8 | 1 |
| reference cannot run: unmapped 4-byte access at 4E8F3C98 | 1 |
| reference cannot run: unmapped 4-byte access at 31AD2EF0 | 1 |
| reference cannot run: unmapped 4-byte access at 39FEC674 | 1 |
| reference cannot run: unmapped 4-byte access at F582DE78 | 1 |
| reference cannot run: unmapped 4-byte access at 67323215 | 1 |
| reference cannot run: unmapped 4-byte access at 637D6AD8 | 1 |
| reference cannot run: unmapped 4-byte access at 30F37C20 | 1 |
| reference cannot run: unmapped 4-byte access at 39B5D66C | 1 |
| reference cannot run: unmapped 4-byte access at 163E52B8 | 1 |
| reference cannot run: unmapped 4-byte access at 99842BBA | 1 |
| reference cannot run: unmapped 4-byte access at E67B2A75 | 1 |
| reference cannot run: unmapped 4-byte access at A0FDA48A | 1 |
| reference cannot run: unmapped 4-byte access at 06A9E5B0 | 1 |
| reference cannot run: unmapped 4-byte access at 30AA2DFC | 1 |
| reference cannot run: unmapped 4-byte access at 13579BEF | 1 |
| reference cannot run: unmapped 4-byte access at 823D9DEC | 1 |
| reference cannot run: unmapped 4-byte access at F158E10C | 1 |
| reference cannot run: unmapped 4-byte access at 8397739C | 1 |
| reference cannot run: cannot execute '.word 0xFFFF' at 06049C76 | 1 |
| reference cannot run: unmapped 4-byte access at CC45A554 | 1 |
| reference cannot run: unmapped 4-byte access at 7E324804 | 1 |
| reference cannot run: unmapped 4-byte access at 91E66534 | 1 |
| reference cannot run: unmapped 4-byte access at D65548B8 | 1 |
| reference cannot run: unmapped 4-byte access at E723A7E6 | 1 |
| reference cannot run: unmapped 4-byte access at 8B4EC7F8 | 1 |
| reference cannot run: unmapped 4-byte access at 4F7F22C3 | 1 |
| reference cannot run: unmapped 4-byte access at C6EB8EF0 | 1 |
| floating/string/wide constant | 1 |
| reference cannot run: unmapped 4-byte access at 53FAE368 | 1 |
| reference cannot run: unmapped 4-byte access at 76548A18 | 1 |
| reference cannot run: unmapped 4-byte access at 19596A88 | 1 |
| reference cannot run: unmapped 4-byte access at D7818AD4 | 1 |
| reference cannot run: unmapped 4-byte access at 913D4014 | 1 |
| reference cannot run: unmapped 4-byte access at 05ED3E93 | 1 |
| reference cannot run: unmapped 4-byte access at 1B55F3A1 | 1 |
| reference cannot run: unmapped 4-byte access at 725D01C9 | 1 |
| reference cannot run: unmapped 2-byte access at 648AD502 | 1 |
| **Total** | **549** |

## CLI and concurrent installation

Addresses/`--all`, `--vectors`, `--jobs`, optional `--json`, single-address `--candidate`, `--version` and `--output-dir` remain supported. Default outputs still go to `work`; this task explicitly selected isolated `/tmp` output directories. Exit codes remain 1 for any FAIL, 0 for PASS/PASS-void/SKIP-only results, and 2 for invalid usage. Temporary preprocessing/build/state files are isolated and cleaned.

The other agent continued calling the unchanged v1.02 `difftest.py` while v1.03 was developed and validated separately. Installation atomically replaces that file at the end; existing processes keep their loaded implementation. `difftest_v102.py` remains byte-identical to the previous entry point. This task changed only the requested difftest/test Python files and this report, did not modify project sources or the other agent's files, and made no git commit.
