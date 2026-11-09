/* actor_jump_anim_select [0x0222baf8]: Existing map corroborated by source: airborne/special-move anim selector (sibling of 0x0222ae40); 4-way classify of unk84 (0x82000/0x318000/0x60000/else) -> 3-level anim table g_060866f8[group][sub][dir*4+col]; unk74=0x3000, unk14*=2 */
#include "dh.h"
/* 0x0222baf8 (actor_jump_anim_select) - player-actor airborne/special-move animation selector
 * (entity=r4); sibling of actor_facing_anim_select (0x0222ae40). Forces unk74=0x3000, unk60=0x2000,
 * then classifies the motion-state flags unk84:
 *   unk84 & 0x82000           : group=2, unk88=7, sub=0.
 *   else unk84 & 0x318000     : group=3, unk88=0, sub = (u8)unk49 > 5 ? 1 : 0.
 *   else unk84 & 0x60000      : group=3; if (g_06096e90[unk34]) func_02223e70(entity); then
 *                               func_02227704(entity); unk88=0; sub = (u8)unk49 > 5 ? 1 : 0.
 *   else                      : group=3; if (unk84 & 0x30) func_02227704(entity) else if (unk84 &
 *                               0x400) func_02223e70(entity); unk88=0; sub = (u8)unk49 > 5 ? 1 : 0.
 * Common tail: func_0222acf8(entity); unk14 *= 2; unk84 = 0x2000; unk43 = 4;
 *   dir = (u16)unk62 >> 13 (if unk44: (dir+4)&7); col = (u16)(unk60 + 0x4000) >> 14;
 *   unk5C = (*(*(g_060866f8[group]))[sub])[(dir*4 + col)*2]   (3-level anim table lookup);
 *   unk46 = 0; if (unk8C & 0x00ffffff) { func_0223a9d0(entity, 3); unk45 = 0; }
 *   g_06099780[unk34 - 1] = g_060987bc.
 * Branches resolved (braf_resolve): bsrf -> 0x02223e70, 0x02227704, 0x0223a9d0; bsr -> 0x0222acf8.
 * unk34/43/44/45/46/49 byte, unk5C/60/62 word, unk14/74/84/88/8C long.
 * fidelity: HIGH - dir index is unk62 >> 13 (shlr8+shlr2+shlr2+shlr @0x22bc14-0x22bc1a),
 * byte offset into the anim row is (dir*4 + col)*2 per shll2 r4 @0x22bc2e + add r5 + shll r0. */
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern void actor_reset_pose_history(void *entity);          /* 0x02223e70 (box set B) */
extern void actor_resolve_facing(void *entity);          /* 0x02227704 (box set A) */
extern void actor_compute_motion_components(void *entity);          /* 0x0222acf8 */
extern void actor_commit_effect_handoffs(void *entity, s32 a1);  /* 0x0223a9d0 */

void actor_jump_anim_select(void *entity) {
    s32 flags = M32(entity, 0x84);
    s32 group;
    s32 sub;
    u32 dir;
    u32 col;
    void *tbl;

    M32(entity, 0x74) = 0x3000;
    M16(entity, 0x60) = 0x2000;

    if (flags & 0x82000) {
        group = 2;
        M32(entity, 0x88) = 7;
        sub = 0;
    } else {
        group = 3;
        if (flags & 0x318000) {
            M32(entity, 0x88) = 0;
            sub = ((u8)M8(entity, 0x49) > 5) ? 1 : 0;
        } else if (flags & 0x60000) {
            if (R16A(0x06096E90 + M8(entity, ACTOR_SLOT_ID) * 2) != 0)
                actor_reset_pose_history(entity);
            actor_resolve_facing(entity);
            M32(entity, 0x88) = 0;
            sub = ((u8)M8(entity, 0x49) > 5) ? 1 : 0;
        } else {
            if (flags & 0x30)
                actor_resolve_facing(entity);
            else if (flags & 0x400)
                actor_reset_pose_history(entity);
            M32(entity, 0x88) = 0;
            sub = ((u8)M8(entity, 0x49) > 5) ? 1 : 0;
        }
    }

    actor_compute_motion_components(entity);
    M32(entity, 0x14) *= 2;
    M32(entity, 0x84) = 0x2000;
    M8(entity, 0x43) = 4;

    col = (u16)(M16(entity, 0x60) + 0x4000) >> 14;
    dir = (u16)MU16(entity, 0x62) >> 13;
    if (M8(entity, 0x44) != 0)
        dir = (dir + 4) & 7;

    tbl = PA((uintptr_t)PA(0x060866F8 + group * 4) + sub * 4);
    M16(entity, ACTOR_ANIM_REQUEST) = R16A((uintptr_t)tbl + (dir * 4 + col) * 2);

    M8(entity, 0x46) = 0;
    if (M32(entity, 0x8C) & 0x00FFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    R32A(0x06099780 + (M8(entity, ACTOR_SLOT_ID) - 1) * 4) = R32A(DH_FRAME_TICK_ADDR);
}
