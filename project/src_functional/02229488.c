/* actor_hit_state_step [0x02229488]: Existing map corroborated by source: DECOMPILED: actor hit/grab state handler (switch on unk88 0x45-0x49); 0x47 spawns partner/effect via func_0220ff72(g_06099580, template@0x02226EB8 [ptr_resolve]) and links spawned->unk8->unk10/14; tails func_02216110/animation_update/actor_pose_dispatch */
/* Round 4: 0602d488 stores handler byte and tail-branches to existing shared body 0602d4b4; preserve that boundary rather than duplicating its body. Evidence: work/decomp4/callee_missing.asm. */
#include "dh_work.h"
#include "dh.h"
/* 0x0229488 (actor hit/grab state step) - per-actor state handler dispatched by the actor system
 * (entity = r5, ctx = r4). Sets the base request unk43 = 0x1B, bails if busy (unk4C != 0), then:
 *  - if no hit timer is pending (unk56 == 0): for the grab state (unk88 == 0x49) flip facing
 *    unk26 += 0x8000, latch unk4A = unk34 and run actor_pose_dispatch(entity); otherwise request
 *    unk43 = 0x1F and set unk84 = 0x20000000. Either way tail into func_02216110(entity).
 *  - otherwise switch on the state unk88:
 *      0x45/0x46: restore anim unk5C = g_06096C74[unk34], unk41 = 1, actor_facing_resolve; if the
 *                 grabbed flag unk8C & 0x02000000 is set run actor_pose_load and request unk43=0xD,
 *                 else reset to idle (unk41=1, unk84=1, unk88=0, unk43=1); tail animation_update.
 *      0x47:      flip facing unk44 ^= 1, unk4B = unk67, animation_update, unk84 = 0x20000, then
 *                 SPAWN a partner/effect actor via func_0220ff72(g_06099580, template@0x02226EB8)
 *                 and link it: spawned->unk8->unk10 = this entity, spawned->unk8->unk14 =
 *                 g_060995B0[unk4B]->unk8; request unk43 = 0x19.
 *      0x48:      unk84 = 0x1000, timer unkC4 = 0x20, request unk43 = 0xA.
 *      0x49:      flip facing unk26 += 0x8000, unk4A = unk34, tail actor_pose_dispatch(entity).
 * NOTE (spawn system): the 0x47 spawn uses func_0220ff72 with g_06099580 as the pool/list context
 * and a fixed ROM template @0x02226EB8 (relevant to the port's empty-spawn-list probe).
 * fidelity: HIGH - widths from the loads (unk43/41/44/4A/4B/34/67 byte, unk26/52/56/5C word,
 * unk88/84/8C/C4/10/14 long); spawn template @0x02226EB8 resolved via ptr_resolve.py
 * (mova 0x2295dc - 0x2724); computed branches resolved via braf_rewrite.py (incl. the braf tails
 * to func_02216110 / actor_anim_frame_update / func_0222adaa). */
extern void *task_spawn_child_work40(s32 ctx, void *template);  /* spawn/alloc object */
extern void audio_queue_actor_slot_cue(void *entity);
extern void actor_resolve_facing(void *entity);             /* actor_facing_resolve */
extern void actor_dispatch_pose_and_type(void *entity);             /* actor_pose_dispatch */
extern void actor_load_pose_animation(void *entity);             /* actor_pose_load */
extern void actor_anim_frame_update(void *ctx, void *entity);  /* animation_update */
extern s32  g_06099580;      /* spawn pool/list context */
extern s32  g_06096C74[];    /* per-slot restore-anim id */
extern void *g_actor_slot_tasks[];   /* per-slot object pointers */
extern u8   g_02226EB8[];    /* partner/effect spawn template (in-ROM) */

extern void player_tick_hit_grab_body(void *ctx, void *entity);
void actor_hit_state_step(void *ctx, void *entity) {
    ACTOR_WORK(entity)->state_handler = 0x1B;
    player_tick_hit_grab_body(ctx, entity);
}
