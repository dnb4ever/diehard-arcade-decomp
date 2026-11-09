/* homing_follow_task [0x0223236c]: Existing map corroborated by source: obj=task->unk8, src=task->unk14->unk8. src bit0 gate. (src symbols_band23mid.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223236c - homing/target-follow task. task->unk8 = obj, task->unk14->unk8 = src.
 * If src->unk3C bit0 is clear, kill the task (task->unk0 = 0). Otherwise set the
 * object's world position (unk8/unkC/unk10) = src position + the object's local
 * offset (unk14/unk18/unk1C). Choose a target actor by mode = g_06099594 +
 * g_06099598: 0 = self, 1 = actor A (g_060995b4->unk8), 2 = actor B
 * (g_060995b8->unk8), 3 = whichever of A/B is nearer in the XZ plane (squared
 * distance via fixmul_16_16). Set the facing field unk4C = obj.x +-0x8000 (toward
 * the target along X), snapshot unk50/unk54 = obj y/z, then tail-call
 * drawlist_prepend(obj).
 * fidelity: HIGH - fixmul jsr (mova 0x232408 + 0xFFFE0DC0 -> 0x022131c8) and tail
 * braf -> 0x0222eea2 confirmed. Mode > 3 keeps self as target (dead path). */
extern void *g_060995b4;                     /* actor A pointer */
extern void *g_060995b8;                     /* actor B pointer */
extern s32   g_06099594;                     /* target-mode selector (hi) */
extern s32   g_06099598;                     /* target-mode selector (lo) */
extern s32   fixed_mul16(s32 a, s32 b);    /* fixmul_16_16 */
extern void  collision_queue_actor(void *entity);    /* drawlist_prepend */

void homing_follow_task(void *task) {
    void *src = MP(TASK_PTR(task, parent), 8);
    void *obj = TASK_PTR(task, work);
    void *cand_a;
    void *target;

    if (!(M32(src, 0x3C) & 1)) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(obj, 8)    = M32(src, 8)    + M32(obj, 0x14);
    M32(obj, 0xC)  = M32(src, 0xC)  + M32(obj, 0x18);
    M32(obj, 0x10) = M32(src, 0x10) + M32(obj, 0x1C);

    cand_a = MP(g_060995b4, 8);
    switch (g_06099594 + g_06099598) {
    case 0:
        target = obj;
        break;
    case 1:
        target = cand_a;
        break;
    case 2:
        target = MP(g_060995b8, 8);
        break;
    case 3: {
        void *cand_b = MP(g_060995b8, 8);
        s32 ax = M32(cand_a, 8) - M32(obj, 8);
        s32 az = M32(cand_a, 0x10) - M32(obj, 0x10);
        s32 bx = M32(cand_b, 8) - M32(obj, 8);
        s32 bz = M32(cand_b, 0x10) - M32(obj, 0x10);
        s32 da_x = ax < 0 ? -ax : ax;
        s32 da_z = az < 0 ? -az : az;
        s32 db_x = bx < 0 ? -bx : bx;
        s32 db_z = bz < 0 ? -bz : bz;
        s32 dist_a = fixed_mul16(da_x, da_x) + fixed_mul16(da_z, da_z);
        s32 dist_b = fixed_mul16(db_x, db_x) + fixed_mul16(db_z, db_z);
        target = (dist_a < dist_b) ? cand_a : cand_b;
        break;
    }
    default:
        target = obj;   /* unreachable; original leaves stale r13 */
        break;
    }

    M32(obj, 0x4C) = M32(obj, 8) +
                     ((M32(target, 8) - M32(obj, 8)) > 0 ? -0x8000 : 0x8000);
    M32(obj, 0x50) = M32(obj, 0xC);
    M32(obj, 0x54) = M32(obj, 0x10);
    collision_queue_actor(obj);
}
