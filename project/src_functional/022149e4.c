/* 0x022149e4  mtx_rotate_zyx: rotate the active matrix by three u16 angles read from a
 * 3-halfword struct `angles` (@0=ax, @2=ay, @4=az), applied in Z,Y,X order:
 *   mtx_rotate_z(angles[2]); mtx_rotate_y(angles[1]); mtx_rotate_x(angles[0]);
 * Called by mtx_push_rotate_apply (0x21080a).
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). Angles loaded mov.w then
 *   extu.w -> (u16); mtx_rotate_x is a braf tail jump. Deltas resolve to the rotate_z/y/x
 *   primitives 0x213c4c / 0x213bf4 / 0x213ba0. */
#include "dh_band21.h"

void mtx_rotate_x(s32 angle);   /* 0x02213ba0 */
void mtx_rotate_y(s32 angle);   /* 0x02213bf4 */
void mtx_rotate_z(s32 angle);   /* 0x02213c4c */

void mtx_rotate_zyx(u16 *angles) {
    mtx_rotate_z(angles[2]);
    mtx_rotate_y(angles[1]);
    mtx_rotate_x(angles[0]);
}
