/* background_tick_perspective_floor [0x022083a0]: Seven task states build 240-row perspective triples through display transfer cursor. */
#include "dh_work.h"
#include "dh.h"
/* 0x022083a0  scene_math_task_2083a0 - per-frame task for the perspective
 * "road/floor" scene set up by scene_display_init (0x2082be, which installs this
 * as the handler). State machine on task->unk10 (0..6); obj = task->unk8 holds
 * the animated parameters. Single function [0x022083a0,0x20898a) (rts @0x208986).
 *
 * Fixed-point helpers: func_022131c8 = 16.16 multiply, sh2_hw_divide (0x02213234)
 * = 16.16 divide. State 0 builds a 0xF0-row perspective table into the ring buffer
 * whose write cursor is at 0x06099510 (read cursor, +4, store value at cursor-4);
 * each row emits 3 longs (a 1/z-scaled intensity, a y coordinate, and 1/z). For
 * rows >= 0xA0 the scale sp0 = mul(0x999,(row-0xA0)<<16)+0x10000 and the three
 * longs are mul(0x1600000, (sp0-0x10000)/sp0), (0xA0-(row-0xA0)/2)<<16, and
 * 0x10000/sp0; nearer rows use the identity values. The 4x loop unroll in the ROM
 * is collapsed here to a single-row loop (0xF0 % 4 == 0, same effect).
 *
 * m2c corrections (braf_resolve authoritative; the m2c body and --asm rewriter
 * both mis-assigned several deltas, exactly as in 0x0222c39c):
 *   @0x208886 -> func_02213290(0x25E78000,0x400)       (m2c: a 6-arg mis-name)
 *   @0x20889a -> func_0221675e(0xC00,..,6 args)         (m2c named it func_02213290)
 *   @0x20890a -> sound_queue_push(0xA01402FF)           (m2c named it func_02220ade)
 *   @0x208970 -> func_022214d2(0x160,0x5F,0,0,vr,vr)    (m2c named it func_02215f6c)
 *   @0x2087e4 -> func_02210020() (rng, no args; m2c passed a spurious arg)
 *   the two PIC fn-ptrs resolve (ptr_resolve) to func_0221632e (mova 0x2083bc +
 *   0xDF72, 4 args) and func_022213a8 (mova 0x2083c4 + 0x18FE4, 2 args).
 *   The computed branch @0x208874 is the 0x208870-0x20887e literal pool mis-decoded
 *   as a bsrf (the stc-sr phantom the note flags) - jumped over, NOT a real call.
 *
 * fidelity: HIGH - every call resolved via braf_resolve/ptr_resolve and matched to
 *   the defining TU's real signature; ring-buffer writes and unroll verified vs
 *   fn.py --asm. No tst-#imm sign-extend traps; no register-count shifts. */

#define CURSOR M32(DH_DISPLAY_TRANSFER_CURSOR_ADDR, 0)          /* ring-buffer write cursor */
#define PANEL  MP(DH_VDP2_REGISTER_SHADOW_ADDR, 0)           /* active display panel ptr  */

extern s32  fixed_mul16(s32 a, s32 b);                         /* 0x022131c8 16.16 mul */
extern s32  sh2_hw_divide(s32 num, s32 den);                     /* 0x02213234 16.16 div */
extern u32  game_random_next(void);                                 /* 0x02210020 rng */
extern void vdp2_queue_attribute(s32 kind, s32 cursor, s32 packed, s32 count); /* 0x0221632e (extern) */
extern void vdp2_set_layer_scroll(s32 a, s32 b);                         /* 0x022213a8 */
extern void vdp2_set_layer_zoom(s32 a, s32 b);                         /* 0x0222142c */
extern void memory_clear_vec4_blocks(s32 base, s32 count);                  /* 0x02213290 */
extern void tilemap_copy_wrap_wide(s32 a0, void *a1, void *a2, s32 a3, s32 a4, s32 a5); /* 0x0221675e */
extern void vdp2_configure_layer1_interpolation(s16 b0, s16 b1, s32 o0, s32 o1, s32 s0, s32 s1);     /* 0x022214d2 */
extern void vdp2_apply_layer1_commands(s32 req);                              /* 0x02220ade (extern) */
extern void sound_queue_push(s32 req);                           /* 0x02215f6c */

static void rb_push(s32 v) {
    s32 p = CURSOR + 4;
    CURSOR = p;
    M32(p - 4, 0) = v;
}

void background_tick_perspective_floor(void *task) {
    void *obj = TASK_PTR(task, work);
    s32   idx;

    switch (TASK_S8(task, state)) {
    case 0:
        vdp2_queue_attribute(2, CURSOR, 0x25E3F400, 0x2D0);
        for (idx = 0; idx < 0xF0; idx++) {
            s32 sp0;
            if (idx < 0xA0) {
                sp0 = 0x10000;
            } else {
                sp0 = fixed_mul16(0x999, (idx - 0xA0) << 16) + 0x10000;
            }
            /* row long 0: intensity */
            if (idx < 0xA0) {
                rb_push(0);
            } else {
                rb_push(fixed_mul16(0x1600000, sh2_hw_divide(sp0 - 0x10000, sp0)));
            }
            /* row long 1: y coordinate */
            if (idx < 0xA0) {
                rb_push(idx << 16);
            } else {
                s32 t = idx - 0xA0;
                rb_push((0xA0 - ((t + (t < 0)) >> 1)) << 16);
            }
            /* row long 2: 1/z */
            if (idx < 0xA0) {
                rb_push(0x10000);
            } else {
                rb_push(sh2_hw_divide(0x10000, sp0));
            }
        }
        M16(PANEL, 0xF8) &= 0xFF;
        M32(obj, 0x8) = 0x28;
        TASK_S8(task, state) = 1;
        /* fall through */
    case 1: {
        s32 f = M32(DH_SCENE_EVENT_FLAGS_ADDR, 0);
        if (f & 0x20000) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= 0xFFFDFFFF;
            TASK_S8(task, state) = 4;
            return;
        }
        if (!(f & 0x10000)) {
            return;
        }
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= 0xFFFEFFFF;
        TASK_S8(task, state) = 2;
        return;
    }
    case 2:
        M16(obj, 0x32)  = 0x36;
        M16(PANEL, 0xF8) &= 0xFF;
        M16(PANEL, 0xF8) |= 0x600;
        M32(obj, 0x8) += 0x1E;
        TASK_S8(task, state) = 3;
        /* fall through */
    case 3: {
        s16 n;
        M32(obj, 0x8) += 5;
        n = (s16)(M16(obj, 0x32) - 1);
        M16(obj, 0x32) = n;
        if (n > 0) {
            vdp2_queue_attribute(0xE, CURSOR, 0x25F00200, 1);
            rb_push(game_random_next() & 0x7FFF7FFF);
            vdp2_configure_layer1_interpolation((s16)M32(obj, 0x8), 0x76, 0, 0, 0x38000, 0x38000);
            return;
        }
        vdp2_queue_attribute(0xE, CURSOR, 0x25F00200, 1);
        rb_push(0);
        M8(0x060B3D50, 0) = 1;
        vdp2_set_layer_scroll(0, 0);
        vdp2_set_layer_zoom(0x10000, 0x10000);
        M16(PANEL, 0xF8) &= 0xFF;
        TASK_S8(task, state) = 1;
        return;
    }
    case 4:
        memory_clear_vec4_blocks(0x25E78000, 0x400);
        tilemap_copy_wrap_wide(0xC00, (void *)0x25E78000, (void *)0x2267607C, 0xB, 6, 0x200);
        M32(obj, 0xC)  = 0xF0;
        M32(obj, 0x18) = -2;
        M8(0x060B3D50, 0) = 1;
        vdp2_set_layer_scroll(0, M32(obj, 0xC) * -0x10000);
        vdp2_apply_layer1_commands(0x06050C2C);
        M16(PANEL, 0xF8) &= 0xFF;
        M16(PANEL, 0xF8) |= 0x600;
        TASK_S8(task, state) = 5;
        /* fall through */
    case 5:
        if (M32(obj, 0xC) <= 0) {
            M32(obj, 0xC)  = 0;
            M32(obj, 0x18) = 0;
            TASK_S8(task, state) = 6;
            vdp2_apply_layer1_commands(0x06050C48);
            if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0) {
                sound_queue_push(DH_SOUND_EFFECT_SPAWN_COMMAND);
            }
            M16(obj, 0x30) = 0x4E;
            M32(obj, 0x10) = 0x10000;
        } else {
            M32(obj, 0xC) += M32(obj, 0x18);
        }
        M8(0x060B3D50, 0) = 1;
        vdp2_set_layer_scroll(0, M32(obj, 0xC) * -0x10000);
        return;
    case 6:
        if (M16(obj, 0x30) > 0) {
            s16 v;
            s32 vr;
            M16(obj, 0x30) = (s16)(M16(obj, 0x30) - 1);
            v = M16(obj, 0x30);
            vr = (v & 1) ? (((s32)v << 7) + 0x10000) : 0x10000;
            M32(obj, 0x10) = vr;
            vdp2_configure_layer1_interpolation(0x160, 0x5F, 0, 0, vr, vr);
        }
        return;
    default:
        return;
    }
}
