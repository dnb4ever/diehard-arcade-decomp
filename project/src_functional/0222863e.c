#include "dh_work.h"
/* actor_update_task [0x0222863e]: Dispatches actor state, updates hit shake, collision registration and per-slot output words. */
#include "dh.h"
/* 0x0222863e (actor task main-step) - task handler installed by func_02228508 as task->unk0.
 * entity = task->unk8. Dispatches the entity's current per-state handler through the function
 * pointer table g_06091100[entity->unk43](task, entity), then:
 *  - if the hit/stun timer unk4E is nonzero: set unk20 = (frame_parity & 1) << 8 (shake), decrement
 *    unk4E; when it reaches 0 clear unkBC and unk20;
 *  - run the per-frame actor callees func_0222ca98, func_0222eee4, func_0223df30(entity);
 *  - publish the actor's screen position from its per-zone record (slot = unk34):
 *    rec = g_06098734 + (s8)(24*(slot-1))   [0x02286aa exts.b truncates the scaled offset]
 *    g_0609979C[slot-1] = M16(rec,0x10), g_060997A0[M8(entity,0x34)-1] = M16(rec,0x12).
 * fidelity: HIGH - unk43/unk4E/unk34 byte, unk20 word, unkBC long, rec->unk10/12 word,
 * position arrays word; frame parity from g_060987BC; record offset truncated to s8 after
 * scaling (matches exts.b r4 at 0x02286aa). */
extern void (*g_actor_state_handlers[])(void *task, void *entity);  /* per-state handler table */
extern void draw_actor_hier_shadowed_entry(void *entity);
extern void collision_insert_grid(void *entity);
extern void hud_update_player_counters(void *entity);
extern s32  g_frame_tick;        /* free-running frame counter */
extern u8   g_player_input_records[];      /* per-zone records, 0x18 stride */
extern s16  g_0609979C[];
extern s16  g_060997A0[];

void actor_update_task(void *task) {
    void *entity = TASK_PTR(task, work);
    u8 *rec;
    s32 slot;

    g_actor_state_handlers[ACTOR_WORK(entity)->state_handler](task, entity);
    if (ACTOR_WORK(entity)->hit_shake_ticks != 0) {
        s32 n;
        M16(entity, 0x20) = (s16)((g_frame_tick & 1) << 8);
        n = (u8)(ACTOR_WORK(entity)->hit_shake_ticks - 1);
        ACTOR_WORK(entity)->hit_shake_ticks = (s8)n;
        if (n == 0) {
            M32(entity, 0xBC) = 0;
            M16(entity, 0x20) = 0;
        }
    }
    draw_actor_hier_shadowed_entry(entity);
    collision_insert_grid(entity);
    hud_update_player_counters(entity);
    slot = ACTOR_WORK(entity)->slot_id;
    rec = g_player_input_records + (s8)((slot - 1) * 0x18);
    g_0609979C[slot - 1] = M16(rec, 0x10);
    g_060997A0[ACTOR_WORK(entity)->slot_id - 1] = M16(rec, 0x12);
}
