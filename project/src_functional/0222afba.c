#include "dh.h"
/* 0x022afba (actor_pose_resolve_b) - sibling of func_0222ae40 (0x22ae40): resolve an actor's
 * 8-direction x 4-quadrant pose/animation from its facing angle, then publish its object pointer
 * (entity=r4). Two modes keyed on state flags unk84:
 *   if (unk84 & 0x60000): func_02227704(entity) (facing_resolve); unk88 = 0, request unk43 = 3,
 *       state flag -> 0x1000, table selector -> 1; if g_06096E90[unk34] != 0 run
 *       func_02223e70(entity).
 *   else: if (unk84 & 0x30) func_02227704(entity); else if (unk84 & 0x400) func_02223e70(entity);
 *       unk88 = 0, request unk43 = 4, state flag -> 0x2000, table selector -> 3;
 *       func_0222acf8(entity); unkC8 += 0x600; unk78 <<= 1.
 *   common tail: unk84 = state flag; frame = (*g_060866F8[sel])[dir*4 + quad] with
 *       dir = (u16)unk62 >> 13 (+4 & 7 when facing-flip unk44 set) and
 *       quad = (u16)(unk60 + 0x4000) >> 14; unk5C = frame; unk46 = 0; if (unk8C & 0x00FFFFFF)
 *       { func_0223a9d0(entity, 3); unk45 = 0; } func_022160a0(entity); publish
 *       g_06099780[unk34 - 1] = g_060987BC.
 * fidelity: HIGH - bsrf resolved: func_02227704 (0x227704), func_02223e70 (0x223e70),
 * func_0223a9d0 (0x23a9d0), func_022160a0 (0x2160a0); direct bsr func_0222acf8. unk34/43/44/45/46
 * byte, unk5C/60/62 word, unk78/84/88/8C/C8 long; g_06096E90 s16[], g_060866F8 ptr-table,
 * g_06099780 ptr[], g_060987BC pointer. */
extern void actor_reset_pose_history(void *entity);
extern void actor_resolve_facing(void *entity);              /* actor_facing_resolve */
extern void actor_compute_motion_components(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a);
extern void audio_queue_actor_cue(void *entity);
extern s16   g_06096E90[];
extern void *g_060866F8[];
extern void *g_06099780[];
extern void *g_frame_tick;

void actor_pose_resolve_b(void *entity) {
    s32 flag;
    s32 sel;
    void *tbl;
    u32 dir;
    u32 quad;

    if (M32(entity, 0x84) & 0x60000) {
        actor_resolve_facing(entity);
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 3;
        flag = 0x1000;
        sel = 1;
        if (g_06096E90[(s8)M8(entity, ACTOR_SLOT_ID)] != 0)
            actor_reset_pose_history(entity);
    } else {
        if (M32(entity, 0x84) & 0x30)
            actor_resolve_facing(entity);
        else if (M32(entity, 0x84) & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 4;
        flag = 0x2000;
        sel = 3;
        actor_compute_motion_components(entity);
        M32(entity, 0xC8) = M32(entity, 0xC8) + 0x600;
        M32(entity, 0x78) = M32(entity, 0x78) << 1;
    }

    M32(entity, 0x84) = flag;
    dir = (u32)MU16(entity, 0x62) >> 13;
    quad = (u32)(u16)(M16(entity, 0x60) + 0x4000) >> 14;
    if (M8(entity, 0x44) != 0)
        dir = (dir + 4) & 7;
    tbl = MP(g_060866F8[sel], 0);
    M16(entity, ACTOR_ANIM_REQUEST) = M16(tbl, (dir * 4 + quad) * 2);
    M8(entity, 0x46) = 0;
    if (M32(entity, 0x8C) & 0x00FFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    audio_queue_actor_cue(entity);
    g_06099780[(s8)M8(entity, ACTOR_SLOT_ID) - 1] = g_frame_tick;
}
