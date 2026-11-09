/* effect_tick_blink_countdown [0x022462d0]: Decrements work+88, draws alternate frames then releases slot on expiry. */
#include "dh_work.h"
#include "dh.h"
/* 0x022462d0 - func_022462d0(task): countdown tick for a drawn actor/effect.
 *   entity = *(task+8);
 *   c = entity->88; entity->88 = c - 1;
 *   if (c > 0) {                               // still counting
 *       if (((c - 1) & 1) != 0)                // every other frame
 *           func_02246282(entity);             // draw (tail call)
 *       return;
 *   }
 *   // counter expired:
 *   slot = (s8)entity->52;
 *   if (slot > 8) g_060996d4[slot - 8] = 0;    // release HUD slot
 *   *(s32*)task = 0;                            // kill task (clear handler ptr)
 * fidelity: HIGH - no phantom branches. 'c' tested with cmp/pl on the ORIGINAL
 * value; the (c-1)&1 test reads the stored decremented value. slot is a signed
 * byte; g_060996d4 freed as a byte. The task->0 store deactivates the task. The
 * draw call is a tail branch (bra func_02246282); modeled as a plain void call. */
extern void actor_draw_transformed(void *entity);        /* 0x02246282 */
extern u8   g_effect_slot_occupied_legacy[];                        /* HUD slot allocation table */

void effect_tick_blink_countdown(void *task)
{
    void *entity = TASK_PTR(task, work);
    s32 c = M32(entity, 88);

    M32(entity, 88) = c - 1;

    if (c > 0) {
        if (((c - 1) & 1) != 0)
            actor_draw_transformed(entity);
        return;
    }

    {
        s32 slot = M8(entity, 52);
        if (slot > 8)
            MU8(g_effect_slot_occupied_legacy, slot - 8) = 0;
    }
    TASK_S32(task, handler) = 0;
}
