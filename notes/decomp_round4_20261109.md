# Functional decomp round 4 — 2026-11-09

v1.08 is promoted with **170 PASS / 863 PASS-void / 0 FAIL / 66 SKIP**. All **931** previously passing functions retain their exact PASS/PASS-void status. **102 SKIPs convert: 13 PASS and 89 PASS-void.** These are bounded differential checks under explicit ABI, input and stub contracts; two newly enabled void entries still bypass their active states and have coverage floors.

Work stayed in `tools/decomp/`, `project/src_functional/`, `notes/floors.tsv`, this report and `work/decomp4/`. Existing changes in `port/`, `tools/saturn-recomp/` and other notes were left untouched. No delegation, GPU use, privileged command, commit or unrelated process termination occurred. `notes/symbols.txt` needed no edit.

Evidence paths below are relative to `work/decomp4/` unless qualified. The frozen full scan is `promotion_gate/difftest_all.json`; `audit.json` checks all 1,099 addresses and input hashes. The exact tested `difftest_v108.py` is copied into `tools/decomp/difftest.py`. The former live tester already matched `difftest_v107.py` byte for byte; that rollback, its ABI/hints and v1.06/v1.05 rollbacks remain unchanged. `promotion.json` records hashes.

| Full scan: 16 requested vectors plus inherited/additive profiles | PASS | PASS-void | FAIL | SKIP |
|---|---:|---:|---:|---:|
| Fresh live v1.07 baseline | 157 | 774 | 0 | 168 |
| Frozen v1.08 promotion gate | 170 | 863 | 0 | 66 |

`baseline/difftest_all.json` exactly reproduces the supplied baseline. `gate1/` and `final_gate/` are exploratory scans, not promotion evidence: subsequent fixes, stronger handler fixtures and final literal-width handling changed inputs. The separate frozen promotion scan has no FAIL, passing-status regression or unexplained drift. `manifest_before.json` and `frozen_manifest.json` distinguish the 18 edited C sources from preserved prior work.

## SKIP inventory

This first table retains round 3's primary classes for an address-by-address cohort comparison. “Remaining” can have a deeper blocker after its first limit was removed. `inventory_before.json`, `audit.json` and `remaining.json` retain every address, reason and setup.

| Starting primary class | Before | New PASS | New PASS-void | Remaining |
|---|---:|---:|---:|---:|
| unmapped pointer / input setup | 51 | 5 | 33 | 13 |
| callee / indirect signature | 22 | 0 | 17 | 5 |
| escaping private stack | 19 | 4 | 11 | 4 |
| C generator / aggregate or scalar types | 17 | 2 | 9 | 6 |
| hardware / device access | 16 | 0 | 4 | 12 |
| image write / pointer provenance | 10 | 1 | 6 | 3 |
| undeclared live registers / SR state | 10 | 1 | 6 | 3 |
| entry bytes / segmentation | 9 | 0 | 0 | 9 |
| compiler runtime register ABI | 7 | 0 | 0 | 7 |
| variadic interface | 6 | 0 | 3 | 3 |
| overbroad compiler-runtime entry guard | 1 | 0 | 0 | 1 |

The remaining 66 reclassified by their current blocker are below. Cache-purge address-space accesses move to hardware; ROM-proven interior entries move to segmentation. This is distinct from the cohort table above.

| Current primary blocker | SKIPs |
|---|---:|
| entry bytes / segmentation | 14 |
| hardware / device access | 13 |
| unmapped pointer / input setup | 10 |
| compiler runtime register ABI | 8 |
| callee / indirect signature | 7 |
| escaping private stack | 4 |
| C generator / aggregate or scalar types | 3 |
| image write / pointer provenance | 2 |
| undeclared live registers / SR state | 2 |
| variadic interface | 2 |
| bounded loop / asynchronous wait | 1 |

## Inputs and signatures

The largest gain is pointer setup: **38/51** starting pointer SKIPs now pass. The first harness defect was an address-space mismatch in retry: memory faults name physical addresses while loaded initial pointers can retain the `0x2xxxxxxx` bus alias. v1.08 compares physical forms for provenance matching. It still changes only an argument or a seedable initial RAM word proven by a preceding ROM load, excludes program bytes/private stack, rejects words already written by the function, and retains the 16-retry cap. No execution is repaired mid-run. `retry_phys.json` records the 14 first conversions with their exact retry sources; `difftest_hints_v108.json` records this provenance evidence.

Structured fixtures replace unbounded random graphs with ROM-shaped inputs: a two-node NULL-terminated task list; two definition entries plus NULL and a sufficient free chain; first-fit heap headers with exact/split/no-fit requests; a released allocation actually present in its owner's list; sprite queues with one descriptor and a subsequent -1 sentinel; finite transform lists; task/work chains; nested direction-animation tables; eight distance bands with four signed response choices; bounded stage/type/player indices; and grid heads with finite links. Every written byte and ordered store remains compared. The fixtures are reproducible synthetic instances of layouts proved by the ROM, not claims that random bytes are valid game states. `pointer_shapes.asm`, `pointer_shapes2.asm`, `pointer_tail.asm`, `pointer_bugs.asm`, `grid_payload.asm` and each hint's `why` identify the evidence.

Real captured state was available: `work/converge/round3/saved-frames/stack-0.bin`, described by `port/converge/evidence/round3-saved-frame.json` and its capture/replay code. Its layout is a four-byte kind, 132-byte CPU context, then high/low RAM banks. The high-RAM image marker and offsets were checked before extracting BIOS callback slots. `live_slots.json` records the capture SHA-256 and every extracted word. These are the captured runtime's BIOS callback identities, used as stub boundaries; this does not execute or claim verification of BIOS implementations. `live_calls.asm` proves the caller argument words independently.

**17/22** starting signature SKIPs now pass. Original immutable tables provide the enemy script targets at `06090230`, the type-completion targets at `0608da88`, and the shared attribute-switch halfword destinations at `0601a3f0`. Only explicitly declared call targets become external tail boundaries; unknown computed jumps still SKIP. The completion fixture restricts the original type map to selectors 0..2: selector 3 addresses following nonpointer bytes, and remains a separate floor. Formatter calls use the inherited stack-only contract with concrete original call-site arities. No generic unknown-callee stub was added.

## Harness contracts

- **Sparse and vector outputs:** `view_bounds_from_actors` writes only X/Z words at +0/+8 of each output. Its Y words remain untouched. The transform helper has a full 12-byte output with distinct deterministic words. `output_tokens` normalizes only these described private pointers; sparse `stores` and vector fills occur identically on both sides. Caller consumption, ordinary-memory effects and call traces remain compared. `stack_abis.asm` proves the stores; all six camera callers now complete.
- **Private input payloads:** grid dispatch snapshots the 12-byte position and 16-byte extent inputs. The two join-glyph callers have an entry-specific three-byte payload contract, matching their two constructed bytes and NUL. That contract does not apply to other callers or normalize ordinary pointers. Payload mutations affect both the call trace and stub result. Unknown escapes remain SKIP.
- **Caller-owned stack:** entry contracts name a bounded offset/size in entry SP..SP+256. These bytes are included in ordered stores and final footprints even inside the normally private window, including crossing stores. Their entire final contents are snapshotted and compared, so unchanged bytes are covered too. Weighted transform outputs add stack-output vectors after retaining original vectors. The by-value text command at `060140c8` copies its eight-byte input block to entry SP and lowers that explicitly declared aggregate parameter to its guest address; the whole caller span is checked. Other aggregate/variadic entry ABIs remain unsupported.
- **Writable image data:** only font setup entries may write the exact 16-byte callback-slot span `06090294..060902a3`. Every byte value and chronological store is compared. Writes outside that span still SKIP, including aliases; ROM bytes are not patched to prepare inputs. Other image-write SKIPs were resolved by valid argument/work-pointer shapes rather than permitting arbitrary image writes. The LZ state overlapping the image tail remains unsupported.
- **C generation:** lexical renaming preserves distinct shadowed locals and outer bindings; aggregate globals retain guest layout; constant-expression array extents are evaluated without host execution; address-taken parameters get initialized private guest storage; SH-2 long is 32 bits, while existing long-long temporaries remain 64 bits. Returning a private address without a relocation contract still rejects. Floating types, unsupported aggregate returns and variable arrays remain explicit limits.
- **Devices:** no device window was added. More entries opt into the existing v1.07 DIVU/SCU programming fixture and the existing ROM-backed SR.S=0 assumption. Timer-0's display-toggle-disabled path verifies its SCU write and sound-service boundary; enabled VDP2 CCCTL behavior remains a floor. Cache purge space, SMPC, VRAM/audio transactions, interrupts, transfers and timing were not fabricated.

These recipes are in `tools/decomp/difftest_abi_v108.json`; input profiles and captured-slot evidence are in `tools/decomp/difftest_hints_v108.json`. No return, memory, ordered-store, callee or device-read comparison was removed to obtain a pass.

## C corrections and ROM evidence

**18 C files changed: 15 behavior/declaration bugs and three shared-boundary representations.** Every final source passes 64 requested vectors with varied stubs. `originals/` saves each original verbatim; `fix_replays.json` records the ordinary old/new replay. Eleven originals directly FAIL there. `bug_proofs.json` supplies independent valid-memory/typed-call fixtures that make the other four buggy originals FAIL and their corrections pass 64 vectors. The three boundary adaptations are kept separate from semantic bug claims.

| Function(s) | Correction and original evidence |
|---|---|
| `02204868`, `02204afa`, `02205232` | Store HWRAM task callbacks rather than cart addresses. Original PIC handler calculations and transitions are in `live_calls.asm` / `fix3.asm`; old callback stores reproduce 06-versus-02 memory differences. |
| `02205c96` | Store the scene selector only inside an accepted directional gate. `05c96.asm` shows the branch bypassing the store; the old C wrote unchanged values on inactive gates. Ordered comparison catches the extra writes. |
| `0220ef02` | Remove the steady-state return after the look-at call. ROM `060130a4` falls through to all three velocity integration stores at `060130a6..130bc` (`fix3.asm`). |
| `022156da`, `0221582a` | Sign-extend descriptor control byte before OR 2 and halfword store. MOV.B at `06019760` / `06019884` preserves negative high bits (`pointer_shapes.asm`). |
| `0221af30` | Declare the registry as an address array, not a pointer-valued cell. ROM `0601ef4c` loads literal `06099584` and directly reads +30/+34 (`pointer_tail.asm`). A mapped decoy chain makes the old C write the wrong actor in `bug_proofs.json`. |
| `0221da60` | Restore view-task -> work dereference order: ROM `06021a6c..1a72` loads the view task before its +8 work pointer (`pointer_bugs.asm`). A mapped decoy chain exposes the old state advance. |
| `02222d3c`, `02222e44` | Install HWRAM font callbacks. Original PIC callback stores are at `06026da0..26dd0` / `06026e90..26ebc` (`image_slots.asm`). The writable-slot contract exposes the old cart-address values. |
| `022303a6` | Call the four-word billboard renderer `060194fc`, not invalid entry `060194fa`. `06034402` and its PIC literal prove the target (`callee_missing.asm`); the typed negative replay compares the differing call targets. |
| `0223ed14` | Call sprite renderer `060193f6` with `(entity,0x80000,1)`. The old C called audio queue `06019f6c`; `06042d4a` proves the renderer (`callee_missing.asm`). |
| `02243e22` | Retain separate AND and OR stores to actor+E0 after the timing gate (`fix3.asm`). Final memory matched before correction, but its intermediate store was missing. |
| `02243fe0` | Path C computes its angle directly from peer coordinates. `06048298..482a4` uses peer X/Z; the C reused path B's projected target (`43fe0.asm`, `sweep_firstcall.json`). All three bounded sweep variants are exercised. |

Shared-boundary adaptations: `02214c66` calls existing coalescer `06018c8a` after unlinking; `02229488` stores its handler byte then calls existing shared body `0602d4b4`; `022161dc` names its unchanged reset helper `func_02216258` so the three original helper calls have their ROM identity. `callee_missing.asm` proves all three boundaries. These changes let both sides use the same declared stub boundary; they are not evidence that the former inline implementations had incorrect whole-program semantics.

## Floors and practical limits

`floors.py` updates only resolved prior v1.07 default-SKIP rows and appends current evidence. It closes **102** such rows and adds **71** rows: 2 noise, 44 tool-limit, 10 lever-exists, 15 drafting-gap. `floor_changes.json` retains every edit. Broader pre-existing coverage/model floors remain. Attempt records are confined to `work/decomp4/attempts.jsonl` and scan-local `attempts/` directories, following this task's scope rather than changing the shared attempt ledger.

Two new void passes, `02202e84` and `0221f9c4`, have no writes/calls in the default vectors: generator support verifies their inactive-state interface, not their active bodies. `02217ae4` checks finite negative grid searches and return 0; the seeded first-cell node is outside those query windows, so positive overlap behavior is a coverage floor. `0220c964` is also effectless but returns a compared value. None is labelled dead code. `audit.json` retains these lists.

The remaining 66 include soft-float/compiler interfaces, full variadic formatter entries, escaping action frames without output/alias contracts, device/purge spaces, deeper pointer graphs and bad/interior entries. ROM inspection confirms `02213570` / `0221377c` start inside coordinate-adjust blocks before their neighboring prologues, and `0221c8f4` / `02229bce` consume missing live r0 setup (`live_regs.asm`, `pointer_tail.asm`). Their required entry/segmentation work was documented instead of inventing register constants. `0221cfa4` is a drafting gap: its selector-chain C does not represent the ROM's ten-state switch (`pointer_bugs.asm`). Invalid boot-pool literals and entry bytes were not repaired. LZ's mutable state overlapping image bytes needs a separate precise contract. Passing selected modes does not qualify excluded invalid modes, broader spawn states, timeout limits or device branches.

## Validation and reproduction

- **1,099/1,099** GCC `-O1` objects compile; `ld -r` links the complete tree. Diagnostic-bearing files remain **16**, matching baseline (`build/report.json`, `build.log`).
- **126/126 tests pass:** 111 inherited checks plus 15 v1.08 contract/mutation tests (`suites.json`, `suites_promotable.log`). Existing rejection fixtures were versioned to test newly supported lexical scopes/long literals, while old versions retain their rejection assertions. New checks cover retry provenance, immutable input protection, lexical binding mutations, sparse/vector fills, caller-span crossing writes/final-byte mutation, real stack-output execution, writable-image mutation sensitivity, entry-scoped payloads and capture hashes.
- **All 18 edited C files pass 64 requested vectors**, with varied stubs; four additional negative-control fixtures also FAIL before and pass 64 after (`fix_replays.json`, `bug_proofs.json`).
- The frozen full suite preserves all **931** prior passing statuses. Promotion occurs only after the hash/status audit. Promoted live tests repeat the complete 126-test suite (`promoted_suites.log`).

```sh
tools/decomp/venv/bin/python tools/decomp/difftest.py --all --jobs 4 --output-dir work/decomp4/recheck
DIFFTEST_MODULE=difftest tools/decomp/venv/bin/python work/decomp4/suites.py
python3 work/decomp4/build_check.py
tools/decomp/venv/bin/python work/decomp4/replays.py
tools/decomp/venv/bin/python work/decomp4/bug_proofs.py
python3 work/decomp4/audit.py
```

1. v1.08 is live: 170 PASS / 863 PASS-void / 0 FAIL / 66 SKIP.
2. All 931 previous PASS/PASS-void statuses are preserved.
3. 102 SKIPs convert: 13 PASS and 89 PASS-void.
4. Pointer/setup resolves 38/51; signatures resolve 17/22.
5. Fifteen C bugs are corrected with ROM-backed negative replays.
6. Three shared-call boundary representations are made explicit.
7. Caller stack spans, payloads and permitted image stores remain compared.
8. All 18 edited C files pass 64 vectors; all 126 tests pass.
9. All 1,099 objects compile and the full relocatable tree links.
10. Remaining limits have floors; rollbacks and concurrent work are preserved; no commit.
