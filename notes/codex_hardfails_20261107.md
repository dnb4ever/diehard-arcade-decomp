# Hard-fail repairs — 2026-11-07

Eight repairs are installed and **PASS-void, 16/16 vectors each**, under the unmodified live difftest v1.04. Two complete dispatcher rewrites hit an oracle limitation and were **reverted byte for byte**, as requested. Their attempted rewrites remain available for review; neither is claimed as passing.

Only the ten assigned source paths, `work/codex_hardfails/`, and this note were written. No commits, checker edits, GLM edits, or changes to other source files were made. Attempt logs were directed into the scoped work directory to avoid the concurrent GLM run's outputs.

Starting-tree details differ from the task's shorthand: all ten original files compiled with the actual whole-tree GCC command. In `0223ac9c`, the original source already uses the table address found in the ROM literal pool; the reproducible bug is a byte index where the original loads a word, plus relocated handler addresses. The actual disassembly and baseline results govern the repairs below.

## Results and first differences

Memory values below are hexadecimal bytes; `none` means that side never wrote the byte. Call indices and vector indices are zero-based. “None” in the after column means no difference across all 16 standard vectors.

| Function | Final source status | Root cause / repair | First difference before | First difference after |
| --- | --- | --- | --- | --- |
| `0221b5a0` | Reverted; oracle blocked | Incomplete ten-state FSM, invented descriptor-pointer load, incorrect descriptor widths/callees, and unconditional state increment. Complete attempted rewrite saved. | v0: memory `00280410`, ref `none`, C `08` | Attempt: no differences through 14 vectors, then oracle SKIP at v14. Restored source: original difference unchanged. |
| `0221b8e4` | **PASS-void** | Implemented all 12 states: constructors, list processing, countdowns, choice selection, shared state bodies, flags, and task deletion. Replaced invented model-pointer assignment and incorrect initial flag mask. | v0: memory `002a0418`, ref `cc`, C `none` | None |
| `0221cad4` | Reverted; oracle blocked | All eleven state bodies were empty. Complete attempted rewrite includes constructors, countdowns, branch selection, runtime scaling, shared bodies, and task deletion. | v0: memory `002a0418`, ref `c1`, C `none` | Attempt: no differences through 2 vectors, then oracle SKIP at v2. Restored source: original difference unchanged. |
| `0221df80` | **PASS-void** | Implemented all 17 states, including the repeated spawn/fade/select cycles and terminal waits. Corrected initial constructors and returned-node use, relocated handlers, and removed invented initial stores. | v0: memory `002a0418`, ref `d8`, C `none` | None |
| `0223ac9c` | **PASS-void** | Model index at entity `+5e` must be signed 16-bit, not signed 8-bit. Handler arguments must use the PIC-generated HWRAM addresses. | v5: memory `06096c7b`, ref `01`, C `00` | None |
| `0220b1c0` | **PASS-void** | Stored task handler used cart address instead of relocated HWRAM. Position-table byte offset requires signed-word truncation after multiplication; angle-table byte offset requires unsigned-byte truncation after multiplication. | v0: memory `00280054`, ref `06`, C `02` | None |
| `02205b20` | **PASS-void** | Cleared flag bit 1 instead of bit 0; table selector bytes incorrectly zero-extended; installed callback used wrong address form; constructor tail omitted both actual arguments. | v0: memory `002a043f`, ref `4c`, C `4d` | None |
| `02207360` | **PASS-void** | Build-request guard was inverted; updater handler argument used cart address; updater flag stored at `+60` instead of `+3c`. | v0: call 0, ref `06024828(06050910)`, C has no call | None |
| `0222ca98` | **PASS-void** | Final shadow used `partid + 2`, skipping the current part ID. Original reads the current cursor without advancing it. | v3: memory `00280044`, ref `13`, C `3f` | None |
| `02223ee0` | **PASS-void** | Replay cleanup compared against an unsigned word where SH-2 uses a signed word. Nonzero `func_0223c7f6` result must jump to commit rather than fall through cleanup. Input-record byte offsets also require the original signed-byte truncation. | v2: memory `0028008e`, ref `none`, C `00` | None |

## Dispatch reconstruction

Disassemblies, annotated with literal values, are in `work/codex_hardfails/<addr>.asm`. The large combat FSM's replay/commit tail is also saved separately as `02223ee0.tail.asm`. Relevant state tables and shared bodies:

- `0221b5a0`: ten entries at cart `0221b5e8`; branch base `0221b5dc`. States 3/5 share a wait body; 4/6 share list processing and arithmetic right scaling; 8 has the same processing tail; 9 deletes the task. Descriptor initialization uses the constructor result's `+8` pointer and three **word** stores at `+24/+26/+28`. The descriptor's `+10` field retains the original literal value `00210068`.
- `0221b8e4`: twelve entries at `0221b92c`; branch base `0221b91e`. States 7/9 share processing; 3/8 share a wait. State 5 tests the old secondary timer against zero and then continues through state 6's primary timer work in the same frame.
- `0221cad4`: eleven entries at `0221cb1c`; branch base `0221cb0e`. States 6/8 share processing; state 4's secondary-timer work continues into state 5's wait/select body. State 3 also tests the old secondary timer against zero.
- `0221df80`: seventeen entries at `0221dfdc`; branch base `0221dfcc`. States 2/5/8/11/14 share delayed spawn; 3/6/9/12 share fade; 4/7/10/13 share random choice and increasing-delay setup. States 15/16 finish the sequence.

These sources preserve the existing entry names and signatures. Callee declarations were corrected where needed. PIC data/handler values are kept in their original address form; ordinary calls retain named functions. No logic depends on a stub's generated value.

## Exact checker limitation and required reversions

Both blocked rewrites call `0222a878` (`scene_value_select`, runtime entry `0602e878`), store its returned descriptor pointer in entity `+30`, and later reload that field to read descriptor `+4`. This is genuine original behavior. The v1.04 pointer-return analysis follows register provenance but does not track the return through the intervening store/reload. Consequently, `0602e878` is absent from `pointer_returns`; its zero-argument stub returns the unmapped ordinary hash `983591c1`, rather than an object pointer. The subsequent original load faults at `983591c5`.

| Attempt | Compared vectors | First blocked vector | Original instruction / runtime PC | Exact checker reason |
| --- | --- | --- | --- | --- |
| `0221b5a0.attempt.c` | 14 | 14 | cart `0221b64a`, runtime `0601f64a`: `mov.l @(4,r3),r2`, with `r3=983591c1` | `reference cannot run: unmapped 4-byte access at 983591C5` |
| `0221cad4.attempt.c` | 2 | 2 | cart `0221cb5c`, runtime `06020b5c`: `mov.l @(4,r3),r2`, with `r3=983591c1` | `reference cannot run: unmapped 4-byte access at 983591C5` |

Evidence: `iter2.json` and `trace_blockers.log` under the work directory. `trace_blockers.py` wraps the original oracle step only to record the failing instruction, registers, and preceding calls; it neither changes memory nor changes results. Both source files were restored from `<addr>.orig.c`. Their final checker failures are therefore expected and retain their baseline first differences. The full rewrites are preserved as `<addr>.attempt.c`; no fake pointer substitutions or stub-specific branches were installed.

## Other instruction evidence

- `0223ac9c`: cart `0223aeec` loads the literal `0607c3bc` from pool `0223af54`, matching the C table base. At `0223aee8`, `mov.w @(r0,r14),r2` reads entity `+5e`, then doubles it before loading an unsigned table word. The original C used `M8(entity, 0x5E)`. Handler arguments resolve to `060412a0`, `060413d0`, and `06041558`.
- `0220b1c0`: `exts.w` at `0220b1e6` truncates the scaled position offset; `extu.b` at `0220b212` truncates the scaled angle offset. The task-handler pointer formed at `0220b302..308` is runtime `0600f354`.
- `02205b20`: `mov #-2,r3` followed by `and` clears bit 0. Indexed loads at `02205bc6` and `02205be8` are signed byte loads. The constructor tail explicitly loads task into r4 and selected script into r5.
- `02207360`: `tst #1` / `bf` at `0220737a..37c` enters the builder when bit 0 is set. The final store at `0220752c` is `mov.l r0,@(60,r2)` where 60 is decimal, i.e. `+3c`.
- `0222ca98`: `0222d074..078` reads the current part-ID pointer directly, with no extra word offset.
- `02223ee0`: `02225c9c..ca2` uses a sign-extended replay cursor in a signed `cmp/ge`. After a nonzero helper result, `02225d42..d4a` reloads the category and branches to `02225f5a` (commit), bypassing the animation-record tail. The input record offsets are sign-extended bytes at `02223f00` and `02223f14`.

## Validation and artifacts

Baseline and final commands:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py \
  0221b5a0 0221b8e4 0221cad4 0221df80 0223ac9c \
  0220b1c0 02205b20 02207360 0222ca98 02223ee0 \
  --jobs 3 --output-dir work/codex_hardfails/final \
  --json work/codex_hardfails/final.json
```

The baseline used `before/` and `before.json`; intermediate runs used `iter1` through `iter5`. Final tally: **8 PASS-void, 2 FAIL on deliberately restored originals**. The two attempted repairs report SKIP rather than PASS. Originals were saved before editing as `work/codex_hardfails/<addr>.orig.c`.

The whole-tree gate compiles each translation unit with:

```sh
gcc -O1 -c -I project/src_functional <file> -o /dev/null
```

**All 1,099 files compiled; zero errors.** Seventeen translation units emitted warnings, including the restored original `0221b5a0`; none of the eight installed repairs emitted diagnostics under this gate. No source changed during its individual compile. Full per-file results and source hashes: `work/codex_hardfails/whole_tree_compile.json`. This repository's documented whole-tree gate is per-file compilation; `notes/decomp_tail_opus_20261030.md` states there is no whole-tree link step. Scoped `git diff --check` passed.

PASS here is bounded evidence from 16 isolated emulator/stub vectors, not gameplay validation. The checker bounds state indices to 0..7, so higher switch states were reconstructed from their table targets and bodies, not claimed as exercised by those vectors.
