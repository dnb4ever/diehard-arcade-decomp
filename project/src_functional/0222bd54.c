/* actor_enter_pose_mode [0x0222bd54]: Requests the direction-selected pose, clears attack arming and dispatches the pose-set handler. */
#include "dh.h"
/* 0x0222bd54 (actor_state_enter_mode_dispatch) - a player-actor state-entry helper (entity=r4).
 * Calls func_022160a0(entity) (a shared setup/clear), programs the actor's current animation id
 * (unk5C) from the 2-entry word table g_06086304 selected by the low bit of unk4D, arms the anim
 * restart flag (unk41=1) and clears unk48, then tail-dispatches through the per-game-mode handler
 * table g_060911bc[g_0609979a](entity, entity->unk64).
 *   g_0609979a is the global mode/variant selector seen across the FSM (also read in player_join_next).
 * Branch resolved (braf_resolve): bsrf -> 0x022160a0. Indirect call = mov.l table read at
 * 0x060911bc indexed by the s16 at 0x0609979a (stride 4, long = fn-ptr). unk41/48/4D byte, unk5C/64
 * word. Cross-checked statement-for-statement vs m2c_fn --asm. fidelity: HIGH. */
#define R16A(a)  (*(s16 *)(uintptr_t)(a))

extern void audio_queue_actor_cue(void *entity);     /* 0x022160a0 */

typedef void (*mode_handler)(void *entity, s16 arg);

void actor_enter_pose_mode(void *entity) {
    audio_queue_actor_cue(entity);
    M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086304 + (M8(entity, 0x4D) & 1) * 2);
    M8(entity, 0x41) = 1;
    M8(entity, 0x48) = 0;
    (*(mode_handler *)(uintptr_t)(DH_ACTOR_POSE_HANDLERS_ADDR + R16A(DH_ACTOR_POSE_SET_ADDR) * 4))(entity, M16(entity, 0x64));
}
