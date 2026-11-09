/* scene_prop_keyframe_task [0x02202a28]: Moves and shakes a composite prop through timed keyframes. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202a28 - task handler: keyframe-driven object mover/shaker (cutscene prop or camera).
 * entity = task->unk8; task->unk10 is a byte keyframe index (also the first-run flag).
 * On first run (index 0) seeds position (entity->unk8/C/10 = fixed 0xFFFC0000/0xFFF70000/
 * 0x00120000), velocity seed (unk14=0x8000, unk18=unk1C=0), 16-bit accumulators
 * (unk24/26/28=0), a flag (unk37=1), sets index=1, and loads the dwell counter
 * entity->unk58 from keyframe table 0x0604EDC0[index].field_C (stride 0x10).
 * Each frame: integrate velocity from table 0x0604EDC0[index] (fields +0/+4/+8 into
 * unk14/18/1C), add velocity into position (unk8/C/10), add the 16-bit deltas from table
 * 0x0604EEC0[(s8)(index*6)] (+0/+2/+4 into unk24/26/28), then call func_0221b330(entity).
 * Decrement unk58; when it goes negative, advance to next keyframe unless index==15 (then
 * task->unk0=0, i.e. finish). On advancing, reload unk58 from the new keyframe; when the new
 * index==4, poke the VDP shadow struct *g_06098f08 (+0xF0=5, +0xF2/F4/F6=0).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. unk58 decrement stores before the >=0 test
 * (cmp/pz); table1 32-bit stride 0x10, table2 s16 stride 6 indexed by (s8)(index*6). */

extern void model_draw_composite_parts(void *entity);

void scene_prop_keyframe_task(void *task) {
    void *e = TASK_PTR(task, work);
    u8 idx;
    s32 *t1;
    s16 *t2;

    if (TASK_S8(task, state) == 0) {
        WORLD_POSE(e)->position_x = (s32)0xfffc0000;
        WORLD_POSE(e)->position_y = (s32)0xfff70000;
        WORLD_POSE(e)->position_z = 0x00120000;
        M32(e, 0x14) = 0x00008000;
        M32(e, 0x18) = 0;
        M32(e, 0x1c) = 0;
        WORLD_POSE(e)->angle_x = 0;
        WORLD_POSE(e)->angle_y = 0;
        WORLD_POSE(e)->angle_z = 0;
        M8(e, 0x37) = 1;
        TASK_S8(task, state) = 1;
        idx = TASK_U8(task, state);
        M32(e, 0x58) = M32((uintptr_t)0x0604edc0 + idx * 0x10, 0x0c);
    }

    idx = TASK_U8(task, state);
    t1 = (s32 *)((uintptr_t)0x0604edc0 + idx * 0x10);
    M32(e, 0x14) += t1[0];
    M32(e, 0x18) += t1[1];
    M32(e, 0x1c) += t1[2];
    WORLD_POSE(e)->position_x += M32(e, 0x14);
    WORLD_POSE(e)->position_y += M32(e, 0x18);
    WORLD_POSE(e)->position_z += M32(e, 0x1c);
    t2 = (s16 *)((uintptr_t)0x0604eec0 + (s8)(idx * 6));
    WORLD_POSE(e)->angle_x += t2[0];
    WORLD_POSE(e)->angle_y += t2[1];
    WORLD_POSE(e)->angle_z += t2[2];

    model_draw_composite_parts(e);

    M32(e, 0x58) -= 1;
    if (M32(e, 0x58) < 0) {
        if (TASK_U8(task, state) != 15) {
            TASK_S8(task, state) += 1;
            idx = TASK_U8(task, state);
            M32(e, 0x58) = M32((uintptr_t)0x0604edc0 + idx * 0x10, 0x0c);
            if (TASK_U8(task, state) == 4) {
                M16(MP(DH_VDP2_REGISTER_SHADOW_ADDR, 0), 0xf0) = 5;
                M16(MP(DH_VDP2_REGISTER_SHADOW_ADDR, 0), 0xf2) = 0;
                M16(MP(DH_VDP2_REGISTER_SHADOW_ADDR, 0), 0xf4) = 0;
                M16(MP(DH_VDP2_REGISTER_SHADOW_ADDR, 0), 0xf6) = 0;
            }
        } else {
            TASK_S32(task, handler) = 0;
        }
    }
}
