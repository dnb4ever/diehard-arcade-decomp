# Functional decomp round 5 — 2026-11-09

v1.09 is live: **178 PASS / 890 PASS-void / 0 FAIL / 31 SKIP**. **35 source SKIPs convert: 8 PASS and 27 PASS-void.** All **1033** starting PASS/PASS-void statuses are preserved exactly. Twelve conversions are corrected source-to-entry bindings; eleven of those check duplicate drafts of already represented bodies. These counts describe source checks across 1,099 files, not newly discovered unique functions. No new void pass is effectless.

Evidence below is under `work/decomp5/` unless qualified. The promotion evidence is `final_gate/difftest_all.json`, `frozen_manifest.json` and `audit.json`. The exact tested `tools/decomp/difftest_v109.py` is copied into `tools/decomp/difftest.py`; v1.08, its ABI/hints, v1.07, v1.06 and v1.05 rollbacks remain unchanged. `promotion.json` records hashes. No commit was made. Work is confined to the requested paths; concurrent `port/`, `tools/saturn-recomp/` and unrelated notes changes were left untouched. No delegation, GPU, privileged command or unrelated process termination occurred.

| Full suite, 16 requested vectors plus inherited additive profiles | PASS | PASS-void | FAIL | SKIP |
|---|---:|---:|---:|---:|
| Fresh v1.08 baseline | 170 | 863 | 0 | 66 |
| Frozen v1.09 promotion gate | 178 | 890 | 0 | 31 |

`baseline_source_drift.json` identifies one baseline source read after its correction: the still-SKIP pool-labelled `0223e3ba`. `baseline_original_e3ba.json` repeats its original bytes under v1.08 and gets the same invalid-entry SKIP. Every initially passing source matches the starting manifest. The earlier `gate1/` and `promotion_gate/` retain a real proximity FAIL; `promotion_final/` predates the last two fixtures. `gate_before64/` predates the deeper 64-vector timeline correction and the bounded matrix fixture. These are exploratory evidence and are not promotion gates. The final scan has zero FAIL, no passing-status regression and no frozen-input drift.

## SKIP histogram

The first table uses round 4's current-primary-blocker classification, applied address by address. A retained SKIP can move to a deeper class: the repaired matrix entry now reaches cache purge space, increasing the hardware count. The malformed early boot pools remain input/image-provenance limits, separate from the 14 entry suspects.

| Primary blocker | Before | After |
|---|---:|---:|
| entry bytes / segmentation | 14 | 1 |
| hardware / device access | 13 | 14 |
| unmapped pointer / input setup | 10 | 4 |
| compiler runtime register ABI | 8 | 3 |
| callee / indirect signature | 7 | 2 |
| escaping private stack | 4 | 0 |
| C generator / aggregate or scalar types | 3 | 3 |
| image write / pointer provenance | 2 | 1 |
| undeclared live registers / SR state | 2 | 1 |
| variadic interface | 2 | 2 |
| bounded loop / asynchronous wait | 1 | 0 |

Floor classes count the latest applicable registry row per starting SKIP and the new retained-SKIP rows, respectively. They are distinct from blocker classes. `floor_changes.json` records every closure and addition.

| Floor class | Before | Remaining |
|---|---:|---:|
| tool-limit | 44 | 26 |
| drafting-gap | 15 | 5 |
| lever-exists | 5 | 0 |
| noise | 2 | 0 |

## Entry and segmentation audit first

All 14 suspects were reviewed before pointer/signature work. `entries.asm`, `entry463c8.asm`, `entry_refs.json`, `entry_callers.asm` and `boundaries.json` preserve raw bytes, neighboring prologues and original reference evidence. PC-relative call candidates are labelled as candidates; their caller instructions are retained for inspection. No ROM byte or literal was repaired.

| Draft/source label | Callable entry | Final source check |
|---|---|---|
| 00239ce8 | 02239ce8 | PASS |
| 0220fc16 | 0220fc18 | PASS-void |
| 02213570 | 022135a0 | PASS-void |
| 0221377c | 022137a0 | SKIP |
| 02213d18 | 02213d24 | PASS-void |
| 022154fa | 022154fc | PASS-void |
| 0222077e | 0222078c | PASS-void |
| 0223de0e | 0223de20 | PASS-void |
| 0223e3ba | 0223e3d0 | PASS-void |
| 02245c76 | 02245c88 | PASS-void |
| 0221c8f4 | 0221c8f2 | PASS-void |
| 02229bce | 02229bca | PASS-void |
| 022463c8 | 022463b8 | PASS-void |
| 02001b8e | unproved/outside main image | SKIP |

Pool labels `0220fc16`, `02213d18`, `022154fa`, `0222077e`, `0223de0e`, `0223e3ba` and `02245c76` precede callable bodies. The claimed `02245c86` entry is itself the final pool halfword: actual code starts at `02245c88`. The two matrix labels start inside coordinate-adjust arms; their C drafts describe the full neighboring functions. `0221c8f4`, `02229bce` and `022463c8` omit required prologue/live-register setup beginning at `0221c8f2`, `02229bca` and `022463b8`. The seven-digit `0239ce8.c` filename intends the already represented selector at `02239ce8`.

The ABI's `source_entries` binds these C drafts to their evidenced callable entries while preserving source identity and recording the binding in every result. `canonical()` still preserves actual interior/pool addresses; this is an explicit source-binding correction. Function symbol resolution uses the same bindings. `notes/symbols.txt` corrects the relevant semantic labels. Eleven passing bindings duplicate existing source bodies, and `02229bce` repairs its callable start. The corrected matrix-base entry `0221377c` still SKIPs on cache address-array writes. The out-of-image `02001b8e` remains unbound: resemblance to the main-image no-op does not prove a boot mapping.

## Inputs and typed call boundaries

New ROM-shaped fixtures include eight valid actor/effect wrapper chains, bounded movement modes, a finite active collision list, one-quad runtime records, a nonempty model-ID list followed by -1, one VDP priority node among 64 terminated lists, and finite LZ stream/match/dequeue state. Model records 2/3 come from the original cart table at `02d00000`; no immutable table is rewritten. Retry and varied stubs remain enabled, with the inherited 16-retry/provenance restrictions. Every retry still rebuilds initial state rather than repairing a live execution.

`action_tables.json` reads the original `060918a0 -> 0608d89c` handler table and the quad-dispatch tables. Three actor dispatchers use the original seven targets and four-byte output-slot contracts. Caller return values, poststeps, ordinary memory and ordered stores remain compared. Motion selection describes action as an output and side as an input/output: its initial side word is captured in the call trace before deterministic stores. Output choices 0..4 make valid motion-table paths reachable. The editor has entry-specific four-byte `{index,-1}` input payloads for its two upload calls. Formatter calls retain the original stack-only argument adapter and immutable-format arity selector; no catch-all unknown-callee stub was added.

`live_slots.json` extracts service identities `06000638`, `06000644`, `0609029c` and `060902a0` from `work/converge/round3/saved-frames/stack-0.bin`, with capture SHA-256 and the checked 136-byte header/context offset. Stats-screen signatures come independently from `stats.asm`: two words for the BIOS value service, three for the resident field service, one for glyphs and two stack words for each numeric formatter. The identities establish stub boundaries, not BIOS implementation equivalence.

Default collision vectors have a nonempty active list and empty buckets; `positive_grid.json` separately passes 64 vectors with a candidate bucket inside the query window, choosing both narrow-phase targets. Broader world/grid states remain a coverage floor. The rebound matrix draft uses 1..4 vertices, matching the original callers' byte-sized nonempty count rather than arbitrary register words; its unconstrained 64-vector probe retained a timeout. All 35 new passing sources have at least 64 requested vectors in `audit.json`'s per-address evidence map; the full status scan uses 16 plus inherited additive profiles.

## Harness extensions and contracts

- **Mutable image inputs:** `image_inputs` is entry-scoped and bounded independently of `image_writes`. LZ may seed only `06091e14..06091e1f`, its state header overlapping the image's final 12 data bytes, and may write those same 12 bytes. Stats may seed only the two proven runtime callback slots `0609029c..060902a3`; it receives no image-write permission. Font-setup ROM stores prove these slots are mutable data. Other image bytes remain protected. All effects, byte-normalized final write footprints and chronological stores remain compared. Tests reject unauthorized entries, aliases outside spans and crossing writes.
- **Exact block copy:** C calls to `0604e280` lower to the independently transcribed word-copy recipe; the reference executes original bytes inline. Copies of at most 64 bytes descend, longer copies ascend and perform the original final overread. Counts must be multiples of four and at most `0x40000`; unsupported extents reject. Tests compare overlapping copies, store order and values against ROM and detect a changed copied word.
- **Ordinary actor boundary:** inline compiler-runtime space starts at `0604e06c`; `0604e000..0604e056` is the ordinary actor handler. Previously the broad range bypassed its call boundaries and executed into unrelated functions. The new test observes its real service/animation calls.
- **Incoming/residual registers:** proximity exposes its actual incoming r5 as a second C word argument. Its first purge boundary receives that value. ROM `060188b8..188bc` sets r5=0 in a branch delay slot before every return; the entry-specific callee contract supplies that residue for the second purge. The original leaf is independently tested with a nonzero input and a wrapped endpoint. Purge effects remain outside the typed stub boundary; no cache-purge bus model was added.
- **Preserved comparisons:** the existing caller-owned stack spans still compare crossing stores and whole final snapshots. Existing private payload/output tokens remain limited to described spans; unknown escapes still reject. No return, callee, device-read, final-memory or ordered-store comparison was removed. No new hardware/device window was mapped. VDP register programming and shared ready schedules use the inherited explicit models.

## C bugs found and corrected

Eleven C sources changed. `originals/` preserves the starting sources; `fix_replays.json` records nine direct original-source FAILs and two interface/setup SKIPs, then eleven passing 64-vector corrections. `bug_proofs.json` supplies independent valid-memory/ABI adapters for those two SKIPs; both adapted originals FAIL and both corrections pass 64. These distinguish a real behavior defect from an old tester-interface limit.

| Source | Correction and ROM evidence |
|---|---|
| `02206e2e` | Read context from r5, not r4; use MOV.W for layer +38 instead of combining layer/column halfwords; test bit80 rather than every bit above bit6. `setup_calls.asm` 0600ae32/ae5c and `fixes.asm`/`setup_end.asm` 0600af84/b100. A valid inactive r4 context with active r5 makes the old source FAIL without an unmapped candidate access. |
| `0220cdf6` | Retain existing focus instead of running the neighboring function's focus-initialization block; the first trig call is sine, not cosine. `pointer_tail.asm` / `fixes.asm`: entry branches directly into the loop, and 06010ee8 + original PIC delta targets 06017190. |
| `02213d18` | Store each corner's depth before negated Y. `entries.asm` and original body at 06017d24; ordered comparison exposed the prior reversal after binding the correct entry. |
| `022143bc` | Return decremented match length on a continuing copy; sign-extend refilled control bytes before logical shift; consume the second short-match length bit before its offset byte. `lz.asm` / `lz_action_tail.asm`, especially 06018418..18424 and 06018498..184aa. |
| `02227ca8` | Preserve incoming r5 for the first purge call instead of hardcoding zero; expose that input in C while retaining the proved second-call zero. `proximity_abi.asm`, 0602bcc4..bcce. The exploratory full scan's newly enabled FAIL is retained. |
| `0222da90`, `0222db44` | Clear scratch words in +8,+4,+0 order. `pose.asm`, 06031b34..31b38 and 06031be4..31be8. The earlier final footprint matched but the chronological trace did not. |
| `0223e3ba` | Use four-byte scratch stride for its halfword store, not the C array's two-byte stride. `entries.asm`, 060423f0..42404. |
| `022454e4` | Store facing byte +44 before angle halfword +26. `lz_action_tail.asm`, 0604950c..4951c. |
| `022463c8` | Use HWRAM descriptor `0604a678` and completion callback `0604a5fc`, replacing cart-form constants. `timeline.asm` proves the PIC anchor/delta and handler stores. The 64-vector audit first exposed this defect; a finite marker/spawn/sentinel fixture now covers active and completion states. |
| `0224910e` | Supply shift value 1 as well as the source-slot count, and OR its returned mask into +BC instead of discarding it and ORing 1. `runtime_tail.asm`, 0604d150..4d15c. The negative ABI adapter retains the discarded-mask bug and FAILs. |

## Floors, validation and limits

The registry closes **35** resolved v1.08 default-SKIP rows and adds **33** rows: 5 drafting-gap, 26 tool-limit, 1 noise, 1 lever-exists. The retained 31 SKIPs each have a class and an evidence-backed next step. Existing broader coverage/device floors remain; a source passing bounded fixtures does not close unrelated path floors. `remaining.json`, `remaining_entries.asm`, `event_drafts.asm` and the per-function registry text retain the current limits.

Remaining cases include the unproved boot/no-op mapping, malformed boot/config address provenance, resident low-bus stores, cache purge space, SMPC/VRAM/audio transactions, soft-float/compiler scalar-pair ABIs, both full variadic formatter implementations, the live-r3 camera entry, and incomplete event FSM drafts. `0221cfa4` does not implement the ROM's ten-state switch; `0221ec08` implements only part of its eleven-state dispatch; `0221edec` combines unqualified event semantics with a register-pair return. Pinning a guessed register, adding host `stdarg.h`, patching pool bytes or stubbing unknown targets would not qualify those implementations. No such shortcut was installed.

- **1099/1,099 objects compile** with GCC `-O1`; `ld -r` links the whole tree. Diagnostic-bearing files remain **16**, matching baseline (`build.log`, `build/report.json`).
- **133/133 tests pass**, including all 126 inherited checks and seven new contract/mutation tests (`suites_final.log`, `suites.json`). The promoted live import repeats the suite (`suites_promoted.log`).
- All eleven corrected C files and all 35 new passing sources have 64-vector evidence. No new void pass lacks writes/calls. Tests and negative replays retain mismatch sensitivity.
- The frozen full scan preserves all **1033** previous passing statuses and all rollback hashes. Promotion follows the status/hash audit. Scan attempts and targeted records stay under `work/decomp5/`; no shared attempt ledger was edited.

Reproduce from the repository root:

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --output-dir work/decomp5/recheck
tools/decomp/venv/bin/python work/decomp5/suites.py
python3 work/decomp5/build_check.py
tools/decomp/venv/bin/python work/decomp5/replays.py
tools/decomp/venv/bin/python work/decomp5/positive_grid.py
python3 work/decomp5/audit.py
```

1. v1.09 is live: 178 PASS / 890 PASS-void / 0 FAIL / 31 SKIP.
2. All 1033 starting passing statuses are preserved exactly.
3. Thirty-five source SKIPs convert: eight PASS and twenty-seven PASS-void.
4. All fourteen entry suspects are audited; twelve source bindings now pass.
5. Eleven C sources are corrected with ROM evidence and negative controls.
6. All corrected and newly passing sources have 64-vector evidence.
7. Image inputs, stack payloads, register residue and copy order have explicit contracts.
8. All 1,099 objects compile and link; all 133 tests pass.
9. Each retained SKIP has a classified floor and evidence-backed next step.
10. Rollbacks and concurrent work are preserved; no commit or out-of-scope edits.
