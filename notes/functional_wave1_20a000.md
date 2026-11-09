# Functional decomp - WAVE 1, band [0x0220a000, 0x02213000)

Cart offsets 0x20a000..0x213000 (SH2 base 0x02000000). FUNCTIONAL (equivalence, not
byte-matching) decomp. Every file compiles `gcc -c -fsyntax-only -Wall -Wextra` with ZERO
warnings and was hand-verified against `m2c_fn.py --asm`. All use `#include "dh.h"` and the
M/MU/MP byte-offset accessors; no shared files were edited.

## Band census (from notes/function_triage.tsv, non-misseg real code)
- 119 real-code functions in band; 47 already done before this wave (address-named .c files).
- This wave added **24** new functions; **3** deferred with reason; ~45 remain.

## Functions added (24) - all HIGH fidelity, verified vs asm

### Camera framing subsystem (complete 6-handler state machine + helpers)
- `0220b3cc` / `0220b544` - near/tight camera: steady <-> approach pair
- `0220b8d4` / `0220ba48` - medium camera: steady <-> approach pair
- `0220be2c` / `0220bfa4` - far/loose camera: steady <-> approach pair
  All six: func_0220d9ec(cam,&box[3],&box[0]) fills an XZ min/max box; derive look-at point,
  distance-driven zoom, eye offset vector, eye position and pitch; smooth; X-clamp to unk84;
  shake tick; hysteresis handoff to the partner handler; tail func_0214a7a (matrix build).
  Computed branches resolved with braf_resolve; next-handler pointers with ptr_resolve.
- `0220e0aa` camera_smooth_lerp - eases eye pos/angles toward targets (leaf).
- `0220d98e` camera_shake_decay - recoil counter decay + Y nudge table (leaf).
- `0220e000` count_objects_in_rect - counts slots strictly inside an XZ rectangle (leaf);
  this is the visibility probe used by the two target-acquire handlers below.
- `0220cca0` camera_target_acquire / `0220cb5c` camera_target_acquire_leap - pick a player,
  bounds-check against the zone record, run the func_0220e000 probe, commit on success.
  (Note: 0220cca0 passes ent+0x9C for BOTH probe corners - a degenerate rectangle in the
  original binary; reproduced faithfully. 0220cb5c passes ent+0x90 and ent+0x9C.)
- `0220d70c` camera_follow_object - follow a tracked handle's X (double indirection via
  the object array), clamp to zone, sine-bob height (sin_lookup func_02213190).
- `0220e978` attach_to_parent_space - copy a source pos/angle triple into an object and add
  the selected parent's Z offset (parent chosen by the same mode selector as func_0220d9ec).

### Movers / spawn
- `0220b6b2` object_spawn_init - seed boss/object position+angles from a per-enemy template
  or fixed defaults, offset by the zone base record, reset shared spawn state, clear the
  debris slots, install func_0220b85c, build matrices (func_0214a7a).
- `0220d310` patrol_pingpong_mover - bounce an object between zone bounds at +/-0xE00
  (initial side chosen by rng_next func_02210020).

### Look-at angle builders (feed the sprite/transform pipeline)
- `0221055c` lookat_angles_xz (called by the done handler func_0220d262), `022105ca`
  (precomputed-delta variant), `02210622` (XY-plane variant). Use the out-of-band helpers
  func_022130b4 (atan2-style), func_022131c8 (fixmul 16.16), func_0213164 (sqrt 16.16).

### Resident-command coordinate registration
- `0220aa70` / `0220ab34` - two-phase fp(0/2) commands through *g_06000644 for four actor
  coordinate fields (siblings of the already-done 0x0220ac0c family, offsets +0x70.. / +0x74..).

### HUD / debug-text printers (cursor globals + print thunks)
- `0220ad58` entity position, `0220ade6` 6 slot u16s, `0220ae90` 4 zone records,
  `0220af94` a 5-field table record. The print thunk *g_060902A0 passes its args on the
  stack; modelled as a variadic `void (*)(s32, ...)` with the format string first.

## Deferred (3) - with reason
- `0220a9c2` - makes a non-standard-ABI sibling call (arguments set in r0/r1, not r4-r7) to
  out-of-band func_0224a120 whose signature cannot be verified from this side; the rest of
  the function (resident-cmd + slot select) is clear but the call is unverifiable.
- `0220cdf6` - contains a `bsrf r0` whose target register is produced by `stc sr,r0`
  (status-register value, not a literal-pool delta), so braf_resolve cannot resolve it and
  the dispatch target is indeterminate. Unverifiable.
- `0220d9ec` camera_compute_bbox - large (0x614): a switch with no default that seeds the
  min/max box from uninitialised callee-saved registers (hardware-undefined for the unused
  selector values) and a 4x-unrolled min/max loop with nested byte-table lookups
  (g_06052194 stride 0xC) requiring per-access asm verification. Its interface is already
  captured and exercised faithfully by the six camera framing handlers above, so decompiling
  the body is not blocking; left as high-value remaining work.

## Band remaining (~45, not attempted this wave)
Stage/debug HUD dumps with bsrf + loops: 0220a390, 0220a4f0, 0220a648, 0220a7dc.
Jump-table state machine: 0220b0c6. Huge (0x606): 0220c17c.
Fn-ptr dispatch + braf tail: 0220c782. Target loop (bsrf): 0220c964.
div1 math: 0220bc20, 0220d424, 0220d58c, 0220d8d0.
Mid/high callC (mac/div/dispatch, mostly 0x1d8..0x680): 0220e6d4, 0220e7c4, 0220ea06,
0220ebde, 0220ef02, 0220f0ec, 0220f28e, 0220f454, 0220f5b8, 0220f6f6, 0220f880, 0220f9d4,
022100f8, 0221039a, 0221047c, 02210690, 02210876, 022109d4, 02211108, 022114b0, 0221185c,
022119a4, 02211b04, 0221214a, 02212350, 02212b20, 02212c80, 02212e00.
(Most need braf/ptr resolution + careful asm tracing; several are table dispatchers.)

## In-band callees referenced but owned elsewhere / still undone
func_0214a7a (0x2214a7a, out of band - matrix build, called by all camera handlers),
func_02213190 sin_lookup, func_022130b4 angle, func_022131c8 fixmul, func_0213164 sqrt
(out of band), func_0220b85c (in band, installed by object_spawn_init - still undone).
