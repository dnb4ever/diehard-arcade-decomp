# Unified runtime: dh-portB now has BOTH the round logic AND the sound

Date: 2026-10-22. dh-portB is now the single canonical tree carrying the faithful
coin->round-entry logic **and** the un-stubbed slave/SCSP sound path, building to one
`saturn_stv` binary. The sound edits (previously only in dh-portA) were merged in; dh-portA
was read-only for reference. No git was run (parent session commits after verifying evidence).

## What was merged (4 files, from dh-portA -> dh-portB)

Common runtime base = `b22688d`.

1. `tools/saturn-recomp/runtime/src/sound/sound.cpp` — **copy** (dh-portB's was pristine vs base).
   Adds `static bool g_stv_autostart;`, `void sound_stv_autostart(bool)`, the SNDON bridge in
   `sound_tick()` (detects the "Ve" driver signature at sound-RAM 0x100 then calls `sound_power(true)`),
   and the env-gated `STV_SND_DIAG` diagnostic.

2. `tools/saturn-recomp/runtime/src/machine/saturn.h` — **copy** (dh-portA = dh-portB + exactly one line).
   Adds the `void sound_stv_autostart(bool on);` declaration; dh-portB's own `stv_input_script` decl
   is preserved (dh-portA already carried it, so the copy keeps both).

3. `tools/saturn-recomp/runtime/src/machine/machine.cpp` — **merge** (slave no-op removal only).
   dh-portB's ONLY delta vs base was the `if (c.cpu == 1) { ... c.pc = c.pr; return; }` slave no-op
   block in `sh2_call_unknown`. dh-portA's ONLY delta vs base was the replacement explanatory comment
   with that block removed (UN-STUBBED). Since dh-portB had no other machine.cpp edits, the combined
   delta == dh-portA's file exactly, so that is what dh-portB now holds. The scheduler's legitimate
   `if (c.cpu == 1)` in `sh2_poll` (present in base) is untouched.

4. `port/main_stv.cpp` — **merge** (5 sound deltas spliced onto dh-portB's round flags; verified the
   diff vs pre-merge dh-portB is exactly these 5 hunks and nothing else — all round config, CFG798,
   CFG654, cold-boot default, COINSEQ/DEMOSTOP/CLEARSLOT2, stvinput preserved):
   - `wav` added to the string decls + `bool no_sound = false;`
   - `--wav FILE` and `--no-sound` arg parsing
   - `g_cfg.wav = wav;`
   - sound bring-up block after `onchip_reset(0)`: `if (!no_sound) sound_stv_autostart(true);`
   - `sound_close();` at shutdown (before `std::fflush(stderr)`)
   NOTE: dh-portA's main_stv.cpp was NOT copyable — it had diverged and LACKS dh-portB's round config,
   so only the sound deltas were taken from it.

Files intentionally NOT touched: `bios.cpp`, `video/vdp1.cpp`, `video/vdp2.cpp` differ between the
trees but are round-side divergence (RBG camera / DIP diagnostics), not sound — out of scope.

## Build command (dh-portB flow, succeeds clean)

    cd ~/decomp/dh-portB/tools/saturn-recomp
    cmake --build ~/decomp/dh-portB/port/recomp-build --target saturn_stv -j"$(nproc)"

(No recomp regen needed — no recomp source changed. This is the last step of relearn_portB.sh.)
Result: rebuilds machine.cpp + sound.cpp + main_stv.cpp, relinks saturn_stv, `[100%] Built target`.

## Proof 1 - SOUND (same binary)

    ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
        --vblanks 900 --wav /tmp/final.wav --out run/final_snd

Log: "STV: sound driver loaded (reset SP 004000 PC 000120); starting 68000 (SNDON equivalent)";
"sound: 661891 samples, 629857 not silent; 68000 on".
WAV: 2,647,608 bytes. Independent check (python wave): 2 channels, 44100 Hz, 16-bit,
661891 frames, peak amplitude 6699, 1,258,258 / 1,323,782 int16 samples non-zero. NON-SILENT.

## Proof 2 - ROUND (same binary)

    STV_BOOTMODE=0 STV_CFG798=40 STV_COINSEQ=100 STV_DEMOSTOP=980 STV_COV=1 \
        ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
        --vblanks 4000 --stvinput "99999:COIN" --out run/final_round

(NO STV_CLEARSLOT2.) COV at 4000 vblanks:
    attract_handler           ran=1
    player_join_commit_fsm    ran=1
    round_controller_dual     ran=1
    scene_run_handler         ran=1
    scene81_game_sm           ran=1
    round_controller_join     ran=0
Matches the expected faithful chain exactly.

IMPORTANT on vblanks: the chain needs ~4000 vblanks to propagate all the way to scene81.
At 1400 vblanks only attract_handler fires; at 2500, player_join_commit_fsm + round_controller_dual
fire; scene_run_handler + scene81_game_sm need ~4000. This is a pre-existing timing property of
the COINSEQ/DEMOSTOP path, not an effect of the merge (verified: pristine pre-merge dh-portB gives
an identical COV for identical vblank counts).

## Proof 3 - DEFAULT-OFF sanity

A plain run with no env knobs now has sound ON by default (482429/514462 non-silent) — this is the
intended new default inherited from the sound tree, NOT a regression; `--no-sound` restores the old
render-only path (68000 held, 0 non-silent). The non-sound default behavior is byte-identical to
pre-merge dh-portB: merged `--no-sound` attract frame at VB600 md5 = 19ae78ccd73d5a75054a0e400da8b6cd,
which EXACTLY equals the pristine pre-merge default frame md5. The boot-mode (cold 1), DIP (0x60) and
panel/credit (0x37F) default log lines are unchanged. The sound-on default frame differs from the
sound-off frame only because the attract animation is timing-sensitive and releasing the 68000 shifts
CPU scheduling (expected; video path unchanged).

## Patch

`diehard-stv/port/runtime-patches/round-runtime-dh-portB.patch` refreshed (round + sound). It covers
runtime/ only (sound.cpp, saturn.h, machine.cpp + the pre-existing round runtime edits in
bios.cpp/vdp1.cpp/vdp2.cpp); `port/main_stv.cpp` is outside runtime/ and tracked separately.
