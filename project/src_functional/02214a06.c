/* mtx_rotate_zyx_at24 [0x02214a06]: Rotates the active matrix by the object's unsigned Z, Y and X pose angles. */
/* 0x02214a06  mtx_rotate_zyx_at24: rotate the active matrix by three u16 angles read from
 * `obj` at offsets 0x24/0x26/0x28, applied in Z,Y,X order:
 *   mtx_rotate_z(*(u16*)(obj+0x28));
 *   mtx_rotate_y(*(u16*)(obj+0x26));
 *   mtx_rotate_x(*(u16*)(obj+0x24));
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0; also bsr'd internally by
 *   0x214a7a). Angles loaded mov.w @(disp,r14) then extu.w -> (u16); rotate_x is a braf
 *   tail jump. Deltas resolve to 0x213c4c / 0x213bf4 / 0x213ba0. */
#include "dh_band21.h"

void mtx_rotate_x(s32 angle);   /* 0x02213ba0 */
void mtx_rotate_y(s32 angle);   /* 0x02213bf4 */
void mtx_rotate_z(s32 angle);   /* 0x02213c4c */

void mtx_rotate_zyx_at24(u8 *obj) {
    mtx_rotate_z(*(u16 *)(obj + 0x28));
    mtx_rotate_y(*(u16 *)(obj + 0x26));
    mtx_rotate_x(*(u16 *)(obj + 0x24));
}
