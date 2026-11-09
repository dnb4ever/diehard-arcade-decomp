/* effect_orient_from_parent [0x02231a80]: Rotates a forward vector by the parent angles and derives the effect's world direction and aim. */
#include "dh.h"
/* 0x02231a80 - orient entity: rotate a forward vector by the param angles, store the
 * resulting world direction and derive aim (pitch/heading) angles.
 *   fwd = { 0, 0, g_06087F84[ ((ent->unk62 & 0x3F) - 0xB) * 4 ] }   (table stride 0x10,
 *         reads record[idx].long0 = forward reach/speed for anim state idx)
 *   mtx_push_rotate_apply(fwd, &params->unk24)   -> world vector in g_06098B1C[0..2]
 *   ent->unk14/18/1C = g_06098B1C[0/1/2]
 *   look-at angles from origin to that world vector:
 *     func_0221055c(0,0,0, g_06098B1C[0], g_06098B1C[1], g_06098B1C[2], &elev, &head)
 *   ent->unk24(w) = elev; ent->unk26(w) = head + 0x4000.
 * fidelity: HIGH - unk62 masked 0x3F word, table stride 0x10, vector 32-bit (16.16),
 *   angle outputs 16-bit; bsrf 0x21080a/0x21055c resolved; m2c dropped the stack-arg
 *   marshalling for the 8-arg call (hand-reconstructed from the push sequence). */
extern void mtx_push_rotate_apply(s32 *vec, u16 *angles);  /* mtx_push_rotate_apply -> g_06098B1C */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *elev, s16 *head);   /* look-at angles */
extern s32 g_06087f84[];   /* 16-byte param records (stride 0x10) */
extern s32 g_transformed_vector[];   /* matrix-apply output vector (3 longs) */

void effect_orient_from_parent(void *ent, void *params) {
    s32 fwd[3];
    s16 elev;
    s16 head;

    fwd[0] = 0;
    fwd[1] = 0;
    fwd[2] = g_06087f84[((M16(ent, 0x62) & 0x3F) - 0xB) * 4];
    mtx_push_rotate_apply(fwd, (u16 *)((u8 *)params + 0x24));

    M32(ent, 0x14) = g_transformed_vector[0];
    M32(ent, 0x18) = g_transformed_vector[1];
    M32(ent, 0x1C) = g_transformed_vector[2];

    camera_compute_lookat_angles(0, 0, 0, g_transformed_vector[0], g_transformed_vector[1], g_transformed_vector[2], &elev, &head);
    M16(ent, 0x24) = elev;
    M16(ent, 0x26) = (s16)(head + 0x4000);
}
