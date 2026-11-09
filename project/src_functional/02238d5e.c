/* effect_trail_fall_task [0x02238d5e]: Integrates a growing gravity trail in all three axes until it reaches the floor. */
#include "dh_work.h"
#include "dh.h"
/* 0x02238d5e - falling trail sub-object task handler (3-state machine), sibling of
 * 0x02238cd2. Spawned by actor_anim_step_task (0x0222f4e8) and
 * projectile_phase_track_task (0x0222f6a8) via spawn_obj_variant 0x0220feec
 * (handler = mova+0x9802 / mova+0x965A = 0x238d5e).
 * task->unk8 = entity; task->unk10 = state byte.
 *   state 0: seed (unk30=0x17B, unk32=0, unk58=0x20000); -> state 1 (fallthrough).
 *   state 1: unk58 += 0x8000; vy (unk18) -= 0x600; pos (unk8/C/10) += vel
 *            (unk14/18/1C); once y (unkC) < 0x500 advance state;
 *            tail draw_child_sprite(entity, unk58, 1).
 *   state 2: deactivate task (task->unk0 = 0).
 * fidelity: HIGH - hand-checked vs asm 0x238d5e..0x238ddc; tail braf -> 0x022153f6
 *   (draw_child_sprite), r6 (flags) = 1 set in a branch delay slot on both paths;
 *   the y compare is signed (cmp/ge). */
extern void sprite_draw_child(void *entity, s32 scale, s32 flags);  /* 0x022153f6 draw_child_sprite */

void effect_trail_fall_task(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x30) = 0x17B;
        M16(ent, 0x32) = 0;
        M32(ent, 0x58) = 0x20000;
        TASK_S8(task, state) = TASK_S8(task, state) + 1;
        /* fallthrough */
    case 1:
        M32(ent, 0x58) = M32(ent, 0x58) + 0x8000;
        M32(ent, 0x18) = M32(ent, 0x18) - 0x600;
        M32(ent, 0x08) = M32(ent, 0x08) + M32(ent, 0x14);
        M32(ent, 0x0C) = M32(ent, 0x0C) + M32(ent, 0x18);
        M32(ent, 0x10) = M32(ent, 0x10) + M32(ent, 0x1C);
        if (M32(ent, 0x0C) < 0x500)
            TASK_S8(task, state) = TASK_S8(task, state) + 1;
        sprite_draw_child(ent, M32(ent, 0x58), 1);
        return;
    case 2:
        TASK_PTR(task, handler) = (void *)0;
        return;
    default:
        return;
    }
}
