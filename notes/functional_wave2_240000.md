# Functional decomp - Wave 2, band 0x240000-0x248000 (cart) = runtime 0x02240000-0x02247fff

Dual SH-2 (big-endian), equivalence-oriented (not byte-matching). Every file below compiles with
`gcc -c -fsyntax-only -Wall -Wextra` producing ZERO warnings, and each was hand-verified against
`m2c_fn.py --asm`. PIC branches resolved with braf_rewrite; PIC data/function pointers resolved with
ptr_resolve; globals/regs declared per-TU. find_entry was run on every target first (band has real
pool contamination and several intra-function labels mis-listed as units - see "resisted" below).

This band is actor-combat AI: attack selection, reach/aim tests, effect/projectile spawning, and the
per-frame behaviour dispatcher.

## Functions ADDED this wave (15 entries in 14 files, all HIGH fidelity)

Behaviour dispatch / per-frame:
- `022401d6` func_022401d6 - actor behaviour re-entry. Resets unk48/41/88, looks up the sub-state handler
  `g_060918A0[unkBA & 0x3F][unk108]` and calls it as `fn(actor, &frame)` where `frame` is a 12-byte stack
  block passed in r5 (`mov r15,r5`); the handler writes `frame[0]`, which is both forwarded to
  func_022418fa and returned. Pending-hit path (unkFC!=0 && unk56<0x96) runs reaction 0x245c88 and stamps
  g_0609624A[unk34<<5]=0x295. The indirect call's r5=&frame is the key detail m2c mislabels as "sp".

Effect / projectile spawning (allocator family 0x0220ff28 / 0x0220feec, PIC templates):
- `02241a10` func_02241a10 - spawn & aim a muzzle-flash/shot. Allocator 0x0220feec, template `0x0221279c`
  (ptr_resolve). sin=0x213190, cos=0x2131a8, fixmul=0x2131c8 (consistent with 0x022415ee). rng value is
  diced into three 12-bit signed jitters (unk2A/2C/2E), r>>8 arithmetic, r>>16 via shlr16+exts.w.
- `022434ae` func_022434ae - spawn a burst: `count=arg2->unk0` entries from the `arg2->unk4` velocity list
  (0x10 stride), allocator 0x0220ff28, template `0x022324da`. Position = actor pos + actor delta.
- `02243590` func_02243590 - spawn one falling item/pickup, allocator 0x0220ff28, template `0x02232526`.
  Velocity.z = -arg2; unk3C=1 only when `!(arg1->unk5C & 3)`.
- `02243db4` func_02243db4 - commit a dodge/step in direction arg2. Blocked-lane flip via
  g_06096EC0[dir]==0x71 -> dir^=2; spawns the move actor through func_02244820 and parks the returned
  handle in g_06097128[unk34] (0x24 stride, pointer slot).

Attack decision / reach tests:
- `0224519e` func_0224519e - per-frame "start an attack?" vs the latched target in slot unk120. Per-distance
  attack table at unk168, timing gate func_02245fc4, throw check func_022452be. Returns 0/1/2.
- `02244e54` func_02244e54 - sibling of 0x0224519e that keys off the facing side (sign of dx) and returns a
  richer action 0..3 (kind 1 extra-guards on target->unk84 & 0x40D; kind 2 can escalate to 3).
- `02242fde` func_02242fde - lunge permission test: z within +/-0x6666, timing gate clear, and the
  target-select result (func_022452be) is not a grabbed/downed actor (unk34 in [3,6], !(unkE0&0x200000)).
- `022452be` func_022452be - reach/aim test. Combat boxes in g_060B6D9C (0x48 stride); builds a 7-word probe
  block on the stack and calls segment-query func_02217838 with `&blk[0x14]` (pos) and `&blk[4]` (aim);
  returns the struck object (hit->unk8) or 0. Half x-distance via arithmetic shar.
- `022454e4` func_022454e4 - pick the next motion/action index and feed func_02244988. Facing comes from
  unk4D bit1; an idle special-case (unkE4==6, unk6A<3) returns 5/6 early; otherwise func_02245632 fills
  (action, side) out-params, func_02245a6a may flip the turn bit, and the id is `unk16C[action][side]`
  (two-level table) or g_06091A3C[side] for action 4. Returns the action.

Small state helpers:
- `02245a6a` func_02245a6a - resolve the active facing/side flag from global combat mode g_06098564 and the
  actor's slot g_060995D0[unk34*0x20] (modes 0..3 + default).
- `02245d1c` func_02245d1c - try to strike the target latched in slot unk12C via the parallel arrays
  g_060E0F0C/2C/4C and point_in_box (func_0224492c). (The m2c "unit" 0x02245d68 is an internal label
  inside this function, not a separate entry - see resisted.)
- `02246180` func_02246180 - rng-driven direction pick from actor->unk154[(r>>2)&3]; fallback unk148.
- `02246254` func_02246254 + `0224627e` func_0224627e - a sound/voice task state-step with TWO ROM entry
  points sharing a tail at 0x246282. 0x0224627e is the next-frame handler (reads task->unk8 and fans the
  object out to four subsystem updaters under a depth guard g_06098878 += / -= 0x30). 0x02246254 is the
  install entry: it packs task->unkF/unk12 into obj->unk30, writes 0x0224627e as task->unk0, then runs the
  body. Modelled as two functions; the setup calls the shared body (equivalent since task->unk8 is stable).

## Resisted (deferred, with reason)
- `02241b9c`, `02244d6a`, `02245bbe` - find_entry reports real=False (pool/data contamination inside the
  unit); not genuine entries. Skipped per "skip misseg/data-as-code".
- `02244e50`, `0224537a` - 4-/8-byte "functions" that disassemble to a bare `rts; mov.l @r15+,r14`
  (shared epilogue fragments of the preceding function), not standalone entries.
- `02245d68` - sits INSIDE func_02245d1c's extent (unit 0x245c76-0x245db2 lists it out of order); it is an
  internal branch target, decompiled as part of 0x02245d1c.
- `02402c0` (58 branches), `0x02246678`/`0x02246ab8`/`0x02247106`/`0x02247e14`/`0x0243628` etc. - large
  table/pointer-dispatch state machines (each has >=1 `mov.b @(r0,r8)` / `bsrf r0` jump table that
  braf_rewrite correctly refuses to collapse). These need the 0x24a300-0x24a520 dispatch band (out of
  range for this wave) and switch/M2C_ERROR stubbing; left for a dispatch-focused pass.
- `02447d4`, `02244820`, `02244670`, `0x02244fdc`, `0x02243be0`, `0x02245632`, `0x02246308` - complete m2c
  but with table-jumps, M2C_ERROR tail-dispatch, uninitialised-stack reads, or very intricate
  global-pointer arithmetic; defer-with-reason rather than ship unverifiable (quality over count).

## Band remaining (real entries still UNDONE in 0x240000-0x247fff)
240054, 2402c0, 2418fa, 241eb0, 242272, 2425d0, 242a10, 243066, 243628, 243be0, 243e22, 243fe0, 2444b6,
244670, 2447d4, 244820, 244fdc, 245632, 2458ec, 246308, 246678, 246ab8, 247106, 2473ee, 247a8c, 247d6e,
247e14.
(Most of the remainder are the large table-dispatch combat/cutscene state machines noted above, plus the
two-way-dependent cluster 0x02447d4/0x02244820/0x02244670 whose callees mutually M2C_ERROR. Several -
243be0, 244670, 245632, 246308 - have complete m2c output and are good next targets for a careful pass.)
