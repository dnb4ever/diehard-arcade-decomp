#!/usr/bin/env python3
"""Write the requested round-4 narrative from validated acceptance evidence."""
import json,hashlib,re
from pathlib import Path
W=Path('work/converge/round4');E=Path('port/converge/evidence/round4-equivalence.json')
def main():
 e=json.loads(E.read_text());a=e['aggregate'];p=e['performance'];rows=e['aggregate_by_function'];selected=e['selected'];s=e['snapshot'];old=e['rebase_audit']
 cert=json.loads((W/'qualification.json').read_text())['functions']|json.loads((W/'qualification-compute.json').read_text())['functions']
 vectors=sum(cert[r['address']]['vectors'] for r in selected);free=sum(a['residue_free'][2:]);before=sum(a['rebase'][2:]);after=sum(a['final'][2:]);logical=a['total_logical_instructions'];emitted=sum(a['total_emitted'][2:]);new=after-before
 reached=sum(sum(rows.get(r['address'],[0]*4)[:2])>0 for r in selected)
 targetnames={'02214e2c':'Radix sort, full prologue','02214e2e':'Same radix body, short prologue','02213ca4':'Translate current matrix','02213b5c':'Transform matrix translation column','0221326c':'Forward word copy'}
 table='\n'.join(f'| `{addr}` | {name} | {sum(rows.get(addr,[0]*4)[:2]):,} | {sum(rows.get(addr,[0]*4)[2:]):,} |' for addr,name in targetnames.items())
 perfrows='\n'.join(f'| {v} | {p[v]["fields_s"]:.2f} | {p[v]["cpu_percent"]:.1f}% | {p[v]["cpu_ms_per_field"]:.3f} |' for v in ('stock','round4'))
 runrows='\n'.join(f'| {r["variant"]} {i+1} | {r["fields_s"]:.2f} | {r["cpu_percent"]:.0f}% | {r["cpu_ms_per_field"]:.3f} |' for i,r in enumerate(p['runs']))
 scene_log=(W/'runs/final-fullgame/fullgame/run.log').read_text();scene_first={scene:int(re.search(rf'SCENETRACE VB (\d+) scene={scene} ',scene_log)[1]) for scene in (73,79,80,81)}
 full=e['fullgame']['final-fullgame'];fixed=e['binary']['round4']['sha256'];stock=e['binary']['stock']['sha256']
 report=f'''# Readable C convergence — round 4

2026-11-09. **The current P4.15 stock build, the refreshed 211-entry round-3 cohort, and the expanded 216-entry cohort each complete 170,000 VBlanks through ending Scenes 73/79/80/81. Both override builds exactly match stock: 170,000 state records, 566 complete-RAM samples, 34 PNGs and the filtered trace.** Each also matches all 22,200 standard reference records and 56 PNGs. Overrides remain opt-in.

C coverage rises **{a['rebase_calls_percent']:.3f}% → {a['final_calls_percent']:.3f}% of calls** and **{a['rebase_logical_percent']:.5f}% → {a['final_logical_percent']:.5f}% of logical guest instructions**. The five new entries account for **{new:,} instructions of real radix/matrix/copy work**; the two radix entries share one algorithm. The corresponding P4.15 emitted-work share rises **{a['rebase_emitted_percent']:.3f}% → {a['final_emitted_percent']:.3f}%**. These denominators are kept separate below.

The final 1x performance result is **{p['stock']['fields_s']:.2f} fields/s stock versus {p['round4']['fields_s']:.2f} override**, with **{p['stock']['cpu_ms_per_field']:.3f} versus {p['round4']['cpu_ms_per_field']:.3f} CPU ms/field**. Throughput changes {p['throughput_percent_change']:+.2f}%; CPU cost changes {p['cpu_cost_percent_change']:+.2f}%. The performance expectation is **{'not met: the override remains slower' if p['cpu_cost_percent_change']>0 else 'met in these paired measurements'}**. Equivalence and coverage do not establish a host speedup.

## Frozen inputs and rebase

The source snapshot is `work/converge/round4/snapshot/`, copied from **{s['started_utc']} to {s['completed_utc']}**. It contains current functional C/headers, symbols, floors, unchanged v1.09 oracle/ABI/hints, cartridge, generated P4.15 program, port sources and runtime/recompiler. Inputs are independent copies, not hardlinks. The separate observer runtime changes only full-RAM transport frequency and is used identically by stock, override and profiler. The original 1x renderer, coroutine scheduling and fingerprinted `sh2_stv_idle_batch` remain active.

The collector rechecks **{e['pinned_files']:,} pinned files**, **{e['capture_runtime_files']} observer-runtime files**, and the frozen SDL dependency manifest. No pinned-input drift is present. Snapshot-manifest SHA256: `{e['snapshot_manifest_sha256']}`. Byte-for-byte regeneration changes **zero generated files** (`generation-check.json`). All installed source hashes match fresh qualification certificates.

The starting HEAD was `c57ef4a`. Another job advanced HEAD to `497a450` while this work ran; only three Mac packaging inputs differ from this snapshot (`port/p411_app_mac.c`, `port/p411_build_app.sh`, `port/p411_bundle_mac.py`). They are recorded in `scope-audit.json`. This job did not edit them. The Linux generated program/runtime/reference inputs remain those pinned at the start. Pre-existing `notes/pm_board.md` and untracked `tools/decomp/difftest_v104.py` are left alone.

All **211 inherited entries survive**: fresh v1.09 qualification has **41 PASS / 170 PASS-void / 0 FAIL / 0 SKIP**, **14,720 passing vectors**, including additive profiles. No entry is silently dropped or rebound. **{old['source_changed']} C source hashes changed** since round 3; every selected original machine body is unchanged. `rebase-audit.json` records every source identity, current semantic name, callable PC, old/new source hash and all current explicit source-entry bindings. The generator resolves `source_entry(canonical(address))`; duplicate drafts and pool/interior labels are not independently installed. The seven corrected semantic bindings are honored through the frozen current symbol/oracle inputs. None of the 26 round-4/5 behavior-fix source filenames intersects the inherited cohort or the five additions. The corrected quad duplicate `02213d18` binds to already installed `02213d24`; the canonical source is retained once. Likewise, existing `02245c88` is not rebound to its preceding pool label. Fixed deferred emitters `022156da`/`0221582a` remain excluded pending their live contracts. This accounts for the fixes without claiming they are newly converted work.

The final cohort is **21 direct C recipes, eight resumable C entries and 187 transactional residue adapters**. Its **216 installed entries pass {vectors:,} vectors**, no FAIL/SKIP. `02213ce4` also passes 64 vectors but is not installed. **{reached}/216** entries execute in the standard schedules. Full identities/counts are in [portable evidence](../../port/converge/evidence/round4-equivalence.json); [contracts](../../port/converge/round4_contracts_compute.json) and [reproduction](../../port/converge/ROUND4.md) are provided under `port/converge/`.

## Compute work and exact boundaries

| Cart entry | New C computation | Standard calls | Baseline guest instructions |
|---|---|---:|---:|
{table}

The radix C retains the primary reverse record walk, uncached secondary forward walk, stable middle-byte buckets and final six-bit buckets. Arena-pointer stores, old-head reads, node next/entity/key writes and head publication keep their original order. Final-pass key words stay untouched. The full entry saves r14..r8; the short entry saves r13..r8 and retains the original final r14 load from entry SP. It is a distinct machine interface, not a second unique sort algorithm. No live coverage is claimed for the short entry.

Every taken radix backedge polls at the original post-delay-slot state: **3448..3453** or **3455..3460**. Resume starts after the already-spent poll; source/bucket cursors, node/arena values, counters, T and branch locals are recovered from existing registers/continuation fields. The matrix backedges retain IDs **3302** and **3288**. Copy retains **3237**, including both pointer advances and the count decrement before its poll. No new safe point, budget shortcut, continuation ID or serialized context field is introduced.

Matrix computation uses **SR.S=0**, checked fail-closed on every entry/resume. Signed 32x32 products accumulate modulo 2^64; C extracts bits 16..47 and adds the row offset modulo 2^32. Full MACH/MACL survive at every checkpoint and exit, not merely the returned translation. The input vector is reread from original guest spills after each handoff, preserving interrupt/slave changes and aliasing. Translation-column setup keeps the ROM's interleaved source loads/spills. The copy C preserves forward overlap propagation rather than substituting memmove.

The compute probe passes **316 vectors, 9,882 exact complete-context/both-RAM checkpoint comparisons, 1,787 serialized resumes and 311 forced unwinds**. It tests both CPU identities, both radix prologues, empty/duplicate-key lists, cache aliases, overlapping copies, matrix/spill aliases, MAC overflow and post-poll input mutation. Nonzero frozen cartridge bytes and a byte mutation test cover immutable-ROM reads. Negative probes retain SR.S, device-read and ROM-write rejection. All original inherited checks pass again: 140,000 direct ABI vectors, 256 genuine compiled-tail calls, 16,400 stack/alias/direct-contract vectors, 4,000 poll vectors with 55,975 checkpoint/resume comparisons, 3,000 budget/live-occupancy vectors, and 1,000 pulse vectors with 17,994 checkpoints/resumes and forced unwinds.

Four real combat entry fixtures (radix full entry, both matrix entries, copy) contain the entry context and both complete RAM banks, with exact capture-binary hashes. Independent replay compares original bodies with the installed C. The captured radix case adds **267** spent-poll comparisons, **six** serialized resumes and an unwind. The other captured entries finish within their available budget; they prove complete return context/RAM, while synthetic probes provide their spent-poll/resume evidence. Two additional current-runtime caller-owned stack fixtures (`02214194` and `022414ee`) also replay against original bodies with complete context and RAM. These fixtures are not claimed as captures of all possible graphs or of the unused radix entry.

The first expanded attempt rejected **`22db30a4`**, an actual read-only cartridge source in `0221326c`, at attract VB8563 and autoplay VB14595. Its new helper had accidentally omitted the inherited immutable-cartridge read contract. The corrected helper uses exactly the existing bridge's normalized cartridge bounds, complete 32-bit reads only; writes remain RAM-only. Failed prefixes remain under `runs/attempt-ram-only-*`. No MMIO, cache purge or general BIOS permission was added. This was one missing existing contract exposed by two schedules, not repeated expansion through deeper bus layers.

## Equivalence against current stock

The stock binary is newly built from the same snapshot, not taken from round-3 archives. The same fixed November 8 RTC input is supplied to all runs; no RAM byte or record field is masked. Standard schedules retain complete BGRX frames, both full 1 MiB RAM banks and semantic state every VBlank. Full-game mode retains every-VBlank semantic state, both complete RAM banks every 300 VBlanks, and PNGs every 5,000 VBlanks. FIFO payloads are hashed and discarded.

| Schedule | State/frame records per build | Identical PNGs, each override vs stock |
|---|---:|---:|
| No-coin attract | 9,000 complete frame/RAM/state | 30/30 |
| Game over → attract → second game | 9,000 complete frame/RAM/state | 16/16 |
| Combat | 2,100 complete frame/RAM/state | 5/5 |
| Legacy multi-cart combat | 2,100 complete frame/RAM/state | 5/5 |
| Ending autoplay | 170,000 state; 566 full-RAM samples | 34/34 |

All filtered deterministic traces match for stock/rebase/final; every expected index must exist. The separately instrumented stock profiler reproduces all standard hashes. Per-function/per-CPU wrapper counts equal profile body-entry counts for both cohorts. Audit mode checks the inherited direct recipes against complete original context/effects. A separate final audit-off combat capture also matches all 2,100 records and five PNGs.

First ending-scene VBlanks are **{scene_first}**. The final full-game record-stream SHA256 is `{full['frames_sha256']}`; trace SHA256 is `{full['trace_sha256']}`. AUTOPLAY/SCENETRACE lines match exactly. These also reproduce the historical round-3 record stream, but current stock is the actual acceptance oracle. AUTOPLAY may press START during ending scenes; this establishes ending-route traversal, not an unskipped cinematic. Full-game pixels/RAM are sampled, not captured every VBlank. Save-file/UI round trips and hardware/MAME fidelity are not acceptance claims.

Stock executable SHA256: `{stock}`. Final override SHA256: `{fixed}`. Every final replay manifest pins that exact override binary. Sources, failed trials, independent fixtures, qualifications and logs remain under `work/converge/round4/`.

## Honest work accounting

Logical denominator: **{logical:,} guest instructions**, exactly the historical round-3 reference total. P4.15 batches **{sum(a['batched_idle_instructions']):,}** empty-slave instructions; **{emitted:,}** emitted instructions still execute. Each skipped empty iteration represents 31 original instructions and five spent safe points. Instrumentation records the actual helper budget delta, attributes the skipped work only to compiled `022011b0`, and preserves batching. No host/HLE service or 68000 work is invented or included.

| Measure | Rebased 211 | Expanded 216 |
|---|---:|---:|
| C body entries | {sum(a['rebase'][:2]):,} | {sum(a['final'][:2]):,} |
| C calls / all {sum(a['total_emitted'][:2]):,} entries | {a['rebase_calls_percent']:.3f}% | {a['final_calls_percent']:.3f}% |
| Baseline guest instructions supplied by C | {before:,} | {after:,} |
| C / logical guest instructions | {a['rebase_logical_percent']:.5f}% | {a['final_logical_percent']:.5f}% |
| C / actually emitted guest work | {a['rebase_emitted_percent']:.3f}% | {a['final_emitted_percent']:.3f}% |

Residue-free C supplies **{free:,} instructions**, **{a['residue_free_logical_percent']:.5f}% of logical work**. Transactional C still duplicates generated register computation; it is not counted as residue removed. Newly converted computation is **{new:,} instructions**, a **{100*new/logical:.5f} percentage-point logical increase**, or **{100*new/emitted:.3f} percentage points of emitted work**. Reporting 25.73% alone would hide the P4.15 denominator change. The long run is not separately instruction-profiled; coverage is the four standard schedules only.

## 1x performance

Profiler and ABI auditing are off. Each run uses 3,600 fields, `STV_AUTOPLAY=100`, unpaced 1x, frameskip off, Sharp, 960x720 dummy/software SDL, identical RTC/inputs and the same final picture. Three runs per variant execute sequentially in reversed order after this job's correctness/build work finishes. CPU 100% means one logical CPU; CPU ms/field uses GNU time user+system totals. All pictures are presented/reused correctly, with zero omitted pictures; all final PNG hashes match.

| Build | Aggregate fields/s | Aggregate CPU | CPU ms/field |
|---|---:|---:|---:|
{perfrows}

| Run in measurement order | Fields/s | CPU | CPU ms/field |
|---|---:|---:|---:|
{runrows}

Shared-host load remains uncontrolled; every load triple and stats sample is retained. Earlier exploratory results and six measurements overlapping correctness are retained, including their late host-load spike. That earlier set measured {p['during_correctness']['stock']['fields_s']:.2f} versus {p['during_correctness']['round4']['fields_s']:.2f} fields/s and {p['during_correctness']['stock']['cpu_ms_per_field']:.3f} versus {p['during_correctness']['round4']['cpu_ms_per_field']:.3f} CPU ms/field. No outlier is deleted. The bridge now uses inline storage for small effects/read samples and initializer-list ABI descriptors, retaining the same vector fallback, transaction limits, alias classification, ordered-write and context audits. These remove avoidable allocations but do not erase the remaining C lowering/transaction/recipe overhead. {'Performance still blocks promoting this build as a speed improvement.' if p['cpu_cost_percent_change']>0 else 'These measurements establish a point-estimate improvement on this host, not a cross-platform guarantee.'} Hardware instruction/cycle counts and Mac/Windows performance were not measured.

## Floors and next plan

The two-layer stop rule remains in force. `02244874` / `022448d0` / `0224492c` remain excluded for live device/bus paths. `02244988` retains its inlined-helper boundary; `02203ec2` / `02237e6c` retain unsupported nonterminal calls; `02219ad2` retains its nested-pointer floor; the ISR hardware path stays excluded. The inherited BIOS model remains only immutable physical `00000208..00000213`; it is stock HLE data, not physical-BIOS evidence. No oracle proposal or shared floor row was edited.

Profile priorities are concrete. `02214f90` accounts for **1,699,429,823** reference instructions but polls VDP1 status and copies into VRAM through real compiled callees; it needs a device/copy/ordering contract. `022137a0` (**687,380,665**) and `022135a0` (**164,283,905**) retain projection/cache-purge contracts. `02213e0c` (**354,594,462**) needs a resumable nonterminal custom-register call to the depth selector. Deferred emitters `022156da` / `0221582a` (**145,488,483 / 34,624,751**) need captured valid descriptor/character/command graphs and exact store/checkpoint ownership before installation. Bounded v1.09 source PASS does not supply these live contracts. No attempt whitelisted a device or repeatedly changed invalid inputs until a run passed.

Next work should first reduce bridge data-access/recipe overhead with the same alias and chronological-store probes, then repeat 1x paired CPU measurements. Capture the high-share command/projection graphs with the relevant device state before converting those paths; RAM-only fixtures cannot prove an MMIO contract. General escaping C frames/callbacks, arbitrary coroutine conversion, save/UI round trips, physical BIOS fidelity and cross-platform validation remain separate requirements.

All job edits stay in `port/converge/`, `work/converge/`, and this requested report. The ordinary port, functional sources and owned oracle/runtime trees were not edited by this job. No commit, delegation, sudo, GPU access, prohibited-directory access or unrelated process termination was used.

1. Froze current P4.15 inputs and requalified all 211 inherited entries; no drops.
2. Resolved current names/bindings through v1.09; 107 source hashes changed, machine bodies did not.
3. Expanded to 216 entries: 21 direct, eight resumable, 187 retaining transactional residue.
4. Added radix, both translation computations and forward copy with exact checkpoint/MAC contracts.
5. Both cohorts match 170,000 ending records, 566 full-RAM samples, 34 PNGs and trace.
6. Both cohorts match all 22,200 standard records, 56 PNGs and four traces.
7. New compute probes pass 9,882 checkpoints, 1,787 resumes, and real captured-entry replays.
8. C coverage is {a['final_calls_percent']:.3f}% of calls and {a['final_logical_percent']:.5f}% of logical guest work.
9. Final 1x is {p['stock']['fields_s']:.2f} stock / {p['round4']['fields_s']:.2f} override fields/s; CPU cost changes {p['cpu_cost_percent_change']:+.2f}%.
10. Device/call/pointer floors remain excluded; performance and broader platform/save contracts remain next work.
'''
 Path('notes/port/converge_round4.md').write_text(report)
 print('Wrote notes/port/converge_round4.md')
if __name__=='__main__':main()
