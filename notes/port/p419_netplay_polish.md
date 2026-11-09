# P4.19 — netplay polish

2026-11-09, base `8852d7f`. This task made no commit. Pre-existing PM-board/decomp edits
are outside this milestone. Work proceeded in audio, traversal, verifier order. Other sessions later
advanced shared HEAD to `349c637` (P4.20/P4.21 prototypes); those changes were
preserved and are excluded from this release path.

## 1. Speculative audio

The default is **Netplay audio: Low latency**; **Glitch-free** retains P4.16's
confirmed-only eight-field lead. The setting is saved in `[netplay] glitch_free`;
CLI `--netplay-audio low-latency|glitch-free` overrides it.

Low latency sends every newly generated PCM poll to an absolute-sample host
queue. SDL's pull callback takes only the requested device block. Until that
callback consumes a sample, rollback may replace it. Submitted samples cannot
be retracted. Re-simulation compares canonical old/new samples, replaces only
unsubmitted samples, and blends old playback into corrected playback over 132
stereo frames (2.99 ms), continuously across small sound polls. An independent
mutex protects producer and callback. Rollback neither restarts nor clears SDL.
One second of submitted history is retained for difference measurements.

Startup uses the same 30 ms silence as offline plus the ordinary device block
latency; the simulation/confirmation delay is zero fields. `--wav` still receives
only the confirmed stream through the unchanged eight-field confirmation path.
`STV_NETPLAY_PLAYED=path.s16` records the actual pre-gain PCM submitted to SDL,
little-endian stereo 44.1 kHz; the audio verifier also wraps it as `played.wav`.
This is the device-input stream, not a speaker or DAC capture. Corrections,
changed submitted/unsubmitted frames, empty callback blocks, queue occupancy
and large adjacent-sample steps are reported separately. Signal discontinuities
are measurable candidates; physical audibility requires a listening test.

The validation below was completed before traversal implementation.

Audio verification: ordinary SDL build and isolated Windows cross-build /
ZIP pass; all **169 runtime tests** pass, including five audio tests also run
under ASan/UBSan. The historical offline oracle passes **56 PNGs, four full and
four guest traces, four complete WAVs**, including 9,000-field no-coin attract.
Shot-1900 MD5: 1x `9bd5386ea7439621c19674381086e8fd`, 2x
`8f1d9a1f01a2b77cd9d4f82b22b71b37`.

The fixed-RTC 9,000-field P4.18 baseline and speculative build match **151
checkpoints, entire final payload, PNG and confirmed WAV**, not merely each
other's endpoints (`work/p419/audio/baseline-equality.json`). The test-only
`p419_audio_datagram.c` wraps the existing file shim and fixes `time()` for
that cross-build comparison; production clocks are unchanged.

The first final zero-impairment measurement reports mean generation-to-SDL
submission **29.45 ms**, maximum 78.85 ms, plus the **23.22 ms** dummy device
block: approximately **52.67 ms** mean pre-hardware latency, zero confirmation
fields and **zero empty blocks**. The same offline paced script reports SDL
queue ranges around 9–44 ms plus the same device block. This measures the
ordinary device lead, replacing the previous ~194 ms confirmation/device lead.
It does not measure a physical DAC/speaker. Full impairment acceptance follows.

Initial audio acceptance completed before traversal implementation; the table
below is the final frozen-build repeat. Four **9,000-field**
pairs match the fixed-RTC P4.18 baseline at all **151 checkpoints**, final full
payload, PNG and confirmed WAV (`port/p419_audio_results.py`). The final parallel
matrix deliberately runs all four pairs together on this shared host; its
scheduling/underrun figures are stress observations, not isolated performance.
Values below are host / guest; latency is generation-to-SDL submission, before
the normal 23.22 ms device block.

| One-way / loss / jitter | Mean submission ms | PCM correction events | Changed unsubmitted frames | Changed submitted frames | Large signal steps | Empty callback blocks |
|---|---:|---:|---:|---:|---:|---:|
| 0 / 0% / 0 | 32.44 / 32.23 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 2 |
| 50 / 1% / ±10 | 31.33 / 32.25 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 9 / 16 |
| 100 / 3% / ±20 | 43.83 / 56.54 | 1 / 1 | 104 / 592 | 59 / 0 | 0 / 0 | 368 / 417 |
| 150 / 5% / ±30 | 38.11 / 36.42 | 2 / 3 | 571 / 725 | 1635 / 2380 | 0 / 0 | 1867 / 1931 |

A large signal step means adjacent PCM frames differ by >16384 in either
channel. **Audible seam count is unmeasured** (no speakers); zero threshold
crossings does not prove inaudibility. Empty blocks are reported honestly:
low-latency audio cannot conceal prolonged network/prediction-cap stalls or
host overload. No dropped chunks or SDL queue resyncs occur. The final frozen build produces **seven correction events** across the
100/150 ms peers: **1,992 unsubmitted stereo frames corrected**, **4,074
submitted differences retained**, and **zero large signal steps**. The normal
zero-impairment mean is 32.44 / 32.23 ms to SDL submission plus the 23.22 ms
device block: **55.66 / 55.45 ms**, in the ordinary offline lead range and
without an eight-field confirmation delay. Impaired/overloaded runs retain the
reported stalls/backlog; no constant-latency promise is made under them.
The **seven** audio unit tests cover partial consumption, cross-poll fades,
unchanged replay, submitted-only differences, bounded history/latency,
deferring the full fade past an unchanged gap, and preserving a pending fade
across an identical second rollback. Glitch-free additionally passes a
900-field 100 ms / 3% / ±20 pair. Serial evidence for 0/50/100 ms is retained under
`work/p419/audio/final-matrix`; its later 150 ms attempt was discarded after
a rebuild changed the executable between cases. The complete four-case
acceptance above used one frozen build. Earlier serial four-case evidence
is also retained under `work/p419/audio/matrix`. Reproduce:

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p419/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p419/runtime-tests -j6
work/p419/runtime-tests/runtime_tests
python3 port/p419_verify_audio.py --parallel --window --frames 9000 --tag release-acceptance
python3 port/p419_audio_results.py --tag release-acceptance
python3 port/p419_verify_audio.py --audio glitch-free --window --frames 900 --cases 100:3:20 --tag glitch-free
```

Evidence: `work/p419/audio/measurements.json`, `baseline-equality.json`,
`regression/manifest.json`, `smoke-md5.json`, `tests.log`, `sanitized.log`,
`build.log`, `build-windows.log`, and each peer's separate `played.wav`.

The final CLI audit caught and fixed an override being assigned before the
harness resets `SaturnConfig`. Both CLI audio choices now survive that reset,
and headless settings load the saved preference too. The repeated Glitch-free
run explicitly asserts that the speculative callback/report is absent; it
passes (`work/p419/audio/glitch-free`). Low-latency matrix behavior is unchanged.

## 2. Router mappings and connection codes

The runtime delta is `p419-nat-traversal.patch`, applied after the audio patch.
All sockets are IPv4, nonblocking POSIX / Winsock; no third-party library was
added. Windows uses its system IP Helper API (`iphlpapi`) for adapter/default
route discovery; Linux reads `/proc/net/route`, macOS uses routing `sysctl`,
and POSIX LAN addresses come from `getifaddrs`. The LAN candidate prefers the
interface sharing the default gateway subnet. DNS runs in an owned-data
background resolver with a bounded caller wait; a slow system resolver cannot
hold the launcher/exit path open.

Mapping tries PCP MAP, NAT-PMP public-address/UDP-map, then SSDP M-SEARCH and
UPnP IGD WANIPConnection (v1/v2) or WANPPPConnection v1. SOAP uses
GetExternalIPAddress, AddPortMapping and DeletePortMapping. Only a renewable
**600-second UDP lease** is requested; permanent-only UPnP routers fail
without creating a permanent rule. The granted lifetime controls renewal at
half life. Mapping I/O uses a separate socket, preserving gameplay receive
ownership. Normal exit, launcher cancellation and connection failure remove
the lease; a crash is bounded by its expiry. Renewal failures/endpoint changes
are surfaced as status. Private/local HTTP destinations, bounded messages,
strict port/nonce checks and bounded nonblocking I/O constrain discovery.

STUN uses RFC 5389 Binding requests from the **actual game socket**. The default
servers are `stun.l.google.com:19302,stun.cloudflare.com:3478`; both may be
changed in Connection settings / `[netplay] stun_servers`, or disabled with
`stun = false`. Responses must match the transaction and endpoint and decode
IPv4 XOR-MAPPED-ADDRESS (MAPPED-ADDRESS fallback); supplied FINGERPRINTs are
checked. Different public ports/addresses at the two servers trigger the
symmetric-NAT / forwarding message; a single response explicitly leaves NAT
type unknown. Matching mappings do not establish that every NAT permits
punching. While codes are displayed, 15-second STUN keepalives preserve the
binding. Socket ownership is serialized before punching/gameplay; a changed
public binding requires fresh codes.

A `DHA1-` code is unpadded base64url, at most **64 characters**, containing a
version, up to four IPv4/port candidates (LAN, mapped port, STUN results),
64-bit random session nonce, 32-bit source-fingerprint suffix and CRC32. It
contains no save state or ROM data. Both peers send 29-byte probes to all
candidates every 100 ms. Probes/replies validate both exchanged nonces,
fingerprint and CRC; the first valid reply selects the peer endpoint. Replies
continue during the following ordinary P4.16 handshake to allow the other peer
to finish selection. The lower nonce becomes P1/host; both derive the same
session identity. The original full source/ROM/input-delay handshake remains
authoritative. Codes are neither encryption nor a relay. Fresh code exchange
is required after refreshing or restarting the app.

Player flow:

1. **Host (player 1):** choose the UDP port (default 7845). Automatic mapping
   starts in the launcher. Copy the external endpoint on success, or share the
   displayed LAN endpoint on the same network. Friend selects Join by IP.
   If mapping fails, the status explains that direct internet hosting may need
   UDP forwarding. Both press Start.
2. **Join by IP (player 2):** enter/paste numeric IPv4:port, then press Start.
   This keeps the existing direct connection route.
3. **Connect by code:** select a local port (0 requests an ephemeral port),
   Generate / refresh my code, Copy my code and send it to the other player
   through your usual communication channel. Both paste the other's code with
   Paste code, then both press Start. Start stays disabled until the peer code
   is valid and matches the build; own-code/build errors are explained. Player
   roles are assigned automatically. Symmetric NAT warns that internet play
   needs forwarding; LAN or explicit mapped-port candidates may still work.

Both still use local P1 controls, and the guest drives cabinet P2. Source,
ROMs and input delay must match; host settings, 1x and the existing feature
locks apply. The separate macOS build from `8852d7f` predates this milestone;
a matching P4.19 macOS build is needed for its new fingerprint/features.

CLI equivalents:

```bash
port/play.sh --netplay-host 7845
port/play.sh --netplay-join 203.0.113.10:7845
# Code peers: exchange the printed DHA1- strings; ports may differ.
port/play.sh --netplay-code 7845 --netplay-peer-code DHA1-PEERS_CODE
# --netplay-code-out FILE and --netplay-peer-code @FILE support automation.
# --netplay-no-map / --netplay-no-stun disable those discovery mechanisms.
# --netplay-stun-servers HOST:PORT,HOST:PORT overrides the defaults.
```

Protocol references: [RFC 5389 STUN](https://www.rfc-editor.org/rfc/rfc5389),
[RFC 5769 test vectors](https://www.rfc-editor.org/rfc/rfc5769),
[RFC 6886 NAT-PMP](https://www.rfc-editor.org/rfc/rfc6886),
[RFC 6887 PCP](https://www.rfc-editor.org/rfc/rfc6887),
[UPnP WANIPConnection service](https://upnp.org/specs/gw/UPnP-gw-WANIPConnection-v1-Service.pdf).

Validation uses **181 runtime tests**, including ten traversal tests and the
published RFC 5769 IPv4 vector. Wrong transaction/type/length/family/fingerprint,
all STUN truncations, NAT-PMP errors/deletion, PCP nonce/port layout, malformed
URLs, namespaced IGD descriptions, code corruption/limits and punch validation
are covered. The ten traversal tests also pass ASan/UBSan. Injected local
responders exercise mapping/fallback/renew/delete and destination-dependent
STUN results without requiring sockets.

`port/p419_verify_traversal.py` additionally runs the **production socket I/O**
against separate local STUN, PCP, NAT-PMP, SSDP and SOAP responders. Because the
sandbox denies sockets, `p419_datagram_shim.c` extends the P4.16 test transport
with multiple file-backed descriptors, getsockname, and bounded HTTP request /
response files. Both Content-Length and chunked HTTP pass. A two-second granted
PCP lease proves timed renewal by the asynchronous maintainer and removal on
exit. The complete code-based pair uses two STUN responders, exchanges fresh
codes, punches, runs the normal handshake and compares confirmed checksums,
full final payload, PNG and WAV for 900 fields. A direct-IP pair additionally
uses automatic PCP mapping and checks that exit removes it. None of the test
shims or probes is included in the shipped app.

`port/p419_verify_ui.py` injects actual SDL mouse events into the 960×720
launcher, generates a code, copies it via SDL clipboard and pastes a valid peer
code, enabling Start. Inspected `work/p419/traversal/ui/{host,join,code}.png`:
all controls/status fit, code fields and copy/paste buttons are readable.
Windows cross-build/package succeeds; its new `IPHLPAPI.DLL` import is an OS
library, not an extra bundled dependency. Real routers, real public STUN/NAT,
physical speakers and execution on Windows/macOS remain untested here.

Traversal acceptance completed before verifier repair:
`work/p419/traversal/final-acceptance/report.json` passes all six network fixtures,
the timed lease lifecycle, **900-field code and direct-IP game pairs**, with
**16 matching checkpoints per pair**, full final machine bytes, PNG and WAV.
The direct pair verifies an automatic PCP rule is created and deleted. Final
runtime tests are **181/0**; sanitized codec tests are **10/0**. Logs and
inspected screenshots are under `work/p419/traversal/`. The final settings
check gives explicit CLI STUN servers precedence over saved settings, including
when the CLI explicitly chooses the default servers. Short final repeats are
retained separately in `final-repeat/`.

```bash
python3 port/p419_verify_traversal.py --tag final-acceptance
python3 port/p419_verify_ui.py
# PM: identical fixtures over real kernel UDP and localhost HTTP:
python3 port/p419_verify_traversal.py --transport udp --tag real-sockets
```

Real router firmware, public STUN DNS/replies, WAN hole punching, symmetric NAT
on actual routers and CGNAT cannot be exercised here. Local fixtures establish
wire/parser/lifecycle/protocol behavior, not successful internet traversal.


The final code-mode audit gates origin transfer and Ready/Start on a validated
full Hello, even though punching has already selected/locked the endpoint.
This prevents code mode from bypassing full source/ROM/delay validation.
An added **code-mode delay-mismatch** pair rejects both peers; normal code and
direct pairs still pass. Evidence: `work/p419/traversal/final-acceptance/` and
the actual AppImage repeat in `work/p419/traversal/release-appimage/`.

## 3. UDP verifier repair

`p416_verify_netplay.py` creates its output root once before artifacts, compiles
and loads a shim **only for file transports**, and never invokes a C compiler
or relies on a shim/output recreation for `--transport udp`. Zero impairment
joins the host directly over kernel loopback UDP. Nonzero latency/loss/jitter/
corruption starts `p416_udp_proxy.py`, waits for readiness and points the guest
at it; both directions receive the selected impairments. Each verifier child
owns a process group; cleanup terminates only its own children/proxy. Reports
include proxy counters, which are written on SIGTERM as well as Ctrl-C. The
proxy now supports CRC damage in addition to delay/jitter/loss.

`--transport files-proxy` exercises that same proxy routing and subprocess
lifecycle through the explicit file-backed socket adapter. Shim impairments
are disabled in that mode, so all impairment comes from the actual Python
proxy. It is labelled **test-only file-backed sockets through UDP proxy**,
never kernel UDP. File sockets now support Python's SOCK_CLOEXEC, nonblocking
ioctl and UDP select readiness in addition to the C++ discovery path.
The old protocol and SDL key-lock verifiers use the current adapter and
explicitly disable discovery for their direct loopback tests.

Verification: direct file transport **0 / 100 ms** pairs and file-backed proxy
**0 / 50 / 100 / 150 ms** pairs each pass **900 fields, 16 checkpoints,
complete final payload, PNG and confirmed WAV**. At 50 ms / 1% / ±10, the proxy
received 4,320 packets, dropped 49 and forwarded 4,266 (five pending on exit).
Other cases and packet counters are in `work/p419/verifier-proxy-impaired/`;
the direct zero-proxy case is in `verifier-proxy/0ms-0pct`. Delay mismatch,
injected desync notification, host-origin override and active-session key
locks still pass (`work/p419/protocol.log`, `ui-locks.log`). Kernel UDP probe
returns **EPERM**, logged in `work/p419/udp-blocked.log`; no real socket or
internet success is claimed.

The final played-stream recorder includes the initial 30 ms priming silence
in addition to callback PCM. Use `audio/release-acceptance/**/played.wav` for
the final complete captures and fade implementation. Early recordings omit
the priming prefix; all versions keep the same confirmed WAV/guest state.


## Final builds, regressions and release checks

All three requested fixes are implemented. Real sockets remain an explicit
acceptance gap imposed by the sandbox. Earlier P4.16 rollback burst/performance
limits remain: this work does not establish a strict 16.7 ms recovery budget.
The audio latency figures measure PCM generation to submission/device lead;
the chosen input delay remains a separate control-response cost.

| Check | Result / evidence |
|---|---|
| Ordinary SDL build | PASS; `work/p419/build-window.log`. |
| Runtime suite | **181 tests, 0 failed**; `work/p419/runtime-tests.log`. |
| Sanitized audio / traversal tests | **7/0 and 10/0**; `audio/sanitized.log`, `traversal/sanitized.log`. |
| Windows x64 build + refreshed ZIP | PASS; `build-windows.log`, `package-windows.log`. |
| Windows runtime tests | Cross-build PASS (`windows-tests-build.log`); not executed. |
| Linux release + bundled SDL + AppImage/tar | PASS; `build-linux.log`, `package-linux.log`; cached pins only. |
| Runtime / Windows patch reconstruction | **90 / 142 files**, byte-identical, zero fuzz/offset; `patch-check.log`. |
| Ordinary / Linux release / actual AppImage offline regressions | Each matches **56 PNGs, four full/guest trace pairs, four complete WAVs**, including 9,000-field no-coin attract; `regression/{final-window,final-linux,final-appimage}/manifest.json`. |
| Requested shot-1900 MD5s | All three binaries: 1x **9bd5386ea7439621c19674381086e8fd**, 2x **8f1d9a1f01a2b77cd9d4f82b22b71b37**; `smoke/{final-window,final-linux,release-appimage}/report.json`. |
| Final P4.19 vs P4.18 confirmed baseline | **151 checkpoints + full final payload + PNG + confirmed WAV identical**, 9,000 fields; `final-baseline-equality.json` and all four final audio cases. |
| Actual AppImage direct/proxy netplay | **0 / 100 ms**, 900 fields each, all 16 checkpoints/full machine/PNG/WAV equal; `netplay-appimage.log`. |
| Actual AppImage connection setup | STUN/router fixtures, timed lease lifecycle, 300-field code and direct mapped pairs pass; `traversal-appimage.log`. |
| Proxy CRC corruption | 600 fields, delay 1, 50 ms / 1% / ±10 / 10% CRC damage passes; `proxy-corrupt.log`. |
| Packaged payloads | AppImage/tar independently extracted and all entries match AppDir; ZIP integrity passes; no ROMs, states, test shims or probes; `release/report.json`. |
| glibc compatibility | Maximum **2.30** (game 2.29, SDL 2.30); actual **2.31 loader** smoke passes the exact 1x MD5. |

Linux, Windows and Linux release netplay identity is **`4902f4bd0959dcbe`**.
Exact quick-state build identities differ by compiler/platform/release options
as intended. All remain local artifacts; nothing was committed or published.
Package READMEs now include the common Host / code / IP and audio instructions
from `port/p419_netplay_help.txt`. The Windows dependency allowlist recognizes
its new system IP Helper import; bundled DLLs remain SDL3/libc++/libunwind.

Reproduction (all paths relative to the repository root):

```bash
port/p46_build_window.sh
cmake -S tools/saturn-recomp/runtime/tests -B work/p419/runtime-tests -DSATURN_NO_SDL=ON
cmake --build work/p419/runtime-tests -j6
work/p419/runtime-tests/runtime_tests
JOBS=6 port/p412_build_win.sh
JOBS=6 port/p417_build_linux.sh
python3 port/p417_package_appimage.py
python3 port/p419_verify_patches.py
python3 port/p419_verify_regression.py --binary port/run/P46/build-sdl/saturn_stv --tag window
python3 port/p419_verify_regression.py --binary work/p417/build-linux/saturn_stv --tag linux
APPIMAGE_EXTRACT_AND_RUN=1 python3 port/p419_verify_regression.py --binary work/p417/DieHardArcade-x86_64.AppImage --tag appimage
python3 port/p419_verify_smoke.py
python3 port/p419_verify_release.py
```

Refreshed owner artifacts (full audit: `work/p419/artifact-report.json`):

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `work/p417/DieHardArcade-x86_64.AppImage` | 5,421,560 | `034928913c6e7f7c5fa7929c870e9c134fc2ebd9c81b951d9613ec989abad79f` |
| `work/p417/DieHardArcade-x86_64.tar.gz` | 4,951,939 | `4bb2e6ce22cba8ae89af118225679e76466fe197db41499de346a3c2a36641c8` |
| `work/win/DieHardArcade-windows-x64.zip` | 5,633,815 | `15da2efe3dcf9ad3c0b1894da1fca91775febe4e38bca47c8d2b0a4db135b79e` |

## PM real-socket and physical checks

Run on unrestricted **Linux** from this checkout. The verifier uses GNU time;
its `--transport udp` branch neither compiles nor loads the file shim.

```bash
# Original failing mode: two kernel-UDP peers, direct loopback, no proxy.
python3 port/p416_verify_netplay.py --window --transport udp --frames 9000 --cases 0:0:0 --tag ../p419/pm-udp-zero
# Nonzero cases automatically launch/stop the real UDP proxy and record counters.
python3 port/p416_verify_netplay.py --window --transport udp --frames 9000 --cases 50:1:10,100:3:20,150:5:30 --tag ../p419/pm-udp-impaired
python3 port/p416_verify_netplay.py --window --transport udp --frames 900 --delay 1 --cases 50:1:10 --corruption 10 --tag ../p419/pm-udp-corrupt
# Native sockets + localhost HTTP against all local discovery fixtures and game pairs.
python3 port/p419_verify_traversal.py --transport udp --frames 900 --tag pm-real-sockets
# Actual AppImage peers on real UDP.
APPIMAGE_EXTRACT_AND_RUN=1 python3 port/p416_verify_netplay.py --binary work/p417/DieHardArcade-x86_64.AppImage --window --transport udp --frames 9000 --cases 0:0:0,100:3:20 --tag ../p419/pm-appimage-udp
```

Each windowed verifier peer records `played.s16` and `played.wav` separately
from confirmed `audio.wav`, plus correction/latency/discontinuity counters in
its log/report. Listen to the played WAVs and real speakers; no audible-seam
count, DAC latency or speaker acceptance has been established in this sandbox.

For a manual real UDP proxy test, start these three commands in separate
terminals (use `port/play_mac.sh` or the Windows executable on those systems):

```bash
port/play.sh --netplay-host 7845 --input-delay 3 --netplay-no-map
python3 port/p416_udp_proxy.py --listen 127.0.0.1:7846 --host 127.0.0.1:7845 --latency 100 --jitter 20 --loss 3 --report work/p419/pm-proxy.json
port/play.sh --netplay-join 127.0.0.1:7846 --input-delay 3 --netplay-no-map
```

On real routers, first choose Host and verify the displayed external mapping,
its short lease/renewal in the router and its removal on exit. Then use two
internet peers: both Generate, exchange and paste codes, both press Start.
Repeat with mapping disabled, STUN disabled (LAN-only), alternate STUN servers,
ordinary NAT, symmetric NAT and CGNAT where available. Verify the explicit
forwarding warning and direct-IP fallback. Native Mac/Windows peer execution,
real router/firewall/NAT reachability, public STUN, hardware controllers/Deck,
physical audio and cross-OS performance remain PM checks. The previous
P4.16 PM addendum establishes historical loopback/cross-platform results;
it does not establish these new P4.19 traversal or speaker results.

This task issued no git commit, dependency download, sudo, GPU access,
convergence-tree write or unrelated process termination. Pre-existing PM/decomp changes
and independently appearing P4.21 files were left alone; they are absent from
the P4.19 patches, builds and packages.


## Ten-line summary

1. Added default speculative audio with mutable queued PCM and continuous 3 ms rollback fades.
2. Final normal audio lead is about 55.5 ms including the dummy device block, in the offline range.
3. Four 9,000-field pairs retain all 151 baseline checkpoints, full final state, PNG and confirmed WAV.
4. Final impaired audio records seven corrections, 1,992 corrected frames and zero large signal steps; listening remains untested.
5. Added leased PCP/NAT-PMP/UPnP mapping with renewal, external endpoint display and exit cleanup.
6. Added configurable STUN, compact codes, simultaneous punching, symmetric-NAT warnings and Host/IP/code clipboard flows.
7. Repaired kernel-UDP verifier setup and added automatic impaired proxy cases, exercised through explicit file sockets here.
8. All 181 runtime tests, 7/10 sanitized audio/traversal tests, exact offline oracles and zero-offset patch reconstructions pass.
9. Refreshed Windows ZIP and Linux AppImage/tar; glibc 2.31 loader, payload audit and packaged netplay checks pass.
10. Real UDP/NAT/public-STUN/speaker/Mac/Windows execution remain PM checks; this task made no commit.

## PM real-socket addendum (2026-11-09)

Run by the PM on the build host outside the Codex sandbox; log `work/p419/pm-runs.log`.

| Run | Result |
|---|---|
| `p416_verify_netplay.py --window --transport udp --frames 9000 --cases 0:0:0` | PASS (rc 0, 188 s) |
| same, `--cases 50:1:10,100:3:20,150:5:30` | PASS (rc 0, 563 s) |
| same, `--frames 900 --delay 1 --cases 50:1:10 --corruption 10` | PASS, 16 checksums, PNG/PCM/full machine equal |
| `p419_verify_traversal.py --transport udp --frames 900` | PASS: STUN, symmetric refusal, PCP, NAT-PMP, IGD, IGD chunked, lease lifecycle, code pair, direct PCP, code-delay mismatch |
| AppImage `--transport udp --frames 9000 --cases 0:0:0,100:3:20` | FAIL once (rc 1): both peers stalled ~25 s at field 8520 and hit the 15 s peer timeout |

The AppImage failure is attributed to host overload, not netplay: load average was about 87 with both GPUs at 100 % from unrelated jobs, both processes stalled in the same wall-clock window (host 6.47 VBlanks/s over 27.7 s), and all confirmed checksums matched up to the stall. The rebuilt P4.22 AppImage passes the same matrix (see `p422_mods_integration.md`, PM addendum). Open idea: a reconnect/grace option for long local stalls instead of a hard 15 s timeout.
