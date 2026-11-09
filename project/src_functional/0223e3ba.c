/* aim_effect_spawn_source_alias [0x0223e3ba]: Duplicate bound to 0223e3d0; retains four-byte scratch stride fix and slot setup. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223e3ba (callC) - spawn a tracking/aim effect task into the shared pool.
 * No-op once g_060e0e88 (live count) reaches 7.
 * entity->unk34 selects a 0x20-byte record in g_06096248: its unkE is saved to
 * g_060e11ea[unk34] and replaced with g_060e10a8[arg1][entity->unk68] (s16 grid,
 * row stride 8). Then the first free pool slot (g_060e0f0c[slot]==0) is taken:
 * task = func_0220ff0a(g_06099580, func_0223db2a); g_060e0e88++;
 * g_060e0f2c[slot]=0; g_060e0f0c[slot]=arg1; g_060e0f4c[slot]=task.
 * New entity = task->unk8: unk34=slot, unk38=0x20000. Caller entity gets
 * unk8C |= 2, unk6D = 1, unk6F = slot.
 * fidelity: HIGH. bsrf resolved func_0220ff0a; installed handler func_0223db2a
 * resolved via mova+add (ptr_resolve). */
extern s32   g_060e0e88;
extern s32   g_060e0f0c[];
extern s32   g_060e0f2c[];
extern void *g_060e0f4c[];
extern u8    g_06096248[];
extern s16   g_060e11ea[];
extern u8    g_060e10a8[];
extern void *g_06099580;
extern void *task_spawn_actor_work(void *pool, void *handler);
extern void  effect_tick_scaled_tumbling(void *task);

void aim_effect_spawn_source_alias(void *entity, s32 arg1) {
    s32 count = g_060e0e88;
    s32 u34;
    void *vrec;
    s32 slot;
    void *task;
    void *nent;

    if (count >= 7)
        return;

    u34 = M8(entity, 0x34);
    vrec = &g_06096248[u34 * 0x20];
    /* ROM 060423f0..42404 uses a four-byte scratch stride. */
    M16(g_060e11ea, u34 * 4) = M16(vrec, 0xE);
    M16(vrec, 0xE) = *(s16 *)(&g_060e10a8[arg1 * 8 + (u16)M16(entity, 0x68) * 2]);

    slot = -1;
    do {
        slot++;
    } while (g_060e0f0c[slot] != 0);

    task = task_spawn_actor_work(g_06099580, (void *)effect_tick_scaled_tumbling);
    g_060e0e88 = count + 1;
    g_060e0f2c[slot] = 0;
    g_060e0f0c[slot] = arg1;
    g_060e0f4c[slot] = task;

    nent = TASK_PTR(task, work);
    M8(nent, 0x34) = (s8)slot;
    M32(nent, 0x38) = 0x20000;

    M32(entity, 0x8C) |= 2;
    M8(entity, 0x6D) = 1;
    M8(entity, 0x6F) = M8(nent, 0x34);
}
