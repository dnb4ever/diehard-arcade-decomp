/* effect_tick_landing_sound [0x0223d558]: Falling object variant emits landing sound and manages audio readiness gates. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223d558 - per-frame update for a falling object that triggers sound on
 * landing (child obj = entity->unk8). Dispatch on entity->unk10.
 * case 0: launch - obj->unk8 += unk14*2; unk14 = unk40; unk1C = unk48;
 *   func_022173f4(obj, (void*)0x0608d3c8, 2). If the audio channel is live
 *   (g_06098670 == 1 && g_06098830 >= 2): set g_06098830 = 2, g_0609882c->unkD = 1
 *   and clear the sound objects unkAE/B0/B2. Advance state.
 * case 1: save prev pos, apply gravity (unk70 -= 0x360, unk18 = unk70), spin,
 *   func_022173f4(obj, ..., 1). Keep falling while (g_060b88a4 & 0xF) == 0 and
 *   unk5D == 0 and unkC > 0x500 -> func_0222eea2(obj). Otherwise it landed:
 *   switch entity to handler 0x0603501c, reset unk10, set obj->unk30/32, run
 *   func_022153f6(obj, 0x80000, 1), trigger the landing sound/rumble via the
 *   g_06098670/830/834/0609882c channel, then tail-call
 *   func_0220fece(entity, (void*)0x0603ce94).
 * Common tail: push draw offset (g_06098878 += 48), run mtx_load_abc/0214a40/
 *   0214a06/0215a2c, pop.
 * branches resolved: bsrf 0x2173f4,0x2153f6,0x22eea2,0x2149aa,0x214a40,0x214a06,
 *   0x215a2c; braf tail 0x20fece. fidelity: HIGH (mtx_load_abc arg-less here). */
extern s32  g_play_session_mode;
extern s32  g_view_request_priority;
extern s32  g_06098834;
extern void *g_view_task;
extern s32  g_060b88a4;
extern s32  g_matrix_stack_cursor;
extern void collision_sweep_pose(void *obj, void *tbl, s32 n);
extern void sprite_draw_child(void *obj, s32 a, s32 n);
extern void collision_queue_actor(void *obj);
extern void task_spawn_current_work6c(void *entity, void *arg);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *obj);
extern void mtx_rotate_zyx_at24(void *obj);
extern void model_submit_culled(void *obj, s32 b);

void effect_tick_landing_sound(void *entity) {
    void *obj = MP(entity, 8);
    void *snd;

    switch (M8(entity, 0x10)) {
    case 0:
        M32(obj, 8) += M32(obj, 0x14) * 2;
        M32(obj, 0x14) = M32(obj, 0x40);
        M32(obj, 0x1C) = M32(obj, 0x48);
        collision_sweep_pose(obj, (void *)0x0608d3c8, 2);
        if (g_play_session_mode == 1 && g_view_request_priority >= 2) {
            snd = TASK_PTR(g_view_task, work);
            g_view_request_priority = 2;
            M8(g_view_task, 0xD) = 1;
            M16(snd, 0xAE) = 0;
            M16(snd, 0xB0) = 0;
            M16(snd, 0xB2) = 0;
        }
        M8(entity, 0x10)++;
        break;
    case 1:
        M32(obj, 0x4C) = M32(obj, 8);
        M32(obj, 0x50) = M32(obj, 0xC);
        M32(obj, 0x54) = M32(obj, 0x10);
        M32(obj, 0x70) -= 0x360;
        M32(obj, 0x18) = M32(obj, 0x70);
        M16(obj, 0x28) += 0x200;
        M16(obj, 0x24) += 0x100;
        collision_sweep_pose(obj, (void *)0x0608d3c8, 1);

        if ((g_060b88a4 & 0xF) == 0 && M8(obj, 0x5D) == 0 && M32(obj, 0xC) > 0x500) {
            collision_queue_actor(obj);
            break;
        }

        MP(entity, 0) = (void *)0x0603501c;
        M8(entity, 0x10) = 0;
        M16(obj, 0x30) = 0x187;
        M16(obj, 0x32) = 0;
        sprite_draw_child(obj, 0x00080000, 1);
        if (g_play_session_mode == 1 && g_view_request_priority >= 1) {
            snd = TASK_PTR(g_view_task, work);
            g_view_request_priority = 1;
            M8(g_view_task, 0xD) = 1;
            M16(snd, 0xAE) = 0;
            M16(snd, 0xB0) = 0;
            M16(snd, 0xB2) = 0;
        }
        if (g_play_session_mode == 1) {
            g_06098834 |= 1;
            M16(TASK_PTR(g_view_task, work), 0xAA) = 10;
        }
        task_spawn_current_work6c(entity, (void *)0x0603ce94);
        return;
    default:
        break;
    }

    g_matrix_stack_cursor += 48;
    mtx_load_abc();
    mtx_apply_vec_at8(obj);
    mtx_rotate_zyx_at24(obj);
    model_submit_culled(obj, 0);
    g_matrix_stack_cursor -= 48;
}
