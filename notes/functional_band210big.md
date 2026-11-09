# Functional decomp report - band 0x210000-0x220000 (worker 210big)

Header: `dh_band21.h` (absolute accessors MEM8/16/32, MEMU16/32, PTR). All files in
`project/src_functional/`, each compiles `gcc -O1 -c -Wall -Wextra -I project/src_functional`
with **zero output** (unused-param / uninitialized reads would fail). Names/globals in
`notes/symbols_band210big.txt`.

## Delivered (12 functions, all HIGH fidelity, zero-warning)

| SH2 addr | file | size | role |
|----------|------|------|------|
| 0x0221f088 | 0221f088.c | 171 | evt_sequence_step - 5-state scripted-event step |
| 0x0221c690 | 0221c690.c | 212 | boss_intro_director - 9-state timed spawn/aim sequence |
| 0x0221c4ac | 0221c4ac.c | 242 | prop_open_close_sweep - 2-panel door swing + draw |
| 0x0221a1a0 | 0221a1a0.c | 345 | hud_input_prompt - HUD timer + 2-button poll |
| 0x02219f26 | 0219f26.c  | 317 | prompt_hud_orbit - orbiting prompt HUD + 3-button poll |
| 0x02219c44 | 0219c44.c  | 372 | prompt_hud_rng - rng-threshold accept-key variant |
| 0x0221898c | 0221898c.c | 308 | gate_swing_update - 7-state sin gate swing + draw |
| 0x0221bf72 | 0221bf72.c | 412 | setpiece_director - 9-state encounter director |
| 0x0221d6b6 | 0221d6b6.c | 469 | launch_arc_segment - ballistic segment walker (hand-reconstructed) |
| 0x02216c38 | 0216c38.c  | 527* | stage_init_dispatch - small dispatcher (see note) |
| 0x0221a80c | 0221a80c.c | 678* | oscillator_arm_phase1 - sinusoidal swing arm (1 of 3-fn lump) |
| 0x0221e256 | 0221e256.c | 704 | coop_lever_director - 2-player lever (varying-arg calls) |

(* census sizes for 0x216c38 and 0x21a80c count a lumped fn.unit, not the real function.)

## Targets NOT done (already covered elsewhere - verified, not duplicated)
- **0x02212b20** (473): already present as `02212b20.c` (built by the dh.h worker,
  actor_update_sway). Skipped to avoid duplication.
- **0x0221cf56** (272): `find_entry.py` redirects it (pool-contaminated) to real entry
  **0x0221d176**, which already exists (`0221d176.c`). Skipped.

## Key findings / corrections
- **0x216c38 is a tiny dispatcher, not a 527-byte function.** fn.units lumped three real
  entries into one unit. The real func_0216c38 only latches two params, seeds the camera
  anchor 0x0609742C from 0x0606080C[stage], then bsr 0x216c88 and bra 0x216f7a (tail call).
  Both 0x216c88 and 0x216f7a are confirmed-real separate entries; they carry the big loops
  m2c mis-attributes to 0x216c38. m2c also drops the macl saves; fn.py shows them.
- **0x21a80c is a 3-function lump.** Three sibling state machines each end with a braf
  tail-jump to the shared tail 0x246282. func_0221a80c is only the FIRST (switch 1); the other
  two are separate entries not in scope. The task-noted varying-arg indirect calls are NOT in
  func_0221a80c (its calls are all direct bsrf to known helpers).
- **0x21d6b6 needs hand-reconstruction** (m2c aborts: "Cannot find branch target .L21d816").
  Function ends with a braf tail-jump to 0x246282; the trailing literal pool is then mis-swept
  into a phantom label. Rebuilt from fn.py + braf_resolve; tail-jump modelled as a final call.
- **Shared tails 0x246282** (braf target of 0x21d6b6 and 0x21a80c) and **0x22ca98** (braf
  target of 0x21e256) are real entries; tail-jumps modelled as the final call of each function.
- **Varying-arg indirect calls (0x21e256, 0x21bf72)**: r10/r11 is a loaded sound_queue_push
  (0x215f6c) function pointer; m2c reports phantom extra args (it sees nearby r5/r6/r7 writes).
  Each jsr @rN sound call actually passes 1 arg (r4). The genuine varying-arg site is 0x21e256
  case 0: func_0221707c(ent,0,0,0,0) is a real 5-arg call (4 in r4-r7 + one pushed to stack,
  with add #4,r15 cleanup); r11 is reused as the 0 source there.
- **mova+add PIC pointer args** (ptr_resolve) hand-resolved: 0221c690 -> 0x237828/0x207bcc/
  0x219c44; 0219f26 & 0219c44 -> 0x21a6a8; 0221bf72 -> 0x215f6c/0x21c2aa/0x219f26/0x237828/
  0x238fdc; 0221e256 -> 0x215f6c/0x219984.
- **ashl_var/ashr_var** (0x24a360 / 0x24a420) called as externs func_0224a360(value,count) /
  func_0224a420(value,count) (r0/r1 ABI not inlined, per band rules).
- **sin_lookup 0x213190 / cos_lookup 0x2131a8 / fixmul_16_16 0x2131c8** are leaf helpers; the
  shifts of their results are arithmetic (shar in ROM).

## Not deferred
All reachable band targets are done. The only two not written (0x212b20, 0x21cf56) are already
covered elsewhere and were deliberately not duplicated.
