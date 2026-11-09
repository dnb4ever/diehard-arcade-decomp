#include "dh_work.h"
#include "dh.h"
/* 0x0220b1c0 (camera_pose_init_task) - scene-start pose/init for a camera-style
 * entity. Same pose-table family as camera_init_pose (0x0220bc20); exposed by
 * fn.extent's over-swallow of the 0x20b0c6..0x20b354 pool group.
 *
 * ABI: task_handler(task). task = r4; entity = task->unk8 (r14).
 * When in-play (g_06098670 != 0) the start pose comes from the per-scene tables
 * selected by sel = *0x0609866C: position triple from 0x06051E8C[(s16)(sel->unk28 * 12)] (12-byte
 * records) and angle triple from 0x06051F94[(u8)(sel->unk29 * 6)] (6-byte records); both the
 * live pose (unk8/C/10, unk24/26/28) and the saved "home" pose (unkB8/BC/C0,
 * unkC4/C6/C8) are set. Otherwise the defaults at 0x06051E78 (pos) / 0x06051E84
 * (angles) are used. A stage world-offset from 0x06060814[g_060B3D54] (44-byte
 * records) is loaded into unk84/88/8C and added to the live position.
 *
 * It then arms the shake/latch globals (g_06098830=0xA, g_06098834=0), clears
 * task->unkD, sets unk68=0x10000 and the shake counters unkA8/AA=0, zeroes the +4
 * field of the eight 8-byte slot records at 0x06098838..0x06098878, installs the
 * per-frame handler task_entity_trampoline (task->unk0 = 0x0600f354), points the
 * entity's own handler slot unk64 at *0x06051E6C, rebuilds the matrices, and runs
 * the object latch (obj_latch_reset to level 3) when engaged.
 *
 * m2c corrections: func_02214a7a(entity, var_r5, temp_r6, 0x06098838) carries three
 * phantom args (leftover regs from the clear loop) - mtx_build_fwd_inv takes entity
 * only. Pointer 0x0220b354 via ptr_resolve (mova+add); call to 0x02214a7a via
 * braf_resolve. No tst #imm8>=0x80 traps. Bounded 0x20b1c0..0x20b354 (single rts).
 * fidelity: HIGH. */

#define AP(a)   ((void *)(uintptr_t)(a))
#define GI32(a) (*(s32 *)(uintptr_t)(a))
#define GS16(a) (*(s16 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern void mtx_build_fwd_inv(void *entity);   /* 0x02214a7a */

void camera_pose_init_task(void *task)
{
    void *entity = TASK_PTR(task, work);
    void *rec;

    if (GI32(DH_PLAY_SESSION_MODE_ADDR) != 0) {
        void *sel = GP(DH_SCENE_DESCRIPTOR_ADDR);
        rec = AP(0x06051E8C + (s16)(SCENE_M16(sel, pose_index) * 0xC));   /* 12-byte pos record */
        CAMERA_M32(entity, target_x) = M32(rec, 0);
        CAMERA_M32(entity, target_y) = M32(rec, 4);
        CAMERA_M32(entity, target_z) = M32(rec, 8);
        WORLD_POSE(entity)->position_x  = M32(rec, 0);
        WORLD_POSE(entity)->position_y  = M32(rec, 4);
        WORLD_POSE(entity)->position_z = M32(rec, 8);
        rec = AP(0x06051F94 + (u8)(SCENE_M8(sel, angle_variant) * 6));      /* 6-byte angle record */
        CAMERA_M16(entity, target_angle_x) = M16(rec, 0);
        CAMERA_M16(entity, target_angle_y) = M16(rec, 2);
        CAMERA_M16(entity, target_angle_z) = M16(rec, 4);
        WORLD_POSE(entity)->angle_x = M16(rec, 0);
        WORLD_POSE(entity)->angle_y = M16(rec, 2);
        WORLD_POSE(entity)->angle_z = M16(rec, 4);
    } else {
        CAMERA_M32(entity, target_x) = GI32(0x06051E78);
        CAMERA_M32(entity, target_y) = GI32(0x06051E7C);
        CAMERA_M32(entity, target_z) = GI32(0x06051E80);
        WORLD_POSE(entity)->position_x  = GI32(0x06051E78);
        WORLD_POSE(entity)->position_y  = GI32(0x06051E7C);
        WORLD_POSE(entity)->position_z = GI32(0x06051E80);
        CAMERA_M16(entity, target_angle_x) = GS16(0x06051E84);
        CAMERA_M16(entity, target_angle_y) = GS16(0x06051E86);
        CAMERA_M16(entity, target_angle_z) = GS16(0x06051E88);
        WORLD_POSE(entity)->angle_x = GS16(0x06051E84);
        WORLD_POSE(entity)->angle_y = GS16(0x06051E86);
        WORLD_POSE(entity)->angle_z = GS16(0x06051E88);
    }

    rec = AP(0x06060814 + GI32(0x060B3D54) * 0x2C);     /* 44-byte stage offset */
    CAMERA_M32(entity, stage_origin_x) = M32(rec, 0);
    CAMERA_M32(entity, stage_origin_y) = M32(rec, 4);
    CAMERA_M32(entity, stage_origin_z) = M32(rec, 8);
    WORLD_POSE(entity)->position_x  += CAMERA_M32(entity, stage_origin_x);
    WORLD_POSE(entity)->position_y  += CAMERA_M32(entity, stage_origin_y);
    WORLD_POSE(entity)->position_z += CAMERA_M32(entity, stage_origin_z);

    GI32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 0xA;
    GI32(0x06098834) = 0;
    M8(task, 0xD) = 0;
    CAMERA_M32(entity, horizontal_range) = 0x10000;
    CAMERA_M16(entity, request_hold_ticks) = 0;
    CAMERA_M16(entity, shake_ticks) = 0;

    {
        u32 p = 0x06098838;
        u32 end = 0x06098838 + 0x40;
        while (p < end) {
            M32(AP(p), 4) = 0;
            M32(AP(p + 8), 4) = 0;
            p += 16;
        }
    }

    TASK_PTR(task, handler) = AP(0x0600f354);        /* install task_entity_trampoline */
    CAMERA_M32(entity, update_handler) = GI32(0x06051E6C);
    mtx_build_fwd_inv(entity);

    if (GI32(DH_PLAY_SESSION_MODE_ADDR) == 1 && GI32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 3) {
        void *q = MP(GP(DH_VIEW_TASK_ADDR), 8);
        GI32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 3;
        M8(GP(DH_VIEW_TASK_ADDR), 0xD) = 1;
        M16(q, 0xAE) = 0;
        M16(q, 0xB0) = 0;
        M16(q, 0xB2) = 0;
    }
}
