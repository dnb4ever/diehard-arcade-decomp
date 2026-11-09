/* enemy_test_reference_facing [0x02245bcc]: State-flag override then selected peer/heading test; mode-zero raw return preserved. */
#include "dh_work.h"
#include "dh.h"
/* 0x02245bcc  test whether actor p faces the correct side of a chosen reference
 * object. If field[0xe0] & 0x40 -> return 1. mode = g_06098564; mode 0 returns
 * the raw field[0x34] value (52, faithful oddity: r0 left holding #52).
 * Otherwise pick a slot index sel:
 *   mode 1 -> 1;  mode 2 -> 2;
 *   mode 3 -> elem = g_060995d0[field[0x34]] (32-byte); a=elem[+4], b=elem[+8]
 *             (field[0x128]==1) ? ((a>b && b<0x24000)?2:1)
 *                               : ((a<b && a<0x24000)?1:2)   (unsigned cmp)
 *   other  -> field[0x34]
 * t = *(g_060995b0[sel] + 8); cmp = (field[8] >= t->field[8]);
 * facing = field[0x44]; return (cmp != 0) == (facing != 0) ? 1 : 0.
 * fidelity: HIGH - exact (cmp/hi,cmp/hs unsigned; cmp/ge signed; facing is a
 * signed byte tested for nonzero). */
extern u32 g_ready_player_mask;
extern s32 g_060995d0[];
extern void *g_actor_slot_tasks_legacy[];
s32 enemy_test_reference_facing(void *p) {
    if (ACTOR_M32(p, state_flags) & 0x40)
        return 1;
    u32 mode = g_ready_player_mask;
    if (mode == 0)
        return 52;
    s32 sel;
    s8 f52 = ACTOR_M8(p, slot_id);
    switch (mode) {
    case 1:
        sel = 1;
        break;
    case 2:
        sel = 2;
        break;
    case 3: {
        s32 *elem = &g_060995d0[f52 * 8];   /* 32-byte stride */
        u32 a = (u32)elem[1], b = (u32)elem[2];
        if (ACTOR_M32(p, player_target_slot) == 1)
            sel = (a > b && b < 0x24000u) ? 2 : 1;
        else
            sel = (a < b && a < 0x24000u) ? 1 : 2;
        break;
    }
    default:
        sel = f52;
        break;
    }
    void *t = TASK_PTR(g_actor_slot_tasks_legacy[sel], work);
    s32 cmp = (ACTOR_M32(p, position_x) >= M32(t, 8));
    s32 facing = (ACTOR_M8(p, facing_right) != 0);
    return (cmp != 0) == (facing != 0) ? 1 : 0;
}
