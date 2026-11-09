#include "dh_work.h"
/* enemy_wave_test_complete [0x022465fc]: Notifies the parent when no actors remain and checks the final enemy's completion flag. */
#include "dh.h"
/* 0x022465fc  react to the active-slot count g_06099584[+8].
 * n==0: obj = *(*(p+20)+8); obj->field[60] |= 8; p->field[0] = 0.
 * n==1: scan slots 3..6 (flag at +12+i*4); at the first enabled slot,
 *       obj = *(*(base+44+i*4)+8); if (obj->field[0xe0] & 0x800) g_06097410=1;
 *       then stop (only the first enabled slot is examined).
 * other n: nothing.
 * fidelity: HIGH - exact (loop i=3..6; both slot sub-branches exit the loop). */
extern u8 g_actor_slot_registry[];
extern s32 g_06097410;
void enemy_wave_test_complete(void *p) {
    s32 n = M32(g_actor_slot_registry, 8);
    if (n == 0) {
        void *obj = MP(MP(p, 20), 8);
        M32(obj, 60) |= 8;
        M32(p, 0) = 0;
        return;
    }
    if (n != 1)
        return;
    for (s32 i = 3; i < 7; i++) {
        if (M32(g_actor_slot_registry, 12 + i * 4) != 0) {
            void *obj = MP(MP(g_actor_slot_registry, 44 + i * 4), 8);
            if (ACTOR_M32(obj, state_flags) & 0x800)
                g_06097410 = 1;
            return;
        }
    }
}
