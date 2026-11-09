# Functional decomp report - "math dispatchers" 0x232610 / 0x232bc4 (SH2)

Worker: mathdisp. Serial, single worker, no forks. No git / shared headers / tools /
port / notes/symbols.txt touched. Files in `project/src_functional/`, named by full
8-hex SH2 address. Band-local symbols in `notes/symbols_bandmd.txt`.

Both functions **shipped** (neither deferred). Each compiles clean:
`gcc -c -fsyntax-only -Wall -Wextra` -> zero warnings. Every computed branch
confirmed with `braf_resolve.py`; entries confirmed real with `find_entry.py`.

## Headline correction

These are **not** math dispatchers. The prior band23mid deferral note called them
"very large dispatchers (div1/mac math)" - that is a misread. The `div1`, `mac.w`,
`mul.l` etc. in the raw `fn.py` listing are **literal pools** (the 0x6098f08 base
pointer, VDP2 register constants, and the signed bsrf deltas) that capstone decodes
as instructions. Both are **scene / stage-intro state machines**: a `switch` on a
state byte at `task->unk10`, each state driving VDP2/VDP1 bring-up, sound, actor/HUD
resets and object spawns. They share a near-identical state-0 setup and callee set
(0x232610 is the 6-state variant, 0x232bc4 the 7-state variant).

## Files shipped (2)

| file | role | states | branches |
|------|------|--------|----------|
| 02232610.c | stage/round intro bootstrap task | 0..5 | 35 real + 1 phantom |
| 02232bc4.c | sibling stage/round intro bootstrap task | 0..6 | 29 real + 1 phantom |

## CFG - 0x02232610  (task=r12, entity=task->unk8=r9, state=task->unk10)

- **state 0**: `video_state_init(5)`; write VDP2 shadow block `*g_06098f08`
  (+0xF8/0xFA/0xFC/0xF0/0xF2/0xF4/0xF6, RMW +0xE0/+0xE/+0, +0x10/+0x14/+0x18/+0x1C);
  `g_060986b8=1`; `vdp2_setup_largefont(7)`; `(*g_06090294)()`; gfx DMA; vdp1 kick;
  `func_02216b14(&g_06088808)`; seed src actor (`*g_0609882c`) from scene desc
  (`*g_0609866c`); **conditional** on a per-stage signature table `g_06072660`
  [stride 0x50, index g_0609868f]: upload_texture_groups / func_02215228 /
  func_0223ac0 only when the desc field differs; then entity_system_reset,
  hud_round_data_init, func_02216c38(desc->unk20,desc->unk24); `task->unk10 -> 1`.
- **state 1**: optional `sound_send_command(desc->unkC,0x6C000)`; set enable bits
  g_060bacac/b0/b4 |= 1; scene_stage_fade_spawn(task); two spawns
  func_0220fece(task, 0x2246308 / 0x222A120); func_02246052; promote phase bytes
  g_06098674[0/1] 1->2; latch g_06099578/g_0609861c/g_06098580/g_06096c90;
  sound_cue_dispatch; player-ready; g_06098568=g_06098564; `task->unk10 -> 2`;
  **falls through into state 2 the same frame**.
- **state 2**: func_0222a8d8 / func_02216d58 / func_02243628. If
  `(entity->unk3C & 8)` and handler `**g_06098678 != 0x2205232`: set entity->unk50
  = players_entities_ready()?70:30, g_06098670=0, func_0220f990, func_0223cd98,
  `->3`. Converge: if `**g_06098678 == 0x2205812`: task->unk14->unkD = -1, `->5`.
- **state 3**: func_02216d58. If `**g_06098678==0x2205232` (restart): g_06098670=1,
  entity->unk50=30, return. Else `--entity->unk50`; on reaching 0: spawn
  func_0220fece(task,0x220B1CE), spawned->unk8->unk50 = fade->unkA
  [fade = g_06088700 + (s16)(desc->unk28*0xC)], players_special_refresh,
  `sound_emit_gated((0x80 - |fade->unkA|) >> 1)`, `->4`.
- **state 4**: func_02216d58; gate `if (entity->unk3C & 2) return`; VDP2 TVMD write
  `*0x25F80000 = *g_06098f08[u16] & 0x7FFF`; g_06099578=0; players_special_refresh;
  actor_status_publish(1)/(2); clear enables g_060bacac/b0/b4 &= ~1;
  sound_queue_push(0xA00003FF); g_060986b8=0; task->unk14->unk8->unk3C |= 1;
  g_0609856c=0; g_06098670=1; g_06098812=0; g_0609880e=0;
  **tail-call (braf)** func_0220fdc8(task->unkE). No state advance.
- **state 5**: leaner variant of 4 (no 16d58/gate/refresh/publish); g_06098670=0;
  func_0220fdc8(task->unkE + 0xFF) as a normal call; g_0609861c=0. No state advance.
- default (state>=6): nop return.

## CFG - 0x02232bc4  (task=r13, entity=task->unk8=r10, state=task->unk10)

- **state 0**: same bring-up as 0x232610 state 0 but the texture/entity/HUD resets
  are **unconditional** (no g_06072660 signature gate); `task->unk10 -> 1`.
- **state 1**: optional sound_send_command; enables |=1; func_0223dcc2(1)/(2);
  g_0609979a=1; scene_stage_fade_spawn(task); scene_layer_loader_spawn_a(task);
  phase bytes g_06098674[0/1] 1->2; func_0220fece(task,0x222A120); latch
  g_0609978c[0/1]=1, g_06099578=1, g_0609861c=1, g_06098580=0, g_0609856c=0;
  sound_cue_dispatch; `task->unk10 -> 2`; entity->unk50=0xC7F; **falls through**.
- **state 2**: func_0222a8d8 / func_02216d58. h = `**g_06098678`.
  - h==0x2205812 -> g_0609856c=2, `->3`, return.
  - h==0x2204AFA || h==0x2205232 -> entity->unk50=0xC7F, goto ready-check.
  - else: g_060997a4=21, g_060997a8=2; **variadic glyph thunk**
    `(*g_060902a0)(0x608881C, entity->unk50 >> 5)` [2 stack args]; `--entity->unk50`;
    if `>0` goto ready-check; else spawn func_0220fece(task,0x2233122) [handler is
    an embedded fn at 0x2233122], g_06098670=0, func_0220f990, entity->unk54=92,
    `->4`, return.
  - ready-check: if `g_06098674[0]&&g_06098674[1]` return; else g_06098670=0,
    func_0220f990, `->6`.
- **state 3**: VDP2 TVMD write; g_06099578=0; enables &= ~1;
  sound_queue_push(0xA00001FF); g_060986b8=0; task->unk14->unk8->unk3C |= 1;
  g_06098670=1; g_06098812=0; g_0609880e=0; g_0609861c=0; g_0609979a=0;
  **tail-call (braf)** func_0220fdc8(task->unkE + 0xFF).
- **state 4**: func_02216d58; pA=g_060995b4, pB=g_060995b8, eA=pA->unk8, eB=pB->unk8.
  If `(u16)eA->unk5A>34 || (u16)eB->unk5A>34`: old=entity->unk54; entity->unk54--;
  if old>0 return. Then if `eA->unk56 && eB->unk56`: func_0222a04e()? ->6 :
  {g_06098670=1, ->5}. Elif `eA->unk56 || eB->unk56`: g_06098670=1, ->5. Else ->6.
- **state 5**: func_02216d58. If `**g_06098678==0x2204AFA` and (byte674==0 ||
  byte675==0): ->6. Then `++entity->unk50`; if `>110` and byte674==2 && byte675==2:
  entity->unk50=0xC7F, ->2.
- **state 6**: func_02216d58; func_0222a0ec; `->3`.
- default (state>=7): nop return.

## Known traps handled

- **Phantom bsrf r6**: braf_resolve reports one UNRESOLVED `bsrf r6` in each function
  (0x2327c4 in 0x232610, 0x232d58 in 0x232bc4). Both are **literal-pool words**
  (value 0x0603) loaded as data by an earlier `mov.w` (0x232676 / 0x232c36), not real
  branches. Excluded. All other branches (35 / 29) resolve to clean entries.
- **ashl/ashr variable-shift helpers** (func_0224a360/0224a420): not used by either
  function (the >>5 and >>1 here are fixed-count `shar`, decoded correctly).
- **Dropped stack-arg marshalling**: the only stack-arg call is the variadic glyph
  thunk in 0x232bc4 state 2 - handled explicitly (two args pushed, reclaimed via
  `add #8,r15`); modelled as a 2-arg call in push order.

## m2c corrections found

1. **0x232610 (*g_06090294) thunk**: m2c renders it `0x06090294(7)`, passing the
   leftover r4=7 from the preceding `vdp2_setup_largefont(7)`. SH2 gcc treats r4-r7
   as call-clobbered and would reload r4 if the thunk took an arg; it does not, so
   the call is **no-arg** (the 7 is dead). Modelled as `g_06090294()`.
2. **Variadic/stale-r4 callees**: m2c over-reports leftover register contents as
   arguments (e.g. the 0x232bc4 glyph call shows phantom `temp_r4, var_r5` before the
   two real stack args). Real arities were taken from the callee prologues:
   `func_02216d58` and `func_02215fac` overwrite/reload r4 immediately -> **no args**;
   `func_02208030` overwrites r5 at its 3rd instruction -> **1 arg (task)**;
   `func_0220fece(parent, tmpl)` forwards r4/r5 to its inner ctor at 0x20fc3c and
   returns the new object in r0.
3. **"math dispatcher" label is wrong** (see Headline correction): these are scene
   state machines; the div1/mac opcodes are literal pools.
4. **Embedded function at 0x02233122**: passed as the handler pointer to
   func_0220fece in 0x232bc4 state 2. It is real SH2 code living in 0x232bc4's data
   tail (fn.py prints it as `.word`s); a separate entry, out of scope here but noted.

## Notes

- The function-pointer compare constants (0x2205232 / 0x2205812 / 0x2204AFA) and the
  func_0220fece handler/template args (0x2246308 / 0x222A120 / 0x220B1CE / 0x2233122)
  are mova+add PC-relative values; recomputed in SH2 space and cross-checked against
  m2c's `&.Lpool_* + delta` renderings.
- Globals are declared per-TU as typed externs; canonical names/types collected in
  notes/symbols_bandmd.txt for the eventual shared header.
