# Functional decomp - Wave1 band 0x230000 <= addr < 0x240000

Big-endian SH-2 (ST-V), functional (equivalence) decomp. Addr = cart offset + 0x02000000.
All files in `project/src_functional/`, include only `dh.h`, each verified
`gcc -c -fsyntax-only -Wall -Wextra` = ZERO warnings and hand-checked vs
`tools/decomp/m2c_fn.py --asm`. PIC resolved with braf_resolve.py / ptr_resolve.py.

## Added this pass (13 functions)

| file | name | conf | notes |
|------|------|------|-------|
| 02230434.c | projectile_seed_task | HIGH | task init: copy slot/flags from source actor, launch vector from g_06087F84[(unk62-0xB)*4 + src.unk44*2]; next -> func_022304b4 |
| 0223026a.c | projectile_arc_task | HIGH | per-frame func_0222fe14; lifetime unk58; kill on expiry / unkC<=0x500 / collision func_02217ae4; else integrate + drawlist + func_060194fc |
| 02230f74.c | effect_scatter_task | HIGH | ++unk58 cap 0x18; spin anim; drawlist while unk58<4; unkC+=0xA00, unk70+=0x4000; tail func_060193f6 |
| 0223144a.c | projectile_track_task | HIGH | pos from g_06097D58 rec; func_0222f3c0 when unk5D; kill on expiry or target unk84&0x4B800 (+ func_02215f6c sound); drawlist tail |
| 02231a1c.c | attach_follow_task | HIGH | copy partner-entity pos; kill on (flags&1 && disarmed) or (flags&2 && counter<=0); drawlist tail |
| 022317c0.c | projectile_launch_task | HIGH | reserve ammo func_0223c74a(oent) (abort if !=0); seed pos/vel/flags; next -> func_02231864 |
| 02232294.c | trail_spark_task | HIGH | slot func_02228106; orient template g_06088088[(s8)(unkF*12)] via func_0221080a; world = cam g_06098B1C + oent; drawlist while unk10<0x2A0000 |
| 02234aae.c | gfx_submit_box8 | MED | builds 4-corner 8x8 quad record, emits via func_0221566e; skipped when g_060987BC&6 |
| 0223a740.c | actor_state_gen_hitreact | MED | action generator (unk6D=1): swap saved anim g_060E11E8<->g_06096248, facing g_060E10B0[unk68], hit-anim g_0608CE74[unk6D]; func_0223b254/func_0223e104; tail func_022260c8 |
| 0223c650.c | pickup_slot_task | HIGH | alloc HUD slot func_02228106 -> unk34; on unk3C!=0 free g_060996D4[unk34-8] & next func_0220fe98; else func_0222eee4 |
| 0223d1b6.c | effect_fall_task | HIGH* | 3-state fall/bounce machine; *MED on the matrix-stack draw helpers (func_022149aa/02214a40/02214a06/02215a2c around g_06098878 +-0x30) |
| 0223e530.c | arbitrate_min_cost | MED | standalone: for levels 3..6 find min-cost entry across active g_06096EC4[] records, write to g_06097058[level]; no PIC |
| 0223eef0.c | effect_spin_rise_task | HIGH | ++unk6C vs unk70; anim unk30=(unk6E&3)+0x1AC; base pos g_06097E08; unk10 nudge; tail func_060193f6 |

### Conventions followed
- Next-state handler stored with `MP(task,0) = (void *)func_...;`; task kill with `M32(task,0)=0`.
- Relocated physics/anim cluster (runtime HWRAM = cart + 0x05E04000) named by runtime addr per
  in-band neighbours: func_060193f6 (cart 0x2153F6), func_060194fc (cart 0x2154FC). Sound push
  kept as func_02215f6c and drawlist as func_0222eea2 (established project names).
- func_02228106 confirmed no-arg (returns free HUD-slot idx+8); the r4 at its call sites is leftover.

## Resisted (deferred) - with reason
- **0x2320d4, 0x231fd4** - call func_024A420 / func_024A360, a helper with a NON-STANDARD register
  ABI (inputs in r0/r1, result returned in r0). m2c mis-lowers it (keeps the pre-call r0 and
  drops the return), so a faithful clean-C call cannot be written without modelling that ABI.
- **0x233122** - indirect dispatch `(*g_060902A0)(r4,r5,0x06088824)` where r4/r5 are NOT established
  at the call site (leftover regs). Picking args would be a guess.
- **0x23967a** - fixed-point graphics math: abs() idiom through func-ptr 0x213190 plus func_02214a2e/
  02213ca4/02213ce4/02215a2c with multi-register arg patterns. High effort / lower confidence.
- **0x2301c4, 0x2305e2, 0x231a80** - heavy globals-as-struct + multiple PIC spawns + sp-return
  structs m2c mangles; need careful per-field modelling.
- **0x23ffcc** - indirect action-dispatch via table[unk108] + 4 bsrf returning into &sp; m2c returns
  an uninit sp slot. Needs the dispatch table semantics.
- **0x23236c** - nearest-target abs-distance selector using a custom multi-arg func-ptr helper.
- **0x233388** - m2c decompile FAILED ("cannot find branch target .L233454") - likely misseg/data.
  **0x233250** depends on it + an indirect spawn, so deferred too.
- **0x23c6bc, 0x232526, 0x23d0fe, 0x23346c, 0x231b14** - tractable (logic understood, same
  matrix-draw / func-ptr-table idioms) but larger; deferred for time, not difficulty.
- Generators flagged "already done" in the task (0x239fc6, 0x23a3a2, 0x23ac9c, 0x23b01e): confirmed
  present/handled in src_functional; not re-done.

## Misseg / data-as-code found (skipped)
- 0x23cfdc (0x8) - bare epilogue fragment (add #4,r15; lds pr; rts), tail of func_0223cf70; not a function.
- find_entry flagged real=False members (pool/data): 231382, 2315fe, 23489a, 23577c, 235d9a, 23805a,
  239ce2/239ce6, 23de18, 23e3ba - left alone.

## Band remaining (undone real entries still open)
Many. Notable still-open clusters for a later wave:
- effect/projectile task handlers: 230cdc, 230f... (various), 231c30, 231db8, 231fd4*, 2320d4*, 232610,
  232bc4, 233250*, 233388*, 234984, 234b58, 235198, 235f0c, 236246, 2363e4, 236af0, 2371c0, 237828 ...
- actor state/combat: 23a9d0 (0x8ba, big), 23b28a, 23b5d8, 23b9b8, 23bcd4, 23bf86, 23c95c, 23cbb0,
  23cd98, 23d2a0, 23d3d0, 23d558, 23d768, 23d9dc, 23db2a, 23dcc4, 23df30, 23e17c, 23e2ea, 23e690 ...
- misc: 230434-family siblings, 238xxx cluster, 239xxx, 23f0xx (23f10e is 0xaf8 - large).
(* = examined and deferred above.)
