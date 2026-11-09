/* cam_projectile_task [0x02236246]: Existing map corroborated by source: proj=task->unk8, actor=g_0609882c->unk8. 3-state: s0 orient (src symbols_band23mid.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02236246 - camera-anchored projectile/spark task. proj = task->unk8;
 * actor = g_0609882c->unk8 (current source actor). 3-state machine on task->unk10:
 *   state 0: orient a +Z (0,0,0x1000) vector by the actor's unk24 angles via
 *     mtx_push_rotate_apply, writing the oriented vector to g_06098b1c. Seed the
 *     projectile position (unk8/C/10 = g_06098b1c*16 + actor pos) and velocity
 *     (unk14/18/1C = g_06098b1c); set fixed fields (unk30=1, unk20=0x100, unk34=8,
 *     unk60=0x10, unk64=0x1999 drag, unk58=0xF0 life, misc zeros); advance, fall
 *     through to state 1.
 *   state 1: snapshot prev pos (unk4C/50/54), integrate velocity, submit the sprite
 *     at depth sdiv32(unk64<<9, 0x10) and prepend to the draw list. If it crosses
 *     the ground plane (unkC > 0) it temporarily clamps unkC to 0, sets the
 *     ground-flag unk20 bit0 and re-submits. Decrement the life counter unk58; when
 *     it hits 0 (or unk5D is set) advance to state 2.
 *   state 2: kill the task (task->unk0 = 0).
 * fidelity: HIGH - all 6 bsrf confirmed; sdiv32 is an r1/r0-ABI signed divide. */
extern void *g_view_task;                                 /* -> current source actor holder */
extern s32   g_transformed_vector[];                               /* oriented vector / cam deltas */
extern void  mtx_push_rotate_apply(void *vec, void *angles);     /* mtx_push_rotate_apply */
extern s32   sdiv32(s32 dividend, s32 divisor);   /* sdiv32 */
extern void  sprite_draw_child(void *obj, s32 depth, s32 flag);  /* sprite_submit1 */
extern void  collision_queue_actor(void *entity);                /* drawlist_prepend */

void cam_projectile_task(void *task) {
    void *proj = TASK_PTR(task, work);
    void *actor = TASK_PTR(g_view_task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        s32 vec[3];
        vec[0] = 0;
        vec[1] = 0;
        vec[2] = 0x1000;
        mtx_push_rotate_apply(vec, (u8 *)actor + 0x24);
        M32(proj, 8)    = (g_transformed_vector[0] << 4) + M32(actor, 8);
        M32(proj, 0xC)  = (g_transformed_vector[1] << 4) + M32(actor, 0xC);
        M32(proj, 0x10) = (g_transformed_vector[2] << 4) + M32(actor, 0x10);
        M32(proj, 0x14) = g_transformed_vector[0];
        M32(proj, 0x18) = g_transformed_vector[1];
        M32(proj, 0x1C) = g_transformed_vector[2];
        M16(proj, 0x30) = 1;
        M16(proj, 0x32) = 0;
        M16(proj, 0x20) = 1 << 8;
        M8(proj, 0x34) = 8;
        M32(proj, 0x68) = 0;
        M8(proj, 0x5F) = 0;
        M16(proj, 0x60) = 0x10;
        M32(proj, 0x64) = 0x1999;
        M16(proj, 0x58) = 0xF0;
        TASK_S8(task, state) += 1;
    }
    /* fallthrough */
    case 1: {
        s32 ypos;
        M32(proj, 0x4C) = M32(proj, 8);
        M32(proj, 0x50) = M32(proj, 0xC);
        M32(proj, 0x54) = M32(proj, 0x10);
        M32(proj, 8)   += M32(proj, 0x14);
        M32(proj, 0xC) += M32(proj, 0x18);
        M32(proj, 0x10) += M32(proj, 0x1C);
        sprite_draw_child(proj, sdiv32(M32(proj, 0x64) << 9, 0x10), 1);
        collision_queue_actor(proj);
        ypos = M32(proj, 0xC);
        if (ypos > 0) {
            M32(proj, 0xC) = 0;
            M16(proj, 0x20) |= 1;
            sprite_draw_child(proj, sdiv32(M32(proj, 0x64) << 9, 0x10), 1);
            M32(proj, 0xC) = ypos;
            M16(proj, 0x20) &= ~1;
        }
        M16(proj, 0x58) -= 1;
        if (M16(proj, 0x58) == 0 || M8(proj, 0x5D) != 0)
            TASK_S8(task, state) += 1;
        break;
    }
    case 2:
        TASK_S32(task, handler) = 0;
        return;
    }
}
