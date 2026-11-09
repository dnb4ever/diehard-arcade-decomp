/* effect_tick_spinning_slot [0x0223b5d8]: Same slot handshake plus RNG-seeded spin pair and trig rotation. */
#include "dh.h"
/* 0x0223b5d8 - per-frame state machine for a tumbling/spinning launched object
 * (sibling of func_0223b28a). Same 7-state layout and slot handshake via
 * g_060e0f2c[unk34], but this object also carries a spin pair unkCC/unkCE that
 * is seeded from the RNG (func_02210020) in state 0, advanced each frame in state
 * 1, and fed through the trig helper sin_lookup to derive the horizontal /
 * vertical wobble (unk26, unkC). State 2 integrates ballistic motion
 * (func_022173f4) with gravity 0x360 and bounces (unkC8 = -(unkC8>>2)) under a
 * 0x1000 floor. All bsrf calls resolved via braf_resolve.
 * fidelity: HIGH. */
extern u8    g_060e0e88[];
extern s16   g_060e1080[];
extern s16   g_actor_pose_set_legacy;
extern s32   g_matrix_stack_cursor;
extern s32   game_random_next(void);
extern s32   sin_lookup(s32 angle);
extern void  collision_sweep_pose(void *ent, void *rec);
extern void  effect_pool_release(void *obj);
extern void  actor_draw_proximity_pose(void *obj);
extern void  mtx_load_abc(void);
extern void  mtx_apply_vec_at8(void *obj);
extern void  mtx_rotate_zyx_at24(void *obj);
extern void  model_submit_culled(void *obj, s32 b);

static void draw_obj(void *obj) {
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(obj);
    mtx_rotate_zyx_at24(obj);
    model_submit_culled(obj, 0);
    g_matrix_stack_cursor -= 0x30;
}

void effect_tick_spinning_slot(void *arg0) {
    void *obj = MP(arg0, 8);
    s32 idx = MU8(obj, 0x34);

    switch (MU8(arg0, 0x10)) {
    case 0:
        M32(obj, 0x84) = (g_actor_pose_set_legacy != 0) ? 0x7FFFFFFF : 0x258;
        M16(obj, 0x30) = g_060e1080[M32(g_060e0e88, 0x84 + idx * 4)];
        switch (M32(g_060e0e88, 0xa4 + idx * 4)) {
        case 0:
            M8(arg0, 0x10) = 4;
            break;
        case 1:
            M16(obj, 0x24) = 0;
            M16(obj, 0x26) = 0x4000;
            M16(obj, 0x28) = 0;
            M8(arg0, 0x10) = 1;
            break;
        case 3:
            M8(arg0, 0x10) = 2;
            break;
        default:
            break;
        }
        M16(obj, 0xCC) = (s16)game_random_next();
        M16(obj, 0xCE) = (s16)game_random_next();
        draw_obj(obj);
        return;

    case 1:
        M16(obj, 0xCC) += 0x410;
        M16(obj, 0xCE) += 0x370;
        M16(obj, 0x26) = (s16)((sin_lookup(M16(obj, 0xCC)) >> 3) + 0x4000);
        M32(obj, 0xC)  = (sin_lookup(M16(obj, 0xCE)) >> 3) + 0x2000;
        switch (M32(g_060e0e88, 0xa4 + idx * 4)) {
        case 0:
            M8(arg0, 0x10) = 4;
            break;
        case 1:
            M32(obj, 0x84) -= 1;
            if (M32(obj, 0x84) > 0) {
                actor_draw_proximity_pose(obj);
            } else {
                M32(g_060e0e88, 0xa4 + idx * 4) = 5;
                effect_pool_release(obj);
                M32(obj, 0xC4) = 0x28;
                M8(arg0, 0x10) = 5;
            }
            break;
        case 3:
            M32(g_060e0e88, 0xa4 + idx * 4) = 1;
            break;
        default:
            break;
        }
        draw_obj(obj);
        return;

    case 2:
        M32(obj, 0xC8) -= 0x360;
        M16(obj, 0x26) += 0x400;
        M16(obj, 0x24) += 0x100;
        M32(obj, 0x18) = M32(obj, 0xC8);
        M32(obj, 0x14) = M32(obj, 0x78);
        M32(obj, 0x1C) = M32(obj, 0x80);
        collision_sweep_pose(obj, &g_060e0e88[4 + idx * 0x10]);
        if (M32(obj, 0xC) <= 0x1000) {
            M32(obj, 0xC) = 0x1000;
            if (M32(obj, 0xC8) > -0x600) {
                M8(arg0, 0x10) = 3;
            } else {
                M32(obj, 0xC8) = -(M32(obj, 0xC8) >> 2);
                M32(obj, 0x78) >>= 1;
                M32(obj, 0x80) >>= 1;
            }
        }
        draw_obj(obj);
        return;

    case 3:
        M16(obj, 0x24) = 0;
        M16(obj, 0x26) = 0x4000;
        M16(obj, 0x28) = 0;
        M32(obj, 0xC) = 0x1000;
        if (M16(obj, 0x56) != 0 && M32(obj, 0x90) != 0) {
            M32(obj, 0x84) = (g_actor_pose_set_legacy != 0) ? 0x7FFFFFFF : 0x258;
            M32(g_060e0e88, 0xa4 + idx * 4) = 1;
            M8(arg0, 0x10) = 1;
        } else {
            effect_pool_release(obj);
            M32(obj, 0xC4) = 0x28;
            M8(arg0, 0x10) = 5;
        }
        draw_obj(obj);
        return;

    case 4:
        if (M32(g_060e0e88, 0xa4 + idx * 4) != 0) {
            switch (M32(g_060e0e88, 0xa4 + idx * 4)) {
            case 1:
                M32(obj, 0x84) = (g_actor_pose_set_legacy != 0) ? 0x7FFFFFFF : 0x258;
                M32(obj, 0xC) = 0;
                M16(obj, 0x24) = 0;
                M16(obj, 0x26) = 0x4000;
                M16(obj, 0x28) = 0;
                M8(arg0, 0x10) = 1;
                return;
            case 3:
            case 4:
                M8(arg0, 0x10) = 2;
                return;
            case 5:
                effect_pool_release(obj);
                M8(arg0, 0x10) = 6;
                return;
            default:
                break;
            }
        }
        return;

    case 5:
        M32(obj, 0xC4) -= 1;
        if (M32(obj, 0xC4) == 0) {
            M8(arg0, 0x10) = 6;
        } else if (M32(obj, 0xC4) & 1) {
            return;
        }
        draw_obj(obj);
        return;

    case 6:
        M32(arg0, 0) = 0;
        return;

    default:
        draw_obj(obj);
        return;
    }
}
