# Functional decomp report - band 0x02200400-0x02210000 (Opus pass)

Single serial worker, no forks. Band census filtered to triage callC/leafC AND
bytefrac_class==code, minus the done-set; 38 genuine-undone candidates. Decompiled the
5 densest/cleanest below (all compile clean under
`gcc -O1 -c -Wall -Wextra -I project/src_functional`, zero diagnostics).

## Added (.c in project/src_functional/)

| file | SH-2 | insn | fidelity | what |
|------|------|------|----------|------|
| func_0220e7c4.c | 0x0220e7c4 | 204 | HIGH | entity_slew_lookat_update: eased slew of a position triple + target triple over a cosine sweep, then lookat_angles |
| func_02203ed4.c | 0x02203ed4 | 368 | HIGH | fx_sprite_ring_task: rotating point-sprite ring with PRNG wobble (0 computed branches) |
| func_0220c964.c | 0x0220c964 | 143 | HIGH | actor_pick_spawn_near_object: slot scan for a clear spawn spot via count_objects_in_rect |
| func_0220376c.c | 0x0220376c | 456 | HIGH | hud_status_service_2p: 2-player status HUD rebuild (densest clean function in band) |
| func_0220a7dc.c | 0x0220a7dc | 195 | HIGH | continue_prompt_handler: "continue?" menu task (countdown + yes/no cursor) |

## Key findings / corrections

- **0x24a06c (sdiv32) ABI confirmed from its body**: dividend=r1, divisor=r0, quotient in
  r0. In 0x20e7c4 the divisor is loaded in the bsrf *delay slot* (`mov.w @(r0,r14),r0` with
  r0=0xAE), so unk74 = sdiv32(0x8000, (s16)unkAE) - NOT the delay-slot value as m2c guessed.
- **mova+add call targets** in 0x20e7c4 / 0x203ed4 resolve to the trig/fixmul library:
  0x213190 = sin_lookup, 0x2131a8 = cos_lookup, 0x2131c8 = fixmul_16_16. (r11=cos, r12=sin
  in 0x203ed4 - order matters; verified against pool deltas 0xf128/0xf108.)
- **0x203ed4 is fully clean (0 computed branches)**; the 2x loop unroll was re-rolled. The
  unk74<<11 step is an explicit shll8/shll2/shll, NOT a dropped register-count shift.
- **ui_glyph thunk *g_0609029C is void(*)(void*)** (single reg arg r4). m2c shows a spurious
  2nd arg (the g_060997A8 mode value left in r5) at every call site in 0x20376c/0x20a7dc -
  dropped. The cursor color/row travel through globals g_060997A4/A8, set before each call.
- **0x20376c shared vs per-player addresses verified byte-by-byte**: the warn-count nibble
  byte is the SAME (0x060987E9 = *0x060987D8+0x11) for both HUD halves; only the record
  (0x060987D8 / 0x060987DC), ready flag (0x060987D4 / D5) and column (0 / 0x16) differ.
- **0x20a7dc** sole bsrf = func_2093f8(r4=(s8)task->unk10 from the delay slot, r5=entity).
  Resident-command fill (fp *g_06000644) was 3x unrolled; re-rolled.
- **0x2153a0 = draw_point_sprite** (sprite display-list emitter) identified while tracing
  0x203ed4; added to symbols_band200.txt.

## Deferred (with reason)

- **0x20ebde (402), 0x20ea06 (236), 0x20ef02 (230)** - a camera-mode family sharing the
  0x20e7c4 slew logic, BUT each dispatches through a byte/long jump table
  (`mov.b @(r0,r9),r0; bsrf r0` and `mov.l @(0,r0),r0; bsrf r0`). Hand-resolving those
  tables is required to avoid silent-wrong; deferred as higher-risk than the time allowed.
  0x20ebde additionally calls 0x24a420 (ashr_var) - the known dropped-shift trap.
- **0x20a390 (147), 0x20a4f0 (143), 0x204362 (181), 0x20a612 (204), 0x20bbfa (221)** -
  HUD/score builders that route through the variadic ui_printf thunk *g_060902A0 and/or
  have prologues m2c mis-models (emits "Read from unset register" / "*NULL"). The variadic
  stack-arg lists (a known trap) need a full manual frame trace each; deferred to avoid
  guessing arg counts.
- **0x202126 (817), 0x20c17c (771), 0x20d9ec (778 leaf), 0x20e1e2 (614), 0x208b98 (595),
  0x207bcc (530)** - large, not yet triaged this pass; next-priority dense targets.

## Candidate bookkeeping
- 0x20734a and 0x203bfa in the raw census are pool-contaminated pre-entries of already-done
  functions (find_entry: -> 0x207360 done / 0x203de8); not real new entries.
