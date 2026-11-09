/* enemy_select_mode_reference [0x02245b48]: Mode 1/2 chooses player; mode 3 compares X distances with original greater-distance choice. */
#include "dh.h"
/* 0x02245b48  select a target object relative to reference arg1->field[0],
 * chosen by the global mode g_06098564. Candidates come from two pointer
 * chains hung off g_06099584+44:
 *   A = *(*(base+48) + 8)        B = *(*(base+52) + 8)
 *   mode 0 -> NULL
 *   mode 1 -> A
 *   mode 2 -> B
 *   mode 3 -> d1=|A[8]-ref|, d2=|B[8]-ref|; (d1>d2) ? A : B
 *   other -> arg1
 * fidelity: HIGH - exact (two-level @(8,..) derefs verified; abs via sign test). */
extern u8 g_actor_slot_registry[];
extern u32 g_ready_player_mask;
void *enemy_select_mode_reference(void *arg1) {
    switch (g_ready_player_mask) {
    case 0:
        return 0;
    case 1:
        return MP(MP(g_actor_slot_registry, 48), 8);
    case 2:
        return MP(MP(g_actor_slot_registry, 52), 8);
    case 3: {
        void *A = MP(MP(g_actor_slot_registry, 48), 8);
        void *B = MP(MP(g_actor_slot_registry, 52), 8);
        s32 ref = M32(arg1, 0);
        s32 d1 = M32(A, 8) - ref; if (d1 < 0) d1 = -d1;
        s32 d2 = M32(B, 8) - ref; if (d2 < 0) d2 = -d2;
        return (d1 > d2) ? A : B;
    }
    default:
        return arg1;
    }
}
