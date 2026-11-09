# Round 2

The immutable refresh is `work/converge/round2/snapshot/`; snapshot start/end UTC
and per-file hashes are in `snapshot-time.json` and `snapshot.sha256.json` beside
it. Do not reuse the pilot snapshot for this round. Default port builds still
link no overrides. No files outside `port/converge/`, the private work area and
the requested final note need changing.

Run from the repository root, with the existing difftest venv:

```sh
work/converge/round2/snapshot/tools/decomp/venv/bin/python port/converge/round2_build.py --stock
work/converge/round2/snapshot/tools/decomp/venv/bin/python port/converge/round2_build.py
python3 port/converge/round2_verify.py --variant stock --label stock-clock --frames
CONVERGE_AUDIT=1 work/converge/round2/build-round2/round2_abi_probe
work/converge/round2/build-round2/round2_poll_probe
CONVERGE_AUDIT=1 work/converge/round2/build-round2/converge_abi_probe
python3 port/converge/round2_verify.py --variant round2 --label round2-202-clock --frames --audit \
  --reference work/converge/round2/runs/stock-clock/manifest.json
work/converge/round2/snapshot/tools/decomp/venv/bin/python port/converge/round2_build.py --mod
python3 port/converge/round2_verify.py --variant mod --schedules combat --audit
```

`round2_overrides.txt` is the automatic C cohort; two separately pinned,
hand-adapted resumable C functions are appended by `round2_build.py`.
`round2_recipes.json` extends the five pilot ABI recipes with nine reviewed
contracts. The nine new contracts read original memory through `abi_read`;
post-store aliases use `effect_read`. C supplies all public stores and returns.
Direct-mode audits compare every context byte and ordered public byte store.

Void bridges preserve the residue's r0, including after a real tail callee.
Internal dispatch switches preserve local labels. An external default can be a
tail, while a C call inlined by the port is excluded until mapped explicitly.
No scheduler polls are hidden inside a speculative transaction.

The two poll functions have readable C loops with original-edge checkpoints.
All locals live across a checkpoint map to existing SH2Context fields; the
adapter resumes *after* the already-spent poll. Each real poll marshals the
complete stock checkpoint state before invoking the existing scheduler.
There are no extra continuation IDs, native stack serialization or runtime
changes. General calls, pointer-carrying stack locals, unrolled loops and MMIO
need further designs and remain excluded.

`--mod` compiles `mods/playfield_shift.c` in place of the original 02215000 C
body. Its only semantic edit is local X coordinate 0x160 -> 0x190 (+48 guest
pixels). The mod-specific copied bridge authorizes changed values only for the
two bytes at initial command-list pointer + 0x54. All other values, addresses,
write order/count, residual registers and callees keep the stock contract.
The normal copied bridge retains the earlier reserved HUD-word scope; normal
entries always pass `mod=false`. There is no runtime switch that enables this
mod in a normal executable. Build fingerprints cover the C, bridge and poll
sources, so normal/mod quick states have different identities.

`round2_profile.py` instruments a separate stock copy. It counts executed
emitted instruction statements, including separate delay slots, and actual
function-body entries (continuation resumes do not count as new calls).
This measures baseline guest instruction work attributable to selected readable
functions; it does not count host instructions or claim residue adapters have
stopped executing their register bodies. Exact source/body hashes and
per-function totals are exported with the final evidence.

The reference harness pins the SMPC host clock to November 8, 2026, 23:00 EST via a test-only time() shim. The game stores weekday at 06098803; crossing midnight otherwise changes full-RAM hashes even in stock. No RAM bytes are masked. The already-running long autoplay pair uses the host dates and is compared by sampled pixels and gameplay/scene traces, not full RAM.

Known exclusions are enforced by `round2_exclusions.json`, even when an old RAM-only certificate says PASS. Two late autoplay bus rejections stop ending promotion this round; the final cohort is re-gated on the four complete reference schedules.
