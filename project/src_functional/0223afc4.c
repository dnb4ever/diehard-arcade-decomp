/* actor_load_hit_pose [0x0223afc4]: Chooses a randomized hit/reaction descriptor and requests its pose and duration. */
#include "dh.h"
/* func_0223afc4(entity): choose a hit/reaction anim record and apply it.
 * If entity->unk34 (state) >= 3, roll rng for a 2-or-4 offset stored in unk98, else 0.
 * Index the 8-byte-record table g_0608CDCC by (unk6D + unk98): copy record word0 ->
 * unk5C, set unk41 = 1, copy record word2 (zero-extended) -> unk90. Returns 5.
 * fidelity: HIGH - offsets 0x34/0x6D/0x98/0x5C/0x41/0x90 and 8-byte stride verified
 *   from asm; func_02210020 = rng_next; unk98 seed is ((rng & 1) << 1) + 2. */
extern u32 game_random_next(void);   /* rng_next */
extern u8  g_0608CDCC[];           /* 8-byte hit/anim records */

s32 actor_load_hit_pose(void *ent) {
    u8 *rec;
    if (M8(ent, 0x34) >= 3) {
        M32(ent, 0x98) = ((s32)(game_random_next() & 1) << 1) + 2;
    } else {
        M32(ent, 0x98) = 0;
    }
    rec = &g_0608CDCC[(M8(ent, 0x6D) + M32(ent, 0x98)) * 8];
    M16(ent, 0x5C) = M16(rec, 0);
    M8(ent, 0x41) = 1;
    M32(ent, 0x90) = (u16)M16(rec, 4);
    return 5;
}
