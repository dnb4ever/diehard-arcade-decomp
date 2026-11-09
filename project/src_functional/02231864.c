/* projectile_fly_task [0x02231864]: Existing map corroborated by source: per-frame launched-projectile motion: optional func_0222f3c0, lifetime unk5A, integrate+drag, drawlist func_0222eea2, tail func_060193f6(ent,unk6C,1) */
#include "dh_work.h"
#include "dh.h"
/* 0x02231864 (projectile task handler, fly/update state) - per-frame motion of
 * a launched projectile.
 * ent = task->unk8; src = task->unk14->unk8 (owner actor).
 * If ent->unk5D != 0 call func_0222f3c0(ent) (homing/track helper).
 * Decrement lifetime ent->unk5A; if the pre-decrement value <= 0, or
 * src->unk84 & 0x4B800 (owner collided/dead), kill: (ent->unk78)->unk12 = 1,
 * task->unk0 = 0, return.
 * Otherwise integrate: save prev pos (unk4C/50/54 <- unk8/C/10), advance
 * unk8 += unk14, unkC += unk18; func_0222eea2(ent) (drawlist); apply drag
 * unk14 -= unk14>>4, unk18 -= unk18>>4; unk6C += 0x3000; tail-call
 * func_022153f6(ent, ent->unk6C, 1).
 * fidelity: HIGH. bsrf resolved 0x22f3c0/0x22eea2; braf tail 0x2153f6 = func_022153f6.
 * src->unk84 read as long (mov.l). r6=1 passed to tail call. */
extern void actor_publish_linked_slot_value(void *entity);
extern void collision_queue_actor(void *entity);
extern void sprite_draw_child(void *entity, s32 a1, s32 a2);

void projectile_fly_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);
    s16 life;

    if (M8(ent, 0x5D) != 0)
        actor_publish_linked_slot_value(ent);

    life = M16(ent, 0x5A);
    M16(ent, 0x5A) = (s16)(life - 1);
    if (life <= 0 || (M32(src, 0x84) & 0x4B800)) {
        M8(MP(ent, 0x78), 0x12) = 1;
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 0x8) += M32(ent, 0x14);
    M32(ent, 0xC) += M32(ent, 0x18);
    collision_queue_actor(ent);
    M32(ent, 0x14) -= M32(ent, 0x14) >> 4;
    M32(ent, 0x18) -= M32(ent, 0x18) >> 4;
    M32(ent, 0x6C) += 0x3000;
    sprite_draw_child(ent, M32(ent, 0x6C), 1);
}
