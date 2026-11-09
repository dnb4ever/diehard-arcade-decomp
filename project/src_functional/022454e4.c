/* enemy_choose_and_start_motion [0x022454e4]: Facing/direction plus output action/side select motion table then actor_init_from_cfg. */
#include "dh_work.h"
#include "dh.h"
/* 0x022454e4  choose the actor's next motion/action index and hand it to the move
 * controller (actor_init_from_cfg). Sets retrigger unk41=1 and facing from unk4D bit1:
 *   (unk4D & 2) ? (unk26=0x4000, unk44=1) : (unk26=0xC000, unk44=0);  side = unk4D & 1.
 * Special case when unkE4==6 and unk6A<3:
 *   if (unkE0 & 0x400000): unk5C = side ? 0xE3 : 0xE1; return 6;
 *   else                 : unk5C = g_0608E2D8[side];   return 5.
 * Otherwise query func_02245632(actor, &action, &side) (fills action, may OR side).
 * If action==0, consult func_02245a6a: compare the chosen peer's x
 *   (g_060995B0[(mode&1)+1]->unk8->unk8) to ours; if on our facing side set the
 *   turn bit unkE0|0x10000 and side|=2.
 * If action==3, mix in a random bit: side = side*2 + (rng()&1).
 * Fetch the motion id: action!=4 -> unk16C[action][side]; action==4 ->
 *   g_06091A3C[side]; call actor_init_from_cfg(actor, id); return action.
 * fidelity: HIGH - widths/branches from asm; two-level peer and table derefs verified. */
extern s16 g_0608E2D8[];     /* facing -> idle anim id */
extern void *g_actor_slot_tasks[];   /* slot -> peer wrapper (cluster 0x06099584 + 44) */
extern s32 g_06091A3C[];     /* fallback motion ids */
extern s32 game_random_next(void);                 /* rng 0x02210020 */
void enemy_choose_action_and_side(void *actor, s32 *action, s32 *side);  /* 0x02245632 */
s32 enemy_select_player_side(void *actor);                 /* 0x02245a6a */
void actor_init_from_cfg(void *actor, s32 id);        /* 0x02244988 */

s32 enemy_choose_and_start_motion(void *arg0) {
    s32 **tbl = ACTOR_MP(arg0, motion_action_tables);
    s32 action = 0;
    s32 side;

    ACTOR_M8(arg0, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    side = ACTOR_M8(arg0, direction_code);
    if (side & 2) {
        ACTOR_M8(arg0, facing_right) = 1;
        ACTOR_M16(arg0, angle_y) = 0x4000;
    } else {
        ACTOR_M8(arg0, facing_right) = 0;
        ACTOR_M16(arg0, angle_y) = (s16)0xC000;
    }
    side &= 1;

    if (ACTOR_M32(arg0, state_index) == 6 && ACTOR_M8(arg0, saved_primary_effect_type) < 3) {
        if (ACTOR_M32(arg0, state_flags) & 0x400000) {
            ACTOR_M16(arg0, animation_requested) = (s16)(side != 0 ? 0xE3 : 0xE1);
            return 6;
        }
        ACTOR_M16(arg0, animation_requested) = g_0608E2D8[side];
        return 5;
    }

    enemy_choose_action_and_side(arg0, &action, &side);
    if (action == 0) {
        s32 res = enemy_select_player_side(arg0);
        if (res != 0) {
            void *peer = TASK_PTR(g_actor_slot_tasks[(res & 1) + 1], work);
            s32 dx = M32(peer, 8) - ACTOR_M32(arg0, position_x);
            if (dx > 0) {
                if (ACTOR_M8(arg0, facing_right) != 0) {
                    ACTOR_M32(arg0, state_flags) |= 0x10000;
                    side |= 2;
                }
            } else if (ACTOR_M8(arg0, facing_right) == 0) {
                ACTOR_M32(arg0, state_flags) |= 0x10000;
                side |= 2;
            }
        }
    }
    if (action == 3)
        side = side * 2 + (game_random_next() & 1);
    {
        s32 id = (action != 4) ? tbl[action][side] : g_06091A3C[side];
        actor_init_from_cfg(arg0, id);
    }
    return action;
}
