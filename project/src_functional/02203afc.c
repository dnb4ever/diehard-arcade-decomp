/* hud_status_one_player_task [0x02203afc]: Reads resident peripheral state and emits periodic/event status records for one player. */
#include "dh_work.h"
#include "dh.h"
/* 0x02203afc - sibling of func_02203576: per-frame controller rumble/HUD-record service,
 * single player (base 0x060987d8), context code 0xB. entity = task->unk8. First the BIOS
 * peripheral read (*g_06000660)(0x060987d8), then two triggers:
 *   - periodic, run when (entity->unk50 & g_060987bc)==0, selected by parity of entity->unk40
 *     (incremented once here): odd -> build record, even -> idle record;
 *   - event, run when (g_0609879a & 3) != 0 -> build record.
 * build: dispatch(*(g_060902cc + base[3]*4)) via *g_0609029c, func_02203de8(base[0]>1?6:5),
 * func_02203e0a(14, base), dispatch(0x0609d7b0); context globals g_060997a4=0xB and
 * g_060997a8 (0xA then 0xB) written before each dispatch.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. Identical record-build pattern to 0x02203576
 * with ctx 0xB and the &3 event mask; dispatch pointer re-fetched each call. */
#define G32(a)   (*(s32 *)(uintptr_t)(a))
#define GU16(a)  (*(u16 *)(uintptr_t)(a))

extern void hud_copy_status_template(s32 icon);           /* 0x02203de8 */
extern void hud_encode_status_item(s32 slot, u8 *base); /* 0x02203e0a */

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

void hud_status_one_player_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void (**disp)(s32) = (void (**)(s32))(uintptr_t)0x0609029c;
    u8 *base1 = (u8 *)(uintptr_t)0x060987d8;

    (*(void (**)(void *))(uintptr_t)0x06000660)((void *)(uintptr_t)0x060987d8);

    if (!(M32(entity, 0x50) & G32(DH_FRAME_TICK_ADDR))) {
        M32(entity, 0x40) += 1;
        if (M32(entity, 0x40) & 1)
            build_record(disp, 0x0B, base1);
        else
            idle_record(disp, 0x0B);
    }
    if (GU16(0x0609879a) & 3)
        build_record(disp, 0x0B, base1);
}
