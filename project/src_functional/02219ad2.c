/* camera_tick_delayed_heading_turn [0x02219ad2]: Delayed target-angle sweep with signed step and optional final snap. */
#include "dh_work.h"
#include "dh.h"
/* 0x02219ad2 (leafC) - 3-phase actor swing/track action. ent = obj->unk8.
 * phase 0: pick a target (g_060995B0[ g_06098568 == 2 ? 2 : 1 ])->unk8, save in ent->field30;
 *   set ent->dur(0x1C) from ent->field18 (or 0xC); choose a signed step ent->field28 (+/-0x555)
 *   and goal ent->field2C = tgt->field26 +/- 0x4000 by ent->field24; advance phase.
 * phase 1: count down ent->field20; while it was still > 0 return, else advance phase.
 * phase 2: tgt = ent->field30; count down ent->dur; while it was > 0 and
 *   (*g_0609882C)->unk8->field68 != 1, add the step into tgt->field26; otherwise snap
 *   tgt->field26 to ent->field2C (when ent->field18 == 0) and end (obj->unk0 = 0).
 * fidelity: HIGH - state byte; ent fields 0x18..0x2C 32-bit, field30 ptr; tgt->field26 16-bit,
 * tgt->field68 32-bit; g_06098568 32-bit. */
extern void *g_actor_slot_tasks[];
extern s32   g_06098568;
extern void *g_view_task_legacy;
void camera_tick_delayed_heading_turn(void *obj) {
    void *ent = MP(obj, 0x8);
    switch (M8(obj, 0x10)) {
    case 0: {
        void *tgt = TASK_PTR(g_actor_slot_tasks[g_06098568 == 2 ? 2 : 1], work);
        int pos = M32(ent, 0x24) != 0;
        MP(ent, 0x30) = tgt;
        M32(ent, 0x1C) = M32(ent, 0x18) != 0 ? M32(ent, 0x18) : 0xC;
        M32(ent, 0x28) = pos ? 0x555 : -0x555;
        M32(ent, 0x2C) = (u16)M16(tgt, 0x26) + (pos ? 0x4000 : -0x4000);
        M8(obj, 0x10) += 1;
    }
    /* fallthrough */
    case 1: {
        s32 c = M32(ent, 0x20) - 1;
        M32(ent, 0x20) = c;
        if (c + 1 > 0)
            return;
        M8(obj, 0x10) += 1;
    }
    /* fallthrough */
    case 2: {
        void *tgt = MP(ent, 0x30);
        s32 c = M32(ent, 0x1C) - 1;
        M32(ent, 0x1C) = c;
        if (c + 1 > 0 && M32(TASK_PTR(g_view_task_legacy, work), 0x68) != 1) {
            M16(tgt, 0x26) += (s16)M32(ent, 0x28);
            return;
        }
        if (M32(ent, 0x18) == 0)
            M16(tgt, 0x26) = (s16)M32(ent, 0x2C);
        MP(obj, 0x0) = 0;
        return;
    }
    }
}
