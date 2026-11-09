/* actor_facing_anim_select [0x0222ae40]: Existing map corroborated by source: per-frame facing/anim selector; picks unk5C from a 2-D table via (unk62 octant, unk60 pitch); sets state flag unk84 (0x1000/0x2000), unk78 facing-velocity; installs hit boxes via func_02227704/func_02223e70; publishes g_06099780[unk34-1] */
#include "dh.h"
/* 0x0222ae40 (actor_facing_anim_select) - player-actor per-frame facing/animation selector
 * (entity=r4). Reads the current motion-state flags unk84 and branches:
 *   unk84 & 0x60000 (walking/8-dir locomotion):
 *       func_02227704(entity);                        (resolve facing via transform)
 *       unk88 = 0; unk43 = 3; next state flag = 0x1000;
 *       if (g_06096e90[unk34] != 0) func_02223e70(entity);
 *       unkC8 = 0;
 *       dir = (u16)unk62 >> 13;  if (unk44) dir = (dir + 4) & 7;   (facing octant + 180 flip)
 *       col = (u16)(unk60 + 0x4000) >> 14;                         (pitch quadrant)
 *       unk5C = (**0x060866fc)[(dir*4 + col)*2];      (anim id from 2-D table via double ptr)
 *   else (2-dir / facing-only):
 *       if (unk84 & 0x30)  func_02227704(entity);
 *       else if (unk84 & 0x400) func_02223e70(entity);
 *       unk88 = 8; unk43 = 7; next state flag = 0x2000;
 *       dir = (u16)unk62 >> 15; if (unk44) dir ^= 1;
 *       unk5C = g_0608672c[dir];
 *       unk78 = ((u16)unk62 > 0x8000) ? 0x8000 : -0x8000;
 *   common tail: unk84 = next flag; unk46 = 0;
 *       if (unk8C & 0x00ffffff) { func_0223a9d0(entity, 3); unk45 = 0; }
 *       g_06099780[unk34 - 1] = g_060987bc;
 * Branches resolved: bsrf -> 0x0223a9d0; mova+add fn-ptrs -> 0x02227704, 0x02223e70.
 * fidelity: HIGH - walking shift is shlr8+shlr2+shlr2+shlr = >>13; the 2-dir branch has an
 *  extra shlr2 = >>15. */
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))

extern void actor_resolve_facing(void *entity);  /* 0x02227704 */
extern void actor_reset_pose_history(void *entity);  /* 0x02223e70 */
extern void actor_commit_effect_handoffs(void *entity, s32 a1);  /* 0x0223a9d0 */

void actor_facing_anim_select(void *entity) {
    s32 flags = M32(entity, 0x84);
    s32 next_flag;

    if (flags & 0x60000) {
        u32 dir;
        u32 col;
        void *tbl;

        actor_resolve_facing(entity);
        next_flag = 0x1000;
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 3;
        if (R16A(0x06096E90 + M8(entity, ACTOR_SLOT_ID) * 2) != 0)
            actor_reset_pose_history(entity);
        M32(entity, 0xC8) = 0;

        dir = (u16)M16(entity, 0x62) >> 13;
        if (M8(entity, 0x44) != 0)
            dir = (dir + 4) & 7;
        col = (u16)(M16(entity, 0x60) + 0x4000) >> 14;
        tbl = *(void **)(uintptr_t)(*(void **)(uintptr_t)0x060866FC);
        M16(entity, ACTOR_ANIM_REQUEST) = R16A((uintptr_t)tbl + (dir * 4 + col) * 2);
    } else {
        u32 dir;

        if (flags & 0x30)
            actor_resolve_facing(entity);
        else if (flags & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 8;
        next_flag = 0x2000;
        M8(entity, 0x43) = 7;

        dir = (u16)M16(entity, 0x62) >> 15;
        if (M8(entity, 0x44) != 0)
            dir ^= 1;
        M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x0608672C + dir * 2);

        if ((u16)M16(entity, 0x62) > 0x8000)
            M32(entity, 0x78) = 0x8000;
        else
            M32(entity, 0x78) = -0x8000;
    }

    M32(entity, 0x84) = next_flag;
    M8(entity, 0x46) = 0;
    if (M32(entity, 0x8C) & 0x00FFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    R32A(0x06099780 + (M8(entity, ACTOR_SLOT_ID) - 1) * 4) = R32A(DH_FRAME_TICK_ADDR);
}
