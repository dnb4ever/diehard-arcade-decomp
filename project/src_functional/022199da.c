/* camera_tick_sine_sweep [0x022199da]: Selects player and view work, applies sine to view heading for 27 ticks. */
/* 0x022199da  obj_sweep_update: 2-state (obj[0x10]) object driving a sprite angle.
 * sub = obj[8].
 *   state 0: sel = (*0x06098564 == 2) ? 2 : 1;
 *            sub[0x30] = ptr_array[0x060995b0][sel]->unk8;
 *            sub[0x34] = (*0x0609882c)->unk8;   (sprite record pointer)
 *            sub[0x24] = 0; sub[0x28] = 27; sub[0x1C] = obj[0x0F] ? -0x25E : 0x25E;
 *            obj[0x10]++  (fall through to state 1).
 *   state 0/1: sub[0x24] += sub[0x1C];
 *            sub[0x34]->unk26 = (sin_lookup(sub[0x24]) >> 2) + sub[0x18];
 *            if (--sub[0x28] == 0) obj[0] = 0.
 * fidelity: faithful - bsrf resolved to sin_lookup (0x213190), shar >>2. */
#include "dh.h"

extern s32 sin_lookup(s32 angle);

void camera_tick_sine_sweep(void *obj) {
    void *sub = MP(obj, 8);
    s8 state = M8(obj, 0x10);

    if (state != 0 && state != 1) {
        return;
    }

    if (state == 0) {
        s32 sel = (M32(DH_READY_PLAYER_MASK_ADDR, 0) == 2) ? 2 : 1;
        M32(sub, 0x30) = M32(MP(DH_ACTOR_SLOT_TASKS_ADDR + sel * 4, 0), 8);
        MP(sub, 0x34) = MP(MP(DH_VIEW_TASK_ADDR, 0), 8);
        M32(sub, 0x24) = 0;
        M32(sub, 0x28) = 27;
        M32(sub, 0x1c) = M8(obj, 0x0f) ? -0x25e : 0x25e;
        M8(obj, 0x10) = (s8)(state + 1);
    }

    M32(sub, 0x24) += M32(sub, 0x1c);
    {
        void *spr = MP(sub, 0x34);
        M16(spr, 0x26) = (s16)((sin_lookup(M32(sub, 0x24)) >> 2) + M32(sub, 0x18));
    }
    M32(sub, 0x28) -= 1;
    if (M32(sub, 0x28) == 0) {
        M32(obj, 0) = 0;
    }
}
