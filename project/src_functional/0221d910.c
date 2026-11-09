/* prop_tick_cued_animation [0x0221d910]: Model 0x610 and paired scene cues; four task phases animate then end. */
#include "dh.h"
/* 0x0221d910 (entity FSM) - 4-state task handler dispatching on the state byte at
 * arg+0x10, operating on the entity work record at arg+0x8 (r14/w).
 *   0: w[0x30] = 0x610 (sprite id); if sound enabled (*0x060e0ddc != 0) push SFX
 *      0xa0120aff and 0xa0120cff, and additionally 0xa0120eff if the live global
 *      (*0x0609866c) has unk28 == 1; state++. Then falls through into state 1.
 *   1: w[0x58]++; if new value > 0x40 { w[0x5c] = 0x20; state++; }
 *   2: w[0x5c]++; if new value < 0 state++ (store always happens).
 *   3: w[0x58]--; if new value <= 0 { w[0x58] = 0; state = 1; }
 *   default: no state action.
 * Tail (always): y = w[0x58]; if (*0x06098664 != 1) store (u16)y at 0x0609861e and
 * 0x06098620; w[0x28] = (u16)((func_02210020() & 0x1fff) + 0xf000).
 * If the live global (*0x0609866c)->unk28 == 1, run the transform block:
 *   ptr = 0x06098878; *ptr += 0x30; mtx_load_abc();
 *   mtx_apply_vec_at0(w+8); mtx_rotate_zyx(w+36);
 *   r = (y << 8) + 0xc000; func_02213ce4(r, r, 0x00010000);
 *   func_02215a2c(w, 0); *ptr -= 0x30.
 * fidelity: HIGH - delay-slot stores (counter write before branch test), repeated
 *   reads of *0x060e0ddc, and call ordering preserved; bsrf offsets resolved to
 *   func_022149aa / func_02214a50 / func_022149e4 / func_02213ce4 / func_02215a2c. */

extern void sound_queue_push(s32 req);
extern u32  game_random_next(void);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at0(u32 obj);
extern void mtx_rotate_zyx(u32 angles);
extern void matrix_scale_columns(s32 x, s32 y, s32 z);
extern void model_submit_culled(u32 arg0, s32 arg1);

void prop_tick_cued_animation(u32 arg)
{
    u32 w = MU32(arg, 8);
    s32 y;
    u32 r;

    switch (M8(arg, 16)) {
    case 0:
        M16(w, 0x30) = 0x610;
        if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push(DH_SOUND_ANIMATED_PROP_BEGIN_A_COMMAND);
        if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push(DH_SOUND_ANIMATED_PROP_BEGIN_B_COMMAND);
        if (M16(MU32(DH_SCENE_DESCRIPTOR_ADDR, 0), 0x28) == 1 &&
            M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push(DH_SOUND_ANIMATED_PROP_VARIANT_COMMAND);
        M8(arg, 16) = (s8)(M8(arg, 16) + 1);
        /* fallthrough into state 1 */

    case 1: {
        s32 v = M32(w, 0x58) + 1;
        M32(w, 0x58) = v;
        if (v > 0x40) {
            M32(w, 0x5c) = 0x20;
            M8(arg, 16) = (s8)(M8(arg, 16) + 1);
        }
        break;
    }

    case 2: {
        s32 v = M32(w, 0x5c) + 1;
        M32(w, 0x5c) = v;
        if (v < 0)
            M8(arg, 16) = (s8)(M8(arg, 16) + 1);
        break;
    }

    case 3: {
        s32 v = M32(w, 0x58) - 1;
        M32(w, 0x58) = v;
        if (v <= 0) {
            M32(w, 0x58) = 0;
            M8(arg, 16) = 1;
        }
        break;
    }

    default:
        break;
    }

    y = M32(w, 0x58);
    r = ((u32)y << 8) + 0xc000;
    if (M32(0x06098664, 0) != 1) {
        M16(0x0609861e, 0) = (s16)y;
        M16(0x06098620, 0) = (s16)y;
    }
    M16(w, 0x28) = (s16)((game_random_next() & 0x1fff) + 0xf000);

    if (M16(MU32(DH_SCENE_DESCRIPTOR_ADDR, 0), 0x28) == 1) {
        u32 p = DH_MATRIX_STACK_CURSOR_ADDR;
        M32(p, 0) = M32(p, 0) + 0x30;
        mtx_load_abc();
        mtx_apply_vec_at0(w + 8);
        mtx_rotate_zyx(w + 36);
        matrix_scale_columns(r, r, 0x00010000);
        model_submit_culled(w, 0);
        M32(p, 0) = M32(p, 0) - 0x30;
    }
}
