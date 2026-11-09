# diehard-stv

Functional decompilation of **Die Hard Arcade** / *Dynamite Deka* (Sega ST-V "Titan" arcade hardware, 1996; dual Hitachi SH-2), plus a **native PC port** (Linux, macOS, Windows, Steam Deck) built on a Saturn static recompiler.

Non-commercial fan preservation and research project. Not affiliated with or endorsed by Sega or the owners of the Die Hard name.

## No game data in this repository

This repository contains **no ROMs, BIOS images, game code, graphics, music or sound**. To build or run anything you need your own legally obtained Die Hard Arcade / Dynamite Deka ST-V romset and ST-V BIOS. The port's recompiled game code is generated locally from your ROM at build time and is never distributed. Please do not open issues or pull requests containing game data.

## Status

- **Decomp (functional, not byte-matching):** C for every game function, verified against the original machine code by a differential tester (`tools/decomp/difftest.py`): 178 PASS / 890 PASS-void / 0 FAIL / 31 untestable (each with a documented reason). All functions and fields carry evidence-based names (`notes/symbols.txt`, `notes/game_architecture.md` is a modder's map). Byte-matching is out of reach because the 1996 Hitachi SHC compiler build is lost.
- **Port:** plays the whole game from attract mode to the ending at full speed. Features: launcher (SDL3 + Dear ImGui), keyboard and per-player gamepad layouts, game settings, save states, rewind, turbo/autofire, training options, input recording and replay, optional 2x/3x internal resolution, CRT filter, and **online 2-player netplay with rollback**. Output matches MAME's reference behaviour and is bit-identical across Linux, macOS and Windows.
- **Convergence:** an opt-in build that swaps 216 recompiled functions for the decomp's C and still plays bit-identically to the ending, a first step toward a fully decomp-based, moddable port.

## Layout

- `project/` — the decomp: `src_functional/` (main body), shared headers, build and progress scripts.
- `tools/decomp/` — the toolchain: function splitter, m2c drivers, branch/pointer resolvers, differential tester.
- `port/` — the native port: game front-end (`main_stv.cpp`), build and packaging scripts for each platform, and `runtime-patches/` applied to the recompiler runtime.
- `port/converge/` — the decomp-C override build and its equivalence evidence.
- `notes/` — reverse-engineering notes, symbol table, per-round reports and port write-ups.
- `scripts/` — Ghidra headless scripts.

The recompiler itself is [saturn-recomp](https://github.com/callumjenkins/saturn-recomp) (MIT); it is cloned into `tools/saturn-recomp/` and patched with `port/runtime-patches/` in the order listed in that folder's README.

## Building

You need your own romset. `build_cart.py` assembles the cartridge image from a MAME-format romset. Platform build scripts are in `port/` (`p46_build_window.sh` for Linux, `p48_build_mac.sh` and `p411_build_app.sh` for macOS, `p412_build_win.sh` for a Windows cross-build, `p417_build_linux.sh` for the Linux AppImage); each is documented in the matching `notes/port/` write-up.

## How this was made (AI agents)

I don't write code. This project was built by AI coding agents (Anthropic's Claude through Claude Code, and OpenAI Codex) working under my direction: the functional decompilation and its verification, the toolchain, the reverse-engineering notes, and the native port. My role was direction, decisions, hardware and ROM provisioning, and review.

This public repository is a cleaned snapshot; the step-by-step history lives in a private working repository.
