# Readable C convergence pilot

The default port is unchanged. These scripts use only a frozen private copy in
`work/converge/`. The full design, results and limits are in
[the pilot report](../../notes/port/converge_pilot.md).

Run from the repository root:

```sh
# Freeze once; subsequent builds reuse that snapshot without recopying live work.
python3 port/converge/build.py --variant stock --jobs 4

# Requalify the chosen list against the frozen original ROM, not the live tree.
work/converge/snapshot/tools/decomp/venv/bin/python port/converge/qualify.py --jobs 4
python3 port/converge/build.py --variant pilot10 --jobs 4
python3 port/converge/build.py --variant pilot --jobs 4

# Five direct ABI recipes: complete register-state boundary/overflow checks.
cmake --build work/converge/build-pilot --target converge_abi_probe -j4
CONVERGE_AUDIT=1 work/converge/build-pilot/converge_abi_probe

# Stream every frame and both RAM banks through the existing read-only FIFO.
python3 port/converge/verify.py --variant stock --schedules combat --frames
python3 port/converge/verify.py --variant stock --label stock-long \
  --schedules nocoin,gameover,legacy-combat --frames
python3 port/converge/verify.py --variant pilot10 --schedules combat --frames --audit \
  --reference work/converge/runs/stock/manifest.json
python3 port/converge/verify.py --variant pilot --frames --audit
python3 port/converge/verify.py --variant stock --schedules autoplay --label stock-autoplay
python3 port/converge/verify.py --variant pilot --schedules autoplay --label pilot-autoplay --audit \
  --reference work/converge/runs/stock-autoplay/manifest.json

# Deliberate C mod, separate executable. Normal pilot/default builds omit it.
python3 port/converge/build.py --variant mod --jobs 4
python3 port/converge/verify.py --variant mod --schedules combat --label mod-final

# Final executable check and portable evidence export.
python3 port/converge/verify.py --variant pilot --schedules combat --frames --audit \
  --label pilot-delivery --reference work/converge/runs/stock/manifest.json
python3 port/converge/compare.py --include-autoplay
```

`overrides.txt` selects the expanded list; `pilot10.txt` selects its starting
subset. `build.py --overrides FILE` accepts another list. Qualify a new list with
`qualify.py --overrides FILE` first. Qualification updates the small source-hash
certificate in this directory. Changed runtime bodies invalidate the five pinned
ABI recipes and require review. The existing decomp venv supplies `pycparser`;
no download, installation or interpreter is needed at runtime.

The normal direct path executes the readable C plus its ABI recipe. Set
`CONVERGE_AUDIT=1` to additionally check the original register residue for these
five functions. The other 36 adapters retain register residue unconditionally;
they validate C returns/public writes and commit C data. Ordinary calls,
scheduler polls, hardware accesses and fixed-address C stack locals fail closed.
The supported real callees are terminal tail branches; their own compiled polls
remain intact. `CONVERGE_COUNTS=1` reports per-function/per-CPU call counts.

Edit [mods/sin_half.c](mods/sin_half.c) for the optional demo. Both variants keep
guest pointers as 32-bit addresses. A host pointer must never escape into guest
memory. This Linux pilot uses GNU `_Exit` link wrapping for counters and builds
the existing null host; SDL/macOS/Windows packaging is outside its validation.

Round 2 uses the separate refreshed snapshot and builders documented in
[ROUND2.md](ROUND2.md); its result is [converge_round2.md](../../notes/port/converge_round2.md).
The original pilot commands and certificate remain available.
