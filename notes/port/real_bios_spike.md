# Real ST-V BIOS recompile+boot spike — VERDICT (2026-10-22)

## Decision: real-BIOS boot is NOT the shorter path to a playable round. Keep the HLE path, but use the now-runnable recompiled BIOS as an ORACLE.

## What was proven
- Correct US BIOS = **epr-17952a.ic8** (USA 95/04/25), from diehard.zip ROM_DEFAULT_BIOS("us")
  index 7 in MAME stv.cpp STV_BIOS. (The earlier guesses epr-20091/epr-19730 are JAPAN.)
  epr-18022.ic2 is the Sega Billboard Z80 ROM, NOT in the SH-2 boot path (red herring).
- Byte order: ST-V BIOS loads ROM_LOAD16_WORD_SWAP; word-swapped reset vector = PC 0x20000200,
  SP 0x06100000 (matches the runtime's kResetVectors). Real code begins at ROM offset 0x200.
- RECOMPILE: same tool/flow as the cart (saturnrecomp, --base 0x00000000). 695 functions /
  65694 instructions / 9 .cpp, g++ -std=c++20 -> 11/11 objects, 0 errors. No BIOS-specific
  emit failures. Heavy PIC bsrf/braf (same idiom as the cart).
- BOOT (standalone harness, real 512KB served, recompiled module CRC BF7DBDD7 matched memory):
  reaches reset@0x200 -> WDT + cache address-array init -> SH-2 BSC init (A55A keyed writes) ->
  SCU reg -> SMPC reset handshake -> work-RAM init loops -> high-level boot f_000031C4. This is
  the full low-level hardware bring-up the HLE skips.
- The "security" gate is NOT a CRC: it is the SMPC OREG31 -> SYSRES (0x0D) **cached-reentry
  reset handshake**. Clears with a shim that re-enters the reset vector on SYSRES. (The CRC-16
  self-hang at 0x200748 is the CART's, a separate downstream gate.)

## The blocker (precise, genuine static-recomp boundary)
Stops at f_00003576: `r1 = ld32(0x06000280); jmp @r1`, slot = **0x060014C0 (WORK RAM)** which is
empty -> fatal. The ST-V BIOS **builds/relocates executable routines into work RAM and dispatches
through RAM-resident vector tables.** A static recompile of only the 512KB ROM has no function at
those RAM addresses. Not unemulated silicon, not a security device -- a RAM-relocation architecture.

## Cost if pursued anyway (why it is NOT the shortcut)
Trades two WELL-SCOPED HLE couplings (char-select input decode; live-stage/post-81 activation)
for an OPEN-ENDED problem: map every ROM region the BIOS relocates to RAM and recompile each at
its RAM base (so the runtime multi-module machinery can activate it on the jump), + implement ST-V
SMPC/boot semantics the Saturn-oriented runtime lacks, + some RAM targets may be RAM-BUILT thunks
(not straight copies) that no re-recompile covers and would need HLE regardless. Rough scale:
multi-week-to-months research of uncertain size. VDP1/VDP2 init + cart handoff unexercised past
the blocker; cart CRC-16 self-hang still waits downstream.

## 3 runtime gaps found (what real-BIOS support would need in saturn-recomp; all shimmed in scratch)
- A. Region-7 code addressing: reset self-relocates via braf into 0xE0000000; runtime only masks
  the 0x2xxxxxxx cache-through region. Fix: mask code targets `& 0x1FFFFFFF` universally (core.cpp).
- B. SMPC SYSRES semantics: runtime treats SYSRES as console-reset -> stop; ST-V uses it as a boot
  reset. Fix: set OREG31=cmd + re-enter reset vector preserving memory (smpc.cpp).
- C. bios_rom_read is a 16-byte stub (reset vectors only). The runtime ships with NO real BIOS ROM
  support -- serving the real 512KB is the core enabler (bios.cpp).

## RECOMMENDED next direction (synthesis)
The recompiled BIOS now RUNS -> use it as a **runnable executable oracle**: single-step/instrument
it to extract the exact behavior of the two missing couplings (char-select input decode via the
SMPC/control-panel path; live-stage/post-scene81 activation), then port THOSE specific behaviors
into the existing HLE. Real-BIOS faithfulness without adopting its RAM-relocation boot wholesale.

## Artifacts (p620, OUTSIDE the working port, NOT committed - regenerable from ROM+tool)
~/decomp/bios_spike/: epr-17952a.ic8, bios_be.bin (word-swapped), bios_recomp/ (recompiled C++),
boot/bios_boot.cpp (harness), boot/{bios,core,smpc,machine}_patched.cpp (the shims A/B/C),
boot/build.sh, boot/runout/bios-hw-log.txt. Repro: cd ~/decomp/bios_spike/boot && bash build.sh &&
./bios_boot --bios ../bios_be.bin --cartrom ~/decomp/dh-portB/cart.bin --vblanks 120 --out runout.
(ROM blobs + recompiled BIOS C++ intentionally gitignored per repo rules; this note is the record.)
