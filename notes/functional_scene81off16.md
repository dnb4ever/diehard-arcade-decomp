# Functional decomp + trace — scene-81 off16 handler func_02206c88 (rt 0x0600AC88)

Date: 2026-10-21. FUNCTIONAL (equivalence, not byte-matching) decomp + port trace.
SH2 = cart.bin offset + 0x02000000; runtime(HWRAM) = SH2 + 0x03E04000.
Compiles clean: `gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional
project/src_functional/02206c88.c` -> ZERO output, exit 0.

## TL;DR (honest headline)
`func_02206c88` is **not** the gameplay / round driver. It is the scene-81 **3D view-camera /
model-matrix intro-move handler**. The "stall at state 3" that motivated this task is the camera
intro **completing normally** — state 3 is a deliberately empty terminal case that just keeps
rebuilding the view matrix each frame. It installs no player and cannot sustain a round; the three
pinned gaps (BIOS start-enable, post-81 spawn scene, RBG0 finalize) are untouched by it. This
corrects the pin in round_unblock_Z.md that listed hdl 0600AC88 as "the plausible gameplay/stage
driver … stalls at state 3."

## 1. What it is (decomp)
ABI `handler(task)`, `obj = task->unk8`. The two computed branches resolve (braf_resolve,
trusted targets) to the functions that reveal its real job:
- `bsrf` @0x206cf8 -> **func_022105ca = lookat_angles_xz_delta(dx,dy,dz,&out_a,&out_b)** (aim solver)
- `braf` @0x206d46 (tail) -> **func_02214a7a = mtx_build_fwd_inv(obj)** (build forward+inverse view
  matrices into caches g_06098aec / g_06098abc from obj pos obj+8..10 and Euler angles obj+24..28)

So `obj` is the **camera/model-view object**, and this handler animates it, then rebuilds its
matrix every frame.

4-state cmp/eq switch on `task->unk10` (no jump table — faithful C switch):

| state | behaviour | exit |
|---|---|---|
| 0 | seed pose: X(8)=0x160000, Y(C)=0x20000, Z(10)=-0x100000, angle(24)=0xC000, ang(26/28)=0, unk64=0x4000 | ->1 (fallthrough) |
| 1 | **dolly Z in**: Z += 0x8000/frame; each frame solve aim via `lookat_angles_xz_delta(0x160000-X, -Y, 0x160000-Z, &obj.24, &obj.26)` (Z pre-incremented), then clear obj.26; when Z>=0x160000 snap angle=0xC000, clamp Z=0x160000 | ->2 |
| 2 | **crane Y up**: Y += 0xE000/frame, Z -= 0x2000/frame; when Y>=0x1E0000 clamp Y=0x1E0000 | ->3 |
| 3 | settled (== default): **empty body** | terminal |
| tail (all) | `mtx_build_fwd_inv(obj)` — rebuild the view matrices | every frame |

Approx final camera pose: X=0x160000, Y=0x1E0000, Z≈0x120000 (Z eases ~32 frames of -0x2000 while
Y ramps), angle(24)=0xC000. These feed the active forward matrix g_06098aec (and inverse
g_06098abc) used for the 3D object/sprite transform path (vec3_array_matmul of obj+8..16).

Fidelity HIGH: hand-traced vs `m2c_fn --asm`; m2c mis-merges this function with the adjacent
`func_02206d8c` (scene81_cam_update) because it cannot resolve the tail `braf` — the real prologue
of the next function sits right after this one's literal pool (0x206d4a..0x206d8a). The shipped
file is only func_02206c88 (0x206c88..0x206d48 + pool). The existing project file already carried
the same logic under the name `scene81_off16`; this pass re-ships it under the requested symbol
`func_02206c88` with the camera role proven and the port trace added.

## 2. Why it "stalls" at state 3 (the trace)
It does not stall in the blocking sense. **State 3 waits on nothing** — the case is empty, and no
code (here or elsewhere) ever writes `task->unk10` beyond 3. Reaching state 3 simply means the
two-phase camera move (Z-dolly then Y-crane) has finished; the handler then only rebuilds the view
matrix each frame. There is no flag, actor, render/anim-completion, published context
(g_06099590/g_06099764), or START signal involved in this handler at all.

Port confirmation (`dh-portA`, read-only, default-off probe only):
```
STV_BOOTMODE=1 STV_SM81DIAG=81:4 ./recomp-build/saturn_stv --cart diehard_main.bin \
  --cartrom ../cart.bin --vblanks 5700 --out /tmp/sm81b --stvinput '200:COIN'
```
One coin walks the reel to scene 81 (VB~5176). The off16 node (node 060922A4, hdl=0600AC88,
camera ent=060E13B0) shows: already at st=1 by first sample VB5176, st=1->2 then st=2->3 within the
first ~55 frames, then **PARKED at st=3** for the entire remaining scene-81 lifetime until the
director tears scene 81 down (~VB5484). Throughout, `ent.3c = 0` (no busy/wait bit — this object is
the camera, not an intro object). This matches the decomp exactly: a terminal state 3.

(Side note: this run reuses the previously-found single-coin reset-timeout path to 81. That path
jumps straight to 81 skipping setup 76-80, but off16 is installed by the scene-81 build regardless
of path, so the camera still runs faithfully.)

## 3. Does it install the player / sustain the round? No — and what actually unblocks
`func_02206c88` contains no player-spawn, no slot registrar, no START handling, and no director
write. It cannot install the player or sustain the round; it only drives the camera. It is a
sibling of the rendering gaps, not the spawn gap. Tying back to the three pinned gaps:

1. **BIOS start-enable (0x06098798 bits 4/5):** untouched here. Still required by scene81_game_sm
   (func_02206876), not by off16.
2. **Post-81 player-spawn scene:** untouched here. The player must spawn in the scene the director
   advances INTO after scene 81 (scene 81.next44=0 -> attract today). off16 is the wrong handler
   for this — it is the camera.
3. **RBG0 stage finalize (camera My off-map / floor planes):** off16 is the nearest thing to a
   camera finalizer, but it drives the **VDP1 3D object view matrix** (g_06098aec/abc), **not** the
   **VDP2 RBG0 rotation parameters** (the floor plane/My=1640 off-map issue in round_unblock_Z).
   Those are a separate matrix path; off16 completing correctly does not finalize RBG0.

## 4. Runtime edits
None. The trace used only the pre-existing, default-off `STV_SM81DIAG` probe plus `--stvinput`.
No source, header, tool, or git changes.

## 5. Precise next step toward a sustained round
Stop treating hdl 0600AC88 as a round-loop candidate — it is proven to be the view-camera and it
completes. Return to the real crux from round_unblock_Z.md's DIRECTOR-COUPLING RESULT: the round
is gated on BIOS/system-synthesized signals absent from cart code. Concretely, next:
- (A) **Post-81 spawn home:** decompile/trace the handler the scene director installs AFTER scene 81
  (scene 81.next44 target) — that, not off16, is where the player-character actor must spawn. The
  credit-join chain (022045f8->02228168->02229c00->02229df4) proves the actor+start-flag+context
  path works; it just never writes the director index. Find/synthesize the credit->director couple.
- (B) **RBG0 finalize** (separate from off16): fix the RBG0 plane-map (floor rows point at empty
  planes 0x38/0x2E) and the rotation camera My (off-map 1640) so the playfield is not black — this
  is the VDP2 path, independent of off16's VDP1 view matrix.
- (C) **START gate** (0x06098798 bits4/5) via the existing harness knob while real BIOS emulation
  is absent.

off16 itself needs nothing further — it is DONE and correct.
