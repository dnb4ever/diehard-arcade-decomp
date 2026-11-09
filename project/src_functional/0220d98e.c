/* camera_tick_shake [0x0220d98e]: Applies the next vertical shake offset and clears the shake flag when its timer ends. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220d98e (camera-shake / recoil decay, task handler) - task->unk8 = entity. If the
 * entity's countdown field unkAA (s16) is > 0, decrement it; while still non-zero add the
 * per-step offset table entry g_06052164[unkAA] to the Y coordinate unkC; when it reaches
 * zero clear bit0 of the global flag g_06098834 (shake finished).
 * fidelity: HIGH - unkAA 16-bit signed; offset table 32-bit, indexed by the post-decrement
 * value *4. Leaf. Verified vs asm. */
extern s32 g_06052164;
extern s32 g_06098834;

void camera_tick_shake(void *task) {
    void *ent = TASK_PTR(task, work);
    s16 t = CAMERA_M16(ent, shake_ticks);

    if (t > 0) {
        t = (s16)(t - 1);
        CAMERA_M16(ent, shake_ticks) = t;
        if (t != 0) {
            WORLD_POSE(ent)->position_y += (&g_06052164)[t];
            return;
        }
        g_06098834 &= ~1;
    }
}
