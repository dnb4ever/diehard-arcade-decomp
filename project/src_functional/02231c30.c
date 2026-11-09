/* projectile_emitter_task [0x02231c30]: Existing map corroborated by source: 2-state periodic child emitter. (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02231c30 (task handler) - periodic child-projectile emitter (2 states).
 * ent = task->unk8; oent = task->unk14->unk8 (owner entity). state = (s8)task->unk10.
 * state 0 (init): src = &g_06097D58[(oent->unk34 << 8) + (u8)ent->unk5F * 0x10];
 *   ent->unk8/C/10 = src[0/4/8] (position); func_02231a80(ent, oent) (aim angles);
 *   ent->unk5A(w) = ashl_var(12, g_06096C90) + 1 (lifetime, scaled by level);
 *   task->unk10++; fall through to state 1.
 * state 1 (emit): c = --ent->unk5A(w). Emit this frame only when
 *   ((ashl_var(4, g_06096C90) - 1) & c) == 0 (cadence mask scaled by level):
 *     child = spawn_from_template(task, 0x06035D72); e2 = child->unk8;
 *     copy ent->unk8/C/10 (pos, long), unk24/26/28 (angles, word), unk14/18/1C (vel, long),
 *       unk58 (counter, word) into e2; e2->unk30(w)=0x179; func_02238dde() (post-clone init);
 *     optional sound_queue_push(0xA0130BFF) when g_060E0DDC;
 *     g_0609974C[oent->unk34] (s16) = 0x49 (slot status);
 *     if (ent->unk5A == 0) kill (task->unk0 = 0).
 * default: no-op.
 * fidelity: HIGH - oent->unk34 byte, ent->unk5F u8*0x10, counter unk5A/unk58 word,
 *   angles word, pos/vel long, status array s16 stride 2. func_0224a360 = ashl_var
 *   (value r0 << count r1), standard runtime helper. bsrf 0x24a360 x2 / 0x20ff28 /
 *   0x238dde / 0x215f6c resolved; direct bsr 0x231a80. */
extern void effect_orient_from_parent(void *ent, void *params);         /* orient entity (aim angles) */
extern s32  shc_shift_left(s32 value, s32 count);            /* ashl_var: value << count */
extern void *task_spawn_list_work7c(void *parent, void *template);   /* spawn_from_template */
extern void display_mark_channel_bits(void);                            /* post-clone init */
extern void sound_queue_push(s32 cmd);                         /* sound_queue_push */
extern u8   g_06097d58[];
extern s32  g_06096C90;      /* scale/level driver (shift count) */
extern s16  g_0609974c[];    /* per-slot status words */
extern s32  g_sound_effects_enabled;

void projectile_emitter_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
        M32(ent, 0x8)  = M32(src, 0);
        M32(ent, 0xC)  = M32(src, 4);
        M32(ent, 0x10) = M32(src, 8);
        effect_orient_from_parent(ent, oent);
        M16(ent, 0x5A) = (s16)(shc_shift_left(12, g_06096C90) + 1);
        state = (s8)(state + 1);
        TASK_S8(task, state) = state;
        /* fall through to state 1 */
    }

    if (state == 1) {
        s16 c = (s16)(M16(ent, 0x5A) - 1);
        M16(ent, 0x5A) = c;
        if (((shc_shift_left(4, g_06096C90) - 1) & (s32)c) == 0) {
            void *child = task_spawn_list_work7c(task, (void *)0x06035D72);
            void *e2 = MP(child, 8);
            M32(e2, 0x8)  = M32(ent, 0x8);
            M32(e2, 0xC)  = M32(ent, 0xC);
            M32(e2, 0x10) = M32(ent, 0x10);
            M16(e2, 0x24) = M16(ent, 0x24);
            M16(e2, 0x26) = M16(ent, 0x26);
            M16(e2, 0x28) = M16(ent, 0x28);
            M16(e2, 0x30) = 0x179;
            M32(e2, 0x14) = M32(ent, 0x14);
            M32(e2, 0x18) = M32(ent, 0x18);
            M32(e2, 0x1C) = M32(ent, 0x1C);
            M16(e2, 0x58) = M16(ent, 0x58);
            display_mark_channel_bits();
            if (g_sound_effects_enabled != 0)
                sound_queue_push(DH_SOUND_CLONE_COMMAND);
            g_0609974c[M8(oent, 0x34)] = 0x49;
            if (M16(ent, 0x5A) == 0)
                TASK_S32(task, handler) = 0;
        }
        return;
    }
}
