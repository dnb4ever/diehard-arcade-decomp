# Die Hard Arcade (ST-V) — Hybrid-Recomp Native Port: Feasibility Spike

Date: 2026-10-19. Target: `~/decomp/diehard-stv/cart_fpr.bin` (SH-2 BE program, cart
0x200000–0x290000, runs from HWRAM 0x06004000). Spike goal: test the BRAIN half
(SH-2 → compilable C) and scope the BODY (Saturn hardware cores). Verdict first,
evidence after. **Nothing here is sugar-coated.**

---

## TL;DR verdict

**A hybrid-recomp port of this game is realistic — and the premise of the spike is
better than we thought.** The "XenonRecomp + borrow Xenia's cores" model from the
Xbox 360 scene does **not** apply, because for Saturn/SH-2 a single project already
ships *both halves together*: a mature, game-agnostic SH-2→C++ static recompiler
**and** a purpose-built C++20 runtime (VDP1/VDP2/SCSP/SMPC/SCU/CD/68000 + the
dual-SH-2). You don't bolt a brain onto a borrowed emulator; you feed a binary to
one toolchain.

On our actual binary, the brain half is **proven end to end**:

| Step | Result |
|---|---|
| Function discovery | **1059 functions**, 254 756 code bytes, 29 switch tables, **1** problem fn (the cart header string) |
| Emit to C++ | 1059 functions, **166 626 instructions**, 22 `.cpp`/`.h` files, 11 MB, 4.9 s |
| **Compile** (host `g++ 15.2`, `-std=c++20`) | **22/22 files, 0 failures** — no clang needed |
| **Link** (recomp lib + runtime core) | **5.3 MB executable**, runs |
| **Semantic validation** (vs SH-2 interpreter oracle) | **257 functions, 4067 vectors, 0 differences** |

The known-hard dual-SH-2 synchronization is **already solved** in the runtime (slave
as a cooperative coroutine) and the game's own SSHON/SSHOFF/SINIT path maps straight
onto it. The real, honest risk is **not** the recompiler — it's that every proven port
with this toolkit is a Saturn **disc** game, and ours is an **ST-V arcade cart**. That
is an unproven boot/BIOS/input path, not a CPU problem. That's the climb.

---

## (a) State of Saturn / SH-2 static-recomp tooling

Searched recompendium, awesome-rea, GitHub. Four real candidates; two are serious.

### 1. `vs-sr-dev/saturnkit` — the core toolkit **[the one to use]**
- **Maturity/activity:** actively developed, last commit **2026-10-12**. 6 games driven
  through it (Virtual Hydlide, Deep Fear, Albert Odyssey, DBZ, DoDonPachi, X Japan).
- **Approach:** true **static recompilation** — SH-2 → C++20, one C++ function per
  guest function, delay slots inlined, guarded dispatch for register-indirect calls.
  Not an interpreter (ships an SH-2 interpreter `sh2emu`, but only as the *oracle* that
  validates the recompiled output).
- **Input format:** **raw binary + load address**, `NAME=FILE.BIN@BASE`. No ELF, **no
  symbols, no splits config required** — `recomp.discover` finds functions itself
  (recursive descent + constant propagation + four switch-table forms + prologue seeds).
  Exactly our situation (stripped image, known base).
- **Dual-SH-2:** **yes** — slave SH-2 as a deterministic coroutine, SSHON/SSHOFF/SINIT.
- **Emits compilable C?:** **yes, C++20**, builds a static lib + `selftest`/`saturn` exes.
- **Runtime/HLE shipped:** **yes, full** — `runtime/`: VDP1, VDP2, SCU(+DSP), SMPC, CD
  block, SH-2 on-chip (DMAC/DIVU/FRT), HLE BIOS services, the 68000 (bundled Musashi)
  and the SCSP, plus an SDL3/OpenGL host window. Pure-Python layers 1–4 (3.8+, no deps);
  C++20 runtime.
- **Pure-Python recompiler confirmed running under p620's Python 3.14.** No build tools
  needed to recompile — only to compile the C++ it emits.

### 2. `callumjenkins/saturn-recomp` — productized fork **[watch / borrow from]**
- Fork of saturnkit, **now the more active of the two** (commit **today, 2026-10-19**),
  136 commits. Adds a `game.toml`-driven build (`python -m saturnrecomp.build`), hook
  points, task-switch config, and the most polished runtime (backup RAM, pad-on-
  touchscreen, SDL3). Same recomp core. Same raw-binary input. Same dual-SH-2 model.
- For a *game* port this is arguably the better front-end; saturnkit is the cleaner thing
  to spike against because its CLI is thinner. **I spiked on saturnkit; the result
  transfers directly.**

### 3. `sonsegajp/SaturnRecomp` (and the `jodasaur` mirror) — **not viable yet**
- 11 commits, 19 stars. Despite the name it is an **interpreter + SDL2 runner** today;
  the AOT decoder/analysis exists but **"a complete public AOT emitter is not"** shipped.
  Windows/MSYS2-only build. Emits no C. Skip.

### 4. `tmcleroy/satrecomp` — **too early**
- 8 commits, 0 ports. Same ambition (SH-2→C++ AOT + runtime) and notably has an
  **interpreter fallback** for untranslated code (saturnkit does not). But immature and
  unproven. Worth remembering only for its fallback idea.

**Conclusion:** the field has consolidated onto the saturnkit / saturn-recomp lineage.
It is real, current, and exactly the right shape for our input.

---

## (b) Did SH-2 → compilable C work on our binary? How far?

**Yes — fully, through compile, link, and semantic self-test.** Everything below is
reproducible on p620 (tool cloned to `/tmp/saturnkit`; outputs in `/tmp/dh_recomp`,
`/tmp/dh_selftest`).

1. **Extracted** the proven main segment: `cart_fpr.bin[0:0x8de20]` = cart
   0x200000–0x28de20 → `/tmp/diehard_main.bin`, recompiled **at its true runtime base
   `0x06004000`** (using `notes/rammap.py`'s delta). `cart_fpr.bin` verified byte-equal
   to `cart.bin@0x200000` (`"SEGA ST-V(TITAN)"`).

2. **Discover:** `python -m saturnkit.recomp.discover diehard_main.bin --base 06004000`
   → **1059 functions, 254 756 code bytes, 43 324 data bytes, 29 switch tables**,
   349 mid-function entries, 342 pointer-calls. Only **1 "problem"**: `undefined at
   06004002` — that's descent walking into the cart header string at 0x200000, cosmetic.
   It also auto-recognized the BIOS service pointers the game uses
   (`BIOS.SYS_SETSCUIM/SETUINT/CHGSCUIM/CHGSYSCK/SETSINT` @0x06000300+).

3. **Emit:** `python -m saturnkit.recomp --out dh_recomp dh=diehard_main.bin@06004000`
   → **1059 functions / 166 626 instructions / 22 files / 11 MB in 4.9 s.** The emitted
   C++ is clean and instruction-annotated, e.g. SH-2 state as `SH2Context& c`,
   `ld32/st32` for the bus, delay slots folded into one statement.

4. **This cart's specific hard feature — PIC — is handled correctly.** The ST-V security
   stub and the HWRAM thunks use position-independent `bsrf`/`braf` where the pool word
   is a *relative delta* (our `ram_mapping_detail.md` flagged these). The recompiler emits
   `{ const uint32_t x = 0x06004676u + c.r[3]; … sh2_call(c, x); … }` — i.e. it resolves
   `pc + delta` **at runtime**, which is correct precisely because we recompiled at the
   real load address. The 6 "target not an entry" lines in the report (e.g. `E87CA2A8
   from 06004D46`) are these PIC deltas; they resolve to real addresses at runtime.

5. **Compile:** all **22/22** emitted files compile with stock **`g++ 15.2`,
   `-std=c++20`, 0 errors** (clang recommended by upstream but not required).

6. **Link:** compiled the runtime's `core.cpp` + `services_stub.cpp` + `selftest.cpp`
   (none need SDL — only the full `saturn` HW target does) and linked them with all 22
   recomp objects → **`/tmp/dh_selftest`, a 5.3 MB working executable** that runs.

7. **Semantic proof (the real one):** generated oracle vectors with
   `saturnkit.recomp.selftest … --auto` (257 of 1059 functions are callable in isolation
   → 4067 vectors from the `sh2emu` interpreter) and ran the recompiled binary against
   them: **`257 functions, 4067 vectors, 0 failures`.** The recompiled C++ is
   bit-identical to the SH-2 reference for every function that can be exercised standalone.

**How far it did *not* go (honest limits):**
- Only the **main code segment** was recompiled. ~259 KB of the image's back half is the
  big unclassified run (rodata/tables/packed assets) — correctly left as data.
- **1705 indirect calls / 32 indirect jumps are statically "unresolved."** Most are the
  PIC/HWRAM-thunk pattern that resolves at runtime via `sh2_call` (address lookup into the
  module). That's fine **iff** every computed target lands in recompiled code. Proving
  that requires actually *running* the game — which needs the ST-V boot harness that does
  not exist yet (see (d)). This is the main untested seam, not a known failure.
- The 257/1059 self-tested are the leaf/pure functions; the stateful 800 are only validated
  transitively. Full validation = a running boot.

**Bottom line for (b): the brain half is de-risked.** A stripped ST-V SH-2 image went to
compilable, link-able, semantically-verified C++ with an off-the-shelf toolchain in one
sitting, no per-function hand-work.

---

## (c) The dual-SH-2 problem — showstopper or manageable?

**Manageable. It is already implemented and proven across 6 shipped games.** This was the
scariest item going in and it's the most reassuring coming out.

`runtime/machine.cpp` runs the **slave SH-2 as a cooperative coroutine on its own host
thread**:
- The master runs recompiled straight-line code and yields only at **safe points**
  (back-edges, calls, `ldc …,sr`) — the `SH2_POLL` macro you see peppered through the
  emitted C++. Every *kBudget* safe points it offers the slave a slice.
- **SSHON** (`smpc_cmd_sshon` @0x02214944 in our map) boots the slave from the BIOS vector
  and hands it the CPU until it parks; **SSHOFF** (`smpc_cmd_sshoff` @0x02214918) unwinds
  and parks it; **SINIT** pulses the slave's FRT input-capture and hands it the CPU at
  once. Our symbol map shows the game drives the slave through **exactly these SMPC
  commands**, so it slots onto the existing model with no new mechanism.
- The scheme is deterministic by construction (cooperative, fixed slice counts) — same run,
  same result, which is what makes the differential self-test and the Beetle-Saturn
  correlation in upstream's CI possible.

Why this beats the emulator-borrow approach specifically: a real emulator synchronizes the
two SH-2s by interleaving them in a **cycle-accurate global scheduler**. Recompiled code has
no cycle counter — it's native straight-line C++. The coroutine/safe-point model is the
*correct* impedance match for recompiled code, and it's the reason you can't just lift
Mednafen's dual-SH-2 core (see (d)). **Not a showstopper.** The residual risk is narrow:
a game that depends on *tight* master/slave timing races could expose the slice
granularity — but Die Hard Arcade uses the slave in the ordinary SSHON/worker fashion, and
6 other commercial titles already pass.

---

## (d) The body: emulator-core reuse path + difficulty

**Recommendation: do NOT borrow Mednafen/Kronos/Yabause. Use the saturnkit runtime that
ships with the recompiler, and spend the effort on the ST-V boot/BIOS/input shim instead.**

### Why not borrow an emulator (the spike's original premise, honestly reassessed)
- **Mednafen / Beetle-Saturn** — the accuracy king, but its SH-2 is a cached interpreter
  welded to the global event scheduler and memory bus; the VDP/SCSP cores are driven by
  that scheduler's timing. It is **not a library you call into** — you'd be hosting its main
  loop, which is the opposite of what recompiled code wants. GPL.
- **Kronos** — Yabause fork, SH-2 **dynarec**, performance-focused, sprawling codebase,
  OpenGL-centric VDP. Even harder to carve a clean core out of. GPL.
- **Yabause** — more modular, but inaccurate and effectively unmaintained. GPL.
- In all three, the **dual-SH-2 sync is inside the scheduler** you'd be trying to delete.
  Extracting self-contained VDP1/VDP2/SCSP cores and re-timing them against recompiled,
  cycle-less code is a large project that **reinvents what saturnkit already has**. The
  Xbox-360 analogy breaks because XenonRecomp genuinely had no runtime and Xenia was
  library-ish; here the recompiler *comes with* a runtime built for it.

### The actual body: saturnkit `runtime/` — what's already there
`VDP1` (sprites/polys, rasterizer), `VDP2` (scroll BGs), `SCU`(+DSP), `SMPC`, SH-2 on-chip
(DMAC/DIVU/FRT), **HLE BIOS services**, the **68000 (Musashi)** + **SCSP** audio, an
SDL3/OpenGL window with pad/keyboard, headless mode, tracing, screenshots, WAV dump. All
Saturn/ST-V silicon is the **same chips**, so VDP1/VDP2/SCSP/SCU/SMPC/dual-SH-2 are reused
as-is.

### The glue that is actually needed — and it's ST-V-specific, not CPU-specific
Every proven port is a Saturn **disc** game; `runtime/main.cpp` **requires `--cue GAME.cue`**
and boots through IP.BIN/CD block. Our target boots from an **arcade cart** via the **ST-V
BIOS**. So the port's real engineering is a boot/BIOS shim:

1. **Cart boot harness** (new, replaces the disc path): map `cart.bin` at A-bus 0x02000000,
   **replicate the BIOS program copy** cart 0x200000→HWRAM 0x06004000 (we already *proved*
   it: single linear block, delta 0x05e04000 — `notes/rammap.py`), then enter the master
   PIC stub. **Low risk — the hard part (what the copy is) is already solved.**
2. **Security CRC**: the cart's CRC-16 check ends in a self-branch hang (`notes`
   @cart 0x200748). Must be HLE-stubbed/skipped or it deadlocks at boot. **Small, known.**
3. **ST-V BIOS services**: game calls BIOS via dispatch @0x03000000 and the fn-ptr table
   @0x06000300. saturnkit **already HLEs the Saturn SYS_* services** (`bios.cpp`:
   SETUINT/SETSINT/SETSCUIM/CHGSCUIM/CHGSYSCK…) and `discover` already tagged ours as those
   — big overlap. Gap: ST-V-specific service numbers and the program-copy call need adding.
   **Medium.**
4. **ST-V inputs**: arcade/JAMMA controls arrive through SMPC PDR1/PDR2 (our map:
   `smpc_pdr2_set/clr`), not a Saturn pad. Needs an input mapping in `smpc.cpp`. **Small–
   medium.**
5. **No CD block, no disc, no backup-RAM save** needed — those runtime modules go unused.

### Difficulty of the glue
- Dual-SH-2 sync: **done** (see (c)).
- Hardware cores: **done** (same silicon).
- ST-V cart boot + CRC stub + BIOS service deltas + arcade input: **the genuine new work.**
  Bounded and well-understood thanks to the existing notes, but **unproven with this toolkit
  — nobody has driven an ST-V title through it.** That is the single biggest unknown in the
  whole project, and it's a systems-integration unknown, not a recompiler unknown.

---

## (e) Honest overall verdict, next step, effort scale

### Verdict
**A hybrid-recomp native port of Die Hard Arcade is realistic and worth climbing.** The
half everyone fears — accurate SH-2 translation and dual-CPU sync — is the half that is
*already solved and demonstrated on our own binary today*. What remains is not research; it's
integration: standing up an ST-V cart-boot path on a runtime that currently only boots Saturn
discs. The framing in the spike brief ("borrow Xenia-style cores") is actually the *wrong,
harder* path; the right path is to ride the saturnkit/saturn-recomp runtime and build only
the ST-V shim.

Two caveats kept deliberately un-sugared:
1. **Zero ST-V precedent with this tool.** Boot/BIOS/input is untested territory. Expect the
   usual long tail between "boots" and "plays" — runtime-resolved indirect calls landing
   outside recompiled code, asset/decompress buffers, ST-V BIOS corners.
2. **"1059 functions compile" ≠ "game runs."** Compilation and leaf self-tests are proven;
   whole-program correctness is only provable once it boots. That gate is the ST-V harness.

### Next concrete step (one step, not a plan)
Build the **ST-V cart-boot harness**: a `main_stv.cpp` beside `runtime/main.cpp` that loads
`cart.bin`, performs the proven linear copy to HWRAM 0x06004000, stubs the security-CRC hang,
enters the master at the PIC stub, and runs **headless** (`--vblanks N`, `--trace`) with the
slave coroutine live. Success = it reaches `main_init_entry` → `frame_sync_loop`, installs the
vblank/timer ISRs, and the first VDP1/VDP2 register writes appear. That single milestone
converts the biggest unknown (ST-V boot) into a known, and lets the 1705 runtime-indirect
calls finally be validated. Recompile the **full** cart region (not just the main segment) at
the same time so indirect targets have somewhere to land.

### Rough effort scale (solo, honest)
- **Done already (this spike):** brain proven. ~0, it's in the bag.
- **First headless boot to frame_sync_loop + first VDP writes:** ~**1–3 weeks** (the ST-V
  cart/BIOS/CRC shim against an unfamiliar-but-well-documented runtime).
- **First rendered frame on screen (VDP1/VDP2 output, no input):** ~**1–2 months** cumulative.
- **Playable (input + sound + the long tail of indirect/asset bugs):** ~**4–8 months**
  cumulative, part-time; faster if upstream (saturn-recomp is committing *daily*) adds ST-V
  support or you collaborate.
- **Wildcards that could move those numbers:** ST-V BIOS behaviors we can't see without the
  BIOS ROM; any tight master/slave timing dependency; the size of the indirect-call tail.

**Recommendation: proceed to the boot-harness step.** It is the cheapest action that collapses
the one remaining big uncertainty, and the brain-half result says the foundation is solid.

---

### Reproduction (p620)
```
# tool:   /tmp/saturnkit  (git clone https://github.com/vs-sr-dev/saturnkit)
# input:  /tmp/diehard_main.bin = cart_fpr.bin[0:0x8de20]  (cart 0x200000..0x28de20)
PYTHONPATH=/tmp python3 -m saturnkit.recomp.discover /tmp/diehard_main.bin --base 06004000 --report
PYTHONPATH=/tmp python3 -m saturnkit.recomp --out /tmp/dh_recomp dh=/tmp/diehard_main.bin@06004000
cd /tmp/dh_recomp && g++ -std=c++20 -I/tmp/saturnkit/runtime -c p_dh_*.cpp modules.cpp   # 22/22 OK
# link selftest harness (core+stub, no SDL) + run oracle vectors:
PYTHONPATH=/tmp python3 -m saturnkit.recomp.selftest --out /tmp/dh_vectors.txt \
    --image dh=/tmp/diehard_main.bin@06004000 --test dh --auto
/tmp/dh_selftest /tmp/dh_vectors.txt      # -> 257 functions, 4067 vectors, 0 failures
```
