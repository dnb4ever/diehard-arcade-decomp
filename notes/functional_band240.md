# Functional decomp report - band 0x02240000-0x0224c000 (worker: band240)

## Done this pass
- **`project/src_functional/022444b6.c`** - `func_022444b6(void *actor) -> s32`.
  Per-actor "sweep toward target" collision / path resolver. Compiles clean with
  `gcc -O1 -c -Wall -Wextra` (zero output). fidelity: HIGH, hand-traced vs
  `m2c_fn --asm`. m2c dropped the `bsrf`/`jsr` calls; they were resolved with
  `braf_resolve.py` (bsrf@0x244568 -> 0x022130b4) and by hand for the `mova`+add
  indirect `jsr @r8` (= 0x02217838). All three callees confirmed real via
  `find_entry.py`. See symbols_band240.txt for the discovered symbols/offsets.

## Key corrections / findings
- **Done-set must be checked on the RESOLVED entry, not the census phantom.**
  Several triage rows are pool-contaminated phantoms whose real entries are
  already decompiled. Phantoms that resolve to EXISTING files (do NOT redo):
    - `0x2416ac` -> `0x2416c4` (file 022416c4.c exists)
    - `0x245db2` -> `0x245dcc` (file 02245dcc.c exists)
    - `0x247a72` -> `0x247a8c` (file 02247a8c.c exists)
    - `0x243588` -> `0x243590` (file 02243590.c exists)
    - `0x2447ce` -> `0x2447d4` (file 022447d4.c exists)
- **`0x24a516` is NOT a separate function** - `find_entry.py` resolves it to the
  SAME real entry as `0x24a280` (its census row, 312 insn, is jump-table phantom
  tail of the 24a280 dispatcher). Do not decompile it separately.

## Deferred (with reasons) - the hard residue of the band
All remaining genuinely-undone entries are bsrf-heavy / huge / non-ABI / or
m2c-failing. Deferred rather than risk a silent-wrong decomp:

- `0x2402c0` (census 2379 insn, 5 bsrf) - very large; real code (proper prologue),
  not data. Defer: size + multiple computed calls to hand-trace safely.
- `0x246ab8` (785 insn, 12 bsrf) - 12 computed calls + irregular switch dispatch.
- `0x243628` (739 insn, 4 bsrf) - large per-frame state machine w/ irregular switch.
- `0x243fe0` (619 insn, 4 bsrf) - SAME collision-sweep idiom as 022444b6 but with
  3 near-duplicate loop variants + an extra `unkE0 & 0x40` branch (uses table
  0x060E0F4C). m2c structures it, bsrf resolve to 0x22130b4 x3 and 0x210020.
  Decompilable but ~250 lines of hand-trace across 3 subtly-different loops =
  high mis-transcription risk; needs a dedicated careful pass.
- `0x242a10` (680 insn) - m2c FAILS ("Cannot find branch target .L242cd6"); needs
  full manual disassembly.
- `0x247106` (372 insn) - m2c FAILS ("Cannot find branch target func_2469f2").
- `0x24a280` (leafC, 96 insn) - 64-entry function-pointer DISPATCHER with a
  NON-STANDARD register convention (dispatch index in r0, copy ptrs in r1/r2;
  all show as "Read from unset register"). Jump table at 0x0604E2DC. Trap-class
  per instructions (non-ABI reg convention) - defer, do not invent a signature.
- `0x24a884` (leafC, 307 insn) - NON-STANDARD convention (reads r3/r9/r10 as
  inputs) + suspicious constant-0 bit-packing output. Trap-class - defer.

## Not real functions (shared epilogue fragments - report, do not decompile)
- `0x24537a` (4 insn) = `add #32,r15; lds.l @r15+,pr; rts; mov.l @r15+,r14`
- `0x244e50` (2 insn) = `rts; mov.l @r15+,r14`
  Both are tail-merged function epilogues (no prologue). `find_entry.py` reports
  them "already real" but they are branch targets, not callable entry points.
  m2c emits an empty body with an unused `arg4` (would fail -Wunused-parameter).
