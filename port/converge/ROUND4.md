Round 4 stays opt-in. It uses independently copied sources and the current
P4.15 main-thread SH-2 coroutine/idle-batching runtime in
`work/converge/round4/snapshot/`. Neither the normal port build nor the owned
decomp/runtime sources are edited. The preserved round-3 implementation remains
available. Start from the repository root.

The existing frozen snapshot is reused; `round4_freeze.py` creates a new snapshot
once and rejects an existing destination. Do not replace an acceptance snapshot.

```sh
work/converge/round4/snapshot/tools/decomp/venv/bin/python port/converge/round4_qualify.py
work/converge/round4/snapshot/tools/decomp/venv/bin/python port/converge/round4_qualify.py --compute
work/converge/round4/snapshot/tools/decomp/venv/bin/python port/converge/round4_build.py --stock
work/converge/round4/snapshot/tools/decomp/venv/bin/python port/converge/round4_build.py --rebase
work/converge/round4/snapshot/tools/decomp/venv/bin/python port/converge/round4_build.py
CONVERGE_AUDIT=1 work/converge/round4/build-round4/converge_abi_probe
CONVERGE_AUDIT=1 work/converge/round4/build-round4/round2_abi_probe
CONVERGE_AUDIT=1 work/converge/round4/build-round4/round4_contract_probe
work/converge/round4/build-round4/round2_poll_probe
work/converge/round4/build-round4/round3_pulse_probe
work/converge/round4/build-round4/round4_compute_probe
```

Replay standard schedules with `round4_verify.py --variant stock|rebase|round4
--label LABEL --frames`, adding `--audit` for override audits. Full-game mode
adds `--schedules fullgame`: 170,000 VBlanks, semantic records every VBlank,
both complete RAM banks every 300 VBlanks, and 34 PNGs. The same observer-only
runtime and fixed RTC input are used by every variant. No memory is masked.
Use fresh labels to preserve earlier evidence. Fixture files live below each
schedule's `fixtures/` directory. Give an entry fixture's path to
`round4_compute_probe` or a stack-output fixture to `round4_contract_probe`.
The shared radix interior entry is synthetic-probe qualified and gets zero
live coverage credit when no real call occurs.

`round4_profile.py` instruments frozen stock into `work/converge/round4/profile`.
Build with `round4_build.make`, destination `build-profile`, then replay
`--variant profile --frames`. The collector requires the same standard records,
PNGs and traces, and exact per-CPU wrapper/profile body-entry counts. Every
P4.15 batched empty iteration represents 31 guest instructions and five spent
safe points. The profiler records the helper's actual budget decrement, credits
these only to compiled `022011b0`, and reports both logical guest work and the
smaller emitted-work denominator. It never disables idle batching to inflate
host costs or credits it to readable C.

`round4_compute_pins.json` pins source and original machine bodies;
`round4_contracts_compute.json` documents original checkpoint IDs, stack order,
SR.S=0, full MAC results, aliases and bus bounds. Unsupported device/purge paths,
inlined helpers and general nonterminal calls remain excluded. Direct bridge
transactions use inline storage with an unchanged vector fallback/size limit;
ordered stores, public/private stack checks and audit comparisons remain active.

For profiler-off 1x SDL software measurements, build both variants with
`round4_performance.py stock|round4 --build`, then run `--label LABEL` (3600
fields by default). The script freezes SDL dependencies independently, disables
frameskip, selects dummy/software SDL, uses Sharp at 960x720, and records GNU
process CPU time, elapsed time, every stats sample, final PNG and host load.
Run pairs sequentially with reversed order and finish correctness/build jobs
first. No acceptance directory is overwritten. `round4_performance_collect.py`
retains individual runs and aggregate CPU/throughput; a slowdown must be reported.

`round4_collect.py` validates every final acceptance stream and immutable input,
then emits portable `evidence/round4-equivalence.json`. The requested narrative
is `notes/port/converge_round4.md`. No save/UI, physical BIOS, hardware-cycle,
MAME fidelity, Mac/Windows or arbitrary coroutine transformation claim follows
from these Linux/GCC results.
