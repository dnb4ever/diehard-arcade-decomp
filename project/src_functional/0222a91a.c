#include "dh_work.h"
/* actor_select_scene_variant [0x0222a91a]: Selects variant zero, one or two from scene index bands, with a forced-full override. */
#include "dh.h"
/* 0x0222a91a - classify an actor's distance/intensity band into unk4F from the global fade/level
 * counter g_06098668: < 0x0E -> 0, < 0x27 -> 1, else 2. If g_060987C8 == 1 (a forced-full flag),
 * override unk4F = 2.
 * fidelity: HIGH - counter/flag globals long (signed compares), unk4F byte. */
extern s32 g_scene_index;
extern s32 g_060987C8;

void actor_select_scene_variant(void *entity) {
    s32 level = g_scene_index;

    if (level < 0x0E)
        ACTOR_M8(entity, model_variant) = 0;
    else if (level < 0x27)
        ACTOR_M8(entity, model_variant) = 1;
    else
        ACTOR_M8(entity, model_variant) = 2;
    if (g_060987C8 == 1)
        ACTOR_M8(entity, model_variant) = 2;
}
