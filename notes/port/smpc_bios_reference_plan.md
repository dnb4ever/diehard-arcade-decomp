# SMPC + BIOS reference plan — what the real ST-V provides vs our HLE (2026-10-22)

Reference-study only. No runtime changes. Studied for BEHAVIOR, re-implement never copy
(MAME = GPL, Mednafen = GPL; our repo is PUBLIC). Sources fetched read-only on p620:
`~/decomp/refs/mame/src/mame/sega/{smpc.cpp,smpc.h,stv.cpp}` and
`~/decomp/refs/mednafen-src/mednafen/ss/smpc.c`.

## TL;DR verdict (answers deliverable #3 up front)

**Delivering more SMPC/BIOS *input* signals is NOT the lever that unblocks executive-PLAY-on-a-
live-stage.** The reference study CONFIRMS the opposite of the working hypothesis: the input /
coin / credit / start signal layer the Die Hard cart actually consumes is ALREADY faithfully
HLE'd in `bios.cpp` + `smpc.cpp`, and is proven to deliver (faithful ENTER-PLAY scene 81
reached, a live P1 actor spawns + moves, stage bundle DMAs). The decisive new fact from the
references is an **architectural** one that the prior notes half-stated:

- On real ST-V the control panel does **not** go through the SMPC peripheral (INTBACK) path.
  It goes through a **separate chip, the Sega 315-5649 IOGA at 0x00400000** (JAMMA PORT-A/B/C).
  The SMPC's only ST-V input role is a couple of mirror bits on PDR1/PDR2 (gun trigger,
  test/service mirror) — see MAME `stv.cpp` INPUT_PORTS `PDR1`/`PDR2` (lines ~1509-1529) vs the
  real panel on `PORTA/PORTB/PORTC` (lines ~1531-1545) wired to `SEGA_315_5649` (line ~1218).
- **But the Die Hard cart reads NEITHER the SMPC NOR the IOGA as memory.** Verified: the cart
  recomp has 17 references to the constant `0x00400000`, and EVERY one is a *store of the
  constant into a descriptor field* (e.g. `p_dh_005.cpp` @0x060200F0 writes it into a config
  struct; `p_dh_013.cpp` f_060407F6 stashes it as a pointer). There is **zero** `ld8/ld16/ld32`
  load from the `0x0040xxxx` region anywhere in the recomp. The cart consumes input entirely
  from **BIOS-published work RAM**: raw words 0x06000708/0x06000710, the 0x06000660 panel
  service, and the credit/arm/join/start bytes in 0x060986xx. The real BIOS is the thing that
  reads the IOGA+SMPC and publishes those — and that publishing is exactly what our HLE already
  substitutes.

So the SMPC/BIOS input surface is, for this cart, a *solved* problem. The remaining wall is a
**cart-internal state-machine coupling** (executive <-> scene-director decoupling + per-actor
live/draw gate + RBG0 camera anchor), which the real BIOS would influence only indirectly
through boot-state/timing, not through any discrete peripheral signal. And the real-BIOS boot is
independently blocked (RAM-relocation, see real_bios_spike.md). This is the same plateau the last
five passes converged on; the reference study rules OUT "we're missing an input signal" as the
explanation and pins the lever elsewhere.

---

## 1. What the real ST-V SMPC + BIOS provide, vs our HLE

### 1a. The input topology (the key reference finding)

| Signal | Real ST-V source | Reaches the cart via | Our HLE |
|---|---|---|---|
| P1/P2 joystick + fight buttons | IOGA 315-5649 PORT-A (0x00400001) / PORT-B (0x00400003) | BIOS reads IOGA, cooks, writes raw words **0x06000708 / 0x06000710** | `bios.cpp` STV_RAWINPUT writes 0x708/0x710; cart's own cook func_0220f9d4 decodes. **Present.** |
| Coin 1/2 | IOGA PORT-C bit0/1 (0x00400005) | BIOS coin service 0x06000664 -> credit bank; coin-pending 0x0609880C | `bios.cpp` deliver_coin + stv_coin_read + g_stv_credits. **Present.** |
| Start 1/2 | IOGA PORT-C bit4/5 | BIOS panel report buf[3]/buf[7]==2 (0x06000660 service) | `bios.cpp` 0x06000660 service + stv_input_script START1/START2. **Present.** |
| Service / Test | IOGA PORT-C bit2/3 + SMPC PDR mirror | BIOS config word 0x06000798 bit5 | `bios.cpp` 0x06000798 held at 0x60 (normal, sound on). **Present (as constant).** |
| P1/P2 control-enable | **BIOS/SDK-synthesized, NOT a peripheral at all** | work word 0x06098798 bits 4/5 (and bit2 poll-enable) | `bios.cpp` stv_system_bios_tick re-asserts 0x34 each frame. **Present (as constant).** |
| RTC / region / system status | SMPC INTBACK OREG | 0x06000234+ status area | `smpc.cpp` intback_status(). **Present.** |

The SMPC peripheral (INTBACK) path in our `smpc.cpp` is the **Saturn** controller model —
status/id/ctrl bytes, `collect_peripheral()` / `send_peripheral()` — which faithfully mirrors
MAME `smpc.cpp::read_saturn_ports()` (lines 824-861: `status1`, per-pad `id`, `id&0xf` ctrl
bytes) and `resolve_intback()` / `intback_continue_request()` (the SR 0x40/0xC0/0x80 + PDE
staging, lines 602-694). Mednafen `smpc.c` confirms the same INTBACK OREG layout
(OREG[0]=RTC-valid|!NMI, OREG[1..7]=RTC, OREG[9]=area, OREG[0xA]=status; smpc.c ~1333-1347).
**Our intback_status() already matches this.** This path is essentially unused by Die Hard
(which reads BIOS work RAM, not INTBACK), so its fidelity is not on the critical path.

### 1b. BIOS work-RAM / boot state the cart expects post-handoff

The references don't add anything our HLE lacks here for the signals the cart consumes. The
items the real BIOS seeds that matter (all already in `bios.cpp`): 0x06000798 config, 0x06000654
insert-coin countdown (decremented per frame), 0x06000640 frame timer, 0x06000664 coin service,
0x06000660 panel report, 0x06098798 control-enable. The ONE piece whose hardware mechanism is
genuinely not pinned (flagged in bios.cpp ~327-329): *how the BIOS learns the game's report
buffer address* to write 0x06098798 bits 4/5. The reference resolves the *provenance* question —
these bits are **not** from the SMPC or IOGA; they are BIOS/SDK boot state — so hard-asserting
0x34 is the correct clean-room substitute, and there is no cleaner peripheral source to read.
Confirmed correct to leave as-is.

### 1c. Boot handshake the HLE skips (reset/SYSRES)

MAME `smpc.cpp` (command_register_w 0x0D, m68k_reset_trigger) and Mednafen `smpc.c`
(CMD_SYSRES ~1285, ResetButton* ~1219-1241, SMPC_Reset ~616) show SYSRES as a **boot reset**,
not a console stop. Our `smpc.cpp` treats 0x0D (SYSRES) as `sat_stop` (line 280) because the
HLE harness deliberately bypasses BIOS boot. This only matters if we ever run the real BIOS
(real_bios_spike.md gap B); it is irrelevant to the HLE PLAY path, since the cart is handed off
already-booted. Not on the critical path.

---

## 2. Concrete port plan — minimal SMPC/BIOS re-implementations, by impact

Ordered honestly. The top entries are tiny because the layer is nearly complete; the real work
(items 4-5) is NOT SMPC/BIOS and is listed to be explicit about where the lever actually is.

1. **(Trivial, optional) Nothing required on the input-signal surface.** The study found no
   missing SMPC/BIOS input signal the cart consumes. Any further SMPC INTBACK fidelity is dead
   code for this cart. Recommendation: do not invest here.

2. **(Small, correctness) Make 0x06098798 bits 4/5 provenance explicit, not a magic constant.**
   Already functionally correct (0x34 each frame). Only a documentation/robustness nicety:
   key the re-assert to "2-player cabinet config" so it reads as faithful BIOS boot-state rather
   than a knob. Low impact, zero risk. Re-implementation (not a copy of anything).

3. **(Small, only if real-BIOS path is ever revisited) SYSRES-as-boot-reset semantics in
   smpc.cpp**, per MAME/Mednafen behavior (set OREG31=cmd, re-enter reset vector preserving
   memory) — this is the shim already prototyped in the bios_spike. Irrelevant to the HLE PLAY
   path; include only if adopting real-BIOS boot. Re-implement from behavior.

4. **(The actual lever, NOT SMPC/BIOS) Executive -> scene_run-on-a-live-STAGE coupling.** The
   executive (*g_06098678: attract_handler 0x02204868 -> round_controller_dual 0x02205232 ->
   scene_run_handler 0x02205812) must morph to scene_run while the DIRECTOR sits on a gameplay
   STAGE scene (2..72) whose descriptor +0x1C spawn-list builds the persistent fighters. Today
   the director parks on attract-fade scene 3 (arm=1 freezes the fade reel) and the reset-timeout
   jumps straight to terminal scene 81. This is cart-internal director/executive arbitration
   (the two-task arbitration already partly built: STV_PLAYROUTE / STV_JOINHOLD). The real BIOS
   does not "send a signal" here — it drives the timing/boot-state that lets the setup-scene
   selection game-state complete so the director lands on a live stage. Re-implementing THAT
   selection game-state in HLE is the bounded-but-substantial sub-project.

5. **(The second lever, NOT SMPC/BIOS) Per-actor live/draw gate + RBG0 camera anchor.** VDP1
   sprite-emit gate unk4C (actor_main_state_step 0x02229074) and the RBG0 stage-camera anchor
   never set on the HLE path. These are downstream of item 4 (they need the live round), also
   cart-internal, also not a peripheral signal.

Items 4-5 are the convergent wall every prior pass hit. The SMPC/BIOS reference study's
contribution is to prove they are NOT masked input-signal gaps.

---

## 3. Honest assessment — is this the right lever?

**No — not the SMPC/BIOS input layer.** The reference study changes the diagnosis in a useful
way:

- The working hypothesis entering this task ("absent BIOS-synthesized signals the cart consumes,
  our raw buffer 0x06000708 is zero") is **already resolved** in the current HLE and confirmed
  complete by the references: the cart consumes BIOS work RAM, our HLE publishes it, and it works
  up to ENTER-PLAY. The study found **no additional SMPC/BIOS input signal** the cart reads that
  we omit. (Proof: cart never loads the IOGA/SMPC as memory; all 0x00400000 uses are descriptor
  setup; input flows through 0x708/0x710/0x660/credit bytes, all HLE'd.)

- The genuine remaining wall is a **cart-internal state-machine coupling** (items 4-5), which the
  real BIOS influences only through boot-timing/state, not a discrete signal. So "deliver the
  missing signal" is the wrong frame; "complete the setup-scene selection game-state so the
  director lands on a live stage, then satisfy the per-actor live gate" is the right one.

- Does it still require the real BIOS? **No, and the real BIOS is not even available as a
  shortcut** — real_bios_spike.md proved the real-BIOS boot is blocked by RAM-relocation
  (f_00003576 jumps into empty WORK RAM; the BIOS builds/relocates routines into RAM a static
  recompile of the 512KB ROM can't cover) and halts in low-level boot far before any cart
  director logic. The recompiled BIOS oracle cannot even observe the attract->setup routing. So
  neither "more SMPC/BIOS signals" nor "run the real BIOS" is the path. The path is continuing
  the HLE re-implementation of the two cart-internal couplings (items 4-5), which is faithful,
  bounded per-coupling, and the only option that actually reaches the live round.

Mapping to prior passes: this agrees with scene81_sustain_wall.md UPDATE-e ("both blockers are
the SAME convergent absent-BIOS-synthesized couplings ... the recompiled-BIOS oracle cannot reach
them") and 2026-10-22d ("the missing piece is the BIOS-synthesized credit/arm/start-enable
SIGNALS the cart director consumes"). The refinement this study adds: those credit/arm/start
signals are **already delivered**; what the director still lacks is not a signal but the
completed selection game-state (item 4), which is HLE work, not a BIOS/SMPC input.

### License hygiene
MAME (GPL) and Mednafen (GPL) were read for behavior only. Nothing was copied; the port's
`smpc.cpp`/`bios.cpp` are clean-room (their comments describe hardware behavior, not MAME code).
Any item in the plan above is a re-implementation from documented behavior. Reference clones live
in `~/decomp/refs/` outside the port tree and are gitignored/not committed.

### Reference artifacts (p620, read-only, not committed)
`~/decomp/refs/mame/src/mame/sega/{smpc.cpp,smpc.h,stv.cpp}` (sparse MAME clone),
`~/decomp/refs/mednafen-src/mednafen/ss/smpc.c` (Beetle-Saturn). Our HLE under study:
`dh-portB/tools/saturn-recomp/runtime/src/machine/{smpc.cpp,bios.cpp}`.
