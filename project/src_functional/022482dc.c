#include "dh_work.h"
/* enemy_begin_scripted_response [0x022482dc]: Initializes state six and applies the first response configuration record. */
#include "dh.h"
/* 0x022482dc  actor state init + tail. ABI (r4=actor). Sets up the actor's
 * post-spawn state fields then tail-calls actor_init_from_cfg with the first word of the
 * record pointed to by actor[0x184]:
 *   actor[0xd4]=6; actor[0xd8]=0; actor[0x84]=0x400; actor[0x88]=0;
 *   actor[0xe0]&=0x300; actor[0x108]=0; actor[0x41]=1;
 *   actor_init_from_cfg(actor, *(s32 *)actor[0x184]);   [braf tail, resolved]
 * The code after the braf tail belongs to a separate (unreached) entry and is not
 * part of this function. fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void actor_init_from_cfg(void *actor, s32 arg);

void enemy_begin_scripted_response(void *actor) {
    void *rec = ACTOR_MP(actor, state_animation_descriptor);
    ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_SCRIPTED_RESPONSE;
    ACTOR_M32(actor, behavior_phase) = 0;
    ACTOR_M32(actor, action_category) = 0x400;
    ACTOR_M32(actor, action_state) = 0;
    ACTOR_M32(actor, state_flags) &= 0x300;
    ACTOR_M32(actor, action_phase) = 0;
    ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    actor_init_from_cfg(actor, M32(rec, 0));
}
