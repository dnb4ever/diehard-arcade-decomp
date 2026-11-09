/* scene_tick_periodic_random_spawn [0x0221e844]: RNG low five bits plus sixty sets interval; spawns child 060228c8 then repeats. */
/* 0x0221e844  periodic_spawner: 3-state (obj[0x10]) timer that periodically
 * triggers a spawn. sub = obj[8].
 *   state 0: obj[0x10]++  (fall through to state 1).
 *   state 1: sub[0x40] = (func_02210020(obj) & 0x1F) + 60;  obj[0x10]++.
 *   state 2: if (--sub[0x40] reached 0 or below) { func_0220ff54(obj, 0x060228c8);
 *            obj[0x10] = 1; }   (restart the cycle)
 * fidelity: faithful - bsrf resolved to func_02210020 / func_0220ff54; the spawn
 *   argument 0x060228c8 is an in-ROM descriptor; countdown uses pre-decrement test. */
#include "dh.h"

extern s32 game_random_next(void *obj);
extern void task_spawn_current_work40(void *obj, u32 arg);

void scene_tick_periodic_random_spawn(void *obj) {
    void *sub = MP(obj, 8);
    s8 state = M8(obj, 0x10);

    switch (state) {
    case 0:
        M8(obj, 0x10) = (s8)(state + 1);
        /* fall through */
    case 1:
        M32(sub, 0x40) = (game_random_next(obj) & 0x1f) + 60;
        M8(obj, 0x10) = (s8)(M8(obj, 0x10) + 1);
        break;
    case 2: {
        s32 v = M32(sub, 0x40);
        M32(sub, 0x40) = v - 1;
        if (v <= 0) {
            task_spawn_current_work40(obj, 0x060228c8);
            M8(obj, 0x10) = 1;
        }
        break;
    }
    }
}
