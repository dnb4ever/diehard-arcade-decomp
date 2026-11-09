#include "dh_work.h"
/* actor_update_orbit_task [0x0220d262]: Updates orbit angle, world position from sine/cosine and look-at angles. */
#include "dh.h"
/* 0x0220d262 (task handler, orbit position update) - task->unk8 = entity. Advances the orbit
 * angle unk74 += unk70, then computes a world position from it:
 *   unk8  (X) = fixmul(0x30000, sin(unk74)) + unk84;
 *   unkC  (Y) = unk94 + 0x20000;
 *   unk10 (Z) = fixmul(0x30000, cos(unk74)) + unk98 - 0x50000;
 * and hands the position plus the base coords (unk90/94/98) and the angle-field pair at +0x24/+0x26
 * to func_0221055c for the sprite/transform build.
 * fidelity: HIGH - all fields long (16.16 fixed point); 0x30000 radius, 0xFFFB0000 = -0x50000 Z
 * bias; sin/cos/fixmul are the named math leaves; computed calls resolved via braf_rewrite.py. */
extern s32  sin_lookup(s32 angle);          /* sin_lookup */
extern s32  cos_lookup(s32 angle);          /* cos_lookup */
extern s32  fixed_mul16(s32 a, s32 b);       /* fixmul_16_16 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          void *angp, void *angp2);

void actor_update_orbit_task(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 angle = M32(entity, 0x74) + M32(entity, 0x70);

    M32(entity, 0x74) = angle;
    WORLD_POSE(entity)->position_x  = fixed_mul16(0x30000, sin_lookup(angle)) + M32(entity, 0x84);
    WORLD_POSE(entity)->position_y  = M32(entity, 0x94) + 0x20000;
    WORLD_POSE(entity)->position_z = fixed_mul16(0x30000, cos_lookup(M32(entity, 0x74)))
                        + M32(entity, 0x98) + (s32)0xFFFB0000;
    camera_compute_lookat_angles(WORLD_POSE(entity)->position_x, WORLD_POSE(entity)->position_y, WORLD_POSE(entity)->position_z,
                  M32(entity, 0x90), M32(entity, 0x94), M32(entity, 0x98),
                  (char *)entity + 0x24, (char *)entity + 0x26);
}
