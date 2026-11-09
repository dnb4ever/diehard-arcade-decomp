/* effect_tick_ballistic_collision [0x0223d3d0]: Launch velocity snapshot then gravity, previous pose and collision/draw update. */
#include "dh.h"
/* 0x0223d3d0 - per-frame update for a falling/arcing object (child obj =
 * entity->unk8). Dispatch on entity->unk10.
 * case 0: launch - obj->unk8 += unk14*2; unk6C = 40; copy unk40/unk48 into
 *   velocity fields unk14/unk1C; func_022173f4(obj, (void*)0x0608d3d8, 2);
 *   advance state.
 * case 1: save prev pos (unk4C/50/54), apply gravity (unk70 -= 0x360, unk18 =
 *   unk70), spin (unk28 += 0x200, unk24 += 0x100), run func_022173f4(obj,
 *   &g_060e0e8c[unk34*16], 1). Keep falling while (g_060b88a4 & 0xF) == 0 and
 *   unk5D == 0 and unkC > 0x500 -> func_0222eea2(obj). Otherwise it has landed:
 *   spawn an impact object via func_0220ff0a(entity, (void*)0x02212350), seed its
 *   fields, call func_0221055c with the start/end positions, set entity->unk0 = 0
 *   and return (no draw tail).
 * Common tail: push draw offset (g_06098878 += 48), run mtx_load_abc/0214a40/
 *   0214a06/0215a2c, pop.
 * branches resolved: bsrf 0x2173f4,0x20ff0a,0x21055c,0x22eea2,0x2149aa,
 *   0x214a40,0x214a06,0x215a2c. fidelity: HIGH (mtx_load_abc arg-less here). */
extern u8   g_060e0e8c[];
extern u8   g_060e1208[];
extern s32  g_060b88a4;
extern s32  g_matrix_stack_cursor;
extern void collision_sweep_pose(void *obj, void *tbl, s32 n);
extern void *task_spawn_actor_work(void *a, void *b);
extern void camera_compute_lookat_angles(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f,
                         void *g, void *h);
extern void collision_queue_actor(void *obj);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *obj);
extern void mtx_rotate_zyx_at24(void *obj);
extern void model_submit_culled(void *obj, s32 b);

void effect_tick_ballistic_collision(void *entity) {
    void *obj = MP(entity, 8);
    void *ret;
    void *p;

    switch (M8(entity, 0x10)) {
    case 0:
        M32(obj, 8) += M32(obj, 0x14) * 2;
        M32(obj, 0x6C) = 40;
        M32(obj, 0x14) = M32(obj, 0x40);
        M32(obj, 0x1C) = M32(obj, 0x48);
        collision_sweep_pose(obj, (void *)0x0608d3d8, 2);
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
        collision_sweep_pose(obj, &g_060e0e8c[M8(obj, 0x34) * 16], 1);

        if ((g_060b88a4 & 0xF) == 0 && M8(obj, 0x5D) == 0 && M32(obj, 0xC) > 0x500) {
            collision_queue_actor(obj);
            break;
        }

        ret = task_spawn_actor_work(entity, (void *)0x06016350 /* PC-relative pointer in HWRAM */);
        p = MP(ret, 8);
        M8(ret, 0xF) = g_060e1208[1];
        M32(p, 8)  = M32(obj, 8);
        M32(p, 0xC) = M32(obj, 0xC);
        M32(p, 0x10) = M32(obj, 0x10);
        M8(p, 0x48) = 1;
        M8(p, 0x4A) = M8(obj, 0x34);
        M16(p, 0xB8) = 0;
        M8(p, 0x49) = 0;
        M16(p, 0x64) = 16;
        M32(p, 0x74) = 0x3000;
        camera_compute_lookat_angles(M32(obj, 0x4C), M32(obj, 0x50), M32(obj, 0x54), M32(obj, 8),
                     M32(obj, 0xC), M32(obj, 0x10), (u8 *)p + 0x60, (u8 *)p + 0x62);
        M32(entity, 0) = 0;
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
