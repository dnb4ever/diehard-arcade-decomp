# Functional decomp - Gen2 band 0x230000 <= addr < 0x240000

Big-endian SH-2 (ST-V), functional (equivalence) decomp. Addr = cart offset + 0x02000000.
All files in `project/src_functional/`, include only `dh.h`, each verified
`gcc -c -fsyntax-only -Wall -Wextra` = ZERO warnings and hand-checked vs
`tools/decomp/m2c_fn.py --asm`. PIC resolved with braf_resolve.py / find_entry.py.
`find_entry.py` run FIRST on every target (pool-contaminated census members are
common here: several "undone" census rows resolve to already-done functions -
e.g. 230416->230434, 231a06->231a1c, 23a712->23a740, 23e520->23e530, 23e3ba->23e4a0).

## Added this pass (18 functions in 16 files)

| file | name | conf | notes |
|------|------|------|-------|
| 02230006.c | projectile_fire_seed_task | HIGH | reserve ammo func_0223c74a(src); seed unk34/38/68, launch vec, direction by src->unk44; pos func_022301c4; next -> func_022300e8 |
| 022300e8.c | projectile_sustain_fire_task | HIGH | m2c FAILED (branch-into-branch) -> hand-decoded. every-4th-frame (--unk5A&3) ammo check + reposition func_022301c4; all exit paths -> func_022301ac |
| 022304b4.c | projectile_emit_task | HIGH | spray child sparks via func_0220ff28(task, handler 0x022305e2) up to 31x (unk74); pos from g_06097D58 rec; kill on owner unk84&0x43800 |
| 02231864.c | projectile_fly_task | HIGH | optional func_0222f3c0(unk5D); lifetime unk5A; integrate unk8/C += unk14/18, drag >>4; drawlist func_0222eea2; tail func_060193f6(ent,unk6C,1) |
| 02232178.c | muzzle_spark_task | HIGH | first-frame slot alloc func_02228106; orient func_0221080a(g_0608807C,&unk24); world = cam g_06098B1C[] + src; tail func_0222eea2 if src->unk60>0x80 |
| 0223820c.c | gfx_vram_dma_setup | HIGH | no-arg; 12x func_022145d0(src,dst,len) ROM->VDP + 1x func_02213290(dst,len); func-ptr r14 = mova+0xFFFDC3BC = 0x022145d0 |
| 02238f46.c | effect_flash_scale_task | HIGH | m2c FAILED (tail bra) -> hand-decoded. 3-state scale pop unk34/36/38 + timer unk40; tail func_022386fc(&unk34) |
| 02238fdc.c | effect_pulse_scale_task | HIGH | m2c FAILED (tail bra) -> hand-decoded. 3-state scale pulse; tail func_022386fc(&unk34) |
| 0223cfe4.c | actor_start_action | HIGH | ret 0/-1; gates func_0223e4a0()/func_0223cbb0(action,0,0); anim .unkE swap g_06096248<->g_060e11ea from g_060e10a8; func_0223b254; func_022260c8 |
| 0223d2a0.c | effect_tumble_task | HIGH* | 3-phase gravity/tumble; func_02173f4(ent,&g_060e0e8c[unk34*16]); *MED on matrix-draw cluster func_022149aa/02214a40/02214a06/02215a2c (g_06098878 +-0x30), same as func_0223d1b6 |
| 0223dd96.c | actor_apply_action_record | HIGH | m2c FAILED (bsr targets) -> hand-decoded. apply g_060e0e38[(id-1)*16] to g_060995b0[id]->unk8; func_0223cfe4, func_0223cf70(ent,1/12) |
| 0223de20.c | players_special_refresh | HIGH | no-arg; both players via pm g_06099584 (P1 unk10/unk30, P2 unk14/unk34); gate unk8C&0xFFFFFF && !(&2); func_0223a9d0(actor,3) |
| 0223e2ea.c | hud_score_slots_init | HIGH | idx = unk34-1; func_022380c0(4,g_0608d1cc[idx],12,0); digit slots 7..17 / 35..25; record g_0609860c[idx]; clear g_060e0e58[idx*16] |
| 0223e5e0.c | actor_update_dispatch | HIGH | gate func_0245dcc(ent)->unkD4=0x11,unkD8=func_0223ffcc; dispatch g_0608d624[unkD4](task,ent); func_022ca98; tail func_0222eee4 |
| 0223e5e0.c | actor_render_dispatch | HIGH | SECOND fn in same unit (first ends in braf tail). func_0246208/unk4C gate; dispatch g_0608d7b4[unk6D](ent); tail func_0223dc2c |
| 0223fc06.c | actor_update_dispatch2 | HIGH | twin of actor_update_dispatch, table g_0608d764[unkD4] |
| 0223fc06.c | actor_render_dispatch2 | HIGH | SECOND fn in same unit; twin of actor_render_dispatch, table g_0608d808[unk6D] |
| 0223f054.c | chain_hit_task | HIGH | multi-hit chain vs g_060995b0[slot]->unk8; spawn func_0220feec(task,0x02239500) every 4th count (unkC4<35); damage func_022ab18(target,1) |

### Conventions followed
- Next-state handler stored with `MP(task,0) = (void *)func_...;`; task kill with `M32(task,0)=0`.
- Dispatch tables faithfully modelled as typed function-pointer arrays and called
  indirectly (NO M2C_ERROR stubs): e.g. `g_0608d624[M32(ent,0xD4)](task, ent)`.
- spawn_from_template func_0220ff28 / func_0220feec: `(void *)parent, (void *)template`,
  returns holder whose ->unk8 is the child entity (established project pattern).
- Two unit pairs (0x23e5e0+0x23e632, 0x23fc06+0x23fc58) each hold TWO functions that
  fn.py did not split because the first ends in a `braf` tail-call (not `rts`). Both
  functions of each pair are decompiled in the one .c file (named by the unit entry).

## Resisted (deferred) - with reason
- **0x239082** - projectile draw handler with a 7-call matrix-stack cluster
  (func_022149aa/02214a50/022149e4/022131a8/02213190/02213ce4/02215a2c) whose
  fixed-point args are built by pipelined register math interleaved ACROSS the bsrf
  boundaries and passed partly on the stack (mov.l r6,@-r15). Attributing each
  helper's exact args faithfully is error-prone; same MED/defer class as prior wave.
- **0x238e94** - calls func_024a360 / func_024a420, the NON-STANDARD-ABI helpers
  (inputs r0/r1, result in r0) the prior wave documented; m2c mis-lowers them.
- **0x23a888** (real entry of census 0x23a856) - fixed-point graphics math through
  func_02131a8 / func_02213190 with interleaved multi-arg patterns (same family the
  prior wave deferred at 0x23967a). m2c collapses the two distinct helpers to one.
- **0x23df30** - large (225 insn) per-player HUD builder: 4-arg func-ptr table calls
  (temp_r10) plus func_0223e104/func_0223e17c with many branches; tractable but long
  and higher-risk; deferred for time.
- **0x23ffcc** - indirect action-dispatch via table[unk108] (prior wave already
  deferred); referenced as extern by the actor_update dispatchers.

## Pool-contaminated / already-done census rows skipped (verified via find_entry)
- 230416->230434 (done), 231a06->231a1c (done), 232090->2320d4 (prior-deferred),
  23a712->23a740 (done), 23a856->23a888 (deferred above), 23e520->23e530 (done),
  23f032->23f054 (DONE this pass), 23de0e->23de20 (DONE this pass),
  23e3ba->23e4a0 (done), 23069a->2306c4, 2314f6->23152c (both still open).

## Band remaining (undone genuine entries still open)
Still ~60-70 genuine members open in-band. Notable clusters for a later wave:
- projectile/effect: 2306c4, 23101c, 23115c, 23152c, 231b14, 231c30, 231db8, 232526,
  232610, 232bc4, 233250, 233388, 23346c, 234984, 234b58, 235198, 235f0c, 236246,
  2363e4, 236af0, 2371aa, 237828, 237be4, 237ea6, 23967a, 239462, 239868, 2399bc.
- actor/combat: 23a9b8 (big), 23b28a, 23b5d8, 23b9b8, 23bcd4, 23bf86, 23c7f6, 23c95c,
  23cbb0, 23cd98, 23cf70, 23d558, 23d726, 23db2a, 23df30, 23e17c, 23e690 (big),
  23f10e (big), 23ffcc, 23fcb6, 23fec4.
