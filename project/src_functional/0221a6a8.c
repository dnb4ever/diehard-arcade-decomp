/* hud_recoil_sprite_task [0x0221a6a8]: Draws selected recoil frames while reducing scale and counting down its task lifetime. */
/* 0x0221a6a8  anim_recoil_state: per-frame update for a 3-state (obj[0x10])
 * animation object whose work record is obj[0x8] (= sub).
 *   state 2: obj[0] = 0; done.
 *   state 0: init - sub[0x7C] = (input nibble table == 0);  sub[0x60] *= 3;
 *            sub[0x68]=0x40000; sub[0x58]=60; sub[0x32]=0; sub[0xC]=-0x3800;
 *            sub[0x10]=0x10000; obj[0x10]++  (then fall through to state 1).
 *   state 0/1: for each set bit of sub[0x64] (1,2,4) load a frame pair
 *            sub[0x30] = (s16)tab300[sub[0x7C]+2k]; sub[0x8] = tab34C[sub[0x60]+k]
 *            and emit via func_022153f6(sub, sub[0x68], 4). Then decay sub[0x68]
 *            by 0x8000 while > 0x10000, and count sub[0x58] down; at 0 advance state.
 *   tab300 @0x06054300 (s16 frames), tab34C @0x0605434c (s32), input @0x06000248.
 * Frame IDs are halfword loads at 0601e744/0601e772/0601e7a0.
 * fidelity: faithful - mov.b s8 state, mul-by-3 (x2+x), bit tests on sub[0x64],
 *   PIC pointer resolved to func_022153f6, fixed-point decay. */
#include "dh.h"
#include "dh_work.h"

extern void sprite_draw_child(void *sub, s32 a, s32 b);

void hud_recoil_sprite_task(void *obj) {
    void *sub = TASK_PTR(obj, work);
    s8 state = TASK_S8(obj, state);

    if (state == 2) {
        M32(obj, 0) = 0;
        return;
    }

    if (state == 0) {
        s32 cfg = M32(0x060511dc + (((u32)M32(0x06000248, 0) >> 12) & 0xf) * 4, 0);
        M32(sub, 0x7c) = (cfg == 0) ? 1 : 0;
        M32(sub, 0x60) = M32(sub, 0x60) * 3;
        M32(sub, 0x68) = 0x40000;
        M32(sub, 0x58) = 60;
        M16(sub, 0x32) = 0;
        M32(sub, 0x0c) = -0x3800;
        M32(sub, 0x10) = 0x10000;
        TASK_S8(obj, state) = (s8)(state + 1);
    }

    if (state == 0 || state == 1) {
        s32 i7c = M32(sub, 0x7c);
        s32 i60 = M32(sub, 0x60);

        if (M32(sub, 0x64) & 1) {
            M16(sub, 0x30) = (s16)M16(0x06054300 + i7c * 4, 0);
            M32(sub, 0x08) = M32(0x0605434c + i60 * 4, 0);
            sprite_draw_child(sub, M32(sub, 0x68), 4);
        }
        if (M32(sub, 0x64) & 2) {
            M16(sub, 0x30) = (s16)M16(0x06054300 + (i7c + 2) * 4, 0);
            M32(sub, 0x08) = M32(0x0605434c + (i60 + 1) * 4, 0);
            sprite_draw_child(sub, M32(sub, 0x68), 4);
        }
        if (M32(sub, 0x64) & 4) {
            M16(sub, 0x30) = (s16)M16(0x06054300 + (i7c + 4) * 4, 0);
            M32(sub, 0x08) = M32(0x0605434c + (i60 + 2) * 4, 0);
            sprite_draw_child(sub, M32(sub, 0x68), 4);
        }

        if (M32(sub, 0x68) > 0x10000) {
            M32(sub, 0x68) -= 0x8000;
        }
        M32(sub, 0x58) -= 1;
        if (M32(sub, 0x58) == 0) {
            TASK_S8(obj, state) = (s8)(TASK_S8(obj, state) + 1);
        }
    }
}
