/* camera_tick_heading_turn [0x02219ba4]: Twelve-tick default heading turn of view task work; optional exact final snap. */
#include "dh_work.h"
#include "dh.h"
/* 0x02219ba4 (leafC) - 2-phase actor action. ent = obj->unk8; tgt = (*g_0609882C)->unk8.
 * phase 0 (obj->field10 == 0): set ent->dur(0x1C) from ent->field18 (or 0xC); pick a signed
 *   step ent->field24 (+/-0x555) and target ent->field28 = tgt->field26 +/- 0x4000 selected by
 *   ent->field20, then advance the phase. phase 1: decrement ent->dur; while it was still > 0,
 *   add the step into tgt->field26 and return; otherwise snap tgt->field26 to ent->field28
 *   (only when ent->field18 == 0) and end the action (obj->unk0 = 0).
 * fidelity: HIGH - state byte; dur/step/targ and field18/20 32-bit; tgt->field26 16-bit. */
extern void *g_view_task_legacy;
void camera_tick_heading_turn(void *obj) {
    void *ent = MP(obj, 0x8);
    void *tgt = TASK_PTR(g_view_task_legacy, work);
    switch (M8(obj, 0x10)) {
    case 0: {
        s32 d = M32(ent, 0x18);
        int pos = M32(ent, 0x20) != 0;
        M32(ent, 0x1C) = d != 0 ? d : 0xC;
        M32(ent, 0x24) = pos ? 0x555 : -0x555;
        M32(ent, 0x28) = (u16)M16(tgt, 0x26) + (pos ? 0x4000 : -0x4000);
        M8(obj, 0x10) += 1;
    }
    /* fallthrough */
    case 1: {
        s32 c = M32(ent, 0x1C) - 1;
        M32(ent, 0x1C) = c;
        if (c + 1 > 0) {
            M16(tgt, 0x26) += (s16)M32(ent, 0x24);
            return;
        }
        if (M32(ent, 0x18) == 0)
            M16(tgt, 0x26) = (s16)M32(ent, 0x28);
        MP(obj, 0x0) = 0;
        return;
    }
    }
}
