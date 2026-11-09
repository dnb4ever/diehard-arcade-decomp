Round 3 uses independently refreshed inputs in `work/converge/round3/snapshot/`.
All changes are opt-in and the ordinary port build remains unchanged. Historical
port run payloads are omitted from the source snapshot to keep disk use bounded.

The installed cohort is defined by `round3_overrides.txt` plus the three pinned
resumable bodies. `round3_contracts.json` describes exact caller-owned output
spans and custom register interfaces. `round3_recipes.json` pins machine bodies
and reconstructs the reviewed boundary state without running their algorithms.
The baseline transaction, bus and ordered public-write checks remain enforced.

Reproduce from the repo root using the existing private snapshot:

```sh
work/converge/round3/snapshot/tools/decomp/venv/bin/python port/converge/round3_qualify.py
work/converge/round3/snapshot/tools/decomp/venv/bin/python port/converge/round3_build.py --stock
work/converge/round3/snapshot/tools/decomp/venv/bin/python port/converge/round3_build.py
CONVERGE_AUDIT=1 work/converge/round3/build-round3/round3_contract_probe
work/converge/round3/build-round3/round3_pulse_probe
CONVERGE_AUDIT=1 work/converge/round3/build-round3/converge_abi_probe
CONVERGE_AUDIT=1 work/converge/round3/build-round3/round2_abi_probe
work/converge/round3/build-round3/round2_poll_probe
python3 port/converge/round3_verify.py --variant stock --label stock --frames
python3 port/converge/round3_verify.py --variant round3 --label final --frames --audit
python3 port/converge/round3_verify.py --variant stock --label stock-fullgame --schedules fullgame --frames
python3 port/converge/round3_verify.py --variant round3 --label final-fullgame --schedules fullgame --frames --audit
python3 port/converge/round3_progress.py
work/converge/round3/snapshot/tools/decomp/venv/bin/python port/converge/round3_bios_proposal.py
python3 port/converge/round3_collect.py
```

`round3_verify.py` pins the same host RTC input as round 2. Full-game mode
captures every VBlank's scene/player semantics, full RAM every 300 VBlanks and
34 PNGs. Standard schedules capture full RAM and composed pixels every frame.
No RAM byte is masked. `round3_capture.patch` changes only capture frequency in
the separate diagnostic runtime, used identically by stock/override/profiler.

For a separate stock profile, run `round2_profile.py` with the round-3 snapshot
and `work/converge/round3/profile` output, then build it with
`round3_build.make` into `build-profile` and replay `--variant profile --label
profile --frames`. The collector demands equality of all reference hashes and
of per-function profile/wrapper call counts, including both CPU identities.
Calls made by an unconverted callee receive no C credit. Round 3 qualifies mods
off; use the unchanged round-2 build for its separate mod demonstration.

The BIOS proposal writes a unified patch under `port/converge/` and executes
that proposed text in isolation. It never edits the owned oracle. Cartridge
instructions prove the vector readers' addresses and widths. The immutable
bytes at physical `00000208..00000213` come from the frozen stock HLE model;
there is no claim that these bytes were extracted from a physical ST-V BIOS.
Writes and all other buses remain rejected. Promote that patch only through
the oracle owner's review.

Real caller-stack fixtures are written once per encountered output family in
`work/converge/round3/fixtures/`. They contain a kind word, the entry SH2Context
and both complete RAM banks. Replay one by giving its path to
`round3_contract_probe`; the probe compares original body and C override,
including complete context and private spill memory. Fixed-address oracle
scratch frames are never admitted into the game bridge.
