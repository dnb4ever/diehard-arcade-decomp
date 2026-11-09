# Die Hard Arcade (ST-V) — RECOMP native port: FIRST RENDERED FRAME

Date: 2026-10-19. Follows `notes/port/boot_harness_status.md` (the cart-boot harness).

**Verdict first: the port renders.** The master SH-2 now boots, runs
`frame_sync_loop`, services the VBlank/Timer interrupts, and the VDP2 core
composites the game's **attract-mode night city skyline**, then the
**"INSERT COIN(S)"** attract text over it. It survives **the full 600-VBlank
limit (~10 s of game time) with a clean exit (rc=0)** — not a crash boundary,
just where the run was told to stop. The old fatal was at VBlank 11.

Honest scope line: **VDP2 background + text layers render; no VDP1 sprites yet
(VDP1 draw lists are still empty, 0 commands all 600 frames); no sound (the
slave sound worker is stubbed).**

Deliverable PNGs (headless software compose, `port/run/`):
- `first_frame_vb45.png`   — the skyline as the display first turns on (~VB45)
- `skyline_vb250.png`      — the skyline mid-attract
- `attract_insertcoin_vb599.png` — skyline + green "INSERT COIN(S)" text (VB599)

(`shot-45/120/250/400/599.png` are the raw capture names from the final run.)

---

## 1. The real blocker was not the slave, and not a BIOS body — it was the cartridge

The documented blocker (slave `jsr` to a null handler) and the blocker behind
it (the master's "call to 00000000" in the sound-sequence setup at `0x0601BD6C`
→ `f_060190D4`) had **one shared root cause**, found by instrumenting, not
guessed:

**The A-bus cartridge ROM window was unmapped — every cartridge read returned
open-bus `0xFF`.** `runtime/src/machine/mmio.cpp` serviced the whole A-bus
cartridge area (`0x02000000..0x05000000`, cache-through `0x22xxxxxx`) with
`return 0xFF/0xFFFF/0xFFFFFFFF`. But an ST-V title reads its code, assets **and
its sound/sequence/instrument tables straight out of cartridge ROM**. The watch
(`--watch`, added to the harness) showed `f_060190D4` building pointer tables
from a cartridge table at `0x22700000` (= cart.bin offset `0x700000`); with the
window floating high it read `0xFFFFFFFF` and built **garbage/null function
pointers**. Those are exactly:
- the slave's bogus command code `0x80000800` / null handler, and
- the master's `jsr`/`jmp` to `0` inside the sound setup,
- and (when the null was papered over) a Timer-0 (vec `0x43`) dispatch to a
  garbage `0x20000000` handler.

All three are the same `0xFF` fill read as a pointer. `notes/rammap.py` already
documented the mapping — **cart.bin offset 0 == A-bus `0x02000000`**, linear —
so the fix is to make that read return real ROM.

### The fix (real, not a stub)
`mmio.cpp`: added a cartridge-ROM pointer (`g_cart_rom` / `g_cart_rom_size`) and
changed the A-bus read branch to return `mem_rd(g_cart_rom, a-0x02000000, size)`
when it is mapped (falling back to open-bus beyond the image). The harness
(`main_stv.cpp`, new `--cartrom`) loads the full 20 MiB `cart.bin` and points
the window at it. `sh2_io_read` already normalises both the cached `0x02xxxxxx`
and cache-through `0x22xxxxxx` views onto the 27-bit bus, so both resolve.

With real cartridge data the master no longer reads garbage: the sound setup
builds real pointers, and the old master "call to 0" turns into **ordinary
missing-function faults** — the recompiler's discovery had simply not emitted
some PIC indirect-call targets that only became reachable once real data flowed.

### Slave sound path (the documented route-2 no-op)
A first frame needs no sound, and the slave SH-2 here is purely the sound/command
worker (entry `0x060051B0`). `machine.cpp` `sh2_call_unknown` now, **on the slave
only (`cpu==1`)**, treats an unknown/null call target as an empty handler that
returns at once (`c.pc <- c.pr`) instead of a fatal. The master, every other
game, and the disc boot path are untouched by the `cpu==1` guard. (SSHON timing
was fine — cross-checked; the slave's garbage was the unmapped cartridge, not a
one-frame-early start, so no ordering change was needed.)

### Seeds (ordinary learn loop, now with cartridge data)
With the cartridge mapped, `autolearn.sh` (updated to pass `--cartrom` and to use
a VBlank budget that actually reaches the deep init) learned the real indirect
targets: **33 new seeds across two passes** (to survive 60 frames, then 600).
`port/seeds_extra.txt` is now 58 entries. The loop converged twice — "no
learnable non-entry", clean 60- then 600-VBlank stop.

### Exit hang (why runs looked slow)
`main_stv.cpp` ended with `return 0`, which blocked process teardown on the
parked slave coroutine thread — every run burned its whole `timeout` at exit and
reported `rc=124`. Changed to `std::_Exit(0)` (as the runtime's own `saturn_main`
does). Real speed: **600 VBlanks in ~15 s wall, 60 in ~0.8 s** on p620.

---

## 2. How far it gets now

| Stage | Before | Now |
|---|---|---|
| master boot → `frame_sync_loop` | yes | yes |
| VBlank/Timer ISRs serviced | yes | yes (40/41/43 firing every frame) |
| VDP1/VDP2 register + VRAM/CRAM DMA | yes | yes |
| first fatal | **VBlank 11** (slave null / master call-0) | **none through VBlank 600** |
| VDP2 display enabled (TVMD DISP) | never (off, `TVMD=0x0001` at VB12) | **~VBlank 30–45** |
| **VDP2 background composited** | no (black) | **yes — night city skyline** |
| **VDP2 text layer** | no | **yes — "INSERT COIN(S)" attract text** |
| VDP1 sprites | 0 commands | still 0 commands (none built yet) |
| sound | — | stubbed (slave no-op) |
| run end | fatal | clean stop at the 600-VBlank limit, rc=0 |

What the PNGs show: at ~VB45 the display turns on and the skyline **fades in**
(a dark partial frame → a full gradient-sky skyline). By VB250 it is the full
attract cityscape; by VB599 the green **"INSERT COIN(S)"** attract text is drawn
over it. Frames differ across time (the attract scene is live), confirming it is
a running game loop, not a frozen buffer.

The frame dump itself needed no new compositor: the runtime's `vdp2_compose` +
`png.cpp` already composite the VDP2 layers (and VDP1's framebuffer) to a PNG at
VBlank-IN. The harness just drives it via the runtime's own `--shots N,...`
(and `--dump` for raw VDP memory), wired up in `main_stv.cpp`.

---

## 3. Files touched (all under `port/` and `tools/saturn-recomp/`)

Runtime (`tools/saturn-recomp/runtime/src/`):
- `machine/mmio.cpp` — map the A-bus cartridge ROM (`g_cart_rom`); real fix.
- `machine/machine.cpp` — slave-only (`cpu==1`) no-op for unknown/null call
  targets in `sh2_call_unknown` (sound path), instead of fatal.

Harness (`port/`):
- `main_stv.cpp` — `--cartrom` (load cart.bin into the A-bus window), `--shots`,
  `--dump`, `--watch LO:HI` (work-RAM store watch, names the writing function);
  `std::_Exit(0)` at the end so the parked slave thread does not hang teardown.
- `autolearn.sh` — pass `--cartrom`, default VBlank budget raised to reach the
  deep init, stop-on-`00000000` guard.
- `seeds_extra.txt` — 58 learned PIC indirect-call targets (was 25).

No `fn.py` / `m2c_fn.py` / `match.py` / `tools/compilers/` touched.

### Reproduce (p620)
```
cd ~/decomp/diehard-stv/port
./relearn.sh                                    # regen recomp (all seeds) + build saturn_stv
./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
    --vblanks 600 --shots 45,120,250,400,599 --out run
#  -> boots, runs 600 VBlanks (rc=0), writes run/shot-*.png:
#     the VDP2 city-skyline attract background, with "INSERT COIN(S)" by VB599.
```
Config unchanged: base `0x06004000`, entry `0x06005020`, SP `0x06002000`,
delta `+0x05E04000`. The cartridge window is `cart.bin` byte k at A-bus
`0x02000000 + k` (cache-through `0x22000000 + k`).

---

## 4. Honest limits / next steps

- **No sprites yet.** VDP1 draw lists are empty all 600 frames; the attract
  screen here is VDP2-only (background + text), which matches the real game's
  attract intro. Interactive/gameplay sprites would exercise VDP1 — the next
  thing to verify is whether VDP1 command lists get built once attract advances
  or a coin/start is injected (the harness has no pad input wired yet).
- **No sound.** The slave worker is a no-op; wiring the real ST-V sound path
  (and the 68000 driver) is a separate, large job and not on the render path.
- **600 VBlanks is a chosen stop, not a wall** — raise `--vblanks` to go further;
  if a new PIC target faults, `autolearn.sh` seeds it in one pass.
- The slave `cpu==1` no-op is a deliberate scoped stub; if sound is ever brought
  up, it is the first thing to remove.
