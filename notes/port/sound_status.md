# Die Hard Arcade (ST-V) — RECOMP native port: SOUND path brought up

Date: 2026-10-22. Work in `dh-portA` (round work is in `dh-portB`). Follows
`render_status.md` (first rendered frame) + `boot_harness_status.md` +
`notes/assets/sound/README.md` (the SCSP/68k driver decode).

**Verdict first: the port now processes sound.** The sound 68000 runs the game's
Sega SCSP driver out of sound RAM, the master's attract-mode sound commands flow
through the ring → Timer-0 ISR → the SCSP command mailbox, the 68000 consumes
them, and **the SCSP keys real voices** — 213 key-ons in 300 VBlanks, each with a
valid sample address, loop points, pitch, envelope and pan. A 1500-VBlank attract
run (25 s) produces a continuous non-silent 44.1 kHz stereo WAV (peak 6219, RMS
836, ~1969 zero-crossings/s = real oscillating audio, not DC/noise), with **zero
fatals** and the slave worker running un-stubbed. Headless, so no live device — but
the SCSP slot state and the captured buffer are the proof the driver actually ran.

The single forced bridge: the harness issues the 68000-start (SMPC SNDON) that the
skipped ST-V BIOS boot would issue. Everything downstream is faithful driver/game
behaviour.

---

## 1. The real sound path (traced, not guessed)

Three processors cooperate:

1. **Master SH-2** (game logic): on a sound event calls `sound_queue_push`
   (`func_02215f6c`) which pushes a 32-bit command `0xA0 tt ii FF` (tt = command
   type, ii = sound id) into a **16-entry ring at `0x060E0DF0`** (write idx
   `0x060E0DEC`). Observed attract commands: `A00200FF` (sound_init bring-up),
   `A01001FF`, `A01207FF`, `A01400FF`, and `A0130BFF` (×102 — the recurring
   attract cue).
2. **Timer-0 ISR** (`isr_timer0_confirm` → `sound_queue_service` `0x02215f36`):
   each timer tick dequeues one ring entry (read idx `0x060E0DE8`) and, **iff the
   mailbox `0x25A03C20 == 0` and the driver-ready flag `g_060E0DE4 == 1`**, writes
   the command to the **SCSP command mailbox `0x25A03C20`** (= 68k work RAM
   `0x3C20`). (`sound_send_command` `0x02215e20` is a *separate* path: the
   sample/data-block upload to `0x25A04000`, with its own `0x3EFE==0xFFFF`
   handshake — not the per-note play path.)
3. **Sound 68000** (Musashi, inside the SCSP): runs the driver copied to sound
   RAM 0 (reset SP `0x4000` @ off 0, reset PC `0x120` @ off 4, build string
   `"Ver3.0.7|96 4/23|Tit&Sat by Hiro"` @ off `0x100`). Its SCSP **timer-interrupt
   handler @ `0x572`** reads the mailbox `$c20(a6)` (`0x3C20`), clears it, and
   calls the command processor `0x21e0`, which allocates one of the 20 voice slots,
   reads the tone/sample tables, and programs an SCSP slot (SA/loop/pitch/EG/pan)
   then KYONEX. Its main loop (`0x18E`–`0x1A2`) also advances the byte-stream
   sequencer (`0x6f4`, reading the track buffer at `0x3800` gated by the counter
   `0x3ECC`).

So a game sound = ring push → Timer-0 ISR → mailbox `0x3C20` → 68000 timer IRQ →
slot key-on. **Every stage is now live.**

## 2. What was actually wrong, and what was fixed

### (1) Slave worker — already fixed by the cartridge mapping; no-op now removed
`boot_harness_status.md` mislabelled the slave SH-2 (entry `0x060051B0`) as the
"sound worker" and saw it `jsr` a null handler out of the table at `0x06090274`.
That null was the **same unmapped-cartridge bug** `render_status.md` fixed: the
table lives inside the loaded cart image, and with the A-bus window floating high
it read `0xFFFFFFFF` → null pointers. With the cartridge mapped the table is real
and **the slave dispatches real handlers — it never hit the no-op once in any run
(0 occurrences across 300/600/1500 VBlanks).** The slave is in fact the game's
general command/VDP1 worker (it drives the `0x060E0DD8` bit-7 ready flag), not the
sound CPU. The scoped `cpu==1` no-op in `sh2_call_unknown` is therefore **removed**
(un-stubbed): the slave now faults on a genuinely-missing recomp target exactly
like the master (seed via `autolearn`), rather than being silently neutralised.
Removal verified: still a clean 600/1500-VBlank run, no fault.

### (2) The 68000 never ran — the real blocker (fixed)
`sound_init` (`0x02215da8`) correctly copies the 68k driver + samples into sound
RAM (hw-log: SCSP-RAM writes `0x05A00000..0x05A7FFFC`), **but the game never issues
SMPC SNDON** on our entry path — across 1500 VBlanks of warm boot and 300 of cold
boot (`STV_BOOTMODE=1`), the only SMPC commands are SSHON + INTBACK, never SNDON
(`0x06`). On real hardware the **ST-V BIOS boot stub issues SNDON to release the
sound 68000 before the game runs**; the game's `sound_init` assumes the 68000 is
already live (it only copies the driver + pokes SCSP ctrl `0x400=0x300`). The
harness skips that BIOS stub (`render_status.md`), so with no SNDON the 68000
stayed held, the driver never executed, and the SH-2↔68000 handshake spun to its
cap (`0x25A03C20..0x3EFE` read 501 162× = one timeout).

**Fix (FORCED, labelled):** the harness now issues the BIOS's SNDON. `sound_tick`
releases the 68000 (`sound_power(true)`) the moment the driver image is present in
sound RAM (detected by the `"Ve..."` build string at off `0x100`, with the reset
SP/PC beside it). This is the one non-faithful step; it stands in for the skipped
BIOS boot exactly as `install_bios_workarea` already stands in for the rest of it.
Gated by `sound_stv_autostart(true)` (ST-V harness only — disc games untouched);
`--no-sound` restores the old render-only behaviour.

### (3) Result — SCSP keys real voices
With the 68000 live, the mailbox handshake completes (reads drop 501k → 97k), and
the command processor keys SCSP slots. `--trace` dump, first key-ons:

```
SCSP key on 0: SA 4FDD2 LSA 0000 LEA 2B7F LP0 16bit OCT -3 FNS 07D TL 2E DISDL 3 ... AR 31 RR 31
SCSP key on 1: SA 154B8 LSA 161F LEA 2EBC LP1 16bit OCT -2 FNS 07D TL 29 DISDL 3 ... AR 29 RR 20
SCSP key on 2: SA 12048 LSA 0000 LEA 159F LP0 16bit OCT -3 FNS 25A TL 1F DISDL 3 ... AR 31 RR 31
...   (213 key-ons / 300 VBlanks; SA within sound RAM, pitch/EG/pan all well-formed)
```

`STV_SND_DIAG=1` live snapshot (4 Hz): 68000 PC sweeps the driver
(`0120→1B8→2280→552→6F4→B18…`), IRQ level 2 firing, `mbox3C20` cycling
`A0130BFF`→`0`, active-slot mask growing `0x0000` → `0x07FF` → spreading.

## 3. Evidence / how to reproduce (p620, `dh-portA/port`)

```
cmake --build recomp-build --target saturn_stv            # g++ build, 0 errors
# traced key-on dump + WAV:
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 300 --trace --wav run/out.wav --out run
#   -> "213 SCSP key on" lines; report "sound: 220341 samples, 199825 not silent; 68000 on"
# live 68000/SCSP snapshot:
STV_SND_DIAG=1 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks 600 --out run
# long WAV (25 s): --vblanks 1500 --wav run/out.wav  -> peak 6219 RMS 836, real audio
```

## 4. Exact runtime edits (for patch capture)

All under `dh-portA` (runtime + harness only; no `fn.py/m2c_fn.py/match.py`, no
`tools/compilers/`). v1.01 of the sound bring-up.

- **`tools/saturn-recomp/runtime/src/sound/sound.cpp`**
  - After `g_peak_frames`: added `static bool g_stv_autostart;` and
    `void sound_stv_autostart(bool on) { g_stv_autostart = on; }` (+comment).
  - Top of `sound_tick()`: added the SNDON bridge — if
    `g_stv_autostart && !g_on && g_sound_ram[0x100]=='V' && g_sound_ram[0x101]=='e'`,
    log the reset SP/PC and call `sound_power(true)`.
  - Also in `sound_tick()`: an **env-gated** (`STV_SND_DIAG`) periodic diagnostic
    (`sat_note("SNDDIAG …")`) dumping 68000 PC, IRQ level, `scsp_active_slots()`,
    mailbox `0x3C20`, seq counter `0x3ECC`, ack `0x3EFE`. Off by default.
- **`tools/saturn-recomp/runtime/src/machine/saturn.h`**
  - Added `void sound_stv_autostart(bool on);` next to the other sound decls.
- **`tools/saturn-recomp/runtime/src/machine/machine.cpp`**
  - `sh2_call_unknown`: **removed** the `if (c.cpu == 1) { … c.pc = c.pr; return; }`
    slave no-op block (+rewrote the comment). The slave now faults like the master
    on an unknown call.
- **`port/main_stv.cpp`**
  - New args `--wav FILE` and `--no-sound` (vars `wav`, `no_sound`); set
    `g_cfg.wav = wav;` in the config block (before `sound_init`).
  - After the device-init block: `if (!no_sound) { sound_stv_autostart(true); … }`.
  - Before `std::_Exit(0)`: added `sound_close();` (prints the sample/not-silent
    report + flushes the WAV).

## 5. Faithful vs forced

- **Faithful:** the whole sound engine — the 68000 running the real Sega driver,
  the ring/Timer-0/mailbox command path, the handshake, the sequencer, and the
  SCSP slot programming / key-ons / sample playback. The slave un-stub (the game's
  own handlers now run). The driver + sample copy (the game's own `sound_init`).
- **Forced:** exactly one thing — the harness issues the 68000-start (SNDON) that
  the skipped ST-V BIOS boot would issue. (Plus the `STV_SND_DIAG` diagnostic and
  the `--wav` capture, which are inspection tools, not behaviour.)

## 6. Remaining gaps / next steps

- **No live audio device** (headless by design). Evidence is the SCSP slot dump +
  the WAV. Wiring SDL audio out is a host concern, not an engine one.
- **Sample pitch/tuning not independently verified.** The driver sets OCT/FNS per
  note from the tone tables; the WAV is correct PCM at 44.1 kHz but the authored
  musical pitch of a given cue is only as right as the tone-table decode
  (`notes/assets/sound/README.md` §5 is partial). Spot-checking a known cue
  against MAME would confirm.
- **Active-slot mask trends high** over a long run (many slots show active). Could
  be correct (layered attract bed + SFX) or voices not releasing promptly; worth
  confirming the EG release path against MAME if a cue sounds sustained.
- **SNDON is forced, not faithful.** To make it faithful, run the real ST-V BIOS
  boot (needs the ST-V BIOS ROM) or find/replay the exact SNDON the BIOS issues;
  low value vs. the current bridge.
- The `--stvinput` coin/start timeline can now be used to check **gameplay** SFX
  (hits, voices) beyond attract — the path is identical, only the command ids
  differ.
