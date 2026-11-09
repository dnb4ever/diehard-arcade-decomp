/* scene_zoom_overlay_task [0x022029a4]: Initializes and increments the fixed-point background interpolation scale each frame. */
#include "dh_work.h"
#include "dh.h"
/* 0x022029a4 (task handler, HUD number draw) - task->unk8 = entity. On first run (task->unk10
 * == 0) seeds entity->unk60 = 0x10000 and bumps the one-shot flag task->unk10. Each frame draws
 * via func_02221460(0x160, 0x78, 0, 0, v, v) where v = entity->unk60 (fixed-point value, passed
 * for both the value and a secondary slot), then advances entity->unk60 += 0x60.
 * fidelity: HIGH - task->unk10 byte one-shot, entity->unk60 long; func_02221460 takes four reg
 * args (0x160, 0x78, 0, 0) and two stack args (both entity->unk60); computed call resolved via
 * braf_rewrite.py (bsrf -> func_02221460). */
extern void vdp2_set_transform_slot0(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);

void scene_zoom_overlay_task(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 v;

    if (TASK_S8(task, state) == 0) {
        M32(entity, 0x60) = 0x10000;
        TASK_S8(task, state) += 1;
    }
    v = M32(entity, 0x60);
    vdp2_set_transform_slot0(0x160, 0x78, 0, 0, v, v);
    M32(entity, 0x60) += 0x60;
}
