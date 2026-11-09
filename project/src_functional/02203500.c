/* hud_blink_status_task [0x02203500]: Alternates two HUD status records under the frame-mask gate. */
#include "dh_work.h"
#include "dh.h"
/* 0x02203500 - task handler (controller-rumble / dispatch pulse). entity = task->unk8.
 * Gated: runs only when (g_060987bc & entity->unk50) == 0. Then bumps a per-frame
 * parity counter entity->unk40 and issues two calls through the global dispatch
 * function-pointer slot *g_0609029c, writing the two context globals g_060997a4 (=0xB)
 * and g_060997a8 (=0xA then =0xB) before each. The argument differs by parity:
 *   odd  -> dispatch(*g_060902d4) then dispatch(*g_060902dc)
 *   even -> dispatch(*g_060902f0) then dispatch(*g_060902f0)
 * The dispatch pointer is re-fetched from the global slot before every call.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. gate is tst (bitwise-AND nonzero -> skip);
 * bt.s selects the EVEN branch (T set when counter&1==0). unk40/unk50 are longs. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))

void hud_blink_status_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void (**dispatch)(s32) = (void (**)(s32))(uintptr_t)0x0609029c;
    s32 n;

    if (G32(DH_FRAME_TICK_ADDR) & M32(entity, 0x50))
        return;

    n = M32(entity, 0x40) + 1;
    M32(entity, 0x40) = n;

    if (n & 1) {
        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0xB;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0xA;
        (*dispatch)(G32(0x060902d4));
        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0xB;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0xB;
        (*dispatch)(G32(0x060902dc));
    } else {
        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0xB;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0xA;
        (*dispatch)(G32(0x060902f0));
        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0xB;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0xB;
        (*dispatch)(G32(0x060902f0));
    }
}
