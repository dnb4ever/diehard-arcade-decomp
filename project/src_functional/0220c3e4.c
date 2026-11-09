#include "dh_work.h"
#include "dh.h"
/* 0x0220c3e4 (camera_track_pair) - per-frame camera update that frames TWO focus targets
 * (slots cam->unkAE and cam->unkB0), places the eye at their midpoint, and pulls back far
 * enough to keep nearby active actors in shot. Task handler: arg0 = task; cam = task->unk8.
 *
 * It is the two-target sibling of 0x0220c17c (camera_track_single); the two share one literal
 * pool (0x20c460..0x20c802) - c17c is the code before 0x20c3e4, this is the code after. Both
 * end by calling lookat_angles_xz, camera_smooth_lerp, camera_shake_decay and (tail)
 * mtx_build_fwd_inv, all four resolved via braf_resolve (m2c dropped them as bsrf):
 *   lookat_angles_xz (0x21055c, 8 args, 4 on stack) -> cam->unkC4/C6 (pitch/yaw);
 *   camera_smooth_lerp (0x20e0aa, cam); camera_shake_decay (0x20d98e, task);
 *   mtx_build_fwd_inv (0x214a7a, cam).
 *
 * Each focus point is built on the stack (f0, f1): the slot actor's coord (objPtr[slot]->unk8
 * .unk8/C/10) with Y biased by g_06096C58[...] and clamped to [0xC000, 0x24000]; or, when the
 * slot is empty, the camera home (unk84 x / unk8C z) with Y pinned at 0x10000. cam->unk90/94/98
 * is the midpoint ((f0+f1)>>1 per axis). Zoom (var zoom, seed 0x10000): each present focus adds
 * 0x3000/0x6000 by its actor state bits (focus0 sets zoom=0x13000/0x16000 absolutely off the
 * 0x10000 seed; focus1 and the loop add), then for every OTHER active slot i (i != either
 * focus, flagged or enabled by g_0604EBE4[i-1] & *g_0609977C) add (0xA0000 - dist[unkAE][i])>>4
 * when positive plus the same state bonus; capped at 0x20000. Then:
 *   cam->unk9C = 3*(zoom>>2);  cam->unkA0 = 0x8000;  cam->unkA4 = -(zoom*2) - (zoom>>2);
 * eye (unkB8/BC/C0) = midpoint +/- those (X sign follows unk90<unk84; Y sign follows
 * (unkAE+unkB0)&1). Tilt cam->unkC4 = ((unkAE+unkB0)&1 ? -0x800 : 0x800), halved if zoom>0x18000.
 *
 * dist table: g_06099584 + 0x4C, 8x8 longs (stride 0x20), row = focus slot unkAE.
 *
 * FINDINGS / corrections (both faithfully replicated):
 *  - The first mask, idx0 = cam->unkAE & 7, reads r3 UNSET in the isolated disassembly (no
 *    "mov #7,r3" in this entry's prologue, unlike c17c which sets it; the 2nd focus block does
 *    reload #7). Value 7 is certain: it matches the unkB0 mask here and the c17c twin, and the
 *    table has 8 slots. The SHC build evidently left r3=7 live across the shared region.
 *  - The focus1 Y-bias indexes g_06096C58[unkAE] (the focus0 slot), NOT [unkB0] - an original
 *    off-by-slot quirk confirmed in both asm and m2c; kept as-is.
 *  - The empty-slot branch reads cam->unk88 into the focus Y then immediately overwrites it
 *    with 0x10000 (dead read); dropped here, as in c17c.
 *
 * fidelity: HIGH - hand-traced against m2c_fn --asm (0x20c3e4..0x20c7aa). The 0x8000 eye-Y
 * pool short at unk68 clamp is loaded with mov.w (sign-extended) = -0x8000. */

extern u8  g_actor_slot_registry[];   /* camera object table */
extern s32 g_06096C58[];   /* per-slot focus Y bias */
extern s32 g_actor_slot_masks_legacy[];   /* per-slot enable-mask table */
extern s32 g_0609977C;     /* active-slot mask bits */

extern void lookat_angles_xz(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                             s16 *outPitch, s16 *outYaw);   /* 0x21055c */
extern void camera_smooth_lerp(void *cam);                 /* 0x20e0aa */
extern void camera_shake_decay(void *task);                /* 0x20d98e */
extern void mtx_build_fwd_inv(void *cam);                  /* 0x214a7a */

void camera_track_pair(void *task)
{
    void *cam = TASK_PTR(task, work);
    void *coord;
    s32 f0[3], f1[3];   /* focus points on the stack: [0]=x, [1]=y, [2]=z */
    s32 zoom = 0x10000;
    s32 idx0, idx1, i, t, flags;

    idx0 = M16(cam, 0xAE) & 7;         /* r3 read unset upstream; value 7 (see header) */
    M16(cam, 0xAE) = (s16)idx0;

    if (M32(g_actor_slot_registry, 0xC + idx0 * 4) != 0) {
        coord = MP(MP(g_actor_slot_registry, 0x2C + idx0 * 4), 8);
        f0[0] = M32(coord, 8);
        f0[1] = M32(coord, 0xC);
        f0[2] = M32(coord, 0x10);
        f0[1] += g_06096C58[idx0];
        if (f0[1] > 0x24000)      f0[1] = 0x24000;
        else if (f0[1] < 0xC000)  f0[1] = 0xC000;

        flags = M32(MP(MP(g_actor_slot_registry, 0x2C + idx0 * 4), 8), 0x38);
        if (flags & 0x1E0)
            zoom = 0x13000;
        else if (flags & 0xFE00)
            zoom = 0x16000;
    } else {
        f0[0] = CAMERA_M32(cam, stage_origin_x);
        f0[2] = CAMERA_M32(cam, stage_origin_z);
        f0[1] = 0x10000;
    }

    idx1 = M16(cam, 0xB0) & 7;
    M16(cam, 0xB0) = (s16)idx1;

    if (M32(g_actor_slot_registry, 0xC + idx1 * 4) != 0) {
        coord = MP(MP(g_actor_slot_registry, 0x2C + idx1 * 4), 8);
        f1[0] = M32(coord, 8);
        f1[1] = M32(coord, 0xC);
        f1[2] = M32(coord, 0x10);
        f1[1] += g_06096C58[idx0];     /* indexes idx0, not idx1 (original quirk) */
        if (f1[1] > 0x24000)      f1[1] = 0x24000;
        else if (f1[1] < 0xC000)  f1[1] = 0xC000;

        flags = M32(MP(MP(g_actor_slot_registry, 0x2C + idx1 * 4), 8), 0x38);
        if (flags & 0x1E0)
            zoom += 0x3000;
        else if (flags & 0xFE00)
            zoom += 0x6000;
    } else {
        f1[0] = CAMERA_M32(cam, stage_origin_x);
        f1[2] = CAMERA_M32(cam, stage_origin_z);
        f1[1] = 0x10000;
    }

    CAMERA_M32(cam, focus_x) = (f0[0] + f1[0]) >> 1;
    CAMERA_M32(cam, focus_y) = (f0[1] + f1[1]) >> 1;
    CAMERA_M32(cam, focus_z) = (f0[2] + f1[2]) >> 1;

    for (i = 0; i < 8; i++) {
        if (i == idx0 || i == idx1)
            continue;
        if (M32(g_actor_slot_registry, 0xC + i * 4) == 0 &&
            !(g_actor_slot_masks_legacy[i - 1] & g_0609977C))
            continue;
        t = 0xA0000 - M32(g_actor_slot_registry, 0x4C + idx0 * 0x20 + i * 4);
        if (t <= 0)
            continue;
        zoom += t >> 4;
        flags = M32(MP(MP(g_actor_slot_registry, 0x2C + i * 4), 8), 0x38);
        if (flags & 0x1E0)
            zoom += 0x3000;
        else if (flags & 0xFE00)
            zoom += 0x6000;
    }
    if (zoom > 0x20000)
        zoom = 0x20000;

    CAMERA_M32(cam, orbit_offset_x) = 3 * (zoom >> 2);
    CAMERA_M32(cam, orbit_offset_y) = 0x8000;
    CAMERA_M32(cam, orbit_offset_z) = -(zoom * 2) - (zoom >> 2);

    if (CAMERA_M32(cam, focus_x) < CAMERA_M32(cam, stage_origin_x))
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    else
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) - CAMERA_M32(cam, orbit_offset_x);

    if ((idx0 + idx1) & 1)
        CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    else
        CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) - CAMERA_M32(cam, orbit_offset_y);

    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);

    lookat_angles_xz(CAMERA_M32(cam, target_x), CAMERA_M32(cam, target_y), CAMERA_M32(cam, target_z),
                     CAMERA_M32(cam, focus_x), CAMERA_M32(cam, focus_y), CAMERA_M32(cam, focus_z),
                     &CAMERA_M16(cam, target_angle_x), &CAMERA_M16(cam, target_angle_y));

    CAMERA_M16(cam, target_angle_x) = ((idx0 + idx1) & 1) ? -0x800 : 0x800;
    if (zoom > 0x18000)
        CAMERA_M16(cam, target_angle_x) = (s16)(CAMERA_M16(cam, target_angle_x) >> 1);
    CAMERA_M16(cam, target_angle_z) = 0;

    camera_smooth_lerp(cam);

    {
        s32 d = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, position_x);
        if (d > CAMERA_M32(cam, horizontal_range) + 0x8000)
            CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, horizontal_range) - 0x8000;
        else if (d < -CAMERA_M32(cam, horizontal_range) - 0x8000)
            CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + CAMERA_M32(cam, horizontal_range) + 0x8000;
    }

    camera_shake_decay(task);
    mtx_build_fwd_inv(cam);
}
