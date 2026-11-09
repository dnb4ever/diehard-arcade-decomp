# Functional decomp batch 4 (Opus) — shared callees cracked + boot/camera runway

Direct follow-on to batch 3. Priorities were (1) the shared callees the actor state-step
family tails into, (2) larger anim-family members, (3) broad non-anim runway, (4) a naming
cleanup. All new TUs use the `dh.h` accessor style and compile clean on host gcc
(`gcc -c -fsyntax-only -Wall -Wextra`, **zero warnings**). Every file cross-checked
field-by-field against the raw SH-2 asm (`m2c_fn.py --asm` / `fn.py`) for widths (M8/M16/M32)
and signedness. Core read-only tools (m2c_fn.py / fn.py / match.py) untouched;
braf_rewrite.py / braf_resolve.py / scan_family.py used as-is.

## 1. Functions added this batch (14) — all compile clean

### Shared callees (priority #1 — retroactively sharpen the 0x228xxx–0x229xxx family)
| cart off | file | behaviour |
|----------|------|-----------|
| 0x22c220 | 0222c220.c | **actor_ledge_fall_gate**: reads per-zone record g_06098734[unk34-1] (0x18 stride, byte-truncated index); when its flags + the actor's unk84/unk8C/unk56 + global g_0609979A all permit, arms a fall/launch arc (unkC4 = g_06096C58[unk34]+unkC+0xFFFF4000, unkC8 = (0x30000-unkC4)>>4, unk84/88 latched) and returns -1; else returns 0 |
| 0x227704 | 02227704.c | **actor_facing_resolve**: pushes a matrix frame (g_06098878 += 0x30), runs the per-actor transform at g_06096328[unk34] (0x6C stride) via func_02213fb4/022149e4(x2)/02213538, reads transformed-X g_06098B24, then sets facing (unk26 = 0x4000/0xC000) + flip flag unk44 from its sign |
| 0x23a6dc | 0223a6dc.c | **actor_pose_load**: loads pose record g_0608CE8C[unk6A] (8B stride) → unk5C (anim id) / unk90 (frame count, zero-ext word); forces unk84 = 0x400000 for player slots (unk34 < 3) |
| 0x22aadc | 0222aadc.c | **actor_sound_emit**: for type-0x14 (unkBA&0x3F) or no-hit-timer (unk56==0) actors with sound enabled (g_060E0DDC), pushes (g_06087EE4[unk34-1]<<8)\|0xA00000FF via **sound_queue_push**; run by actor_pose_dispatch (0x22adaa) |
| 0x23a394 | 0223a394.c | **actor_queue_mark**: one-liner g_060E0F2C[unk94] = 1 |
| 0x23b1c0 | 0223b1c0.c | **actor_state_gen_grab**: grab/hold state generator; restores partner anim slot, nudges partner facing (g_0608CEF4[unk6D]), clears grab flags; returns 1 (caller 0x229816 does `state = ret + 0x21`) |

### Larger anim-family members (priority #2 — task-form, per-bsrf arg transcription)
| cart off | file | behaviour |
|----------|------|-----------|
| 0x22a970 | 0222a970.c | **actor_anim_init_task** (task->unk8 = entity): loads anim rec unkD0, zeroes affine fields (unk20/22/24/28), primes unkC4 timer, runs func_0222db44/0221707c/eb84 + animation_update(task,entity), immediately runs then installs func_0222a9fe as task->unk0 |
| 0x22a9fe | 0222a9fe.c | **actor_anim_step_task**: func_0222eba8 + animation_update + func_0222d0e8; on loop-flag unk54&2 decrements unkC4 and advances the anim-script ptr unkD0 on expiry; on end (unk3C==0x10) runs func_02227c22 and transitions task->unk0 → func_0222a5ca |

### Broad runway (priority #3 — boot / camera / HUD call graphs)
| cart off | file | behaviour |
|----------|------|-----------|
| 0x201958 | 02201958.c | **boot_globals_init** (= FUN_02201958, called by main_init_entry): clears game-state HWRAM globals; probes machine via BIOS callbacks `*0x0600063C(3,&g_06098674)` and `*0x06000634(&g_060987EC)` + smpc_intback_read; decodes region/config (g_06000248 >>12, INTBACK buffer byte) → language g_06099794 / player-count g_06099796; sets sound-enable g_060E0DDC=1 |
| 0x20cfac | 0220cfac.c | **camera_follow_update**: camera follow of the active player slot (unkB0) + dynamic zoom accumulated over the 8 player slots (proximity + state flags, clamp 0x20000) + sin/cos orbit + eased position; sets roll unk24=-0x400 / yaw unk26 |
| 0x20d262 | 0220d262.c | **actor_orbit_pos_task**: orbit position update — angle unk74 += unk70, world pos = 0x30000·{sin,cos}(angle)+base; feeds func_0221055c (sprite/transform build) |
| 0x20b354 | 0220b354.c | **task_entity_trampoline**: func_0220d8d0(task,0), then calls the entity's own handler (entity->unk64) with the task, re-installs self as task->unk0 |
| 0x2029a4 | 022029a4.c | **hud_number_draw_task**: draws a fixed-point value (entity->unk60, seeded 0x10000, +=0x60/frame) at screen (0x160,0x78) via func_02221460 (4 reg + 2 stack args, both = unk60) |
| 0x202c5c | 02202c5c.c | **scene_hud_setup_task**: func_0220f990 + func_0223dcc2(1/2), mirrors language g_06099794 into g_06099788, spawns a 0x6C-byte obj (spawn_obj_0x6c, template@0x0220340C), transitions task->unk0 → func_02202dfa |

### Key structural findings
- **Shared callees named** (func_0222c220/02227704/0223a6dc/0222aadc, plus 0x23a394/0x23b1c0):
  these were called as un-named externs by ~10 batch-3 handlers; their roles are now documented
  in symbols.txt, which clarifies what those handlers' `func_0222c220(entity)` / tail
  `func_02227704` / `func_0223a6dc` calls actually do (ledge/fall arming, facing resolve, pose
  load, sound emit).
- **func_02215f6c confirmed = sound_queue_push** from the 0x22aadc call site (command format
  `(id<<8)|0xA00000FF`).
- **sin/cos/fixmul leaves exercised**: 0x20d262 and 0x20cfac both use the named
  func_02213190 (sin_lookup) / func_022131a8 (cos_lookup) / func_022131c8 (fixmul_16_16),
  independently re-confirming those names and the 16.16 convention.
- **PIC mova data pointers resolved by hand**: task-form handlers assign `task->unk0 =
  &next_handler` via a `mova .Lpool_XXXXXX; add #delta` anchor. The label hex IS the cart
  offset, so target = (labelhex + delta) | 0x02000000 — e.g. 0x22a970 → next = 0x22a9ec+0x12 =
  func_0222a9fe; 0x22a9fe → 0x22aa90-0x4C6 = func_0222a5ca; 0x202c5c spawn template =
  0x202c9c+0x770 = 0x0220340C, next = 0x202cac+0x14E = func_02202dfa. The wrapper does NOT do
  this (it only rewrites branches), so each was walked manually.

## 2. Naming cleanup (priority #4)
Batch 3 flagged `func_02153f6` → `func_022153f6` (cart 0x2153f6 + 0x2000000) in 0222077e.c.
Found the **same dropped-digit pattern** in 9 files total and fixed all (behaviour-identical,
extern+call renamed consistently; all recompile clean):
- `func_02153f6` → `func_022153f6` : 0222077e.c, 022396e8.c, 022397c2.c, 0223ef5a.c, 0223efa0.c, 0223efec.c
- `func_022eea2` → `func_0222eea2` (drawlist_prepend) : 022324da.c
- `func_022f4e8` → `func_0222f4e8` : 0222f470.c
- `func_02154fc` → `func_022154fc` : 02231d72.c

Left alone (NOT the same bug): `func_0214e2e` (0214e2e.c — a function named after its own file
stem, internally consistent) and `func_0217d22` in 22170b4.c (misseg-band file, per the
standing "don't touch the misseg band" constraint).

## 3. Real counts (project/progress_functional.py)
```
functions total (fn.units) : 1272
  functional  (src_functional):  237    18.63%   (was 226 -> net +11 this batch)
  nonmatching (src_nonmatching):   73     5.74%
  byte-exact  (src files)      :    5     0.39%
  byte-exact  (oracle EXACT)   :    3     0.24%
  ANY C written (deduped)      :  315    24.76%   (was 304 -> +11)
```
Honest note: **14 new verified .c files were written**, but the counter moved only +11 because
3 of these addresses already had earlier .c files in src_functional (the oft-referenced shared
callees — 0x0222c220 appears as far back as batch 1 and 0x0223a6dc in batch 2 — plus one more)
that these fully asm-verified versions supersede. Net real functional coverage is 237.

## 4. Resisted / not-taken (honest)
- **0x229988 / 0x22988e tail half**: 0x229988 is reached by fall-through and reads r12/r13/r14
  without setting them (m2c emits `saved_reg_r1x`) — i.e. it is a mid-function continuation of
  0x22988e, not a clean standalone start. Deferred to avoid guessing register provenance.
- **0x229488**: real function, m2c body mostly clean, BUT its 0x47 case does a spawn
  `func_0220ff72(*0x06099580, template@0x02226eb8)` with nested writes to spawn->unk8->unk10/14
  and a PIC data-pointer template; I only pulled the asm head, so the switch-case field widths
  are not fully verified. Deferred for a careful full-asm pass (now cheaper: its callees
  func_02227704 / 0223a6dc / 0222adaa are named this batch).
- **0x22a3e2**: **misseg** — fn.py's extent prepends the preceding literal pool, so m2c sees a
  garbage prologue (`.byte func_ff,0xff`, `and.b #32,@(r0,gbr)`, M2C_ERROR). Real prologue is
  further in. Left per the "don't touch the misseg band" constraint.
- **Generator family .c deferred** (0x239fc6, 0x23a3a2, 0x23ac9c, 0x23b01e): these are the
  `state = f(entity)+0x21` generators the family branches to. Each has heavy PIC **data-pointer**
  args (`func_0220feec(*0x06099580, &.Lpool+delta)` spawn templates) and fn-pointer-table loads
  (`temp_r13 = &.Lpool+delta` called as `temp_r13(...)`) that the braf wrapper does NOT lower —
  they'd need per-pointer manual resolution of every template/handler like I did for the
  task-form next-state pointers, but many more per function. Identified + named in symbols.txt
  (roles clear from the m2c drafts) but .c held back as unverifiable-in-bulk this sitting.
- **scan_family false-positives**: several "fully-tractable" entries (e.g. 0x200400, 0x200668,
  0x200d46, 0x200d9a) resolve to out-of-ROM garbage targets (0x49e833f0, 0xe488..., 0x80200dbc)
  — these are misseg units where the PIC resolver produced nonsense. Each candidate was
  cross-checked against asm before use; the bogus ones were skipped.
- **0x20b098**: a tiny `func_02214a7a(task->unk8)` tail-thunk, but fn.py's asm view merges the
  adjacent installer function after the braf tail (ambiguous boundary) — skipped as not worth
  the misseg risk for a one-liner.

## 5. Next-band recommendation
1. **0x229488 + the generator family** (0x239fc6 / 0x23a3a2 / 0x23ac9c / 0x23b01e): now that the
   shared callees are named, these are the remaining high-leverage targets. They need a small
   tool extension: a **mova/add data-pointer resolver** (same `label_cart + delta` math I did by
   hand for next-state pointers and spawn templates) so the `func_0220feec(..., &.Lpool+delta)`
   template args and `temp_r13 = &.Lpool+delta` fn-ptr-table bases resolve automatically. That
   would unlock the whole generator family and the 0x24xxxx anim range in bulk. Recommend
   extending braf_resolve.py with a `resolve_data_ptrs(start)` helper.
2. **The boot / init sequence around 0x201958**: main_init_entry → boot_globals_init is now done;
   the adjacent init functions (0x201590, 0x201b92, 0x201c9c/0x201f20 VDP2 setup, 0x202138
   vdp2_scene_setup) are a coherent, port-relevant cluster — several are fully braf-tractable.
3. **The 0x206xxx–0x208xxx sprite/draw cluster** (scan_family, targets 0x213290 clear_vec4 /
   0x22156c / 0x2145d0 sh2_dmac_copy): a broad low-risk runway, all wrapper-lowered.
Still avoid the misseg band and the 0x24a3xx–0x24a5xx jump-table fragment cluster.
