/* camera_tick_waypoint_path [0x0221ffb0]: Eight-byte XYZ/duration records drive three-axis linear view motion. */
#include "dh.h"
/* 0x0221ffb0  waypoint_mover_update: 3-state object that walks a sprite along a
 * path of s16[4] waypoints {x,y,z,frames} (stride 8), linearly interpolating.
 *
 * w = obj->unk8 (work record). sprite = (*g_0609882C)->unk8. All fixed-point
 * per-frame steps use sh2_hw_divide (0x213234): step = (delta << 8) / (frames<<16).
 * Positions are 16.16 with a +0x200000 world bias on X and Z.
 *
 * state 0: load the path head (*g_0609866C)->unk1C, seed position/velocity from
 *          node[0]->node[1], store frame count (w->unk74) and next node (w->unk78),
 *          advance to state 1.
 * state 1: if w->unk5A set, draw the sprite (func_022153f6). Integrate position by
 *          velocity; when w->unk74 frames elapse, reload from w->unk78: a negative
 *          frame count ends the path (advance state), else seed the next segment.
 * state 2: muzzle/flash timer on w->unk58 (sets g_06098664 at 0x28); blinks via
 *          w->unk6C toggled while g_060987BC&1; draws only on visible frames.
 * All drawing/stepping paths finish by copying w position into sprite+0xB8/BC/C0;
 * a state-2 invisible frame returns without that copy.
 *
 * fidelity: HIGH. braf resolved: two bsrf -> func_022153f6 (draw_child_sprite);
 * fixdiv pointer (mova+add) -> sh2_hw_divide. The dead r6 that m2c surfaced as a
 * 2nd argument is not consumed by sh2_hw_divide and is dropped. */
extern s32  sh2_hw_divide(s32 num, s32 den);               /* 0x02213234 */
extern void sprite_draw_child(void *obj, s32 scale, s32 flags);/* draw_child_sprite */

void camera_tick_waypoint_path(void *obj) {
    void *w      = MP(obj, 8);
    void *sprite = MP(MP(DH_VIEW_TASK_ADDR, 0), 8);
    s8    state  = M8(obj, 0x10);

    switch (state) {
    case 0: {
        u8 *node = (u8 *)MP(MP(DH_SCENE_DESCRIPTOR_ADDR, 0), 0x1C);
        s32 frames = M16(node, 6);

        M16(w, 0x30) = 0x140;
        M32(w, 0x14) = sh2_hw_divide((M16(node, 8)  - M16(node, 0)) << 8, frames << 16);
        M32(w, 0x18) = sh2_hw_divide((M16(node, 0xA) - M16(node, 2)) << 8, frames << 16);
        M32(w, 0x1C) = sh2_hw_divide((M16(node, 0xC) - M16(node, 4)) << 8, frames << 16);
        M32(w, 0x08) = (M16(node, 0) << 8) + 0x200000;
        M32(w, 0x0C) = (M16(node, 2) << 8);
        M32(w, 0x10) = (M16(node, 4) << 8) + 0x200000;
        M32(w, 0x74) = frames;
        MP(w, 0x78)  = node + 8;
        M8(obj, 0x10) = (s8)(M8(obj, 0x10) + 1);
        break;
    }
    case 1:
        if (M16(w, 0x5A) != 0) {
            sprite_draw_child(w, 0x20000, 1);
        }
        M32(w, 0x08) += M32(w, 0x14);
        M32(w, 0x0C) += M32(w, 0x18);
        M32(w, 0x10) += M32(w, 0x1C);
        M32(w, 0x74) -= 1;
        if (M32(w, 0x74) == 0) {
            u8 *node = (u8 *)MP(w, 0x78);
            M32(w, 0x08) = (M16(node, 0) << 8) + 0x200000;
            M32(w, 0x0C) = (M16(node, 2) << 8);
            M32(w, 0x10) = (M16(node, 4) << 8) + 0x200000;
            if (M16(node, 6) < 0) {
                M8(obj, 0x10) = (s8)(M8(obj, 0x10) + 1);
            } else {
                s32 frames = M16(node, 6);
                M32(w, 0x14) = sh2_hw_divide((M16(node, 8)  - M16(node, 0)) << 8, frames << 16);
                M32(w, 0x18) = sh2_hw_divide((M16(node, 0xA) - M16(node, 2)) << 8, frames << 16);
                M32(w, 0x1C) = sh2_hw_divide((M16(node, 0xC) - M16(node, 4)) << 8, frames << 16);
                M32(w, 0x74) = frames;
                MP(w, 0x78)  = node + 8;
                M16(w, 0x5A) = 1;
            }
        }
        break;
    case 2:
        M16(w, 0x58) += 1;
        if (M16(w, 0x58) == 0x28) {
            M32(0x06098664, 0) = 1;
        }
        if (M32(DH_FRAME_TICK_ADDR, 0) & 1) {
            M32(w, 0x6C) ^= 1;
        }
        if (M32(w, 0x6C) & 1) {
            return;                 /* invisible frame: no position copy */
        }
        sprite_draw_child(w, 0x20000, 1);
        break;
    default:
        break;
    }

    /* copy work position into the sprite record */
    M32(sprite, 0xB8) = M32(w, 0x08);
    M32(sprite, 0xBC) = M32(w, 0x0C);
    M32(sprite, 0xC0) = M32(w, 0x10);
}
