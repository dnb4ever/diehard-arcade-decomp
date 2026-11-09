/* effect_pool_spawn [0x0223cbb0]: Allocates a typed effect in the eight-slot pool, installs its handler and seeds position when requested. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223cbb0 (callC) - spawn a typed effect/projectile task into a shared
 * 8-slot pool and return the new task (NULL if capacity reached).
 * Globals: g_060e0e88 live count; parallel 8-entry slot arrays g_060e0f0c
 * (type, 0 = free), g_060e0f2c (arg1), g_060e0f4c (task ptr); per-slot 16-byte
 * record block g_060e0e8c (stride 0x10).
 *
 * Caps: types 13/14 limited to 7 live and also rejected if either player's
 * running tally (ctx g_06099584: unk72/unk73 of unk30->unk8 and unk34->unk8)
 * has reached 4; all other types limited to 8 live.
 * Allocate: first free slot; task = func_0220ff0a(g_06099580,
 * g_06091630[type]); bump g_060e1214 by g_0608cf18[type]; record type/arg1/task
 * into slot arrays; copy the 4-long template g_0608c674[type] into the slot
 * record. Init entity = task->unk8: unk34 = slot, unk38 = 0x20000,
 * unk56 = g_0608cef4[type], unk90 = g_0609979a ? 0x7FFFFFFF : 3. When arg1 == 1,
 * seed entity pos from arg2 (unk8 = arg2->unk0, unk10 = arg2->unk8,
 * unkC = g_0608ceac[type]).
 * fidelity: HIGH. bsrf resolved to func_0220ff0a (task allocate). */
extern s32   g_060e0e88;
extern u8    g_060e0e8c[];
extern s32   g_060e0f0c[];
extern s32   g_060e0f2c[];
extern void *g_060e0f4c[];
extern u8    g_actor_slot_registry;
extern void *g_06099580;
extern void *g_06091630[];
extern s16   g_060e1214;
extern s16   g_0608cf18[];
extern u8    g_0608c674[];
extern s16   g_0608cef4[];
extern s32   g_0608ceac[];
extern s16   g_actor_pose_set_legacy;
extern void *task_spawn_actor_work(void *pool, void *handler);

void *effect_pool_spawn(s32 type, s32 arg1, void *arg2) {
    s32 count = g_060e0e88;
    s32 slot;
    void *task;
    void *entity;
    void *rec;
    s32 *tmpl;

    if (type == DH_EFFECT_TYPE_LIMITED_A || type == DH_EFFECT_TYPE_LIMITED_B) {
        s32 na = 0;
        s32 nb = 0;
        if (count >= 7)
            return 0;
        if (M32(&g_actor_slot_registry, 0x10) != 0) {
            void *e = MP(MP(&g_actor_slot_registry, 0x30), 0x8);
            na = M8(e, 0x72);
            nb = M8(e, 0x73);
        }
        if (M32(&g_actor_slot_registry, 0x14) != 0) {
            void *e = MP(MP(&g_actor_slot_registry, 0x34), 0x8);
            na += M8(e, 0x72);
            nb += M8(e, 0x73);
        }
        if (na >= 4 || nb >= 4)
            return 0;
    } else if (count >= 8) {
        return 0;
    }

    slot = -1;
    do {
        slot++;
    } while (g_060e0f0c[slot] != 0);

    task = task_spawn_actor_work(g_06099580, g_06091630[type]);

    g_060e1214 = (s16)(g_060e1214 + g_0608cf18[type]);
    g_060e0e88 = count + 1;
    g_060e0f2c[slot] = arg1;
    g_060e0f0c[slot] = type;
    g_060e0f4c[slot] = task;

    rec = &g_060e0e8c[slot * 0x10];
    tmpl = (s32 *)&g_0608c674[type * 0x10];
    M32(rec, 0x0) = tmpl[0];
    M32(rec, 0x4) = tmpl[1];
    M32(rec, 0x8) = tmpl[2];
    M32(rec, 0xC) = tmpl[3];

    entity = TASK_PTR(task, work);
    M8(entity, 0x34) = (s8)slot;
    M32(entity, 0x38) = 0x20000;
    M16(entity, 0x56) = g_0608cef4[type];
    M32(entity, 0x90) = (g_actor_pose_set_legacy != 0) ? 0x7FFFFFFF : 3;

    if (arg1 == 1) {
        M32(entity, 0x8) = M32(arg2, 0x0);
        M32(entity, 0x10) = M32(arg2, 0x8);
        M32(entity, 0xC) = g_0608ceac[type];
    }
    return task;
}
