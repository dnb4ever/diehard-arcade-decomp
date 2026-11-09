/* effect_tick_even_frame_emitter [0x0223ee1a]: Live slot and state gates; even frames spawn 06042f5a child with copied slot. */
#include "dh.h"
/* 0x0223ee1a (callC) - idx = *(s8)( *(arg0+8) + 52 ).
 * If 0x06099590[idx]==0, OR ( *( *(0x060995B0[idx]) +8 ) field@0xE0 & 0x800 ): clear *arg0, return.
 * If (*(s32)0x060987BC & 1): return (no spawn).
 * Else: n = spawn_obj_a(arg0, resume=0x06042F5A); obj2 = *(void**)(n+8);
 *   (obj2+32) |= 0x100 (word); (obj2+52) = *(s8)( *(arg0+8) + 52 ) (byte);
 *   copy sprite table 0x06097E08[256*idx] -> obj2+8/12/16;
 *   (obj2+24) = (rng_next() & 0x3FF) + 0x400 (long).
 * Calls: spawn_obj_a cart 0x20feec; rng_next cart 0x210020. resume = func_0223ef5a @ RAM 0x06042F5A.
 * fidelity: MED - control flow faithful; idx byte; tables stride 4/256; field widths per mov. */
extern s32 g_actor_slot_ids;   /* 0x06099584 + 12 */
extern s32 g_actor_slot_tasks;   /* 0x06099584 + 44 */
extern s32 g_frame_tick;
extern s8  g_06097E08;
extern void *spawn_obj_a(void *, void *);
extern s32 rng_next(void);
void effect_tick_even_frame_emitter(void *arg0) {
    void *o = MP(arg0, 0x8);
    s32 idx = M8(o, 52);
    void *q;
    if ((&g_actor_slot_ids)[idx] == 0) {
        M32(arg0, 0) = 0;
        return;
    }
    q = MP((void *)(uintptr_t)(&g_actor_slot_tasks)[idx], 8);
    if (M32(q, 0xE0) & 0x800) {
        M32(arg0, 0) = 0;
        return;
    }
    if (g_frame_tick & 1)
        return;
    {
        void *n = spawn_obj_a(arg0, (void *)0x06042F5A);
        void *obj2 = MP(n, 8);
        void *e = &g_06097E08 + (idx << 8);
        M16(obj2, 32) = (s16)(M16(obj2, 32) | 0x100);
        M8(obj2, 52) = M8(o, 52);
        M32(obj2, 8)  = M32(e, 0);
        M32(obj2, 12) = M32(e, 4);
        M32(obj2, 16) = M32(e, 8);
        M32(obj2, 24) = (rng_next() & 0x3FF) + 0x400;
    }
}
