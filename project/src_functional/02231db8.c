/* rising_split_projectile_task [0x02231db8]: Existing map corroborated by source: 2-state. state0: pos from src, func_02238dde, (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02231db8 (task handler) - rising/splitting projectile task (states 0/1).
 * ent = task->unk8; oent = task->unk14->unk8 (owner). state = (s8)task->unk10.
 * state 0 (init): src = &g_06097D58[(oent->unk34 << 8) + (u8)ent->unk5F * 0x10];
 *   ent->unk8/C/10 = src[0/4/8] (position); func_02238dde() (post-clone init);
 *   optional sound_queue_push(0xA01306FF) when g_060E0DDC; func_02231a80(ent, oent) (aim);
 *   ent->unk30(w)=0x191; ent->unk58(w)=30; ent->unk6C = (ent->unkC >= 0) ? 0x500 : 0xFFE00001
 *   (apex/target height, sign per launch dir); task->unk10++.
 * state 1 (rise/split): if (g_06098580 & 0x20000) { clear it; kill (task->unk0=0); return; }
 *   Split this frame iff ent->unkC > ent->unk6C AND --ent->unk58(w) != 0 AND !(g_06098580 & 0x10000):
 *     child = spawn_obj_variant(task, 0x06035336); e2 = child->unk8;
 *     e2->unk8/C/10 = ent pos; e2->unk32(w)=0; e2->unk64=0x40000;
 *     integrate ent pos by vel (unk14/18/1C); scale vel: unk14 += unk14>>3; unk18 -= 0x100;
 *     unk1C += unk1C>>3; tail renderer func_022154fc(ent, 0x60000, 0x8000, 1).
 *   Otherwise (normal step): task->unk0 = 0x06035FD4 (next handler); task->unk10 = 0;
 *     g_06098580 |= 0x10; if (g_06098580 & 0x10000) g_06098580 &= ~0x10000;
 *     if (!(ent->unkC > ent->unk6C)) ent->unkC = 0x10000 (clamp to apex);
 *     g_06098574[0..2] = ent->unk8/C/10 (publish pos); ent->unk30(w)=0x185; ent->unk32(w)=0;
 *     ent->unk58(w)=0; tail draw_child_sprite func_022153f6(ent, 0x80000, 1).
 * default: no-op.
 * fidelity: HIGH - oent->unk34 byte, ent->unk5F u8*0x10, counter unk58 word, anim unk30/32
 *   word, apex unk6C long, g_06098580 flag bits 0x10/0x10000/0x20000, vel shar>>3 arithmetic,
 *   unk18 += 0xFFFFFF00. m2c mis-disassembled the mid-function literal pool (phantom .L231f4e);
 *   both braf tails (0x2153f6, 0x2154fc) + bsrf (0x238dde, 0x215f6c, 0x20feec) resolved;
 *   direct bsr 0x231a80. */
extern void display_mark_channel_bits(void);                            /* post-clone init */
extern void sound_queue_push(s32 cmd);                         /* sound_queue_push */
extern void effect_orient_from_parent(void *ent, void *params);         /* orient entity (aim angles) */
extern void *task_spawn_child_work88(void *parent, void *handler);    /* spawn_obj_variant */
extern void sprite_draw_child(void *obj, s32 scale, s32 mode);  /* draw_child_sprite (tail) */
extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c);
extern u8   g_06097d58[];
extern s32  g_06098574[];    /* published position vector (3 longs) */
extern s32  g_scene_event_flags;      /* projectile trigger/state flags */
extern s32  g_sound_effects_enabled;

void rising_split_projectile_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
        M32(ent, 0x8)  = M32(src, 0);
        M32(ent, 0xC)  = M32(src, 4);
        M32(ent, 0x10) = M32(src, 8);
        display_mark_channel_bits();
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01306FF);
        effect_orient_from_parent(ent, oent);
        M16(ent, 0x30) = 0x191;
        M16(ent, 0x58) = 30;
        M32(ent, 0x6C) = (M32(ent, 0xC) >= 0) ? 0x500 : (s32)0xFFE00001;
        TASK_S8(task, state) = (s8)(state + 1);
        return;
    }

    if (state == 1) {
        int do_split = 0;

        if (g_scene_event_flags & 0x20000) {
            g_scene_event_flags &= ~0x20000;
            TASK_S32(task, handler) = 0;
            return;
        }

        if (M32(ent, 0xC) > M32(ent, 0x6C)) {
            s16 nv = (s16)(M16(ent, 0x58) - 1);
            M16(ent, 0x58) = nv;
            if (nv != 0 && (g_scene_event_flags & 0x10000) == 0)
                do_split = 1;
        }

        if (do_split) {
            void *child = task_spawn_child_work88(task, (void *)0x06035336);
            void *e2 = MP(child, 8);
            M32(e2, 0x8)  = M32(ent, 0x8);
            M32(e2, 0xC)  = M32(ent, 0xC);
            M32(e2, 0x10) = M32(ent, 0x10);
            M16(e2, 0x32) = 0;
            M32(e2, 0x64) = 0x40000;
            M32(ent, 0x8)  += M32(ent, 0x14);
            M32(ent, 0xC)  += M32(ent, 0x18);
            M32(ent, 0x10) += M32(ent, 0x1C);
            M32(ent, 0x14) = M32(ent, 0x14) + (M32(ent, 0x14) >> 3);
            M32(ent, 0x18) -= 0x100;
            M32(ent, 0x1C) = M32(ent, 0x1C) + (M32(ent, 0x1C) >> 3);
            sprite_draw_billboard(ent, 0x60000, 0x8000, 1);
            return;
        }

        TASK_S32(task, handler) = (s32)0x06035FD4;
        TASK_S8(task, state) = 0;
        g_scene_event_flags |= 0x10;
        if (g_scene_event_flags & 0x10000)
            g_scene_event_flags &= ~0x10000;
        if (!(M32(ent, 0xC) > M32(ent, 0x6C)))
            M32(ent, 0xC) = 0x10000;
        g_06098574[0] = M32(ent, 0x8);
        g_06098574[1] = M32(ent, 0xC);
        g_06098574[2] = M32(ent, 0x10);
        M16(ent, 0x30) = 0x185;
        M16(ent, 0x32) = 0;
        M16(ent, 0x58) = 0;
        sprite_draw_child(ent, 0x80000, 1);
        return;
    }
}
