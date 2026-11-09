#include "dh.h"
/* 0x02247508 / 0x02247530 / 0x02247580 - three entry-variant wrappers that
 * launch func_022473ee (0x2473ee) with a randomly chosen move descriptor.
 *
 * Each reads the descriptor block *(entity+0x174), takes one of its three
 * sub-lists (at +0x0, +0x4, +0x8), picks element 0 or 1 of that list by
 * rng_next()&1, and tail-calls func_022473ee(entity, chosen).
 *
 * In SH-2 the descriptor read is in the rng_next bsrf delay slot:
 *   r2 = *(entity+0x174); r8 = *(void**)(r2 + K);  // K = 0, 4, 8
 *   r0 = rng_next();  r5 = *(u32*)(r8 + (r0 & 1)*4);  bra func_022473ee
 *
 * fidelity: HIGH - hand-traced vs m2c_fn.py --asm 0x2473ee lines for 0x247508/
 * 0x247530/0x247580 (m2c itself bails: "cannot find branch target func_022473ee",
 * i.e. the tail-call out of range). bsrf targets resolve to rng_next (0x210020)
 * via braf_resolve (deltas 0xfffc8b00 / 0xfffc8ad8 / 0xfffc8a88 @ 0x24751c /
 * 0x247544 / 0x247594). */

extern u32  rng_next(void);                            /* 0x210020 */
extern void enemy_begin_scripted_dash(void *entity, void *desc); /* 0x2473ee */

/* 0x02247508 */
void actor_move_launch0(void *entity) {
    void *list = MP(MP(entity, 0x174), 0x0);
    enemy_begin_scripted_dash(entity, MP(list, (rng_next() & 1) * 4));
}

/* 0x02247530 */
void actor_move_launch1(void *entity) {
    void *list = MP(MP(entity, 0x174), 0x4);
    enemy_begin_scripted_dash(entity, MP(list, (rng_next() & 1) * 4));
}

/* 0x02247580 */
void actor_move_launch2(void *entity) {
    void *list = MP(MP(entity, 0x174), 0x8);
    enemy_begin_scripted_dash(entity, MP(list, (rng_next() & 1) * 4));
}
