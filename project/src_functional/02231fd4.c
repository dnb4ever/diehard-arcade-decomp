/* effect_spawner_task [0x02231fd4]: Existing map corroborated by source: 2-state (continuation of 0x02231db8). state0: (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02231fd4 (task handler) - two-state effect spawner (continuation of 0x02231db8).
 * ent = task->unk8.  state = (s8)task->unk10.
 * state 0 (init): spawn_obj_0x6c(task, obj_anim_step 0x0603CE94) (child, discarded);
 *   optional sound_queue_push(0xA01403FF) when g_060E0DDC;
 *   ent->unk58(w)=0; ent->unk6C=0x8000;
 *   child = spawn_obj_variant(task, 0x0603515C); e2 = child->unk8;
 *   e2->unk8/C/10 = ent->unk8/C/10 (position); task->unk10++; fall through to state 1.
 * state 1: n = ent->unk58(w); ent->unk58 = n+1;
 *   if (n > 1): child = spawn_from_template(task, 0x060360D4); e2 = child->unk8;
 *     e2->unk8/C/10 = ent pos; e2->unk70 = 0xA0000;
 *     e2->unk5A(w) = ashl_var(26, g_06096C90) (lifetime scaled by level);
 *     task->unk0 = 0 (kill).  else no-op.
 * default: no-op.
 * fidelity: HIGH - counter unk58 word, apex unk6C long, pos long, unk70 long, unk5A word.
 *   func_0224a360 = ashl_var (value r0 << count r1). bsrf 0x20fece/0x215f6c/0x20feec/
 *   0x20ff28/0x24a360 resolved; spawn handlers via mova+add. */
extern void *task_spawn_current_work6c(void *parent, void *handler);    /* spawn_obj_0x6c */
extern void *task_spawn_child_work88(void *parent, void *handler);    /* spawn_obj_variant */
extern void *task_spawn_list_work7c(void *parent, void *template);   /* spawn_from_template */
extern void  sound_queue_push(s32 cmd);                        /* sound_queue_push */
extern s32   shc_shift_left(s32 value, s32 count);           /* ashl_var: value << count */
extern s32   g_sound_effects_enabled;
extern s32   g_06096C90;     /* scale/level driver (shift count) */

void effect_spawner_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        void *child;
        void *e2;
        task_spawn_current_work6c(task, (void *)0x0603CE94);
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01403FF);
        M16(ent, 0x58) = 0;
        M32(ent, 0x6C) = 0x8000;
        child = task_spawn_child_work88(task, (void *)0x0603515C);
        e2 = MP(child, 8);
        M32(e2, 0x8)  = M32(ent, 0x8);
        M32(e2, 0xC)  = M32(ent, 0xC);
        M32(e2, 0x10) = M32(ent, 0x10);
        state = (s8)(state + 1);
        TASK_S8(task, state) = state;
        /* fall through to state 1 */
    }

    if (state == 1) {
        s16 n = M16(ent, 0x58);
        M16(ent, 0x58) = (s16)(n + 1);
        if (n > 1) {
            void *child = task_spawn_list_work7c(task, (void *)0x060360D4);
            void *e2 = MP(child, 8);
            M32(e2, 0x8)  = M32(ent, 0x8);
            M32(e2, 0xC)  = M32(ent, 0xC);
            M32(e2, 0x10) = M32(ent, 0x10);
            M32(e2, 0x70) = 0xA0000;
            M16(e2, 0x5A) = (s16)shc_shift_left(26, g_06096C90);
            TASK_S32(task, handler) = 0;
        }
        return;
    }
}
