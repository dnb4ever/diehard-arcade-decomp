/* camera_seed_template_task [0x0220b6b2]: Seeds current and target camera poses from the selected template and installs its continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220b6b2 (object/boss spawn-init task handler) - task->unk8 = entity. Seeds the entity's
 * target + current position (unkB8/BC/C0 and unk8/C/10) and angles (unkC4/C6/C8 and unk24/26/28)
 * from either a per-enemy template (when g_06098670 != 0: position table g_06051e8c[(s16)(rec.unk28*0xC)]
 * and angle table g_06051f94[(u8)(rec.unk29*6)], rec = *g_0609866c) or the fixed defaults
 * g_06051e78 / g_06051e84. It then offsets position by the current zone base record
 * (g_06060814 + g_060b3d54*0x2C) into unk84/88/8C, resets the shared spawn state
 * (g_06098830=10, g_06098834=0, task->unkD=0, entity->unk68=0x20000, unkA8=unkAA=0), clears
 * the unk4 field of the 8 debris slots at g_06098838, installs the next handler
 * (task->unk0 = func_0220b85c), sets entity->unk64 = *g_06051e70, builds the matrices via
 * mtx_build_fwd_inv(entity), and - if g_06098670==1 and g_06098830>=3 - clamps g_06098830 to 3 and
 * clears the active object's (*g_0609882c) spin fields unkAE/B0/B2.
 * fidelity: HIGH - position fields 32-bit (16.16), angles 16-bit; table indices signed/unsigned
 * byte-scaled (exts.w / extu.b); computed call resolved to mtx_build_fwd_inv, next-handler pointer
 * resolved to func_0220b85c (ptr_resolve). Verified vs asm. */
extern s32  g_play_session_mode;
extern void *g_scene_descriptor_legacy;
extern s8   g_06051e8c;
extern s8   g_06051f94;
extern s8   g_06051e78;
extern s8   g_06051e84;
extern s32  g_060b3d54;
extern s8   g_06060814;
extern s32  g_view_request_priority;
extern s32  g_06098834;
extern s8   g_06098838;
extern s32  g_06051e70;
extern void *g_view_task;
extern void mtx_build_fwd_inv(void *ent);
extern void camera_run_mode1_handler(void *task);

void camera_seed_template_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *rec;
    s8 *p;

    if (g_play_session_mode != 0) {
        s8 *pt = &g_06051e8c + (s16)(SCENE_M16(g_scene_descriptor_legacy, pose_index) * 0xC);
        s8 *at = &g_06051f94 + (u8)(SCENE_M8(g_scene_descriptor_legacy, angle_variant) * 6);

        CAMERA_M32(ent, target_x) = M32(pt, 0);
        CAMERA_M32(ent, target_y) = M32(pt, 4);
        CAMERA_M32(ent, target_z) = M32(pt, 8);
        CAMERA_M32(ent, position_x)  = M32(pt, 0);
        CAMERA_M32(ent, position_y)  = M32(pt, 4);
        CAMERA_M32(ent, position_z) = M32(pt, 8);
        CAMERA_M16(ent, target_angle_x) = M16(at, 0);
        CAMERA_M16(ent, target_angle_y) = M16(at, 2);
        CAMERA_M16(ent, target_angle_z) = M16(at, 4);
        CAMERA_M16(ent, angle_x) = M16(at, 0);
        CAMERA_M16(ent, angle_y) = M16(at, 2);
        CAMERA_M16(ent, angle_z) = M16(at, 4);
    } else {
        CAMERA_M32(ent, target_x) = M32(&g_06051e78, 0);
        CAMERA_M32(ent, target_y) = M32(&g_06051e78, 4);
        CAMERA_M32(ent, target_z) = M32(&g_06051e78, 8);
        CAMERA_M32(ent, position_x)  = M32(&g_06051e78, 0);
        CAMERA_M32(ent, position_y)  = M32(&g_06051e78, 4);
        CAMERA_M32(ent, position_z) = M32(&g_06051e78, 8);
        CAMERA_M16(ent, target_angle_x) = M16(&g_06051e84, 0);
        CAMERA_M16(ent, target_angle_y) = M16(&g_06051e84, 2);
        CAMERA_M16(ent, target_angle_z) = M16(&g_06051e84, 4);
        CAMERA_M16(ent, angle_x) = M16(&g_06051e84, 0);
        CAMERA_M16(ent, angle_y) = M16(&g_06051e84, 2);
        CAMERA_M16(ent, angle_z) = M16(&g_06051e84, 4);
    }

    rec = &g_06060814 + g_060b3d54 * 0x2C;
    CAMERA_M32(ent, stage_origin_x) = M32(rec, 0);
    CAMERA_M32(ent, stage_origin_y) = M32(rec, 4);
    CAMERA_M32(ent, stage_origin_z) = M32(rec, 8);
    CAMERA_M32(ent, position_x)  += CAMERA_M32(ent, stage_origin_x);
    CAMERA_M32(ent, position_y)  += CAMERA_M32(ent, stage_origin_y);
    CAMERA_M32(ent, position_z) += CAMERA_M32(ent, stage_origin_z);

    g_view_request_priority = 10;
    g_06098834 = 0;
    M8(task, 0xD) = 0;
    CAMERA_M32(ent, horizontal_range) = 0x20000;
    CAMERA_M16(ent, request_hold_ticks) = 0;
    CAMERA_M16(ent, shake_ticks) = 0;

    for (p = &g_06098838; p < &g_06098838 + 0x40; p += 8)
        M32(p, 4) = 0;

    TASK_PTR(task, handler) = (void *)camera_run_mode1_handler;
    CAMERA_M32(ent, update_handler) = g_06051e70;
    mtx_build_fwd_inv(ent);

    if (g_play_session_mode == 1 && g_view_request_priority >= 3) {
        void *obj = TASK_PTR(g_view_task, work);
        g_view_request_priority = 3;
        M8(g_view_task, 0xD) = 1;
        M16(obj, 0xAE) = 0;
        M16(obj, 0xB0) = 0;
        M16(obj, 0xB2) = 0;
    }
}
