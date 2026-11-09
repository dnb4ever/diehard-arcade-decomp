/* actor_tick_pose_sequence [0x02210b58]: Task mode+10 drives pose-object initialization, random motion and draw phases; species unassigned. */
#include "dh.h"

typedef double f64;

extern u32 game_random_next(void);
extern s32 shc_shift_left(s32 value, s32 count);
extern f64 shc_int_to_double_raw_entry(s32 x);
extern s32 shc_double_to_int_raw_entry(f64 x);
extern f64 shc_double_multiply_shared_entry(f64 a, f64 b);
extern s32 collision_sweep_pose(s32 arg0, u32 pose, s32 mode);
extern s32 actor_allocate_effect_slot(void);
extern void collision_queue_actor(u32 entity);
extern void mtx_load_abc_entry(void);
extern void mtx_apply_vec_at8_entry(u32 obj);
extern void mtx_rotate_zyx_at24_entry(u32 obj);
extern void matrix_scale_columns(s32 x, s32 y, s32 z);
extern void model_submit_culled(u32 arg0, s32 arg1);

/* 0x02210b58 - boss/entity state machine driving pose object at *(arg0+8).
 * arg0+16 is the mode byte; arg0+15 is a shift-count byte used with the
 * scaling helper func_0224a360; func_02210020 is the PRNG. The
 * func_0224a830/func_0224a928/func_0224a7a8 triple is a fixed-point
 * multiply by 1.5 via doubles. */
void actor_tick_pose_sequence(u32 arg0)
{
    u32 s = M32(arg0, 8);
    u32 rnd, t, u;

    switch ((s8)M8(arg0, 16)) {
    case 0:
        rnd = game_random_next();
        M32(s, 8) = M32(s, 8) + shc_shift_left((s32)((rnd & 0x7fff) - 0x4000), (s8)M8(arg0, 15) >> 1);

        rnd = game_random_next();
        M32(s, 12) = M32(s, 12) + 0x147a;
        M32(s, 16) = M32(s, 16) + shc_shift_left((s32)((rnd & 0x7fff) - 0x4000), (s8)M8(arg0, 15) >> 1);

        rnd = game_random_next();
        M32(s, 24) = shc_shift_left((s32)(rnd & 0x1fff), (s8)M8(arg0, 15) >> 1) + 0x3000;
        M32(s, 28) = 0;
        M16(s, 36) = 0;
        M16(s, 38) = 0x4300;
        M16(s, 40) = 0;
        M16(s, 42) = (u16)shc_double_to_int_raw_entry(shc_double_multiply_shared_entry(shc_int_to_double_raw_entry(M32(s, 20)), (double)1.5));
        M16(s, 44) = 0;
        M16(s, 46) = 0;
        if (M32(s, 60) != 0) {
            M32(s, 20) = (u32)(-(s32)M32(s, 20));
            M16(s, 42) = (u16)(-(s16)M16(s, 42));
        }
        M16(s, 48) = 0x481;
        t = actor_allocate_effect_slot();
        M8(s, 52) = (u8)t;
        if ((s8)t < 0) {
            M32(arg0, 0) = 0;
            return;
        }
        M32(s, 56) = 0x80000;
        M32(s, 108) = 1;
        M8(s, 93) = 0;
        M8(s, 94) = 0;
        M8(s, 95) = 0;
        M16(s, 96) = 0;
        M16(s, 98) = 0;
        M32(s, 100) = 0xccc;
        M32(s, 104) = M32(DH_ACTOR_SLOT_MASKS_ADDR + (u32)M8(s, 52) * 4, 0) | 0x7fffff00;
        M8(arg0, 16) = 2;
        goto end;

    case 2:
    case 3:
        M32(s, 24) = M32(s, 24) - 0x400;
        /* fall through */
    case 1: {
        s32 y = M32(s, 20);
        if ((y >= 0 && (s16)M16(s, 42) > 0) ||
            (y > 0 && (s16)M16(s, 42) < 0)) {
            M32(DH_UI_CURSOR_COLUMN_ADDR, 0) = 20;
            M32(DH_UI_CURSOR_ROW_ADDR, 0) = 11;
            ((void (*)(u32))M32(0x060902a0, 0))(0x0605356c);
        }
        M32(s, 108) = M32(s, 108) ^ 1;
        M32(s, 76) = M32(s, 8);
        M32(s, 80) = M32(s, 12);
        M32(s, 84) = M32(s, 16);
        M32(s, 76) = M32(s, 76) + ((M32(s, 108) != 0) ? 0x1000 : (u32)-0x1000);
        M16(s, 36) = (u16)((s16)M16(s, 36) + (s16)M16(s, 42));
        M16(s, 38) = (u16)((s16)M16(s, 38) + (s16)M16(s, 44));
        M16(s, 40) = (u16)((s16)M16(s, 40) + (s16)M16(s, 46));
        collision_sweep_pose((s32)s, 0x06052234, 1);

        if (M32(0x060b88a4, 0) & 3) {
            s32 old;
            M8(arg0, 16) = 2;
            t = M32(s, 20) + ((u32)(-(s32)M32(s, 20)) >> 2);
            M32(s, 20) = t;
            old = -(s32)M32(s, 20);
            rnd = game_random_next();
            if ((s32)t > 0) {
                M32(s, 20) = old - (s32)(rnd & 0x3ff);
            } else {
                M32(s, 20) = old + (s32)(rnd & 0x3ff);
            }
            rnd = game_random_next();
            M32(s, 28) = (s32)(rnd & 0x3ff) - 0x200;
            if ((s32)M32(s, 12) <= 0x147a) {
                M32(s, 12) = 0x147a;
                rnd = game_random_next();
                M32(s, 24) = (s32)(rnd & 0xfff) + 0x1400;
            }
            M16(s, 42) = (u16)shc_double_to_int_raw_entry(shc_double_multiply_shared_entry(shc_int_to_double_raw_entry(M32(s, 20)), (double)1.5));
            u = M32(s, 20);
            u = ((s32)u < 0) ? (u32)(-(s32)u) : u;
            if ((s32)u <= 0x1400) {
                M8(arg0, 16) = 3;
            }
        }
        if (M32(0x060b88a4, 0) & 0xc) {
            M32(s, 28) = (u32)(-(s32)M32(s, 28));
        }
        t = (u8)M8(arg0, 16);
        if ((t == 2 || t == 3) && (s32)M32(s, 12) < 0x147a) {
            M32(s, 12) = 0x1eb7 - ((s32)M32(s, 12) >> 1);
            t = ((u32)(-(s32)M32(s, 24)) >> 1);
            M32(s, 24) = t;
            if ((s32)t <= 0x500) {
                M8(arg0, 16) = 1;
                M32(s, 12) = 0x147a;
                M32(s, 24) = 0;
            }
        }
        if ((s8)M8(arg0, 16) == 3 && M32(s, 20) != 0) {
            t = M32(s, 20) + ((u32)(-(s32)M32(s, 20)) >> 4);
            M32(s, 20) = t;
            M16(s, 42) = (u16)shc_double_to_int_raw_entry(shc_double_multiply_shared_entry(shc_int_to_double_raw_entry(t), (double)1.5));
            u = M32(s, 20);
            u = ((s32)u < 0) ? (u32)(-(s32)u) : u;
            if ((s32)u < 0x400) {
                M32(s, 20) = 0;
                M32(s, 24) = 0;
                M32(s, 28) = 0;
                M16(s, 42) = 0;
                M16(s, 44) = 0;
                M16(s, 46) = 0;
            }
        }
        if (M8(s, 93) != 0) {
            M8(arg0, 16) = 4;
            M32(s, 20) = (M32(s, 108) != 0) ? (u32)-0x1000 : 0x1000;
            rnd = game_random_next();
            M32(s, 24) = (s32)(rnd & 0x7ff) + 0x3000;
            M32(s, 28) = (u32)-0x1400;
            M16(s, 42) = 0;
            M16(s, 44) = 0;
            M16(s, 46) = 0x0d00;
            M16(s, 36) = 0;
            return;
        }
        collision_queue_actor(s);
        return;
    }

    case 4:
    case 5:
        M32(s, 24) = M32(s, 24) - 0x400;
        M16(s, 36) = (u16)((s16)M16(s, 36) + (s16)M16(s, 42));
        M16(s, 38) = (u16)((s16)M16(s, 38) + (s16)M16(s, 44));
        M16(s, 40) = (u16)((s16)M16(s, 40) + (s16)M16(s, 46));
        collision_sweep_pose((s32)s, 0x06052234, 1);
        if (M32(0x060b88a4, 0) & 3) {
            M32(s, 20) = (u32)(-(s32)M32(s, 20));
        }
        if (M32(0x060b88a4, 0) & 0xc) {
            M32(s, 28) = (u32)(-(s32)M32(s, 28));
        }
        if ((s32)M32(s, 12) < 0x147a) {
            M32(s, 12) = 0x1eb7 - ((s32)M32(s, 12) >> 1);
            M32(s, 20) = (u32)((s32)M32(s, 20) >> 1);
            M32(s, 24) = (u32)(((s32)M32(s, 24) * -1) >> 1);
            M32(s, 28) = (u32)((s32)M32(s, 28) >> 1);
            rnd = game_random_next();
            M16(s, 44) = (s16)(0x400 - (s32)(rnd & 0x7ff));
            rnd = game_random_next();
            M16(s, 46) = (s16)(0x1000 - (s32)(rnd & 0x1fff));
            if ((s32)M32(s, 24) <= 0x1500 && (s8)M8(arg0, 16) != 5) {
                M8(arg0, 16) = 5;
                M32(s, 108) = 0x12;
            }
        }
        if ((s8)M8(arg0, 16) == 5) {
            t = M32(s, 108) - 1;
            M32(s, 108) = t;
            if ((s32)t >= 0) {
                goto end;
            }
            u = (u8)M8(s, 52);
            if ((s32)u > 8) {
                M8(DH_EFFECT_SLOT_OCCUPIED_ADDR + (u - 8), 0) = 0;
            }
            M32(arg0, 0) = 0;
            return;
        }
        goto end;

    default:
        goto end;
    }

end:
    if (!((s8)M8(arg0, 16) == 5 && (M32(s, 108) & 1))) {
        M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) = M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) + 48;
        mtx_load_abc_entry();
        mtx_apply_vec_at8_entry(s);
        mtx_rotate_zyx_at24_entry(s);
        matrix_scale_columns(0x16000, 0x16000, 0x16000);
        model_submit_culled(s, 0);
        M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) = M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) - 48;
    }
}
