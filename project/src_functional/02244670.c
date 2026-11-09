/* enemy_dispatch_counter_reaction [0x02244670]: Reaction-mode table scans eligible counter-target; launches queued config move on hit. */
#include "dh.h"
/* 0x02244670  reaction dispatch: for the mode selected by g_0608DBDC[mode_idx],
 * look for a valid counter-target among the reaction slots of `rec`; when found,
 * launch the actor's queued move (actor_init_from_cfg) and return 0, otherwise 1.
 * Modes (g_0608DBDC[mode_idx]):
 *   0 -> rec->unk0 = func_022447d4(actor, other, dir16); return 1.
 *   1 -> single slot: if (u16)rec->unk4 == side and rec->unk10 is a live,
 *        compatible object, fire and return 0; else return 1.
 *   2 -> rec->unk0 ^= 1, then scan k=0..4 over the parallel arrays
 *        rec->unk4[k] (s16 side, stride 2) / rec->unk10[k] (obj, stride 4):
 *        stop at a 0xFFFF side terminator (return 1); for a slot whose side
 *        matches and whose object is live+compatible, fire and return 0.
 *   default -> return 1.
 * side = actor->unk44 ? 4 : 12.
 * "compatible" (hit/faction gate, same test used in 0x02244aa8/0x02246678):
 *   (obj->unk38 & 0x10FFFF) && !(g_0604EBE4[obj->unk34] & actor->unkBC)
 *   && !(g_0604EBE4[actor->unk34] & obj->unkC0).
 * The fired move id is **(*actor->unk15C) (three indirections); extra scratch
 *   regs left in r6/r7 at the call are ignored by actor_init_from_cfg (2 args).
 * fidelity: HIGH - mode 1 returns 1 when the single slot is empty (obj==-1),
 *   mode 2 instead continues scanning; both preserved. dir16 is the 5th
 *   (stack-passed, big-endian low half) s16 argument. */
extern s32 g_0608DBDC[];     /* reaction-mode table (0/1/2 per index) */
extern s32 g_actor_slot_masks_legacy[];     /* per-type faction/collision mask table */
s32 enemy_classify_facing_agreement(void *actor, void *other, s16 dir);   /* 0x022447d4 */
void actor_init_from_cfg(void *actor, s32 id);                /* 0x02244988 */

static int compat(void *actor, void *obj) {
    return (M32(obj, 0x38) & 0x10FFFF) &&
           !(g_actor_slot_masks_legacy[M8(obj, 0x34)] & M32(actor, 0xBC)) &&
           !(g_actor_slot_masks_legacy[M8(actor, 0x34)] & M32(obj, 0xC0));
}

static void fire(void *actor) {
    void *p = MP(MP(actor, 0x15C), 0);
    actor_init_from_cfg(actor, M32(p, 0));
}

s32 enemy_dispatch_counter_reaction(void *actor, void *other, void *rec, s32 mode_idx, s16 dir) {
    s32 side = M8(actor, 0x44) ? 4 : 12;
    s32 k;

    switch (g_0608DBDC[mode_idx]) {
    case 0:
        M32(rec, 0) = enemy_classify_facing_agreement(actor, other, dir);
        return 1;
    case 1:
        if ((u16)side == MU16(rec, 4) && M32(rec, 0x10) != -1) {
            void *obj = MP(rec, 0x10);
            if (compat(actor, obj)) {
                fire(actor);
                return 0;
            }
        }
        return 1;
    case 2:
        M32(rec, 0) ^= 1;
        for (k = 0; k < 5; k++) {
            u16 s = MU16(rec, 4 + k * 2);
            if (s == 0xFFFF)
                return 1;
            if (s == (u16)side && M32(rec, 0x10 + k * 4) != -1) {
                void *obj = MP(rec, 0x10 + k * 4);
                if (compat(actor, obj)) {
                    fire(actor);
                    return 0;
                }
            }
        }
        return 1;
    default:
        return 1;
    }
}
