/* prop_tick_decelerating_descent [0x0221c2aa]: Setpiece child subtracts velocity from X, decays by 0xc00 and draws model. */
/* 0x0221c2aa - task-handler template (address-taken, spawned by setpiece_director
 * 0x0221bf72). entity = task->unk8. A small 2-state descent integrator that then draws via
 * a shared draw routine:
 *   state 0: wait until entity->unk8 (a 16.16 position) reaches 0x00270000, then advance.
 *   state 1: decay the velocity entity->unk14 by 0x0C00 per frame; when it drops to <= 0,
 *            clamp it to 0 and advance.
 * Every frame: entity->unk8 -= entity->unk14 (integrate position by the remaining velocity),
 * then tail-call the shared model-draw routine func_0221c368(entity).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. m2c over-swallowed the following function:
 *   the real func_0221c2aa ends at the `bra 0x21c368 / mov.l @r15+,r14` epilogue, which is a
 *   fall-through tail-call into func_0221c368 (a confirmed separate entry: void(void*), the
 *   shared braf tail of func_02242a10). Represented here as an explicit call. */
#include "dh_band21.h"

extern void model_draw_title_composite(void *obj);   /* shared model-draw tail */

void prop_tick_decelerating_descent(u32 task)
{
    u32 entity = MEMU32(task + 8);

    switch (MEM8(task + 0x10)) {
    case 0:
        if (MEM32(entity + 8) < 0x00270000)
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        break;
    case 1:
        MEM32(entity + 0x14) -= 0x0c00;
        if (MEM32(entity + 0x14) <= 0) {
            MEM32(entity + 0x14) = 0;
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        }
        break;
    default:
        break;
    }

    MEM32(entity + 8) -= MEM32(entity + 0x14);
    model_draw_title_composite((void *)(uintptr_t)entity);
}
