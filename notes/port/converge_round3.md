# Readable C convergence — round 3

2026-11-09. **211 overrides complete 170,000 VBlanks of `STV_AUTOPLAY=100` through ending Scenes 73/79/80/81. Stock and the final cohort match all 34 PNGs, all 170,000 scene/player-state records, 566 periodic complete-RAM hashes and the filtered deterministic trace.** The four standard schedules also match all 22,200 complete frame/RAM/semantic records and 56 PNGs. Mods are off; the ordinary port build is unchanged by this work.

The cohort has **21 direct C recipes, three resumable C bodies and 187 transactional residue adapters**. Five existing hot residues were removed. Two custom-ABI functions and the per-frame pulse-measurement routine were added, alongside the four restored stack-output functions and two restored vector wrappers. Readable C serves **51.183% of body entries and 1.05371% of baseline guest instruction work** in the reference schedules; residue-free C covers **1.04931%**. The instruction-work increase is mostly the real per-frame pulse-measurement wait, not a newly converted complete renderer.

## Frozen inputs and isolation

The independent refreshed source snapshot is `work/converge/round3/snapshot/`. Its copy interval is **2026-11-09T06:02:38.055690+00:00 through 2026-11-09T06:02:40.122013+00:00**. This is an interval during concurrent work, not an atomic source revision. Historical run payloads and old build trees are omitted; the actual generated P41 program, required port sources, runtime/recompiler, functional C, oracle/venv, symbols, floor registry and cartridge are copied independently. No hardlinks to mutable sources are used.

The final collector rechecks **8,987 pinned files**, with no drift. Manifest SHA256: `63c35eb732cbd2d675f58f48f92dcb5e1356c5bf0959c70dfadaad9d7537f2ec`. Every installed source also matches its new qualification certificate: **211 passing functions, 14,592 passing vectors, no FAIL or SKIP**. Qualification uses the unchanged frozen v1.07 oracle; the BIOS proposal has separate additional model evidence.

Implementation changes stay in `work/converge/` and `port/converge/`; this requested report is the only delivered path outside them. P4.14's live `port/` and `tools/saturn-recomp/` edits are left alone. Neither `tools/decomp`, its ABI descriptors, `project/src_functional`, STATE.md nor the shared floor registry was edited. Per-function attempt records are private under `work/converge/round3/attempts/`. No commit, delegation, sudo, prohibited-directory/GPU access or unrelated process termination was used. Builds and qualification use at most four jobs. The convergence work area occupies approximately 4.6G; raw frame/RAM streams are consumed and discarded.

## Ending equivalence

Both binaries are built from the same new snapshot. The observer-only [capture patch](../../port/converge/round3_capture.patch) is applied to a separate `capture-runtime/` copy used identically by stock, override and profiler. It changes only when the existing read-only observer transports RAM. It changes no game state, poll, budget, device or scheduler behavior. The same round-2 RTC shim pins both runs to November 8, 2026, 23:00 EST; no RAM byte is masked.

| Ending acceptance | Result |
|---|---:|
| VBlanks / exact scene and player semantic records | 170,000 / 170,000 |
| PNG samples, every 5,000 VBlanks | 34 / 34 identical |
| Both full 1 MiB RAM banks, every 300 VBlanks | 566 / 566 identical |
| Filtered deterministic trace | identical |
| First Scene 73 / 79 / 80 / 81 VBlank | 154125 / 154178 / 154293 / 154894 |

The long-run record-stream SHA256 is `b1dd2e1ddd62a5ab63bfdfd5c419d117ba02a58e399273f7f75c74187f2576e9`. Its filtered trace SHA256 is `9f8b97083b47b266eab5481ae5f361728e2c171d1edfd384899fbc3a2d1d6d60`. The capture retains scene/mode/join/progress/RNG/counter and six actor records, including player identity, HP and XYZ. All common AUTOPLAY and SCENETRACE log lines match too. Every expected record and sample index must be present; comparison does not accept matching partial prefixes. See `work/converge/round3/runs/{stock-fullgame,final-fullgame}/fullgame/` and [portable evidence](../../port/converge/evidence/round3-equivalence.json).

The full-game claim is sampled pixels, every-VBlank semantic state and periodic complete RAM. It is not every-VBlank pixel or RAM capture. AUTOPLAY can press START in ending scenes, so this establishes reaching and traversing the ending route, not an unskipped cinematic or hardware/MAME fidelity. The prior three collision readers remain excluded; no live-device bus was made speculative to reach the ending.

## Caller-owned stack spans

[Contracts](../../port/converge/round3_contracts.json) declare output ownership explicitly:

| Restored function | Entry output span |
|---|---|
| `022414ee` | r5, 4 bytes |
| `02214194` | r6, 12 bytes |
| `02214284` | r6, 8 bytes |
| `0221400c` | r6, 12 bytes; r7, 4 bytes |

Only declared bytes are reclassified as caller-owned public outputs. The conservative private window is retained for everything else. A declared output intersecting callee-private bytes below entry SP is rejected. RAM/cache-through/HWRAM aliases normalize to the same guest bytes. Overlapping declared outputs remain chronological effects; they are not deduplicated into a final memory footprint. Public write addresses, byte values and order must match the machine transaction, and commit preserves the original bus widths and private spill bytes.

The 16,400-vector new contract probe includes 2,400 original-body comparisons for these four functions, overlapping input/output and output/output cases, both CPUs and cache/HWRAM aliases. Negative probes reject spill overlap, writes past the exact declared span, and reordered overlapping stores that produce the same final bytes. This tests order sensitivity rather than merely final memory equality.

A real autoplay call to `02214194` and a real attract call to `022414ee` saved the entry SH2Context and both complete RAM banks when their declared outputs pointed into their caller's stack. The independent probe reloads it and compares the original frozen body with the installed override, including complete context and spill memory. It passes. Fixture `work/converge/round3/saved-frames/stack-0.bin` is 2,097,288 bytes; SHA256 `f763abed1dbfea35a9acbbf9fb4905492b348fcf6cfaea48c670f1bf4594f70c`. [Provenance](../../port/converge/evidence/round3-saved-frame.json) retains the exact replay result. The independent attract fixture is `work/converge/round3/saved-frames/stack-3.bin`; it also passes. [Both fixture hashes](../../port/converge/evidence/round3-saved-frames.json) and replay results are retained. The other two families retain structured alias vectors; these two saved fixtures are not misrepresented as four distinct captured callers.

## Immutable BIOS data model

The cartridge's original loads at `06018a46..06018a4a` and `06018a56..06018a5a` establish three consecutive 32-bit vector reads. The first observed late Scene-72 read is cache-through `20000210`, physical `00000210`, at vector+8; the actual vector base is `00000208`. The bridge admits read widths 1/2/4 only within **`00000208..00000213`**, through the stock runtime's immutable BIOS reader. Writes, crossing reads, other BIOS addresses and MMIO remain rejected. Both vector wrappers `02214a40` and `02214a50` are restored and exercise their genuine compiled matrix-translation tail callee.

[The oracle proposal](../../port/converge/round3_oracle_bios.patch) is **not installed**. [Its runner](../../port/converge/round3_bios_proposal.py) executes proposed source text in isolation against the frozen inputs. Python and native C tests check opt-in behavior, aliases, widths, bounds, forbidden writes/MMIO and a nonzero-byte mutation fixture. Both original-ROM vector wrappers are compared with independently lowered C at the live BIOS addresses. [Evidence](../../port/converge/evidence/round3-bios-model.json) pins the cartridge, original oracle and runtime BIOS source and records the actual load bytes.

The distinction matters: cartridge ROM proves the accesses; the frozen stock **HLE** reader supplies zero-filled immutable bytes at this span. No physical ST-V BIOS dump is present, and these zero bytes are not claimed to be independently ROM-proven physical BIOS content. This proves equivalence to stock's BIOS bus model. Physical-BIOS fidelity remains a separate evidence requirement for the oracle owner.

## Residue removed and custom ABIs

Five installed residues now execute readable C plus small reviewed boundary recipes. Direct-mode auditing compares every SH2Context byte, ordered public effects and the explicit private spill trace. C read samples retain values at their chronological use, which matters when quad output aliases source data. Private saved-register stores remain an explicit ABI obligation, not a re-executed guest algorithm.

| Function | Purpose | Reference calls | Baseline guest instructions |
|---|---|---:|---:|
| `02213d24` | Projected quad corners | 5,976,496 | 322,730,784 |
| `02214818` | SCU descriptor stack push | 3,573,903 | 39,312,933 |
| `02213fb4` | Default matrix load | 113,779 | 4,437,381 |
| `02215000` | VDP1 frame command headers | 13,530 | 1,095,930 |
| `02245e58` | Direction decode / live r5 | 11 | 206 |

The quad builder is the hottest installed residue in the starting profile, followed by the descriptor stack push. Their readable C already supplied data in round 2; this round removes their normal generated algorithm execution. They are not counted again as newly covered semantics. Together the five removals account for **97.05% of the starting residue cohort's reference guest instruction work**, measured on the round-2 baseline; [the numerator and denominator](../../port/converge/evidence/round3-residue-removal.json) are retained.

The explicit descriptors also cover `02213f90`: angle in r4 and sine/cosine results in r4/r5; and `02213d90`: selector in r9, quad in r8, logical result in r1. Sin/cos C output pointers map to declared bridge register slots that never become guest addresses or committed RAM. The live-r5 facing contract checks its byte store and r5 together, preserving undefined r0. In the references, sin/cos executes 1,019,766 times and the quad selector 3,750,347 times. The supplied `tools/decomp/difftest_abi_v107.json` was read and left unchanged.

A stronger probe caught a positional read-sample error in the quad-selector recipe: C can reread a selected maximum/minimum, so fixed sample indices were not stable. The corrected recipe reconstructs its boundary inputs from the unchanged quad fields. The preliminary failed autoplay stops near VB314; its failed status and logs remain under `runs/round3-fullgame/`. It is not used as acceptance. A later long attempt matched 146,364 state records, 487 RAM samples and 29 PNGs before rejecting `2000020c`: I had incorrectly treated the first +8 load at `20000210` as the vector base. The corrected model covers exactly `00000208..00000213`; the actual live base is now used in both zero and nonzero oracle fixtures. That failed attempt is retained under `runs/bios-span-firstload/`. This was an incorrect extent within the same immutable-vector contract, not permission to admit another bus. All final checks use the corrected executable.

## A heavier per-frame C continuation

`02223c74` now runs its readable pulse-measurement loop from `round3_pulse.c`. It updates the real frame tick, measures low/high shared-signal runs, publishes totals and clears the original gate/busy state. It neither replaces the external signal with a constant nor skips the wait. The C loop rereads live RAM after scheduler handoffs. It executes 13,538 times in the reference schedules, accounting for 3,463,487,880 baseline guest instructions.

Every low/high/outer taken backedge maps to the original poll after its delay slot, with existing continuation IDs **5560/5561/5562**. Low count, high-run count and accumulated count map to existing context registers. The adapter recreates full scratch/T/continuation state and original spill bytes. Resumption starts after the already-spent poll, and no native C stack survives an unwind.

The new pulse probe passes **1,000 complete-context/RAM vectors, 17,994 exact checkpoint comparisons, 17,994 serialized resumes and 1,000 forced unwinds**, including both CPUs, budget edges and external signal changes at polls. The inherited adapters repeat **140,000** direct boundary vectors, **256** real compiled-tail calls, **4,000** poll vectors, **55,975** checkpoint/resume comparisons and **3,000** budget/live-occupancy vectors. General suspended calls and escaping C locals remain unsupported; this is one additional reviewed resumable routine.

## Exact references and measured coverage

The separate profiler counts true body entries and dynamically executed emitted guest instructions, including delay slots, across all 1,152 generated cartridge definitions. Continuation resumes do not count as new calls. Counts are attributed to the body that executes them, so unconverted callees do not inherit a caller's C credit. Runtime/HLE services and the 68000 remain outside this SH-2 denominator. The instruction profile itself must reproduce every stock reference record and PNG, and selected per-CPU profile entry counts must equal actual wrapper counts.

| Schedule | Exact frame/RAM/semantic records | PNGs | C calls | C guest work |
|---|---:|---:|---:|---:|
| nocoin | 9,000 | 30/30 | 52.510% | 1.06730% |
| gameover | 9,000 | 16/16 | 50.451% | 1.04609% |
| combat | 2,100 | 5/5 | 50.678% | 1.04607% |
| legacy-combat | 2,100 | 5/5 | 49.883% | 1.03620% |

Aggregate: **16,200,311/31,651,603 calls (51.183%)** and **3,924,774,141/372,472,836,075 guest instructions (1.05371%)**. Residue-free C covers **3,908,399,993 instructions (1.04931%)**, versus round 2's 0.00207%. **135/211** entries execute in the four reference schedules. The long run is not separately instruction-profiled.

The slave `022011b0` idle/command loop remains in the denominator. The increase from round 2's **34.182% calls / 0.10375% instructions** is real baseline work coverage, primarily pulse timing plus custom-ABI quad selection and sin/cos. A wait contributes executed work; this is not a claim that most gameplay/rendering logic is now C. Transactional adapters still duplicate computation, and even direct recipes retain small boundary calculations. No isolated host speedup is claimed.

| CPU | C calls | C guest instructions |
|---|---:|---:|
| Master | 34.369% | 78.77078% |
| Slave | 63.680% | 0.09519% |

Stock executable: **5,537,272 bytes**, SHA256 `bf8883f7bbb83810ad962455eebde61b310a466144e9616f84514d1f709f7092`. Final override: **5,777,784 bytes**, SHA256 `e5f5febcf822c5538499e6464a0f597fc35e50d74ac218686b0657c839fe2475`. All reported final replay manifests pin this exact override executable. Original residue remains available only for audit for the 21 direct entries; normal execution does not run it. Mods are off. Round 3 does not requalify the older opt-in memory mod; its separate round-2 build remains available.

## Blockers and next plan

1. Keep `02244874`/`022448d0`/`0224492c` excluded until their live device/read contract is represented. The narrow immutable BIOS span is not a general bus exemption. The physical BIOS content question belongs with the oracle proposal review.
2. Keep `02244988`'s inlined helper, `02203ec2`/`02237e6c`'s nonterminal calls, `02219ad2`'s pointer floor and the ISR hardware path excluded. No open floor was ground repeatedly or cleared by changing an input until it passed.
3. Next compute-heavy targets are the qualified radix-sort pair and matrix translation, with exact loop/checkpoint and MAC state contracts. Vertex projection and the deferred command emitters retain their existing device/pointer floors. Use captured real graphs to resolve those floors before expansion.
4. Extend direct recipe ownership and alias contracts beyond the proven cases. Caller-owned spans do not enable arbitrary escaping local objects, callbacks or general nested C calls. The per-frame pulse success does not supply a general coroutine transformation.
5. Add actual save-file/UI round trips and cross-platform validation before promotion into the normal port. Current proof is Linux/GCC and serialized checkpoint probes; no serializer layout was changed. The new pulse C, adapter and header are included in the state-source fingerprint.

The two-layer stop rule remains in force. Round 2's explicit stack and BIOS boundaries were revisited with new contracts and independent evidence. Their bus checks were not relaxed into a general whitelist. No further unsupported class was promoted merely because a short run or old PASS label happened to pass.

Reproduce with [ROUND3.md](../../port/converge/ROUND3.md). The source snapshot, qualifications, failed attempts, fixtures, full logs and record streams remain under `work/converge/round3/`; compact hashes, counts and contracts are under `port/converge/`.

1. Refreshed immutable inputs isolate this round from concurrent P4.14 edits.
2. The final cohort has 211 overrides: 21 direct, three resumable and 187 with residue.
3. Both builds reach ending Scenes 73/79/80/81 across 170,000 VBlanks.
4. All 34 ending-run PNGs, 170,000 state records and 566 full-RAM samples match.
5. All 22,200 standard reference records, 56 PNGs and four traces match.
6. Four stack-output functions return with explicit spans and a passing real-frame replay.
7. Two BIOS vector wrappers return with a narrow read-only model and an uninstalled oracle patch.
8. Five old residues are removed; r4/r5, r1-return and live-r5 contracts are explicit.
9. C serves 51.183% of calls and 1.05371% of guest work; residue-free work is 1.04931%.
10. Remaining device, nested-call, pointer, general-continuation and save/UI floors stay documented.
