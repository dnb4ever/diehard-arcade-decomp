/* effect_prescatter_task [0x0223101c]: Existing map corroborated by source: 2-state task. state0: init ent (counter unk58=0, (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223101c (task handler) - two-state scatter-effect launcher task.
 * ent = task->unk8.  state = (s8)task->unk10.
 * state 0 (init): if g_060E0DDC set, sound_queue_push(0xA01403FF);
 *   ent->unk58(w)=0, ent->unk5D(b)=0, ent->unk68 = g_0604EBE4[(s8)ent->unk34],
 *   ent->unk64=0xE000, ent->unk62(w)=0x53, ent->unk60(w)=0x14, ent->unk38=0x40000,
 *   ent->unkC=0x2000; task->unk10++; fall through to state 1.
 * state 1: if ent->unk5D != 0 actor_pair_propagate(ent); n=ent->unk58(w); ent->unk58=n+1;
 *   if (n > 1): child = spawn_from_template(task, effect_scatter_task 0x06034f74);
 *     e2 = child->unk8; copy ent->unk8/C/10 and unk4C/50/54 (long) into e2;
 *     e2->unk68 = g_0604EBE4[(s8)ent->unk34]; e2->unk60(w)=0x1E; e2->unk62(w)=0x14;
 *     e2->unk34(b)=ent->unk34; e2->unk38=0x40000; e2->unk64=0x8000; e2->unk70=0xE0000;
 *     task->unk0 = 0 (kill); return.  else fall to exit.
 * default state / state-1 (n<=1) exit: tail drawlist_prepend(ent).
 * fidelity: HIGH - counter unk58 word, state/unk34 byte, table g_0604EBE4 stride 4.
 * bsrf 0x215f6c/0x22f3c0/0x20ff28; braf 0x22eea2 -> drawlist_prepend. */
extern void  sound_queue_push(s32 cmd);                      /* sound_queue_push */
extern void  actor_publish_linked_slot_value(void *entity);                 /* actor_pair_propagate */
extern void *task_spawn_list_work7c(void *parent, void *handler);  /* spawn_from_template */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend (tail) */
extern s32   g_actor_slot_masks;
extern s32   g_sound_effects_enabled;

void effect_prescatter_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01403FF);
        M16(ent, 0x58) = 0;
        M8(ent, 0x5D) = 0;
        M32(ent, 0x68) = (&g_actor_slot_masks)[M8(ent, 0x34)];
        M32(ent, 0x64) = 0xE000;
        M16(ent, 0x62) = 0x53;
        M16(ent, 0x60) = 0x14;
        M32(ent, 0x38) = 0x40000;
        M32(ent, 0xC) = 0x2000;
        state = (s8)(state + 1);
        TASK_S8(task, state) = state;
        /* fall through to state 1 */
    }

    if (state == 1) {
        s16 n;
        if (M8(ent, 0x5D) != 0)
            actor_publish_linked_slot_value(ent);
        n = M16(ent, 0x58);
        M16(ent, 0x58) = (s16)(n + 1);
        if (n > 1) {
            void *child = task_spawn_list_work7c(task, (void *)0x06034f74);
            void *e2 = MP(child, 8);
            M32(e2, 0x8)  = M32(ent, 0x8);
            M32(e2, 0xC)  = M32(ent, 0xC);
            M32(e2, 0x10) = M32(ent, 0x10);
            M32(e2, 0x4C) = M32(ent, 0x4C);
            M32(e2, 0x50) = M32(ent, 0x50);
            M32(e2, 0x54) = M32(ent, 0x54);
            M32(e2, 0x68) = (&g_actor_slot_masks)[M8(ent, 0x34)];
            M16(e2, 0x60) = 0x1E;
            M16(e2, 0x62) = 0x14;
            M8(e2, 0x34)  = M8(ent, 0x34);
            M32(e2, 0x38) = 0x40000;
            M32(e2, 0x64) = 0x8000;
            M32(e2, 0x70) = 0xE0000;
            TASK_S32(task, handler) = 0;
            return;
        }
        /* n <= 1: fall to tail drawlist_prepend */
    }

    collision_queue_actor(ent);
}
