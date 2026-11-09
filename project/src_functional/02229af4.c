#include "dh_work.h"
#include "dh.h"
/* 0x0229af4 (actor_launch_countdown) - the hidden sibling of actor_launch_dispatch
 * (0x0222992c). It sits immediately after that function's braf tail-call boundary
 * (find_entry/fn.units merges them; real entry 0x229af4, extent 0x229af4..0x229b2e).
 *
 * ABI (task, entity): task = r4, entity = r5 (kept in r14). It HEAD-calls
 * animation_update (0x0222dc2c) first, then runs a settle countdown:
 *   if (entity->unk54 & 2) {                  this frame's loop flag
 *       if (--entity->unkC4 == 0) {           arc timer expired
 *           entity->unkCE = 1;
 *           entity->unkC8 = 0x3C;             (60)
 *           entity->unkCC = 1;
 *           entity->unk43 = 0x1E;             (30) request settle pose
 *       }
 *   }
 * This is the post-animation re-arm counterpart to the dispatcher's case 0x55/0x56
 * settle path (same unkCE/unkC8/unkCC/unk43 fields).
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm. bsrf resolved:
 * animation_update (0x0222dc2c). SH-2 immediates were decimal (#84=0x54, #60=0x3C,
 * #30=0x1E); offsets confirmed against m2c. */

extern s32 animation_update(void *task, void *entity);  /* 0x0222dc2c */

void actor_launch_countdown(void *task, void *entity) {
    animation_update(task, entity);

    if (ACTOR_U16(entity, animation_events) & 2) {
        if (--M32(entity, 0xC4) == 0) {
            M16(entity, 0xCE) = 1;
            M32(entity, 0xC8) = 0x3C;
            M16(entity, 0xCC) = 1;
            ACTOR_WORK(entity)->state_handler  = 0x1E;
        }
    }
}
