# Mechanical difftest repairs — requested 20261107 batch

Executed 2026-11-08 using the live, unchanged `tools/decomp/difftest.py` v1.04. Read `STATE.md`, `notes/difftest_report_v104.md`, the floor registry, symbols, shared headers, and each target’s original disassembly. None of these targets has a blocked floor entry.

**Installed outcome: 1 PASS, 17 PASS-void, 3 deliberately restored FAILs.** Every installed repair completed all 16 default vectors. Three attempted store-order repairs reached oracle/model limitations; their original source was restored byte-for-byte as requested. No function names or entry signatures changed.

Originals: `work/codex_mechfails/<addr>.orig.c`. Full annotated ROM listings: `<addr>.asm` in that directory; listing PCs are cart file offsets (add `0x02000000` for cart addresses, `0x05e04000` to those cart addresses for HWRAM). The two loader listings include neighboring functions because `fn.extent` follows a shared pool; only the requested entry bodies were edited.

The shared `dh.h`/`decomp.h` headers contain no code relocation macro. Within the allowed source-file scope, all 13 passed template/handler values consistently use explicit HWRAM addresses, retaining existing memory macros and ABI types. The named function pointer in `02242fb0` already resolves to HWRAM and required no address change.

Only the requested C files, this note, and `work/codex_mechfails/` were written. Attempt JSONL files stay inside that work directory via `--output-dir`, honoring the narrower task scope. No checker changes, stub-value special cases, commits, or external process termination.

## Per-function results

Vector and event indices are zero-based. `trace #N address:byte` is the first chronological, byte-normalized store difference, shown as **reference / original C**. For calls, the first differing template argument is shown; full target/argument tuples are in the baseline JSON. “None” means no final difference across all 16 vectors.

| Function | Final installed status | Root cause and ROM evidence | First diff before | First diff after |
| --- | --- | --- | --- | --- |
| `022079d0` | PASS-void (16/16) | Cursor was stored after its output word; reserve the slot before the data store and trig call (ROM `207ab0`, `207ad2`, `207af4`). | v0, trace #15: `06099510:00` / `002a3400:00` | None |
| `022080ac` | PASS-void (16/16) | Cursor was stored after the output word; reserve before loading/storing the alternating amplitude or zero (`208136` onward). | v6, trace #0: `06099510:00` / `002a1400:16` | None |
| `02213ca4` | Restored FAIL (attempt SKIP) | Scratch vector copied x/y/z; predecrement ROM stores z/y/x (`213cb0..213cb4`). Attempt reordered it; restored because of SR.S limitation below. | v0, trace #0: `06098ba4:00` / `06098b9c:ff` | Same as before (original restored) |
| `02223862` | Restored FAIL (attempt SKIP) | Character stored before cursor advancement; ROM advances cursor, writes character, then increments count (`2238c8..2238d2`, same order in all emission paths). Attempt repaired helper; restored below. | v0, trace #4: `002820a0:00` / `002a0400:33` | Same as before (original restored) |
| `02223968` | Restored FAIL (attempt SKIP) | OUT_PUTC stored character before cursor; ROM advances cursor first (`2239c0..2239ca`, also zero/digit paths). Attempt reordered macro; restored below. | v0, trace #4: `0028202c:00` / `002a0400:37` | Same as before (original restored) |
| `0223a888` | PASS-void (16/16) | C merged the raw multiply result store and the following +0x1000 store at +0xC8; restore both (`23a90c`, `23a914`). | v0, trace #14: `00280196:df` / `00280196:ef` | None |
| `02242fb0` | PASS-void (16/16) | C cleared +0x0F before +0x10; ROM clears +0x10 first (`242fc4`, `242fca`). | v0, trace #4: `002a0410:00` / `002a040f:00` | None |
| `02246052` | PASS-void (16/16) | C cleared arrays in ascending order; ROM writes +4 before +0, halfword +2 before +0, and pointer array +12/+8/+4/+0 (`2460ac..2460c2`). | v0, trace #42: `06099784:00` / `06099780:00` | None |
| `022078aa` | PASS-void (16/16) | PC-relative spawn handler was a cart address; use HWRAM `0600b9d0` (`207950` mova + 0x7C). | v0, call #60: template `0600b9d0` / `022079d0` | None |
| `022082be` | PASS-void (16/16) | PC-relative scene handler was a cart address; use HWRAM `0600c3a0` (`20831e` mova + 0x80). | v0, call #6: template `0600c3a0` / `022083a0` | None |
| `0222934a` | PASS-void (16/16) | Template symbol denoted cart storage; pass HWRAM `0602aeb8` (`229350` mova - 0x24A0). | v0, call #0: template `0602aeb8` / `02226eb8` | None |
| `022304b4` | PASS-void (16/16) | Child handler was a cart address; use HWRAM `060345e2` (`2304f6` mova + 0xEA). | v0, call #0: template `060345e2` / `022305e2` | None |
| `022305e2` | PASS-void (16/16) | Spawn handler was a cart address; use HWRAM `0603d500` (`230600` mova + 0x8EFC). | v0, call #0: template `0603d500` / `02239500` | None |
| `0223b01e` | PASS (16/16) | Throw template symbol denoted cart storage; pass HWRAM `060411b6` (`23b08c` mova + 0x2126). | v0, call #0: template `060411b6` / `0223d1b6` | None |
| `0223d3d0` | PASS-void (16/16) | Impact handler was a cart address; use HWRAM `06016350` (`23d480` mova - 0x2B134). | v5, call #1: template `06016350` / `02212350` | None |
| `0223eb3c` | PASS-void (16/16) | Spawn handler was a cart address; use HWRAM `06042fec` (`23eb58` mova + 0x48C). Also correct Z bias from +0x8000 to -0x8000 (`23eb70`, `23eba8`). | v0, call #1: template `06042fec` / `0223efec` | None |
| `0223ec44` | PASS-void (16/16) | Spawn handler was a cart address; use HWRAM `06042d14` (`23ec6c` mova + 0xA4). Also correct Z bias from +0x8000 to -0x8000 (`23eca8`, `23ecb4`). | v1, call #1: template `06042d14` / `0223ed14` | None |
| `0223ed94` | PASS-void (16/16) | Spawn handler was a cart address; use HWRAM `06042fa0` (`23ed98` mova + 0x200). | v0, call #0: template `06042fa0` / `0223efa0` | None |
| `0223edce` | PASS-void (16/16) | Spawn handler was a cart address; use HWRAM `06042ef0` (`23edd0` mova + 0x114). | v0, call #0: template `06042ef0` / `0223eef0` | None |
| `02241a10` | PASS-void (16/16) | Muzzle template was a cart address; use HWRAM `0601679c` (`241a34` mova - 0x2F29C). | v0, call #1: template `0601679c` / `0221279c` | None |
| `02243590` | PASS-void (16/16) | Pickup template was a cart address; use HWRAM `06036526` (`243594` mova - 0x1107A). | v0, call #0: template `06036526` / `02232526` | None |

## Preserved attempts and exact checker limitations

Attempt sources for the three reverted functions are saved as `work/codex_mechfails/<addr>.attempt.c`. They are evidence only and are not installed. `iteration1/results.json` records the attempted results. `checker_limits.json` reproduces the failing oracle vectors independently with unchanged harness code; `diagnose_limits.py` records how.

- **02213ca4:** the reordered scratch copy completes vectors 0–2, then vector 3 reports `depends on non-argument registers / undefined r0`. Entry words: `80000000 00000000 00000002 80000000` (the fourth is unused). The oracle seeds SR.S=1 for noise `0x13579bdf` and SR.S=0 for `0x2468ace0`. Original `clrmac` clears the accumulator, not SR.S; the subsequent `mac.l` operations saturate to 48 bits with S=1 and use 64-bit accumulation with S=0. The two ROM executions first disagree at final memory byte `002a040c`: `0x33` versus `0x5c`. Thus even with r0 unchecked the original is not invariant under the checker’s incoming machine state. A C-only rewrite cannot make those two references equal. Original restored.

- **02223862:** the cursor-first helper passes vector 0, then vector 1 reports `reference cannot run: no return after 20000 steps (pc 060278FA)`. Entry words: `00000007 00000020 002820e8 80000000`. The context width at `002820f0` is seeded to `0x45e81ecd` (1,172,840,141); after subtracting the one decimal digit, the ROM enters its padding loop with 1,172,840,140 remaining characters. The checker does not bound this context-field loop. This fails in the original reference before candidate comparison, independently of C store order. Original restored.

- **02223968:** the cursor-first macro passes vectors 0–1, then vector 2 reports `reference cannot run: no return after 20000 steps (pc 060279CC)`. Entry words: `0000001f 00000002 002820c0 00000002`. Context width at `002820c8` is `0x5068c104` (1,349,042,436); after subtracting two hex digits, 1,349,042,434 padding characters remain. The same unbounded context-field loop exceeds the reference budget. Original restored.

No narrower vectors, discarded inputs, changed oracle limits, or candidate-dependent input constraints were used to claim a pass.

## Additional real bugs exposed after address repair

Both jittered-spawn functions initially fail on calls, which the checker compares before memory. After just fixing the address, `0223eb3c` first differs at memory byte `00218759`, reference `0xc6`, candidate `0xc7` (vector 0); `0223ec44` differs at `0021fb11`, reference `0x4f`, candidate `0x50` (vector 1). Their ROM word literal `0x8000` is sign-extended by `mov.w`, so Z adds **-0x8000**, while the old C added +0x8000. Correcting that sign makes both pass all 16 vectors. These intermediate differences are preserved in `iteration1/results.json`.

## Reproduction and validation

```sh
ADDRS=$(cat work/codex_mechfails/addresses.txt)
# Baseline was run before editing; initial sources are preserved.
tools/decomp/venv/bin/python tools/decomp/difftest.py $ADDRS --jobs 4 \
  --output-dir work/codex_mechfails/before \
  --json work/codex_mechfails/before/results.json
# Final installed tree, including the three restored originals:
tools/decomp/venv/bin/python tools/decomp/difftest.py $ADDRS --jobs 4 \
  --output-dir work/codex_mechfails/final \
  --json work/codex_mechfails/final/results.json
python3 work/codex_mechfails/compile_tree.py
```

Baseline: 21 FAIL. Intermediate: 1 PASS, 15 PASS-void, 2 FAIL, 3 SKIP. Final: **1 PASS, 17 PASS-void, 3 FAIL, 0 SKIP**; final exit status 1 is expected because the three restored originals retain their original first differences. Harness, cart, and interpreter hashes agree between baseline and final. All final source hashes agree with the compile check.

The documented whole-tree gate is `gcc -O1 -c -I project/src_functional <file> -o /dev/null` for every C translation unit. **All 1,099 files compiled, zero errors; 17 translation units emitted existing diagnostics. None of the 21 target files emitted diagnostics.** Each source hash was unchanged across its compile. Full output/hashes: `work/codex_mechfails/whole_tree_compile.json`. As documented in `notes/decomp_tail_opus_20261030.md`, this functional tree has no whole-tree link step; `project/build.py` is the separate byte-match verifier, not this compile gate. Scoped `git diff --check` passed.

PASS is bounded evidence from the 16-vector isolated integer/stub model, not a proof or gameplay validation.
