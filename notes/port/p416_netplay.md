# P4.16: two-player UDP rollback netplay

Design recorded before implementation, 2026-11-09. Paths are repository-relative.
No commit; no P4.17 files. Offline execution must retain its existing path.

Implementation and two-process deterministic tests pass. **Live UDP/internet
connectivity and Mac performance remain unverified in this sandbox, and the
strict 16.7 ms worst-case rollback budget is not met by the measured bursts.**
The eight-field safety cap is not a performance guarantee.

## Design (before coding)

Two peers, host P1 and guest P2, exchange only numbered physical panel inputs
once play starts. Each datagram repeats up to 32 inputs and carries a cumulative
receipt acknowledgement, timestamps for ping, and the latest confirmed-state
checksum. Missing remote input repeats the preceding remote panel word. Local
input delay is 0–3 fields (default 3). Prediction is capped at eight fields;
when that limit is reached the simulation waits, pumps events and retransmits.
Late input differing from a prediction invalidates the first affected field.

Use P4.10's complete machine payload at the first resumable post-poll boundary
in each field, including field zero. Keep nine preceding payloads in memory.
Restore the boundary preceding the first wrong input by unwinding through
QuickLoad, then re-simulate to the former present. Do not poll fresh local
controls or pace intermediate fields. Present only the last recovered field.
Network history, acknowledgements and clocks stay outside guest snapshots.

Handshake uses explicit little-endian integers, protocol/source compatibility
and both cartridge hashes. Host transmits the P4.13 power-on origin (first
16 KiB BIOS/settings work RAM, BIOS timer seed, fixed RTC origin), plus backup
RAM. Both execute from power-on; no native machine blob crosses the network.
Settings affecting gameplay come from the host origin. Periodic checksums cover
unmasked work RAM and canonical key CPU/device state at the same safe boundary;
compare only confirmed states, after correction. A mismatch or timeout stops
with a visible error. No automatic state resynchronization in this version.

Audio is speculative until the corresponding safe boundary is confirmed.
Buffer PCM in memory with its absolute sample cursor; discard only speculative
samples past a restored snapshot and release confirmed samples once, in order,
to both the device and WAV writer. Rollback must not clear the device queue.
This trades bounded sound latency for avoiding repeated/dropped rollback audio.
Network stalls can still exhaust a real audio queue and must be reported.

Disable quick save/load, rewind, input movies, autofire, fast-forward, training,
local pause and gameplay speed changes during a session. Use 1x rendering.
Both players use their own first keyboard/controller layout; guest live panel
is routed to P2. Scripted --stvinput uses the numbered P1/P2 panel normally.

Use nonblocking IPv4 UDP with POSIX sockets / Winsock and existing ws2_32 linkage;
no new dependency/download. CLI: --netplay-host PORT / --netplay-join IP:PORT,
--input-delay 0..3. Launcher exposes mode, port/address and delay; HUD shows
status, RTT and rollback count/depth. Wait loops pump quit events. Fixed peer
address/session nonce, strict packet lengths, bounded input horizon, duplicate
validation and a timeout guard the protocol. No NAT traversal/relay/encryption:
host needs a reachable UDP port (default 7845), usually router forwarding.

Validation will use two real game processes and an external deterministic UDP
latency/loss/jitter proxy, with scripted coin/start/combat and scene transitions.
Record confirmed checksums, final PNGs/PCM, rollback frequency/depth, snapshot,
restore and resimulation costs, process CPU and wall time. Run offline reference
PNG/trace/WAV checks, runtime tests, Linux build, Windows cross-build/package and
patch reconstruction. Wine execution belongs to the PM (sandbox blocked).

The eight-field cap is a memory/network bound, not a claim that eight complete
re-simulations fit one 16.7 ms field. P4.15's Mac i9 measurements give 90% of one
core paced and 95.54 fields/s unpaced with real presentation. Snapshot and
re-simulation measurements must establish the feasible average and burst budget;
no Mac result will be invented from Linux timing. Delay 3 reduces corrections.

## Implementation and use

The design above is retained as the pre-code record. The final implementation
uses `p416-netplay.patch`, applied after P4.15. Build with
`port/p46_build_window.sh`; macOS continues to use `port/p48_build_mac.sh` and
`port/play_mac.sh`. No platform-specific source fork or new dependency is added.

Open the launcher, select **Netplay**, choose **Host (player 1)** or
**Join (player 2)**, fill in the UDP port or numeric IPv4:port, and use the same
input delay on both machines. Press **Start** on both. Default delay is 3 fields
(about 50 ms); supported values are 0–3. The handshake rejects different delays.
The host's game settings override the guest's settings for that session. Address,
mode and delay are session choices; they are not saved to the INI.

CLI examples from the repository root:

```bash
# Host: player 1, listens on UDP 7845.
port/play.sh --netplay-host 7845 --input-delay 3
# Guest: replace 203.0.113.10 with the host's actual reachable IPv4 address.
port/play.sh --netplay-join 203.0.113.10:7845 --input-delay 3
# The same arguments work with port/play_mac.sh and DieHardArcade.exe.
```

Both people use their own **P1** keyboard/gamepad bindings and first controller.
The guest's live P1 panel is routed to cabinet P2, including coin and Start.
Each peer contributes only its own player. TEST, SERVICE and cabinet pause are
ignored. The existing independently configurable pad layouts remain available
for offline play. Scripted `--stvinput` continues to use numbered P1/P2 tokens;
`--stvinput @FILE` reads a long panel timeline for verification.

The HUD identifies P1 host / P2 guest, ping in milliseconds (round trip,
including scheduling), rollback count and maximum correction depth. Escape
cannot pause a session; close the window to quit. The other peer stops after a
15-second receive timeout. Connection setup times out after 60 seconds without
valid traffic. Desync produces `NETPLAY STOPPED: desync at field N`, logs it and
shows an SDL error dialog. Quick-state keys report that they are disabled.

For internet play, reserve the host computer's LAN address in the router, then
forward **UDP 7845** on the router to **UDP 7845** at that LAN address. Allow the
app/port through the host firewall. The guest uses the router's public IPv4;
on the same LAN use the host's LAN IPv4 instead. Forward the selected port if
using a different number. TCP forwarding is unnecessary. CGNAT/double NAT can
prevent inbound connections; obtain a reachable public IPv4 or use an existing
network/VPN that makes the peers reachable. No hole punching, rendezvous server,
relay, UPnP, IPv6 or hostname resolution is provided. This reachability
requirement is the main connection limitation. Traffic is not encrypted or
authenticated; share the endpoint with the person you intend to play with.

## Determinism and rollback details

A little-endian, versioned datagram has a session nonce and CRC32. The host
locks the sender's IP/port after a valid hello. Both ROM images have independent
64-bit FNV-1a identities. A separate source fingerprint covers machine/device
sources, guest ABI headers and generated game/continuation code, excluding
compiler/OS and the Windows-only added standard include. It matches in the
Linux and MinGW builds; quick states retain their separate exact-build identity.
ROM/source/delay/verification-frame-limit differences reject the handshake.

The host transfers 81 1-KiB-or-smaller fragments (81,936 bytes total), with CRC,
acknowledgements and retransmission. This is P4.13's **power-on origin** method:
16 KiB BIOS/settings work RAM, the cart boot-mode word and fixed UTC RTC origin,
plus 64 KiB backup RAM. ST-V has no separate modeled EEPROM device; the BIOS
work copies are its NVRAM-derived settings. No quick-state/native struct crosses
the network, so the handshake does not require matching compiler/OS ABIs.
Neither peer executes guest instructions before origin/start synchronization.

During play, packets carry up to 32 unacknowledged input records, cumulative
receipt acknowledgement, ping timestamps and the latest confirmed checksum.
Inputs are sampled once, delayed locally and retained across rollback. A late
mismatch restores the complete local snapshot **preceding the first wrong
input**, then follows the saved SH-2 continuations to the former present.
Repeated/old packets cannot change an already received input. Packet sizes,
CRC, input masks and frame horizon are checked. No prediction extends past the
eight-field cap: wait and retransmit instead. The initial four boot fields lack
resumable slave continuations; inputs there wait for confirmation when their
preceding boundary is unavailable. They are never restored from a missing state.

Ten retained full payloads use roughly 50 MiB at 1x, plus serializer/restore
working storage and a small speculative PCM queue. The same 22 device/CPU state
parts as quick save/rewind are used. A checksum every 60 fields and at the final
verification field covers both complete 1 MiB WRAM banks, virtual time, frame,
RTC origin, canonical registers/SR/budget/continuation of both SH-2s and VDP2
registers. Checksums are published only after the whole input prefix is known
and any correction has finished. Runtime checks intentionally sample key state;
the verifier additionally compares **every byte of the full final payload**,
including SCSP/68000, video, BIOS, interrupts and device state.

Confirmed PCM is emitted once, in order. Pending chunks have absolute sample
cursors; restore discards chunks beyond the saved cursor. The device is never
flushed by a rollback. Playback trails simulation by eight fields (about 134 ms)
and uses a 60 ms device lead. This is an intentional sound-latency tradeoff,
roughly 194 ms before host/device latency, rather than speculative audible sound.
The offline device queue remains unchanged. A stalled/disconnected network or
severely overloaded machine can still run the device empty; there is no claim
that arbitrary outages can be hidden. Physical speakers were unavailable.

The first paced matrix exposed the offline host's 75 ms discontinuity policy
clearing otherwise correct confirmed PCM during catch-up. The final host path
retains those samples, uses the larger lead, and forces windowed netplay pacing
even if `--unpaced` was supplied. Intermediate recovery fields are hidden; the
last recovered field is requested even if ordinary frameskip would omit it.

The host also waits half the median acknowledged origin-fragment round trip
after sending Start, and both peers rebase their pacing origin once before the
first guest instruction. This avoids starting the host one network flight ahead
of the guest; it does not assume synchronized computer clocks. Earlier profiling
runs in `work/p416/matrix/` and `work/p416/final-matrix/` exposed that asymmetry;
final measurements use `work/p416/acceptance/` only.

## Reproduction and coverage

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p416/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p416/runtime-tests -j6
work/p416/runtime-tests/runtime_tests
python3 port/p416_verify_netplay.py --window --tag acceptance
python3 port/p416_verify_protocol.py
python3 port/p416_verify_ui.py
python3 port/p416_verify_netplay.py --frames 900 --delay 0 --cases 50:5:20 --tag delay-zero
python3 port/p416_verify_netplay.py --frames 900 --delay 1 --cases 50:1:10 --corruption 10 --tag corrupt
python3 port/p416_verify_regression.py
python3 port/p416_verify_smoke.py
port/p412_build_win.sh
python3 port/p416_verify_patches.py
```

**The sandbox returns `EPERM` from `socket(AF_INET, SOCK_DGRAM)`**, even for
loopback. The impaired matrix therefore runs two actual game processes with
`p416_datagram_shim.c` preloaded: atomic, file-backed datagrams with sender
addresses, packet boundaries, one-way delay, jitter, loss and optional deliberate
CRC damage. The production protocol, input histories, restore/re-simulation,
rendering and audio code execute normally. This is strong protocol/state
coverage but **not a successful kernel-UDP loopback or internet test**. Production
binaries never link/load the shim. No sandbox permissions were bypassed.

On an unrestricted Linux machine, repeat the direct socket case with:

```bash
python3 port/p416_verify_netplay.py --window --transport udp --cases 0:0:0 --tag live-udp
```

For genuine impaired UDP on Linux/macOS/Windows, the portable, test-only Python
proxy needs no extra package. For example, start these in separate terminals:

```bash
port/play.sh --netplay-host 7845 --input-delay 3
python3 port/p416_udp_proxy.py --listen 127.0.0.1:7846 --host 127.0.0.1:7845 --latency 50 --jitter 10 --loss 1
port/play.sh --netplay-join 127.0.0.1:7846 --input-delay 3
```

Use the Mac play script or Windows executable on those platforms. Repeat with
0/50/100/150 ms one-way delay and 0/1/3/5% loss. The proxy is a test utility,
not an implemented internet relay service; it was syntax-checked here but its
socket execution is blocked by the same sandbox policy.

The matrix records 9,000 original gameplay-driven physical input fields once
(P4.13 input recording), then converts them into **1,757 fixed input events**.
P1's recorded combat decisions and P2's independently phased attacks, movement,
coins and continues are thus deterministic input scripts on both peers, with
no autoplay or training active during netplay. Both players join and the run
crosses combat scenes, game-over/continue and a new game. Each case compares
151 confirmed checkpoints (field 0, every 60 fields, final 9000), the final
PNG, every PCM sample and the entire final 22-part machine payload. Deliberate
state corruption uses an existing developer diagnostic only to exercise the
fail-stop desync path; it is not a supported gameplay option.

Additional evidence: `work/p416/protocol/report.json` rejects mismatched delays,
stops both peers on an injected field-60 desync, and proves that differing local
difficulty/lives INIs converge to the host origin. `work/p416/delay-zero/` tests
zero delay with 50 ms / 5% loss / 20 ms jitter. `work/p416/corrupt/` tests delay 1
and 10% CRC-damaged packets in addition to latency/loss/jitter. Delay 2 is covered
by the SDL key-lock/overlay test. The latter injects actual F4/F5/F7/Backspace/
Escape events using `port/p416_ui_driver.cpp`; both sessions reach field 300,
create no quick state and never enter fast-forward. Visually inspected captures:
`work/p416/ui/launcher.png`, `launcher-join.png`, `host/overlay.png`,
`guest/overlay.png`. The source lives in `port/`; its Linux-only test preload is
not included in any application build.

A final negative-test repeat caught a reporting race after moving receive to the
panel boundary: a peer could detect a desync before publishing its own checksum,
leaving the other peer to time out. The final error path sends the stop reason
immediately and repeats it for 300 ms before closing. Both endpoints then name
the desync. This changes failure notification only; it does not change successful
simulation, input prediction or audio. The long four-case measurement completes
before rebuilding this isolated error-path fix; a fresh successful-session and
negative-test repeat is recorded separately below.

## Final measurements (Linux; Mac acceptance remains open)

Threadripper PRO 5975WX, SDL dummy video/audio, software renderer, 1x, paced,
input delay 3, 9,000 fields per peer. CPU is process user+system / wall;
100% means one logical CPU, including render workers. The host is shared;
frequency/load variation prevents treating these as isolated Mac timings.
The injected latency is **one way**, with uniform +/- jitter and independent
packet loss in both directions. Whole-process fields/s includes setup and the
500 ms final checksum retransmission linger. Log timestamps are virtual time;
wall/CPU times come from GNU time. Source: `work/p416/acceptance/report.json`.

| One-way / jitter / loss | Peer | Rollbacks/field | Re-sim fields/field | Max depth | CPU % | Whole fields/s | Steady 5 s range |
|---|---|---:|---:|---:|---:|---:|---|
| 0 / ±0 ms / 0% | host | 0.0000 | 0.0000 | 0 | 83.5 | 59.61 | 59.70–60.78 |
| 0 / ±0 ms / 0% | guest | 0.0000 | 0.0000 | 0 | 84.2 | 59.63 | 59.69–60.74 |
| 50 / ±10 ms / 1% | host | 0.0626 | 0.0674 | 2 | 100.0 | 59.47 | 59.60–60.72 |
| 50 / ±10 ms / 1% | guest | 0.1024 | 0.1298 | 3 | 105.4 | 59.49 | 59.72–60.73 |
| 100 / ±20 ms / 3% | host | 0.1696 | 0.7484 | 8 | 112.4 | 59.33 | 59.29–60.95 |
| 100 / ±20 ms / 3% | guest | 0.1512 | 0.4030 | 8 | 105.8 | 59.34 | 59.33–60.80 |
| 150 / ±30 ms / 5% | host | 0.1627 | 0.9120 | 8 | 91.7 | 54.15 | 49.75–60.73 |
| 150 / ±30 ms / 5% | guest | 0.1576 | 0.8320 | 8 | 88.9 | 54.18 | 49.88–60.74 |

| Link / peer | Save ms | Load ms | Recovery ms/re-sim field | Largest recovery burst ms | Rollbacks / re-sim fields |
|---|---:|---:|---:|---:|---|
| 0 ms / host | 0.806 | 0.000 | 0.000 | 0.000 | 0 / 0 |
| 0 ms / guest | 0.817 | 0.000 | 0.000 | 0.000 | 0 / 0 |
| 50 ms / host | 0.839 | 0.904 | 8.173 | 21.798 | 563 / 607 |
| 50 ms / guest | 0.836 | 0.883 | 8.859 | 24.504 | 922 / 1168 |
| 100 ms / host | 1.120 | 2.192 | 6.464 | 62.983 | 1526 / 6736 |
| 100 ms / guest | 0.849 | 1.131 | 6.539 | 55.574 | 1361 / 3627 |
| 150 ms / host | 1.128 | 1.985 | 5.357 | 57.765 | 1464 / 8208 |
| 150 ms / guest | 0.891 | 1.675 | 5.021 | 51.183 | 1418 / 7488 |

Save/load columns are measured wall time per operation. Recovery includes
unwind/restore, hidden-field emulation/snapshot capture and the final corrected
presentation; it excludes deliberate pacing waits. Its per-field number is
aggregate recovery time divided by corrected fields, not an exclusive CPU
component. Do not add the columns as independent costs. Maxima include OS
descheduling and shared-host contention. Snapshot cost is paid every ordinary
field; repeated-field captures are counted in the retained logs.

All four pairs pass **151/151 confirmed checkpoints**, final PNG equality,
**6,626,841 stereo PCM frames** / 26,507,408-byte WAV equality and every byte
of the **4,799,097-byte full final machine payload**. All cases also share the
same final PNG and WAV across impairment levels. Whole-state hashes may differ
between cases because each host chooses a new RTC origin; each pair is identical.
**Zero desyncs, zero audio empty/drop/resync events on all eight final peers.**

Final image SHA256: `e335eaa3ffe07bdf5af5c8c10f3bf0166c81278b2b1e4d1277a49bc45a1cb46c`. PCM/WAV SHA256: `f95a5f785f2d2f95fe040227ef40f55d5608988fb17f6c3d702c43d499cd1cd1`.

## Window/delay decision and the Mac budget

Use **delay 3** for internet play. The **eight-field** maximum is chosen to cover
roughly 134 ms of prediction (with the local delay providing another 50 ms)
and to bound memory/recovery. It waits instead of predicting farther. At
50 ms one-way / 1% loss, observed corrections are 1–3 fields; the normal
60-field/s cadence holds. At 100 ms one-way, the cap is reached under loss/jitter
but this Linux run generally maintains cadence using picture omission. At
150 ms one-way / 5% loss, waiting and recovery reduce throughput to about
54 fields/s; that connection quality is not recommended for comfortable play.

**An eight-field recovery cannot be promised inside one 16.7 ms field.** Largest
measured bursts reach **62.983 ms**; even the 50 ms link has **21.798–24.504 ms**
bursts. Ordinary snapshot copying costs **0.806–1.128 ms** and nonzero restore
averages **0.883–2.192 ms**, before the corrected emulation/presentation work.
Frame skipping helps catch up; it does not make those bursts meet the budget.
Thus the requested strict rollback frame-budget acceptance is **unmet**, rather
than silently asserted from average fields/s. The window remains a network
safety bound; shortening it trades fewer corrected fields for more waiting.

The supplied P4.15 Mac i9-9880H measurements are the only Mac numbers available:
**59.69 fields/s, 90% CPU** in a real 1x paced window; **95.54 fields/s** unpaced
(**93.49** mean combat). The latter is about **10.47 ms/field** of critical-path
wall time; the paced CPU figure is about **15.04 aggregate CPU ms/field** at the
16.715 ms raster. These are different quantities. Neither is a P4.16 benchmark.
A copy/restore cost measured on Threadripper cannot establish the remaining
Mac budget. The Mac must measure actual captures, recoveries and speaker audio
with a real SDL window and a reachable peer before this can be called accepted
for the owner's Mac. No macOS build or native Windows execution is claimed here.

## Build, offline regressions and remaining acceptance

- `port/p46_build_window.sh`: passes (`work/p416/build.log`).
- Native runtime suite: **164 tests, 0 failed** (`work/p416/final-tests.log`).
- All **56 reference PNGs, four full/guest traces and four WAVs** remain
  byte-identical, including the 9,000-field no-coin attract cycle and the
  game-over/new-game route (`work/p416/regression/manifest.json`).
- Shot 1900 smoke MD5s: 1x **`9bd5386ea7439621c19674381086e8fd`**;
  2x **`8f1d9a1f01a2b77cd9d4f82b22b71b37`**;
  3x `a48d6414e53a778c69cfe4fbe71ba423` (`work/p416/smoke-md5.json`).
- Windows app cross-build and refreshed ZIP pass using only the cached
  toolchain/SDL archives (`work/p416/windows-build.log`). Runtime tests also
  cross-build (`work/win/p416-tests/runtime_tests.exe`); they are **not executed**.
- Runtime P4.13–P4.16 and Windows staging reconstruct **82 / 134 files** byte
  for byte, **zero fuzz/offset** (`work/p416/patch-validation.log`).
- Existing macOS build/app wrappers remain consistent: runtime CMake includes
  the new portable source, and Bash syntax checks pass. No SDK was downloaded.
- Owner package instructions now include Netplay, CLI flags and port forwarding.
  ZIP integrity/import inventory is checked; no ROM, state or input file is
  packaged. Only the existing SDL3/libc++/libunwind DLLs are needed.

**PM checks:** real UDP loopback with the portable proxy, a real internet pair
and router/firewall forwarding, a native Mac build/budget/speaker test, Windows
peer tests under Wine outside the sandbox, then a real Windows/controller test.
Wine is explicitly left to the PM: its listener sockets are sandbox-blocked.
Cross-platform handshake fingerprints match, but **cross-OS live gameplay has
not been tested**. No automatic full-state resync, relay or NAT traversal is
implemented. Existing HLE BIOS/EEPROM and fidelity limitations are unchanged.

All changes stay in the requested port/runtime/notes/work scope, with no
convergence-tree or P4.17 files, no new runtime dependencies/downloads, sudo,
GPU use, unrelated process termination or git commit. Pre-existing unrelated
working-tree edits were preserved.

## Final error-path repeat and package identity

The final executable repeats the 50 ms / 1% loss / ±10 ms jitter case for
another **9,000 fields on both peers** after the stop-notification change:
**151 checkpoint pairs, full final payload, PNG and PCM all match**, with zero
audio empty/drop/resync events. Host/guest rollbacks are **1027 / 582**, maximum
**3 / 2 fields**, CPU **104.8% / 98.4%**, saves **0.850 / 0.852 ms**, loads
**0.915 / 0.905 ms**, largest recovery bursts **27.259 / 21.060 ms**.
Evidence: `work/p416/post-error-fix/report.json` and both peer logs.
Fresh protocol, SDL key-lock, delay-zero, CRC-corruption, runtime tests, offline
regression/smoke, cross-build and patch reconstruction checks also pass on the
final sources. Both desync endpoints now log the desync reason, not a timeout.

Final owner ZIP: `work/win/DieHardArcade-windows-x64.zip`, **5,581,046 bytes**, **16 entries**; SHA256
`0165932fb31d575a1628e9efe6227357d2b9ba02444e2518d9ed6d1850a5628d`.
Linux/Windows portable handshake code fingerprint is **`387e6bfac0b0d73c`**;
the exact-build quick-state fingerprints deliberately differ by compiler/OS.
Inventory and fingerprints: `work/p416/artifact-hashes.json`.

## Ten-line summary

1. Implemented P4.16 two-player input-only UDP rollback, with a pre-code design retained above.
2. Host/join use port 7845 by default; both players use local P1 controls; the guest drives P2.
3. Matching code/ROMs, host power-on BIOS/NVRAM/RTC origin and confirmed CRCs protect determinism.
4. Delay 0–3 and an eight-field rollback cap use existing complete continuation snapshots.
5. Confirmed PCM, eight-field buffering and a 60 ms device lead avoid rollback queue resets.
6. Four 9,000-field impairment pairs match all checkpoints, full final state, PNG and PCM; no desyncs.
7. Final error-notification repeat also passes 9,000 fields; all eight matrix peers have zero audio glitches.
8. All 164 runtime tests, 56 reference PNGs/four traces/four WAVs and 1x/2x smoke hashes pass.
9. Linux and Windows cross-build/package pass; real UDP, Mac timing/speakers and Wine peers remain PM checks.
10. Strict 16.7 ms rollback bursts remain unmet; no NAT traversal/relay, P4.17 files, convergence edits or commit.

## PM addendum (2026-11-09): real kernel-UDP and cross-platform runs

Run outside the Codex sandbox on the Linux host, same 9,000-field two-player script (work/p416/final-matrix/panels.txt), input delay 3, loopback UDP:

| Host | Guest | Result |
|---|---|---|
| Linux (P4.16 build) | Linux (P4.16 build) | 0 desyncs; final PNG f37fd40b... identical on both and equal to the shim matrix; WAVs identical |
| Linux (P4.16 build) | Windows exe under Wine 11.21 | 0 desyncs; identical final PNG; Wine guest ran ~99 fields/s unpaced, host absorbed 1,091 rollbacks (max 8) |
| Linux AppImage (P4.18) | Windows exe (P4.18) under Wine | 0 desyncs; all 151 checkpoint CRCs identical on both peers; identical final PNG |

The verifier's --transport udp mode fails before running (it recreates its output folder after compiling the shim); the pairs above were run by hand with the same arguments. Internet play across real NATs is still untested.


## macOS verification (P4.16 netplay)

2026-11-09, Intel Mac (i9-9880H, macOS 26.5.1, Apple clang, Homebrew SDL3, built-in Retina).
The Mac copy was reset to `8852d7f` (backup of the previous state in `work/p416mac/backup/`:
untracked `difftest_v104.py`, the P4.15 runtime diff and its untracked files). `p416-netplay.patch`
then `p417-linux-release.patch` applied to the Mac's P4.15 runtime with zero fuzz/offset; the
resulting `runtime/` tree is content-identical to P620's at that time (123 files, md5 per file),
and the generated recompile matches P620's except the absolute runtime path in `CMakeLists.txt`.
No macOS-specific fix was needed. Evidence: `work/p416mac/` (Mac copy) and `work/p416mac/` on P620.

| Check | Result |
|---|---|
| `port/p48_build_mac.sh`, `port/p411_build_app.sh` | Both build, zero warnings |
| Runtime unit tests (clang, `SATURN_NO_SDL`) | **164 tests, 0 failed** |
| Shot 1900, 2000 fields, dummy video/audio, 1x / 2x / 3x | **`9bd5386e...86e8fd` / `8f1d9a1f...b71b37` / `a48d6414...e53a778c`**, P4.8 binary and bundled app binary, 0 FATAL |
| Netplay code fingerprint (Mac build) | `387e6bfac0b0d73c` = the documented Linux/Windows P4.16 value |

App: `port/run/P411/Die Hard Arcade.app` is **v1.05** (Info.plist 1.05/105, startup log string
`Die Hard Arcade 1.05`, ad-hoc signature verifies). v1.04 kept as `Die Hard Arcade v1.04.app`.
The bump is a local, uncommitted edit of `port/p411_app_mac.c` / `port/p411_bundle_mac.py`.

### Mac <-> Linux over the real network

Same 9,000-field two-player script on both (`work/pmnet/panels.txt`, md5 `efcbeb1f...`), headless,
SDL dummy video/audio, `--input-delay 3 --stvinput @panels.txt --shots 9000`. ICMP RTT Mac->P620:
LAN 0.43 ms avg, Tailscale 1.09 ms avg. Headless runs are unpaced, so the faster Linux peer runs
ahead of the Mac (~124 fields/s) and absorbs the rollbacks; these counts are not a paced-play figure.

| Host | Guest | Link | Result |
|---|---|---|---|
| Linux P4.16 build (`stv416`) | Mac P4.8 binary | LAN | **0 desyncs**, `shot-9000.png` **f37fd40b5f2e350045c3ebec6ab6c481** on both, **151/151 `confirmed field=N CRC=...` lines identical** (CRC and scene). Handshake RTT 2 ms. Host 1075 rollbacks / 4532 re-sim, max 8, max burst 62.3 ms; Mac 2 rollbacks, max 2, max burst 15.3 ms, save 1.11 ms, load 2.0 ms |
| Linux P4.18 AppImage (`APPIMAGE_EXTRACT_AND_RUN=1`) | Mac P4.8 binary | Tailscale | **0 desyncs**, f37fd40b... on both, **151/151 CRCs identical**. Handshake RTT 4 ms. Host 804 rollbacks, max 8, burst 70.4 ms; Mac 7 rollbacks, max 2, burst 17.9 ms |
| Mac | Linux | LAN | **Blocked by the macOS Application Firewall** (see below); both peers time out after 60 s |
| Mac (host) | Mac (guest) | loopback | **0 desyncs**, f37fd40b... both, 151/151 CRCs identical; 12 rollbacks max 2, burst 18.9 ms, save 1.29 ms, load 3.4 ms (exercises the macOS host path) |

Cross-OS determinism therefore holds Mac <-> Linux in both builds (P4.16 binary and P4.18 AppImage).

### Paced Metal window with the netplay overlay

Mac guest in a real 960x720 Metal window (1920x1440 backbuffer) with the real audio device, Linux
headless host over LAN, 1,800 fields: **59.7-60.8 VBlanks/s** in every 5 s sample except one
58.55 (the sample containing the full-size PNG capture), **0 pictures omitted, audio 0 empty /
0 dropped / 0 resyncs**, queue 5-94 ms, **96% CPU** (one logical CPU = 100%). Mac: 0 rollbacks,
save 1.33 ms. 31/31 CRCs identical, `shot-1800.png` `23c15b3c18019b59a0f9263cce21bc7b` on both.
`SATURN_WINDOW_SHOT` at VB 1500 (`work/p416mac/net/macguest-paced/overlay-1500.png`, inspected):
the HUD reads `NETPLAY P2 guest | ping 12 ms | rollback 0 / max 0f` at top left over combat; the
picture is otherwise normal. The HUD ping includes scheduling (the unpaced headless host spent most
of its time waiting: 8,209 waits, 290 rollbacks max 8), so 12 ms is not the wire RTT.
Speakers were not listened to by a person.

### Launcher Netplay tab

`SATURN_NETPLAY_SHOT=1 SATURN_LAUNCHER_SHOT=...` on the v1.05 app binary, real Metal window
(`work/p416mac/ui/launcher-netplay-{host,join}.png`, inspected): Netplay tab fills the window;
Host shows Connection / UDP port 7845 with -/+ / Input delay 3 fields; Join shows the Host IPv4:port
field prefilled from `--netplay-join`; both help paragraphs visible, nothing clipped.

### macOS firewall (owner action to host from a Mac)

The Mac's Application Firewall is on with stealth mode. Outbound UDP works (Mac as guest needs
nothing). Incoming UDP to the unlisted, ad-hoc-signed binary is silently dropped, so **the Mac
cannot host until the owner allows it**. The network path itself is fine: a firewall-allowed
`/usr/bin/python3` listener on the Mac received P620's datagrams on LAN and Tailscale addresses.
To allow it: System Settings > Network > Firewall > Options... > "+" > choose
`Die Hard Arcade.app` > "Allow incoming connections" (or click Allow if macOS asks when hosting), or
`sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add "<path>/Die Hard Arcade.app"` then
`--unblockapp` with the same path. Because the app is ad-hoc signed, a rebuilt app may need the
rule re-added. Internet hosting additionally needs router UDP forwarding as already documented.

### Issues / notes

- P620's `port/run/P46/build-sdl/saturn_stv` was rebuilt at 13:11 by concurrent uncommitted P4.19
  work (speculative audio); its code fingerprint became `c3178a877a2cd814`, and the Mac's P4.16
  build was correctly refused with `protocol/source, ROM, delay or frame-limit mismatch`. Tests used
  copies of the PM's P4.16 binary (`work/pmnet/stv416`, the 12:50 committed build) and the P4.18
  AppImage. The refusal text does not say which of code/ROM/delay/frame limit differed; naming it
  (and printing both code fingerprints) would make cross-build mismatches obvious.
- `confirmed field=N ... scene=S` reads the scene from live (possibly up to eight fields ahead)
  state, not the confirmed snapshot: in the AppImage run field 4620 logged scene 5 on the host and
  4 on the Mac with identical CRCs. Cosmetic; compare CRCs only, or log the snapshot's scene.
- Headless logs carry only the handshake RTT; live ping appears only in the HUD.
- The HUD text is small on Retina (logical-size glyphs at top left), consistent with P4.15's note.
