/* 0x0220e7c4: ease camera position/target and recompute look-at angles.
 * ROM 0601289e..1291e calls sine before each of the six fixed multiplies.
 * Keep those calls and stores in their original order.
 */
#include "dh.h"
#include "dh_work.h"

extern s32  sdiv32(s32 dividend, s32 divisor);          /* 0x24a06c */
extern s32  sin_lookup(s32 angle);                     /* 0x213190 */
extern s32  fixed_mul16(s32 a, s32 b);                        /* 0x2131c8 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z,           /* 0x21055c */
                          s32 bx, s32 by, s32 bz,
                          s16 *elev, s16 *head);         /* unk24/unk26 are adjacent s16 */

void entity_slew_lookat_update(void *self, void *destination_position, void *destination_target) {
    s32 t;

    if (M32(self, 0x68) == 1) {
        M32(self, 0x70) = 0x4000;
        M32(self, 0x74) = sdiv32(0x8000, (s32)M16(self, 0xAE));

        M32(self, 0x14) = (M32(destination_position, 0x0) - WORLD_POSE(self)->position_x)  >> 1;
        M32(self, 0x18) = (M32(destination_position, 0x4) - WORLD_POSE(self)->position_y)  >> 1;
        M32(self, 0x1C) = (M32(destination_position, 0x8) - WORLD_POSE(self)->position_z) >> 1;
        M32(self, 0x40) = (M32(destination_target, 0x0) - M32(self, 0xB8)) >> 1;
        M32(self, 0x44) = (M32(destination_target, 0x4) - M32(self, 0xBC)) >> 1;
        M32(self, 0x48) = (M32(destination_target, 0x8) - M32(self, 0xC0)) >> 1;

        M32(self, 0x4C) = WORLD_POSE(self)->position_x  + M32(self, 0x14);
        M32(self, 0x50) = WORLD_POSE(self)->position_y  + M32(self, 0x18);
        M32(self, 0x54) = WORLD_POSE(self)->position_z + M32(self, 0x1C);
        M32(self, 0x58) = M32(self, 0xB8) + M32(self, 0x40);
        M32(self, 0x5C) = M32(self, 0xBC) + M32(self, 0x44);
        M32(self, 0x60) = M32(self, 0xC0) + M32(self, 0x48);

        M32(self, 0x68) = 0;
    }

    M32(self, 0x70) += M32(self, 0x74);
    t = sin_lookup(M32(self, 0x70));

    WORLD_POSE(self)->position_x  = M32(self, 0x4C) - fixed_mul16(t, M32(self, 0x14));
    t = sin_lookup(M32(self, 0x70));
    WORLD_POSE(self)->position_y  = M32(self, 0x50) - fixed_mul16(t, M32(self, 0x18));
    t = sin_lookup(M32(self, 0x70));
    WORLD_POSE(self)->position_z = M32(self, 0x54) - fixed_mul16(t, M32(self, 0x1C));
    t = sin_lookup(M32(self, 0x70));
    M32(self, 0xB8) = M32(self, 0x58) - fixed_mul16(t, M32(self, 0x40));
    t = sin_lookup(M32(self, 0x70));
    M32(self, 0xBC) = M32(self, 0x5C) - fixed_mul16(t, M32(self, 0x44));
    t = sin_lookup(M32(self, 0x70));
    M32(self, 0xC0) = M32(self, 0x60) - fixed_mul16(t, M32(self, 0x48));

    camera_compute_lookat_angles(WORLD_POSE(self)->position_x, WORLD_POSE(self)->position_y, WORLD_POSE(self)->position_z,
                  M32(self, 0xB8), M32(self, 0xBC), M32(self, 0xC0),
                  &WORLD_POSE(self)->angle_x, &WORLD_POSE(self)->angle_y);
}
