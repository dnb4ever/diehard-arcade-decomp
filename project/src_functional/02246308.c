/* enemy_wave_initialize_task [0x02246308]: Selects the descriptor's wave script and installs the steady wave spawner. */
#include "dh_work.h"
#include "dh.h"
/* 0x02246308  scripted enemy-wave spawner (init entry). Installs the steady-state
 * handler func_022463b8 into task->unk0, then picks the script record to start
 * from: raw = g_0609866c->unk18; if raw >= 0 it is already a record pointer,
 * otherwise index the per-difficulty table g_060907e0[~raw] with a 0..3 selector
 * built from g_0609856c's low two bits. Publish it as g_060973f0, seed the
 * controller delay (unk40) and the width/trigger globals from the record, then run
 * the spawn loop for this frame by tail-calling func_022463b8.
 *
 * bounds: [0x02246308, 0x2463b8); ends with a bra to func_022463b8 (the 0x2f4
 *   fn.extent over-swallows that whole shared loop). func_022463b8's arg1/arg2 are
 *   dead (only forwarded to func_02227b60, which ignores them), so this entry's
 *   clobbered r5/r6 at the jump are irrelevant.
 * fidelity: HIGH - verified vs fn.py --asm. */
extern void *g_scene_descriptor_legacy;       /* wave descriptor (->unk18 = start record/index) */
extern s32  g_0609856c;        /* difficulty/route selector */
extern void *g_060907e0[];     /* per-route record tables */
extern s32  g_060973e4;        /* current record width */
extern s32  g_060973e8;        /* trigger X */
extern void *g_060973f0;       /* current script record */
extern void *g_0609196c[];     /* per-type records (->unk18 = width) */

void enemy_wave_spawn_task(void *task);                  /* 0x022463b8 spawn loop */

void enemy_wave_initialize_task(void *task) {
    void *ctrl = TASK_PTR(task, work);
    s32 raw = M32(g_scene_descriptor_legacy, 0x18);
    void *entry;

    TASK_PTR(task, handler) = (void *)enemy_wave_spawn_task;

    if (raw < 0) {
        s32 sel = g_0609856c;
        void *tbl = g_060907e0[~raw];
        entry = MP(tbl, (((sel >> 1) & 2) | (sel & 1)) * 4);
    } else {
        entry = (void *)(uintptr_t)raw;
    }

    g_060973f0 = entry;
    M32(ctrl, 0x40) = (u16)MU16(entry, 0x10);
    g_060973e4 = M32(g_0609196c[(u8)MU8(entry, 8)], 0x18);
    g_060973e8 = (u8)MU8(entry, 0x0B);

    enemy_wave_spawn_task(task);
}
