/* evt_proximity_spawn [0x0221e9c6]: Existing map corroborated by source: 2D box proximity test -> spawn body 0x227906, ret -1/0 */
#include "dh.h"
/* 0x0221e9c6  evt_proximity_spawn - proximity test + conditional spawn helper.
 * Resolves the second half of the previously-deferred pool-contaminated cluster at
 * 0x0221e8c4 (find_entry.py: the census member 0x21e8c4 is a trailing pool of the
 * previous function; the two real entries are 0x0221e8c8 and this one). Called by
 * func_0221e8c8 for each active player.
 *
 * s32 f(a, b): if the 2D distance between a and b exceeds the box
 *   |a->unk8 - b->unk8| >= 0x8000  ||  |a->unk10 - b->unk10| >= 0xA000
 * return 0 (out of range). Otherwise spawn a body via func_0220feec(*0x06099580,
 * handler 0x02227906), copy its position (unk8/unkC/unk10) from the source object
 * g_060995B0[b->unk34]->unk8, set spawned->unk60 = b->unk34 - 1, spawned->unk64 = 8,
 * advance the per-slot counter g_06099788[b->unk34 - 1] += 8 (clamped to
 * g_06099794*2), and return -1 (in range / spawned).
 *
 * fidelity: HIGH. bsrf resolved: func_0220feec 0x20feec (spawn); mova+add handler
 * pointer resolved to 0x02227906 (ptr_resolve.py). The table lookup indexes by
 * (s8)b->unk34 (stride 4) while the counter/unk60 use (s8)b->unk34 - 1 (stride 2),
 * exactly as the asm (two exts.b of the same byte). Matches m2c_fn.py --asm. */

#define GP(a)   (*(void **)(uintptr_t)(a))
#define G16(a)  (*(s16 *)(uintptr_t)(a))

extern void *task_spawn_child_work88(void *parent, void *handler);  /* 0x20feec */
extern void  effect_handoff_sprite_task(void *task);                   /* handler (passed by ptr) */

s32 evt_proximity_spawn(void *a, void *b) {
    s32 dx = M32(a, 0x8) - M32(b, 0x8);
    s32 dy;
    void *obj, *se, *src;
    s32 slot;
    s16 *ctr;
    s16 clamp;

    if (dx < 0) dx = -dx;
    if (dx >= 0x8000) return 0;

    dy = M32(a, 0x10) - M32(b, 0x10);
    if (dy < 0) dy = -dy;
    if (dy >= 0xa000) return 0;

    obj = task_spawn_child_work88(GP(0x06099580), (void *)effect_handoff_sprite_task);
    se  = MP(obj, 0x8);
    slot = (s8)M8(b, 0x34);
    src = MP(MP(DH_ACTOR_SLOT_TASKS_ADDR, slot * 4), 0x8);      /* g_060995B0[slot]->unk8 */

    M32(se, 0x8)  = M32(src, 0x8);
    M32(se, 0xC)  = M32(src, 0xC);
    M32(se, 0x10) = M32(src, 0x10);
    M32(se, 0x60) = slot - 1;
    M32(se, 0x64) = 8;

    clamp = (s16)(G16(0x06099794) * 2);
    ctr = (s16 *)(uintptr_t)(0x06099788 + (slot - 1) * 2);
    *ctr = (s16)(*ctr + 8);
    if (*ctr > clamp) *ctr = clamp;

    return -1;
}
