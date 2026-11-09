/* actor_aim_at_target_record [0x0223a806]: Aims actor XZ toward the indexed stage target and seeds the effect pitch and speed. */
/* 0x0223a806: aim from actor X/Z toward the indexed target record.
 * ROM 0603e818 pushes actor+0x62 as the fifth angle-output argument.
 * Then seed the effect size, scale and mode fields.
 */
#include "dh.h"
#include "dh_work.h"
extern s32 g_060B3D54;
extern u8 g_0606080C[];
extern void store_angle_between(s32 x0, s32 z0, s32 x1, s32 z1, s16 *angle);

void actor_aim_at_target_record(void *actor) {
    u8 *target = g_0606080C + 44 * g_060B3D54;
    store_angle_between(WORLD_POSE(actor)->position_x, WORLD_POSE(actor)->position_z,
                        M32(target, 8), M32(target, 16),
                        (s16 *)((u8 *)actor + 0x62));
    M16(actor, 0x60) = 0x2000;
    M32(actor, 0x74) = 0x1000;
    M8(actor, 0x45) = 0;
}
