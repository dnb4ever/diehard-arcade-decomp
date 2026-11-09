# Die Hard Arcade (ST-V) — RECOMP native port: cart-boot harness status

Date: 2026-10-19. Follows `notes/port/recomp_feasibility.md` (the GREEN spike).
Verdict first: **the ST-V cart boots.** The harness copies the cart program into
HWRAM, stands up the BIOS work area, enters `main_init_entry`, runs
`frame_sync_loop` for multiple frames, takes the VBlank/Timer interrupts, and
drives **both VDP1 and VDP2 register setup plus VRAM/CRAM tile+palette DMA**.
It then turns on the slave SH-2, which boots and runs until it calls a **null
command-handler pointer** — the single, precisely-located next blocker, rooted
in the ST-V BIOS services we had to stub (no ST-V BIOS ROM on hand).

This is well past the milestone the spike set ("reach `main_init_entry` →
`frame_sync_loop` running → first VDP1/VDP2 register writes"). All of that is
done and logged; nothing here is overclaimed — the run still ends in a fatal,
on the slave, and no pixels are drawn yet (sprite command lists are still empty
this early in attract/init).

---

## 1. What was built

### Toolchain (re-cloned, as the spike's `/tmp` copy was gone)
- `tools/saturn-recomp/` — fresh clone of **callumjenkins/saturn-recomp** (the
  productized fork the spike recommended as the game-port front-end; main @
  `b22688d`). Game-agnostic SH-2→C++20 static recompiler + the full C++20
  runtime (dual-SH-2, VDP1/VDP2/SCU/SMPC/SCSP+68000, HLE BIOS). Pure-Python
  layers 1–4 run under p620's Python 3.14; the runtime is C++20.
- Build reality on p620: **no clang++, no ninja, no SDL3** — but **g++ 15.2**
  and **cmake 4.2.3** are present. `saturnrecomp.build`/`prereqs` hard-require
  clang+ninja+SDL, so they are bypassed. We drive `recomp` generation directly,
  then configure the generated CMake with **g++ + Unix Makefiles +
  `-DSATURN_NO_SDL=ON`** (headless `host_null.cpp`). This is the first time the
  **full** runtime (`saturn_machine`: vdp1/vdp2/scsp + Musashi 68000) has been
  built under g++ — it compiled and linked **clean, 0 errors**.

### The recompiled module
- Input is exactly the proven linear copy region: **cart 0x200000–0x28de20**
  (`0x8de20` = 581 152 bytes), extracted to `port/diehard_main.bin`, recompiled
  at base **0x06004000** (`notes/rammap.py`, delta +0x05E04000). Verified
  byte-equal to `cart_fpr.bin[0:0x8de20]`.
- Recompiling *only* the copied segment is deliberate, not a shortcut:
  `recomp`'s module descriptor is `{base, size = len(file), crc32(file)}` and
  `sh2_identify` CRC-checks exactly those bytes in memory at `base`. The region
  the ST-V BIOS copies to RAM **is** 0x200000–0x28de20 (0x28de20 is the last
  non-0xff byte); the back half of `cart_fpr.bin` is 0xff fill with no RAM home,
  and recompiling it would only break the CRC match. So "the full cart region"
  = this segment. Flagged per the spike's note on honesty.
- **916 functions** from the base + 57 symbol seeds; grown to **932** by the
  learn loop below (+16 indirect-call targets). `main_init_entry` (0x06005020),
  `frame_sync_loop` (0x060053C4) and the three ISRs (0x06027CF0/D98/DCA) are all
  emitted entries. (The fork's discovery is more conservative than the spike's
  saturnkit count of 1059 — fewer speculative entries, same real code.)

### The harness (`port/`)
- **`main_stv.cpp`** — a new `main()` beside the runtime's `src/app/main.cpp`.
  It reuses the entire runtime unchanged (two SH-2s, all devices, HLE BIOS, the
  slave coroutine, the safe-point poll/interrupt machine) and replaces **only**
  the disc boot path:
  1. copies `diehard_main.bin` into HWRAM 0x06004000 (`sh2_mem_write`);
  2. lays down the BIOS work area as `bios.cpp::bios_boot` would — the master &
     slave vector tables (each slot a fake ROM address the HLE maps back to a
     `SYS_*` service), `SYS_GETSYSCK`/`SYS_GETSCUIM` defaults, the BUP table,
     SCU mask all-masked;
  3. **stubs the ST-V BIOS low-vector window 0x06000600–0x060006FF** (service
     pointers the skipped PIC boot stub would have installed) to the HLE no-op
     handler — "reuse the Saturn `SYS_*` HLE where they overlap; stub the rest";
  4. seeds the master as the ST-V reset leaves it (VBR 0x06000000, I=15,
     **SP 0x06002000** — the runtime's own `kResetVectors` SP) and jumps
     straight to `main_init_entry`. **Entering there bypasses the whole PIC
     boot stub (cart 0x20013c–0x200258) and with it the security-CRC self-branch
     hang (~cart 0x200748)** — the "stub the security-CRC" requirement met by
     not running it, which is cleaner than patching the hang;
  5. runs headless to `--vblanks N`, driven by the recompiled code's own
     safe-point polls; a fatal dumps where it died.
- **`stv.cmake`** — adds the `saturn_stv` executable via `-DSATURN_EXTRA`,
  linking `saturn_core + saturn_machine + saturn_host(null) + recomp`.
- **`relearn.sh`** — regenerates the recomp from the 57 symbol seeds +
  `seeds_extra.txt`, then rebuilds `saturn_stv`.
- **`autolearn.sh`** — the learn loop: run → scrape `call to ADDR, not an entry`
  → append seed → `relearn.sh` → repeat. Learned 16 indirect targets this way.
- **`seeds_extra.txt`** — 25 learned seeds (PIC `bsrf`/`jsr` targets discovery
  misses because the pool word is a PC-relative delta).

Build/run:
```
port/relearn.sh                                              # regenerate + build
port/recomp-build/saturn_stv --cart port/diehard_main.bin --vblanks 120 --trace --out port/run
```
Constraints honored: nothing under `tools/compilers/` or the forbidden
`fn.py/m2c_fn.py/match.py/unit_grind.py/permute.py` was touched; all new files
live under `port/` and `tools/saturn-recomp/` (the latter only the clone). No
runtime source was modified — the harness is purely additive.

---

## 2. How far it boots (named functions reached, in order)

| Stage | Evidence |
|---|---|
| image copied, module identified | `module dh active at 06004000 (581152 bytes)` — CRC match |
| **`main_init_entry`** (0x06005020) entered | with SP 0x06002000, VBR 0x06000000 |
| `FUN_02201958` (clear globals) ran | completed through its `*0x0600063C` BIOS call (stubbed → r0=0) |
| ISRs registered | `SYS_SETUINT(40,06027CF0)` `(41,06027D98)` `(43,06027DCA)` — exactly the VBlank-IN/OUT/Timer-0 handlers `engine_notes` predicted |
| slave entry registered | `SYS_SETSINT(94, 060051B0)` |
| SCU programmed | `SYS_SETSCUIM(FFFFFFFF)`, `SYS_CHGSCUIM(FFFFFFF4,0)` (unmask VBlank-IN/OUT + Timer0), `SYS_CHGSYSCK(1)` (352-dot clock) |
| **`frame_sync_loop`** (0x060053C4) running | master spins the per-frame executive; at the VBlank limit `pc` sits inside it |
| **interrupts firing & returning** | `interrupts taken: 40:7 41:6 43:10` — the recompiled ISRs run and `rte` correctly through the HLE dispatcher |
| slave SH-2 turned on | `SSHON` → `slave boots at 060051B0` (the sound/command worker) |
| per-frame pipeline reaching deep game code | `last polls in: dh:060185D0 x762, dh:06027C74 x547 (isr region), dh:06019DA8, dh:06017290 …` |

## 3. First VDP writes seen — yes, VDP1 **and** VDP2, plus VRAM/CRAM DMA

From `port/run/hw-log.txt` (every register touched) and the trace:

- **VDP1 registers** `0x05D00000–0x05D0000A` (TVMR, FBCR, PTMR, EWDR/EWLR/EWRR —
  framebuffer mode + erase/plot setup; `vdp1_frame_begin`), written each frame;
  `VDP1 reg 00 <- 0001` (plot trigger) fires from inside the VBlank ISR.
- **VDP2 registers** `0x05F80000–0x05F80058+` (TVMD, RAMCTL and the scroll /
  plane / map registers — `vdp2_scene_setup`), programmed via SH-2 DMAC bursts
  to `0x25F80010`.
- **VDP2 VRAM** filled by DMAC: `0x25E60000` (4096×4B), `0x25E64000` (1280×4B),
  `0x25E65400`, `0x25E66F00` (3904×4B) — cell/tile pattern data.
- **VDP2 CRAM** (colour) filled: `0x25F00220/280/2A0/2C0/2E0/300…/3E0` — palette.
- **SMPC** INTBACK + PDR path exercised (`0x0010001F` COMREG, `0x00100063` SF,
  `0x00100027` OREG, `0x00100077` PDR) — peripheral/input init.
- **SCSP** control poked once (`0x05B00400`) — `sound_init`.

`VDP1 draw N: 0 commands` throughout: the VDP2 background and VRAM are being
built, but no VDP1 sprite command lists are populated yet — expected this early.

## 4. The blocker, and why

```
[0.172 FATAL] call to 00000000, not an entry of an active module (from pr 06005202)
  cpu 1  pr 06005202  r12 060E0DD0  r13 80000800 ...
```

**The slave SH-2 (cpu 1) jsr's a null handler pointer.** The slave entry
`0x060051B0` is the **sound/command worker**: it masks its own IPRA/IPRB, builds
a ring at `0x060E0DD0`/`0x060E0DD4`/`0x060E0DD8` (the sound-queue region from
`engine_notes`: idx@0x060E0DEC, base@0x060E0DF0), and reads a command *code* out
of it (`r13 = *(*0x060E0DD0 | 0x20000000)`). Its dispatcher (disassembled):

```
060051F6  bt 0x06005202              ; r13 == 0 (empty): skip the call
060051F8  mov.l 0x060052C8,r0        ; r0 = 0x06090274  (handler table base)
060051FA  shll2 r13                  ; index = code*4
060051FC  mov.l @(r0,r13),r3         ; r3 = handler table[code]
060051FE  jsr @r3                    ; call it   <-- r3 == 0 here
```

At the fault `r13 = 0x80000800` — a **bogus command code** (high bit set); the
`bt` empty-ring guard did not catch it (it is non-zero), `code*4` indexes far
outside the handler table at `0x06090274`, and the loaded handler is **0**, so
the `jsr` calls null. The ring was handed a garbage command because it is
populated by the master's sound path (`sound_init` 0x02215DA8 →
`sound_queue_push`/`_service`/`sound_send_command` → the SCSP + 68000 driver),
which here runs against our **stubbed** ST-V BIOS services and an un-exercised
68000 sound driver, so what lands in the ring is not a real command.

Root cause class: **the ST-V BIOS stub gap the spike called out.** We have no
ST-V BIOS ROM, so the 0x06000600 service window and the program-copy/sound BIOS
calls are no-ops returning 0. The master survives that (its uses tolerate 0);
the slave's command dispatcher does not — it calls the 0.

Earlier walls on this climb were all the ordinary learn-loop kind (PIC indirect
targets discovery missed) and were fixed by seeding — 25 of them. This one is
different: it is a **data/semantics gap** (the ring must hold a real command and
the table a real handler), not a missing function. The learn loop correctly
stopped — it began "learning" address 0x00000000, the tell that the remaining
target is data, not code. Note the dispatcher *does* have an empty-ring path
(the `bt`), so this is not a seedable branch target — ruling in route 1 below.

## 5. Precise next step

Find why the slave's command ring at `0x060E0DD0` holds a bogus code (and/or the
handler table at `0x06090274` a null slot), and fix the master path that fills
them. Concretely:

1. **Watch the ring and the table.** Re-run with
   `--watch 060E0DD0:060E0DF4` and `--watch 06090274:060902F4`: the store watch
   names the recompiled function (and PC) of every write, so this shows exactly
   which master routine writes the bad command / leaves the handler null, and
   whether it ran at all before SSHON.
2. **Follow the sound path from there.** It will lead into `sound_init`
   (0x02215DA8) / `sound_queue_push` (0x02215F6C) / `sound_send_command`
   (0x02215E20). If a step is an un-emitted function, seed it (ordinary learn
   loop). If it bottoms out in a **stubbed ST-V BIOS / 68000 sound-driver
   service** (the likely case — that is where the real handler pointers come
   from), hand-write that one service as a small runtime shim so the ring/table
   get real values. This is the first ST-V BIOS body worth implementing for
   real rather than stubbing.
3. **Cross-check SSHON timing.** Confirm the master had a chance to push the
   first real command before `SSHON`; if the slave is started a frame early,
   that alone could explain the garbage read, and the fix is ordering, not a
   BIOS body.

Either way the **master half is solid** — it reaches `frame_sync_loop`, services
the VBlank/Timer interrupts, and programs both video chips with VRAM/CRAM loaded.
The remaining work is the sound/slave command plumbing and the handful of ST-V
BIOS service bodies behind it — exactly the integration tail the spike predicted,
now narrowed from "ST-V boot is unproven" to one concrete ring/handler pointer.

---

### Reproduction (p620)
```
# tool:   ~/decomp/diehard-stv/tools/saturn-recomp  (callumjenkins/saturn-recomp)
# harness: ~/decomp/diehard-stv/port/{main_stv.cpp, stv.cmake, relearn.sh, autolearn.sh, seeds_extra.txt}
cd ~/decomp/diehard-stv/port
./relearn.sh                                             # regen recomp (932 fns) + build saturn_stv (g++, headless)
./recomp-build/saturn_stv --cart diehard_main.bin --vblanks 120 --trace --out run
#   -> boots to frame_sync_loop, VDP1+VDP2 writes, ints 40/41/43, SSHON,
#      then FATAL: slave null call at pr 06005202 (sound ring unpopulated)
cat run/hw-log.txt            # every hardware register the run touched
```
Config: base 0x06004000, entry 0x06005020 (`main_init_entry`), SP 0x06002000,
delta +0x05E04000. One-time configure (already done) if `recomp-build` is wiped:
```
CXX=g++ cmake -S recomp -B recomp-build -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE=Release -DSATURN_NO_SDL=ON -DSATURN_EXTRA=$PWD/stv.cmake
```
