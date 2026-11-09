/* enemy_dispatch_distance_response [0x0224806a]: Chooses a descriptor response using actor separation and a random subindex. */
#include "dh.h"
/* 0x0224806a  actor proximity/response dispatch. ABI (r4=self, r5=other).
 * Computes the horizontal distance band between two actors and looks up a response
 * code in a 2-D table, then either applies it or dispatches a handler:
 *   tbl   = self[0x15c]                       (row-pointer array)
 *   diff  = self[8] - other[8]
 *   dist  = |diff|, clamped: max(dist - 0x6666, 0)
 *   idx   = (dist & 0x1c000) >> 14            (0..7 distance band)
 *   sub   = func_02210020(idx, other, diff, other[8]) & 3
 *   code  = ((s32 *)tbl[idx])[sub]
 *   if (code > 0)  { actor_init_from_cfg(self, code); return 1; }
 *   else { fp = g_06090250[~code]; fp(self, (~code) * 4); return 0; }
 * Both bsrf resolved (braf_resolve). fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern s32  game_random_next(s32 idx, void *other, s32 diff, s32 other_x);
extern void actor_init_from_cfg(void *self, s32 code);

s32 enemy_dispatch_distance_response(void *self, void *other) {
    void **tbl = (void **)MP(self, 0x15C);
    s32 diff = M32(self, 8) - M32(other, 8);
    s32 dist = (diff >= 0) ? diff : (M32(other, 8) - M32(self, 8));
    s32 idx;
    s32 sub;
    s32 code;

    dist -= 0x6666;
    if (dist < 0) {
        dist = 0;
    }
    idx = (dist & 0x0001C000) >> 14;
    sub = game_random_next(idx, other, diff, M32(other, 8)) & 3;
    code = ((s32 *)tbl[idx])[sub];
    if (code > 0) {
        actor_init_from_cfg(self, code);
        return 1;
    }
    {
        void (*fp)(void *, s32) = (void (*)(void *, s32))((void **)(uintptr_t)0x06090250)[~code];
        fp(self, (~code) * 4);
    }
    return 0;
}
