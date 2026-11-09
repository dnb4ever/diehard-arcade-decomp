# Readable C convergence — round 2

2026-11-08/09. **202 overrides: 162 new PASS-void functions, 14 direct C functions, 24 other PASS functions with register residue, and two resumable C functions. All 22,200 reference frame/pixel/full-RAM/semantic records, 56 PNGs and four deterministic traces match refreshed stock.** Nine of the pilot’s 36 residue adapters now use reviewed boundary recipes. The default port build is unchanged.

Across the four reference schedules, readable C supplies **34.182% of executed function-body entries** and the semantics of **0.10375% of executed guest instructions**. The residue-free subset supplies **0.00207%** of guest instructions. These distinguish effects supplied by C from removal of the machine algorithm; 186 adapters still run register residue. Large compiled wait loops dominate the instruction denominator.

## Frozen inputs and scope

The refreshed independent copies of `port/`, `tools/saturn-recomp/`, `project/src_functional/`, the difftest tool/venv, symbols, floors and cartridge are in `work/converge/round2/snapshot/`. The snapshot interval was **2026-11-09T04:07:28.865093+00:00 through 2026-11-09T04:07:33.488156+00:00**, or 23:07:28–23:07:33 EST on November 8. This records a copy interval during concurrent work, not an atomic repository revision. Every stock and override build uses those same copied inputs. Dangling packaging symlinks were preserved; an initial copy that tried to dereference them was discarded and refreshed.

Snapshot manifest SHA256: `32d77c320163959ba2c5ababc026fc1cb4d7f239d7b9c3634f1d565bd1d6d307`. The final collector rechecks **9,372 frozen files**, with no drift. It also rechecks every installed C source against its qualification certificate. Stock executable SHA256: `1b402bd7104c21d95f7579f41e24c78c158fcafb1227da2db7eeef3f77c5a1a9`. Override executable SHA256: `9cbd2b8cbeda2ee724934f21bc978dfe2f9823f6bf096694b3b714a2960b64bb`.

Implementation work stayed in `work/converge/` and `port/converge/`; this requested note is the sole report outside them. Live port/runtime/decomp sources and `notes/floors.tsv` were not edited. Per-function attempts are private under `work/converge/round2/attempts/`, following the user’s scope restriction instead of the shared STATE.md log path. No commit, sudo, GPU, prohibited-directory access or unrelated process termination was used. The private convergence trees occupy about 4.1 GiB after deleting nine unused, unpinned copied historical run directories (2,228,343,704 apparent bytes). All 9,372 pinned inputs remain intact; `work/converge/round2/cleanup.json` records the cleanup. Frame/RAM streams are hashed through FIFOs and discarded.

## Qualification, installed classes and exclusions

The refreshed inventory still has 173 no-poll PASS-void functions, including the 110 without declared calls. None has an open-floor/lever-pending row in the copied registry. Two unsupported call shapes are excluded before stronger qualification. Of the other 171, **170 PASS-void and one SKIP** at 64 original vectors, with another 64 hinted vectors where specified by v1.06. The old 41 PASS functions also pass again. The final installed automatic cohort plus the two poll functions accounts for **14,400 passing vectors**.

Void adapters never manufacture a return value: the synthetic C wrapper returns zero only to satisfy the host harness, and the bridge preserves the residue’s actual r0, including the genuine callee’s r0 after a tail. Ordered public writes still must agree byte-for-byte with the machine transaction before C values are committed. The four exact `rts/nop` no-ops are accepted after body review; other inactive bodies are excluded.

An internal dispatch switch must keep its local labels. The pilot’s broad terminal-switch transform misclassified `0221636c`; the first combat trial rejected its missing tail call. The round-2 transform preserves every internal case and converts only the external default into a tail boundary. That repair does not change the C source or its certificate.

| Class | Installed | Meaning |
|---|---:|---|
| PASS-void, transactional | 162 | C supplies public memory; original residue preserves the full machine boundary |
| PASS, transactional | 24 | Existing effect/result overrides still needing residue |
| Direct C + reviewed ABI recipe | 14 | Five pilot recipes plus nine new ones; original body runs only in audit mode |
| Resumable C | 2 | Readable C with original-edge checkpoints; no register-residue execution |
| Total | **202** | 162 new void overrides + 38 retained pilot entries + two poll overrides |

**128/202** entries execute in the four reference schedules, for **10,819,167 C wrapper entries**. Zero-count entries have qualification/boundary evidence, not demonstrated game coverage. Full source hashes, generated-C hashes, per-function counts, CPU splits and instruction totals are in [round2-equivalence.json](../../port/converge/evidence/round2-equivalence.json).

The eleven exclusions from the 173-function void cohort are recorded rather than weakening the bridge. [round2_exclusions.json](../../port/converge/round2_exclusions.json) also prevents generation from reinstalling a known unsupported entry merely because its old RAM-only certificate says PASS. A forced rejected-list test preserves the prior usable output. The exclusions are:

| Function | Stuck class / evidence |
|---|---|
| `02203ec2`, `02237e6c` | Calls outside the supported single terminal-tail contract; rejected before 64-vector installation |
| `02219ad2` | Stronger 64+hinted qualification reaches an unmapped nested pointer; SKIP, not installed |
| `02223d98` | Effectless ISR vectors do not prove its hardware path; not an exact no-op |
| `02244988` | C calls `06048c4e`, while the port has inlined that helper into the body; combat rejected the unsupported call |
| `022414ee` | Attract Scene 8 supplies an output pointer into the caller’s stack; conservative spill/public classification rejects it |
| `02214194` | Autoplay Scene 13 supplies a stack output; actual rejection at approximately VB 14710 |
| `02214a50` | Live vector pointer reaches low BIOS address `20000210` (physical `00000210`) in Scene 72, after VB146340 |
| `02214a40` | Sibling vector reader withheld conservatively; no individual failure claimed |
| `0221400c`, `02214284` | Weighted-output family withheld conservatively after the stack-output wall; no individual runtime failure claimed |

A later autoplay attempt also reaches a nontransactional bus in pilot adapter `02244874`, Scene 62, after VB 95430. That adapter is removed; siblings `022448d0` and `0224492c` are withheld conservatively, without claiming individual failures. The pilot’s recorded short-run passes remain valid; they did not cover this live bus path. The broader cohort now excludes these entries. The full attempted trace and passing prefix remain under `work/converge/round2/runs/round2-final-fullgame/`. The stack-output wall appeared twice. Work on that boundary stopped; the four affected/family entries were excluded and the reference schedules restarted. No floor-registry rows were changed. These functions need explicit caller-owned stack spans distinguished from callee spill bytes, with alias tests.

## Nine residue contracts removed

`022131d4`, `022131fc` (XZ/XYZ distance); `0222f3ea`, `0222f40e`, `0222f432`, `0222f456` (counter/flag gates); `02241556`, `0224158c` (60/90-frame debounce); and `02243fba` (comparison mask) now execute their C bodies and small, body-hash-pinned register recipes. C owns their returns and public stores. The recipes recover live scratch outputs, T, MACH/MACL and pointer advancement; they do not execute the generated instruction stream in normal mode.

Original-memory reads use `abi_read`; reads which must observe C’s earlier store use `effect_read`. The latter matters when a counter output aliases its flags input. Direct adapters now admit C memory effects, reject private-stack effects, and in audit mode compare the complete SH2Context plus the ordered public byte-write trace before committing C’s stores.

The new recipes pass **90,000** complete-context/memory checks across random states, both CPUs, signed/wrap edges, counter expiry, timer thresholds and effect aliases. The probe caught a missing final postincrement in the XYZ recipe; both input pointers now advance by 12, as the port requires. The five old recipes pass another **50,000** full-context checks, and the magnitude tail bridge passes **256 real C→port square-root tail calls**, now entering the resumable square-root override. See `abi-probe.log`, `pilot-abi-probe.log` and `poll-probe.log` under `work/converge/round2/`.

24 of the pilot’s 36 residue adapters remain; nine are removed and three collision adapters are excluded. The remaining residue is not automatically removable merely because C produces the same r0. The direct recipes retain explicit machine-boundary restrictions for mods; arbitrary changes to register-visible control flow still need an updated contract.

## Poll checkpoints and continuation result

Two of the 74 PASS-with-poll functions are implemented in [polls/continuations.c](../../port/converge/polls/continuations.c): `02213164` (fixed square root) and `02228106` (effect-slot allocation). Each passes a separate 256-vector oracle qualification. These are reviewed resumable forms of the frozen readable C, not an automatic general C coroutine transformation.

The C loops call a checkpoint only on the original taken backedge, after its delay slot. At the checkpoint the C++ adapter maps every live local into the existing SH2Context registers, restores the original T/continuation fields, and invokes the existing `SH2_POLL` exactly once. Resume IDs remain **3228** and **5859**, and dispatch resumes after the poll already spent. Slot allocation rereads live RAM after a poll so ISR/slave changes are visible. No runtime source, serializer layout or new continuation table entry is needed. Timing means equality with the frozen port’s safe-point budget and scheduler, not a hardware cycle-accuracy claim.

The boundary probe passes **4,000 full-context/memory vectors, 55,975 complete checkpoint comparisons and 55,975 serialized continuation resumes**. It exercises both CPU IDs, every loop checkpoint, table lengths around 0/23/24/25, arithmetic boundaries and forced scheduler unwinds. Another 3,000 vectors cover negative/zero/positive poll budgets and occupancy changes made by the scheduler callback; the C scan sees the live change on resumption. All state required after an unwind is recoverable from serialized context; native C stack objects are not required. The square-root override is reached in normal game runs. The effect-slot allocator’s actual schedule counts are explicit in the evidence; zero counts must not be read as game-path proof.

General suspended calls, C stack objects that escape, unrolled-loop/source-edge correspondence, MMIO ordering and custom register ABIs remain unsupported. A poll cannot run inside the speculative whole-function transaction. This two-function implementation establishes a contained checkpoint mechanism; it does not make the other 72 PASS-with-poll functions eligible.

## Exact reference evidence and measured coverage

The replays crossed local midnight. Stock and override both changed one startup RAM byte, `06098803`, from Sunday 0 to Monday 1; pixels and semantic traces stayed equal. `0220acf6` returns the SMPC OREG3 weekday nibble, and `022016ac` stores it to `06098800`; this identifies the changed byte without blaming an adapter. A test-only `time()` shim now pins the SMPC RTC input to November 8 at 23:00 EST. Both one-frame RAM dumps then reproduce the original stock hash exactly. Both final stock and override four-schedule captures use this pinned input and also reproduce the original pre-midnight hashes; no RAM byte is masked. Details are in `port/converge/evidence/round2-clock-input.json`. Each standard run hashes every composed BGRX frame, both full 1 MiB RAM banks and the semantic record. The deterministic trace filter is the inherited SCENETRACE/coin/fatal/stop/SCSP/sample-count filter. It is not a full instruction or audio-sample trace. Every capture index must be present. The final override binary, rather than a preliminary candidate, is used for all four rows.

| Schedule | Identical records | PNGs | `frames.jsonl` SHA256 |
|---|---:|---:|---|
| nocoin | 9,000 | 30/30 | `7d040b8e950aac27edfe4cbc925e76087ea917a8e42a61fb998ee33c4ab3a6fb` |
| gameover | 9,000 | 16/16 | `8a6052304c56e6cf98e479f18425386f7248d42083c80bb73f7762e262b41e1d` |
| combat | 2,100 | 5/5 | `5b31a8de38d44299c6b1649621bf23432d8d9ee76b4b299f2ff3385cef18177b` |
| legacy-combat | 2,100 | 5/5 | `66b2dd4dde533bc78fdf2c9e79bb19e9c813708093a16a5a855b82a78fd17f52` |

All four trace hashes and every screenshot hash also match. The separately instrumented stock build reproduces those same full records, PNGs and traces, validating the profiling instrumentation’s lack of guest-visible effects.

The profiler counts actual executed emitted guest instructions, with branch delay slots counted separately, and body entries rather than continuation dispatches. It instruments all **1,152 generated cartridge function definitions**, including entries without readable C. Runtime/host services, BIOS emulation and the 68000 sound CPU are outside this SH-2 denominator. Tail and interrupt entries count as body entries; intra-body branches and continuation resumes do not. For every selected function and both CPUs, profiling entry counts exactly equal the override wrapper counts. Counts are attributed to the owning body, so a selected dispatcher does not receive credit for an unconverted callee.

| Schedule | All body entries | Entries served by C | C calls % | All guest instructions | C instruction % |
|---|---:|---:|---:|---:|---:|
| nocoin | 11,753,556 | 4,292,114 | 36.518% | 150,293,642,528 | 0.10526% |
| gameover | 13,497,347 | 4,436,604 | 32.870% | 151,567,434,748 | 0.10206% |
| combat | 3,286,412 | 1,086,491 | 33.060% | 35,335,957,603 | 0.10822% |
| legacy-combat | 3,114,288 | 1,003,958 | 32.237% | 35,275,801,196 | 0.10017% |

Aggregate: **10,819,167/31,651,603 entries (34.182%)**, **386,452,808/372,472,836,075 guest instructions (0.10375%)**. Residue-free C covers **7,695,933 instructions (0.00207%)**. Instruction counts mean the baseline guest work whose semantics C supplies; residue adapters also execute their generated register computation. They are not a claim that this percentage of host machine code has been removed.

`022011b0`’s slave command/idle loop dominates executed instructions (146.57 billion in attract alone). Retaining that loop in the denominator is intentional. Per-CPU totals permit a more useful gameplay view without hiding the idle work:

| CPU | C body entries / all | C instructions / all |
|---|---:|---:|
| Master | 16.860% | 1.78076% |
| Slave | 47.057% | 0.08307% |

## Executable cost and speed

Executable file size is **5,515,368 → 5,681,120 bytes**, an increase of **165,752 bytes (3.01%)**. ELF section sizes are recorded in the evidence. Automatic lowered C totals 4,235 lines from 5,120 readable-source lines, with 8,850 retained/audit residue lines; the two small poll implementations are additional.

Three combat measurements per executable, without direct ABI audit or FIFO capture, give median **process CPU seconds 67.624 stock / 58.046 override**, and median wall seconds **36.870 / 31.598** for 2,100 VBlanks. Every measured run still matches stock PNGs and trace. Process CPU includes master/slave threads. These runs overlap other work, so the lower observed time is not an isolated speedup claim. Transactions, allocations and duplicate register computation remain a substantial structural cost. All raw measurements and executable hashes are retained in the evidence.

## Off-by-default memory-effect mod

[mods/playfield_shift.c](../../port/converge/mods/playfield_shift.c) changes one constant in the active `02215000` VDP1 command builder: local X **0x160 → 0x190**, shifting the VDP1 playfield **48 guest pixels right**. The separate `--mod` build substitutes that readable C source. It changes guest command-buffer memory; actor gameplay state and the deterministic gameplay trace remain identical in combat.

The mod-specific bridge permits different values only for the two bytes at **initial command-list pointer + 0x54**. Every other address/value, write count/order, residual register and call target/argument retains the stock contract. The pointer is sampled before any transaction commits. Boundary tests accept the original/authorized writes and reject a disabled mod, an out-of-scope write, and a changed trace length. Normal builds pass `mod=false` for every entry. The normal bridge’s reserved HUD-word scope is unreachable with mods off; it was used for an earlier boundary fixture, not claimed as a visible HUD demo.

At VB 1560, **89,336/168,960 pixels** differ. Stock PNG SHA256 `74f3f3ecb1ea9d5733432e96b490703ca5031de42c0c811cbd5e2d85a8a28d9d`; mod PNG SHA256 `0f2852d66b9c64190b1f0e5f5faf19ee0a9082240bdb3d9f00d44fd5f58244a6`. All five combat PNGs are captured; the trace is identical. Normal/mod build fingerprints include the C, bridge and continuation sources and have separate quick-state identities. The mod is compiled out of the normal source selection; no stock build flag or runtime environment variable enables it.

Stock:

![Stock combat](../../port/converge/evidence/round2-stock-1560.png)

Opt-in C memory-effect mod:

![Playfield shifted by readable C](../../port/converge/evidence/round2-mod-1560.png)

## Autoplay

Fresh stock completes 170,000 VBlanks and enters ending Scenes 73/79/80/81. The 207-function override attempt reaches Scene 62 after VB95430, then rejects an out-of-contract bus access in `02244874`. A narrowed 204-function attempt matches 29 sampled PNGs and the scene/player-state prefix through VB146340 (Scene 72), then rejects `20000210` / low BIOS `00000210` in `02214a50`. This is the second live bus wall, so STATE.md’s stop rule is applied: no bus whitelist is weakened, both vector wrappers are excluded, and the final 202-function cohort is re-gated on every standard reference run. **Ending equivalence for the final cohort remains unproven.** The long attempts were not every-frame RAM captures or instruction profiles. Their host-clock start dates also differ; only sampled pixels and gameplay/scene prefixes are compared. Full failed logs and results remain in `work/converge/round2/runs/`; the collector retains their exit status and binary hashes. No full-game success is inferred from a partial prefix.

## Stuck classes and next plan

1. **Caller-owned stack effects vs private spills:** resolve the four withheld output functions with explicit spans, alias/order tests and a real saved-frame fixture. Do not broaden the whole private window.
2. **Inlined helper vs C call boundaries:** split/map `02244988`’s `06048c4e` helper; keep `02203ec2`/`02237e6c` excluded until their complete call contracts are represented.
3. **Remaining 24 pilot residue bodies and 162 new void residues:** prioritize hot matrix/quad/command builders. Custom r8/r9→r1, r4/r5 multi-result and live-r5 contracts remain explicit separate ABI work.
4. **Generic polls and suspended calls:** preserve original edge identities, delay slots and exact budget use; serialize typed locals or map them fully to existing context. The two leaf adapters do not solve general C continuation lowering.
5. **Inactive/device paths and stronger-vector SKIPs:** keep `02223d98`/`02219ad2` and the three collision readers out. Add a ROM-proven read-only BIOS bus model to both oracle and bridge before reintroducing the two vector wrappers; do not whitelist device writes or pretend this late path was covered by RAM-only qualification. Qualification is bounded by tested inputs; game execution plus structured vectors, not merely an old PASS label, should drive expansion.
6. **Acceptance:** add real quick-save/load at converted checkpoints, broader memory-effect mods and repeated isolated performance profiles. Current validation is Linux/GCC; cross-platform/UI/save-load acceptance is not claimed here.

Reproduce using [ROUND2.md](../../port/converge/ROUND2.md). `round2_build.py` uses the refreshed snapshot, `round2_verify.py` runs the four standard schedules, `round2_profile.py` creates the independent profiling copy, and `round2_collect.py` checks/exports evidence. Requalification/inventory scripts and each failed attempt remain under `work/converge/round2/`.

Installed function addresses by class (full hashes, names and dynamic counts are in the linked JSON):

**transactional-residue: 24**

```text
0220c858 0220cab0 02210020 0221047c 0221330c 02213354 02215630 02228134 0222a04e 0222a8d8
0222c220 0223b1c0 0223c74a 0223c7ba 02245382 02245a6a 02245aec 02245b48 02245bcc 02245dcc
02246026 022460c8 022461d8 022462bc
```

**direct: 14**

```text
02213190 022131a8 022131c8 022131d4 022131fc 0222f3ea 0222f40e 0222f432 0222f456 0223e5bc
02241556 0224158c 02243fba 0224617a
```

**PASS-void + residue: 162**

```text
02201b8e 02202dda 02202dfa 022041d4 02204258 02204308 0220686a 022072ce 02207884 0220b098
0220b0c6 0220b0fc 0220d188 0220d204 0220d8d0 0220d98e 0220e0aa 0220e1e2 0220e29e 0220e978
0220f0ec 0220f0ee 0220f15e 0220f816 0220fb98 0220fe98 022100f8 022101ce 02210858 022132d4
02213434 02213d24 02213fb4 022143b8 022147cc 02214818 022149a0 022149aa 02214a2e 02214a60
02215000 02215386 0221566e 02215f24 02215f6c 022161c8 022162fc 0221636c 0221707c 02217c10
02217ca2 02219984 02219ba4 02219c22 0221a5f6 0221b080 0221c2aa 0221ce1c 0221db6e 0221eac0
0221f240 0221f952 022206ae 0222078c 022213a8 0222142c 02221800 02221ffa 022220b0 022223b2
02222574 02222ecc 02223780 02225fac 02226134 02226144 022275e4 02227602 02227c22 02227c5c
02229640 0222a0ec 0222a5ca 0222a91a 0222ad7a 0222bd90 0222bdf2 0222eb72 0222eb84 0222eba8
0222ec3c 0222eea2 0222eee4 0222f3c0 0222f470 0222f640 0222f84e 022301ac 0223194a 022319b2
02231a1c 02231b0e 02231d72 022324da 02233178 022386c6 022386fc 0223875c 02238ba4 02238c4e
02238cd2 02238d5e 02238dde 02238df4 02238f46 02238fdc 02239462 0223964e 022396e8 022397c2
02239f78 0223a394 0223a6dc 0223ac50 0223b254 0223d0c6 0223dcc2 0223dd28 0223eef0 0223ef5a
0223efa0 0223efec 022416c4 0224174e 022417a4 022417ea 02241b2c 02241d18 02241d6e 02241e06
02242f9c 02242fb0 02244aa8 02244abc 02244c4e 02244d0a 02244d40 02244d6a 02244dc8 02245c88
02245e58 02245e80 02246052 02246254 0224627e 02247064 022470a2 022470da 0224840a 02249214
02249f08 02249fb0
```

**resumable-C: 2**

```text
02213164 02228106
```

1. Refreshed, timestamped snapshots isolate all work from concurrent port/decomp edits.
2. 202 overrides are installed, including 162 of the 173 no-poll PASS-void entries.
3. All 22,200 reference records, 56 PNGs and four traces match fresh stock.
4. Readable C serves 34.182% of body entries and 0.10375% of guest instructions.
5. Nine old residue adapters are removed; 14 functions now use direct C recipes.
6. Boundary checks pass 140,000 complete-context vectors plus 256 real tail calls.
7. Two C poll functions preserve 55,975 tested checkpoint/resume states.
8. The opt-in command-buffer mod visibly shifts gameplay 48 pixels and keeps its trace.
9. Stock reaches the ending; override ending equivalence remains unproven after the Scene 72 bus wall.
10. Stack outputs, inlined calls, custom ABIs and general suspended calls remain explicit blockers.
