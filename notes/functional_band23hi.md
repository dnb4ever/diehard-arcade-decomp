# Functional decomp report - band 0x02237000-0x02240000 (high 0x23xxxx)

Worker: band23hi. All files written to project/src_functional/, each verified
against m2c_fn.py --asm, every computed branch confirmed via braf_resolve.py,
and compiled with `gcc -c -fsyntax-only -Wall -Wextra` (ZERO warnings).

## Added .c files (12)
| file | summary | fidelity |
|------|---------|----------|
| 02237ea6.c | HUD damage/score popup updater; two symmetric slots factored into a static helper hud_popup() | HIGH |
| 02238364.c | table-slot (0..7) dispatch; tail-calls g_0609029c or func_0216858 | HIGH |
| 02239194.c | debris sprite per-frame state machine + trig/draw tail | HIGH (MED on terminal draw-helper arg shapes) |
| 0223a888.c | polar->cartesian motion projection (trig helpers) | HIGH |
| 0223c7f6.c | actor action/animation state machine (switch on unk6D) | HIGH |
| 0223cd98.c | sweep 8 active-effect slots, retire finished ones (source was 4x unrolled; rewritten as a loop) | HIGH |
| 0223d0fe.c | begin fixed action #1 on an actor (specialized sibling of func_0223cfe4) | HIGH |
| 0223d3d0.c | falling/arcing object; spawns impact object + func_021055c on landing | HIGH |
| 0223d558.c | falling object that triggers landing sound/rumble, transitions to handler 0x0223101c | HIGH |
| 0223d9dc.c | projectile/grenade update; fuse -> detonate spawns debris | HIGH |
| 0223fec4.c | particle spawner looping over a 16-byte-entry template list | HIGH |
| 0223ffcc.c | reset entity fields, select+invoke action handler via 2-level table | HIGH |

## Key findings / corrections
- find_entry corrections applied (triage first-member was pool-contaminated):
  0x2371aa->0x2371c0, 0x22382e8->0x238364, 0x2239ce6->0x239ce8,
  0x23a856->0x23a888, 0x23a9b8->0x23a9d0, 0x23d3ae->0x23d3d0,
  0x23d726->0x23d768, 0x23d98a->0x23d9dc. The triage addresses 0x23805a,
  0x2239ce2, 0x23de18, 0x23e3ba are phantom/pool rows (real=False).
- g_06098878 is a draw/transform offset accumulator: handlers do
  g_06098878 += 0x30 before, and -= 0x30 after, the terminal draw-call group
  (func_02149aa/0214a40/0214a06/0215a2c). This push/pop wraps the per-object
  render in 02239194/0223d9dc/0223d3d0/0223d558.
- g_060e0f0c / g_060e0f2c / g_060e0f4c are three parallel effect-slot arrays
  (state, substate, object ptr) all based at g_060e0e88+0x84/0xa4/0xc4.
- The falling-object family (d3d0/d558/d9dc) share the save-prev-pos +
  integrate-velocity + func_022173f4 pattern and a common draw tail.
- func_02149aa receives no register set by its callers in this band
  (operates on g_06098878 global state); modelled as void(void).

## Deferred (with reason)
- 0x239fc6 (0x3ce) and 0x23a3a2 (0x33a): the two large action apply/cancel
  handlers. Fully decodable (standard-ABI calls) but dense multi-switch
  bit-manipulation; deferred for a focused pass to guarantee zero logical
  errors rather than risk them in a wide batch.
- 0x23db2a (0x198) and 0x23d768 (0x218): both call the func_024a420 /
  func_024a518 / func_024a7a8 / func_024a830 helper family, which takes its
  arguments in r0/r1 (nonstandard register ABI, confirmed by disassembling
  0x24a420). This cannot be expressed faithfully in functional C.
- 0x2399bc / 0x239462 / 0x23fcb6: m2c could not lower these (table-driven
  bsrf dispatch via mov.b @(r0,rN), and a `stc tbr` instruction); need manual
  reconstruction of the jump tables.
- 0x23cfdc (8 bytes): degenerate epilogue-only stub (add #4,r15; restore
  pr/r14; rts) with no body - not meaningful as functional C.
- Remaining large genuine functions still open: 0x2371c0 (0x668), 0x237828
  (0x3a4), 0x237be4 (0x22a, string formatter), 0x23b28a, 0x23b5d8, 0x23b9b8,
  0x23bcd4, 0x23bf86 (0x6a8), 0x23c95c (0x254 init table-copy), 0x23e690
  (0x78a), 0x23f10e (0xaf8).
