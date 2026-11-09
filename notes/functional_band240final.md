# Functional decomp - band 0x240000-0x248000 final pass

Scope: the remaining larger/ambiguous undone entries in the band. Each listed
target over-swallowed one or more trailing functions (the known `fn.extent`
trap); every real sub-entry was bound individually and decompiled.

Toolchain: `tools/decomp/venv/bin/python3` over fn.py / m2c_fn.py / braf_resolve.py
/ find_entry.py / decomp_bounded.py. Every file compiles
`gcc -O1 -c -o /dev/null -Wall -Wextra -I project/src_functional` with ZERO output.

## Files added (19)

| .c (SH2)   | from target   | bound as | role |
|------------|---------------|----------|------|
| 02241eb0   | 0x02241e82*   | [241eb0,242226) braf-tail | actor move/chase SM (unkD8 0..8) |
| 02242226   | 0x02241e82    | [242226,242272) rts split | snap actor to home pose |
| 02242272   | 0x02242272    | [242272,2423da) braf-tail | stagger/getup stepper (unkD4 0..3) |
| 02242410   | 0x02242272    | [242410,2424c4) braf-tail | thrown/launched stepper (unk10 0..2) |
| 022424c4   | 0x02242272    | [2424c4,242570) rts split | in-flight thrown handler |
| 022425d0   | 0x022425d0    | [2425d0,24288a) braf-tail | grenade/bomb stepper (unk10 0..7) |
| 022428bc   | 0x022425d0    | [2428bc,242a10) rts split | smoke puff stepper + draw |
| 02243066   | 0x02243066    | [243066,2433e8) braf-tail | grab/throw thrower SM (unkD4) |
| 022433e8   | 0x02243066    | [2433e8,2434ae) rts split | grabbed-victim overlay + draw |
| 02246308   | 0x02246308    | [246308,2463b8) bra tail  | wave spawner INIT entry |
| 022463b8   | 0x02246308    | [2463b8,2465fc) rts       | wave spawner loop handler |
| 02246678   | 0x02246678    | [246678,2467b4) braf-tail | spawned-actor initialiser |
| 0224680c   | 0x02246678    | [24680c,2469a0) braf-tail | approach/enter-screen handler (unkD4 0..1) |
| 022469a0   | 0x02246678    | [2469a0,2469f2) braf-tail | spawned-actor state dispatcher |
| 022473ee   | 0x022473ee    | [2473ee,2475a8) cmp/eq    | begin dash/charge move |
| 022475a8   | 0x022473ee    | [2475a8,247924) rts split | dash per-frame update (unkD8 0..3, nested) |
| 02247924   | 0x022473ee    | [247924,247a16) rts split | dash turn-around decision |
| 02247a16   | 0x022473ee    | [247a16,247a3e) rts split | dash recovery variant A |
| 02247a3e   | 0x022473ee    | [247a3e,247a66) rts split | dash recovery variant B |

\* census 0x02241e82 is a 14-byte literal pool (pool-contaminated); the real
function begins at 0x02241eb0 (find_entry confirms).

0x022401d6 (also on the target list) was already done (022401d6.c), verified and
left untouched.

Within the 0x02246678 extent, func_022469f2 was already done (022469f2.c); only
the three undone entries (246678, 24680c, 2469a0) were added.

## Findings / corrections

- **fn.extent over-swallow is the rule here.** Six of the eight target extents
  contained 2-5 real functions each, split at braf-tail epilogues or the next
  prologue (verified with braf_resolve + fn.py prologue/rts scans).
- **0x02246308 / 0x022463b8 are a two-entry / shared-body pair.** 02246308 is the
  first-frame init: it picks the start script record (direct pointer, or indexed
  via g_060907e0[~raw] with a g_0609856c-derived 0..3 selector), seeds the spawn
  globals, installs 022463b8 as the next-frame handler, and tail-jumps to it.
  Modeled as init + `func_022463b8(task)` (correct because the loop's would-be
  arg1/arg2 are dead - see below).
- **Dead-argument call sites (important for signatures):**
  - `func_02228106()` and `func_02210020()` take NO args (both overwrite r4 on
    entry); the r4-r6 set up before several call sites are dead.
  - `func_02215f6c(cmd)` takes only the sound command; r5/r6 at call sites dead.
  - `func_02227b60(template)` takes only the state-handler template; the spawn
    loop's r5/r6 (and thus 022463b8's nominal arg1/arg2) are dead.
- **func_0224a360(value, count) is the `r0 << r1` shift helper** (jump-table of
  `shll`, not the standard r4/r5 ABI). Call sites here compute `1 << playerIdx`
  to build a per-player slot bitmask (actor->unk68).
- **Installed handler pointers resolved:** 0x0224680c is installed by both
  func_02242272 and func_02243066; func_02246308 installs func_022463b8;
  func_02246678 installs g_060901b8[unk104]; func_022463b8 installs func_022465fc;
  func_022425d0 installs func_022462d0. All verified via mova+disp arithmetic.
- **No tst #imm8 sign-extension hazards** in any of these functions - masks are
  plain `and rN` / `tst rN,rN` against pool longs, so m2c's rendering is safe.
- Field width per access taken from m2c casts cross-checked against fn.py --asm
  (mov.b/mov.w/mov.l). Note offset 0x58 is a 16-bit heading in the actor FSMs
  (02241eb0/02243066) but a 32-bit counter in the projectile objects
  (022425d0/022428bc) - different object layouts, handled per-function.

## Deferred

None. All eight target extents are fully cleared; the band
0x240000-0x248000 has no remaining genuine undone entries in these clusters
(022469f2 and the small leaf/callC entries were already done in prior passes).
