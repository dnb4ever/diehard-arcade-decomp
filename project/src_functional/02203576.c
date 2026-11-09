/* hud_status_two_players_task [0x02203576]: Reads resident peripheral state and emits periodic/event status records for both players. */
#include "dh_work.h"
#include "dh.h"
/* 0x02203576 - per-frame controller rumble/HUD-record service for both players.
 * entity = task->unk8. First issues the BIOS peripheral read (*g_06000660)(0x060987d8).
 * For each player (base 0x060987d8 ctx 0, base 0x060987dc ctx 0x16) there are two triggers:
 *   - a periodic one, run when (entity->unk50 & g_060987bc)==0, selected by the parity of a
 *     per-frame counter entity->unk40 (incremented once, in player-1's periodic block);
 *   - an event one, run when a bit of the flag word g_0609879a is set (bit0 -> p1, bit1 -> p2).
 * The "active" path builds a record at 0x0609d7b0: dispatch(*(g_060902cc + base[3]*4)) via the
 * global function-pointer slot *g_0609029c, then func_02203de8(base[0]>1?6:5) (23-byte icon copy)
 * and func_02203e0a(14, base) (5-byte status record), then dispatch(0x0609d7b0) to submit it. The
 * "idle" path (even parity) dispatches *g_060902f0 twice. The context globals g_060997a4 (ctx)
 * and g_060997a8 (0xA then 0xB) are written before every dispatch; the dispatch pointer is
 * re-fetched from its slot each call.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsr->0x203de8/0x203e0a (sibling leaves),
 * computed calls via *g_0609029c; base[0]/base[3] read unsigned; g_0609879a read as u16. */
#define G32(a)   (*(s32 *)(uintptr_t)(a))
#define GU16(a)  (*(u16 *)(uintptr_t)(a))

extern void hud_copy_status_template(s32 icon);          /* 0x02203de8 */
extern void hud_encode_status_item(s32 slot, u8 *base);/* 0x02203e0a */

static void build_record(void (**disp)(s32), s32 ctx, u8 *base) {
    G32(DH_UI_CURSOR_COLUMN_ADDR) = ctx;
    G32(DH_UI_CURSOR_ROW_ADDR) = 0x0A;
    (*disp)(G32((uintptr_t)0x060902cc + base[3] * 4));
    hud_copy_status_template(base[0] > 1 ? 6 : 5);
    hud_encode_status_item(14, base);
    G32(DH_UI_CURSOR_COLUMN_ADDR) = ctx;
    G32(DH_UI_CURSOR_ROW_ADDR) = 0x0B;
    (*disp)((s32)0x0609d7b0);
}

static void idle_record(void (**disp)(s32), s32 ctx) {
    G32(DH_UI_CURSOR_COLUMN_ADDR) = ctx;
    G32(DH_UI_CURSOR_ROW_ADDR) = 0x0A;
    (*disp)(G32(0x060902f0));
    G32(DH_UI_CURSOR_COLUMN_ADDR) = ctx;
    G32(DH_UI_CURSOR_ROW_ADDR) = 0x0B;
    (*disp)(G32(0x060902f0));
}

void hud_status_two_players_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void (**disp)(s32) = (void (**)(s32))(uintptr_t)0x0609029c;
    u8 *base1 = (u8 *)(uintptr_t)0x060987d8;
    u8 *base2 = (u8 *)(uintptr_t)0x060987dc;

    (*(void (**)(void *))(uintptr_t)0x06000660)((void *)(uintptr_t)0x060987d8);

    if (!(M32(entity, 0x50) & G32(DH_FRAME_TICK_ADDR))) {
        M32(entity, 0x40) += 1;
        if (M32(entity, 0x40) & 1)
            build_record(disp, 0, base1);
        else
            idle_record(disp, 0);
    }
    if (GU16(0x0609879a) & 1)
        build_record(disp, 0, base1);

    if (!(M32(entity, 0x50) & G32(DH_FRAME_TICK_ADDR))) {
        if (M32(entity, 0x40) & 1)
            build_record(disp, 0x16, base2);
        else
            idle_record(disp, 0x16);
    }
    if (GU16(0x0609879a) & 2)
        build_record(disp, 0x16, base2);
}
