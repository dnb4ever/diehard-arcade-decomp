#include "dh_work.h"
/* actor_dispatch_pose_and_type [0x0222adaa]: Dispatches pose table, resets transient fields, updates timestamp and invokes per-type handler. */
#include "dh.h"
/* 0x0222adaa (actor pose + per-type dispatch) - shared continuation reached by many actor
 * state handlers (the braf->0x22adaa family). Steps:
 *  1. dispatch the current pose/anim routine from table g_060911BC, indexed by the global
 *     anim-set selector g_0609979A, passing (entity, entity->unk64);
 *  2. run func_0222aadc(entity);
 *  3. clear unk80 / unk7C / unk48 / unk52 and set unk41 = 1;
 *  4. if the global frame counter g_060987BC has advanced more than 0x1E ticks past
 *     entity->unk110 (unsigned), clear unk10C; then refresh unk110 = g_060987BC;
 *  5. tail-dispatch the per-type handler from table g_060911C4, indexed by
 *     (entity->unkBA & 0x3F), passing (entity, &g_060987BC).
 * fidelity: HIGH - widths from the loads (unk64/unk52/unkBA word, unk48/unk41 byte,
 *  unk80/7C/10C/110 long); two fn-pointer-table indirect calls (jsr/jmp) + bsr func_0222aadc;
 *  the >0x1E staleness test is unsigned (cmp/hi). */
typedef void (*pose_fn)(void *entity, s16 param);
typedef void (*type_fn)(void *entity, void *frame);
extern pose_fn g_actor_pose_handlers[];   /* pose/anim dispatch, index = g_0609979A */
extern type_fn g_actor_type_handlers[];   /* per-type handler dispatch, index = entity->unkBA & 0x3F */
extern s16  g_actor_pose_set;
extern s32  g_frame_tick;        /* global frame counter */
extern void actor_queue_pose_sound(void *entity);

void actor_dispatch_pose_and_type(void *entity) {
    g_actor_pose_handlers[g_actor_pose_set](entity, M16(entity, 0x64));
    actor_queue_pose_sound(entity);
    M32(entity, 0x80) = 0;
    M32(entity, 0x7C) = 0;
    ACTOR_M8(entity, action_pending)  = 0;
    ACTOR_M16(entity, interaction_latch) = 0;
    ACTOR_WORK(entity)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
    if ((u32)(g_frame_tick - ACTOR_M32(entity, event_tick)) > 0x1E)
        ACTOR_M32(entity, event_count) = 0;
    ACTOR_M32(entity, event_tick) = g_frame_tick;
    g_actor_type_handlers[ACTOR_M16(entity, type_flags) & DH_ACTOR_TYPE_MASK](entity, &g_frame_tick);
}
