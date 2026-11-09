/* scene_actor_script_task [0x0223f95c]: Advances the task-side scene script, slot state and position continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223f95c - func_0223f95c: cutscene/script sequencer step (task-side state machine).
 * task = r13, ent = task->unk8 (r14). Dispatches on (s8)task->unk10:
 *   0: ent->unk6C = func_0224a360(task->unkF, arg1)  [r5 passed through unmodified];
 *      ent->unk70 = &g_06091878[task->unkF * 2]; ent->unk74 = ent->unk8;
 *      ent->unk78 = ent->unkC; ent->unk34 = (s8)func_02228106();
 *      func_0221707c(ent, tbl[0], tbl[1], tbl[2], tbl[3]) with
 *      tbl = &g_0608d520[task->unkF * 4]; falls through to state 1.
 *   1: ent->unk30 = g_0608d518[task->unkF]; ent->unk60 = *(ent->unk70 + ent->unk68*4);
 *      ent->unk64 = 0; task->unk10 = 2; falls through to state 2.
 *   2: if (!(g_06098580 & ent->unk6C)) skip out; else ent->unk30 = g_0608d510[task->unkF];
 *      ent->unk5C = 15; task->unk10++; falls through to state 3.
 *   3: if (--ent->unk5C & 1): r = func_02210020(&ent->unk8 [stack arg]); note the
 *      compiler's branchless select: if r >= 0 -> r &= 1, addend 0x1000, slot = r;
 *      else addend -0x1000 (0xfffff000), slot = r & 1 (same low bit after shll2*? :
 *      r<<2 then &implicit); *(s32*)((u8*)ent->unk8_area + (r&1)*4) += addend.
 *      Note the target base is the stack-stored word (ent + 8), i.e. ent->unk8 as
 *      a base pointer slot. else: ent->unk8 = ent->unk74; ent->unkC = ent->unk78;
 *      if (g_060e0ddc != 0) sound_queue_push(0xA01101FF); falls into state 4.
 *   4: if (ent->unk5C != 0) done; else advance ent->unk64, copy the 16-byte record
 *      at ent->unk60[(ent->unk64-1)*16] into ent->unk14/18/1C and set ent->unk58 =
 *      rec[12]; ent->unk3C |= 1; if (ent->unk1C < 0) { func_0223fec4(task, ent,
 *      &g_0608d5c0[task->unkF * 4]); if (g_060e0ddc != 0) sound_queue_push(0xA01305FF); }
 *      task->unk10 = 5.
 *   5: if (--ent->unk58 != 0) func_022170ea(ent, 3); else advance ent->unk64;
 *      v = rec[12]; if (v != 0) { if (v < 0) { v = -v; ent->unk3C &= ~1; }
 *        ent->unk58 = v; ent->unk14/18/1C = rec[0/4/8]; }
 *      else { ent->unk3C &= ~2; if (!(ent->unk68 ^= 1)) { g_06098580 &= ~ent->unk6C;
 *               task->unk10 = 1; }
 *             else { ent->unk60 = *(ent->unk70 + ent->unk68*4); ent->unk64 = 0;
 *               task->unk10 = 4; } }
 * Exit: if (!(ent->unk80 & 1)) tail-call (braf) func_02246282(ent).
 * fidelity: HIGH - all bsrf/braf targets resolved: 0x24a360, 0x228106, 0x21707c,
 * 0x210020, 0x215f6c (x2), 0x2170ea, 0x23fec4, 0x246282 (tail). */
extern u8  g_0608d510[];
extern u8  g_0608d518[];
extern u8  g_0608d520[];
extern u8  g_0608d5c0[];
extern u8  g_06091878[];
extern volatile s32 g_scene_event_flags;
extern volatile s32 g_sound_effects_enabled;

extern s32  shc_shift_left(s32 value, s32 count);
extern s32  actor_allocate_effect_slot(void);
extern void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4);
extern u32  game_random_next(void);
extern void sound_queue_push(s32 req);
extern void actor_resolve_movement(void *record, s32 a1);
extern void effect_spawn_template_children(void *arg0, void *arg1, void *arg2);
extern void actor_draw_transformed(void *task);

void scene_actor_script_task(void *task, s32 arg1, s32 arg2, s32 arg3) {
    void *ent = TASK_PTR(task, work);
    s8 slot_b;
    u32 slot;
    s32 mode = (s8)TASK_S8(task, state);

    if (mode == 0) {
        slot_b = (s8)M8(task, 0xF);
        slot = (u8)slot_b;
        M32(ent, 0x6C) = shc_shift_left(slot_b, arg1);
        M32(ent, 0x70) = (s32)(g_06091878 + slot * 8);
        M32(ent, 0x74) = M32(ent, 8);
        M32(ent, 0x78) = M32(ent, 0xC);
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        actor_register_movement_slot(ent,
                      M32(g_0608d520, slot * 16 + 0),
                      M32(g_0608d520, slot * 16 + 4),
                      M32(g_0608d520, slot * 16 + 8),
                      M32(g_0608d520, slot * 16 + 12));
        mode = 1;
    }
    if (mode == 1) {
        slot = (u8)(s8)M8(task, 0xF);
        M16(ent, 0x30) = (s16)MU16(g_0608d518, slot * 2);
        M32(ent, 0x60) = M32(M32(ent, 0x70), M32(ent, 0x68) * 4);
        M32(ent, 0x64) = 0;
        TASK_S8(task, state) = 2;
        mode = 2;
    }
    if (mode == 2) {
        if (!(g_scene_event_flags & M32(ent, 0x6C))) {
            mode = -1;
        } else {
            slot = (u8)(s8)M8(task, 0xF);
            M16(ent, 0x30) = (s16)MU16(g_0608d510, slot * 2);
            M32(ent, 0x5C) = 15;
            TASK_S8(task, state) = (s8)TASK_S8(task, state) + 1;
            mode = 3;
        }
    }
    if (mode == 3) {
        s32 c = M32(ent, 0x5C) - 1;
        M32(ent, 0x5C) = c;
        if (c & 1) {
            u32 base = (u32)MP(ent, 8); /* word stored on stack for callee */
            s32 r = (s32)game_random_next();
            s32 addend;
            if (r >= 0) {
                r &= 1;
                addend = 0x1000;
            } else {
                addend = (s32)0xFFFFF000;
            }
            M32(base, (r << 2)) += addend;
        } else {
            M32(ent, 8)  = M32(ent, 0x74);
            M32(ent, 0xC) = M32(ent, 0x78);
            if (g_sound_effects_enabled != 0)
                sound_queue_push((s32)0xA01101FF);
        }
        mode = 4;
    }
    if (mode == 4) {
        if (M32(ent, 0x5C) == 0) {
            s32 idx = M32(ent, 0x64) + 1;
            u8 *rec;
            M32(ent, 0x64) = idx;
            rec = (u8 *)M32(ent, 0x60) + (idx - 1) * 16;
            M32(ent, 0x58) = M32(rec, 12);
            M32(ent, 0x14) = M32(rec, 0);
            M32(ent, 0x18) = M32(rec, 4);
            M32(ent, 0x1C) = M32(rec, 8);
            M32(ent, 0x3C) |= 1;
            if ((s32)M32(ent, 0x1C) < 0) {
                slot = (u8)(s8)M8(task, 0xF);
                effect_spawn_template_children(task, ent, g_0608d5c0 + slot * 16);
                if (g_sound_effects_enabled != 0)
                    sound_queue_push((s32)0xA01305FF);
            }
            TASK_S8(task, state) = 5;
        }
    } else if (mode == 5) {
        s32 cnt = M32(ent, 0x58) - 1;
        M32(ent, 0x58) = cnt;
        if (cnt + 1 != 0) {
            actor_resolve_movement(ent, 3);
        } else {
            s32 idx = M32(ent, 0x64) + 1;
            M32(ent, 0x64) = idx;
            {
                u8 *rec = (u8 *)M32(ent, 0x60) + (idx - 1) * 16;
                s32 v = M32(rec, 12);
                if (v != 0) {
                    if (v < 0) {
                        v = -v;
                        M32(ent, 0x3C) &= (s32)0xFFFFFFFE;
                    }
                    M32(ent, 0x58) = v;
                    M32(ent, 0x14) = M32(rec, 0);
                    M32(ent, 0x18) = M32(rec, 4);
                    M32(ent, 0x1C) = M32(rec, 8);
                } else {
                    s32 flip;
                    M32(ent, 0x3C) &= (s32)0xFFFFFFFE; /* & -2 */
                    flip = M32(ent, 0x68) ^ 1;
                    M32(ent, 0x68) = flip;
                    if (flip == 0) {
                        g_scene_event_flags &= ~M32(ent, 0x6C);
                        TASK_S8(task, state) = 1;
                    } else {
                        M32(ent, 0x60) = M32(M32(ent, 0x70), M32(ent, 0x68) * 4);
                        M32(ent, 0x64) = 0;
                        TASK_S8(task, state) = 4;
                    }
                }
            }
        }
    }

    if (!(M32(ent, 0x80) & 1))
        actor_draw_transformed(ent);
}
