/* 0x022149b6  mtx_rotate_zyx_regs: rotate the active matrix by three angles passed in
 * registers (r4=ax, r5=ay, r6=az), applied in Z,Y,X order:
 *   mtx_rotate_z(az); mtx_rotate_y(ay); mtx_rotate_x(ax);
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). Each angle is spilled as
 *   a 16-bit value (mov.w) then zero-extended (extu.w) before the call, i.e. (u16); the
 *   mtx_rotate_x call is a braf tail jump. bsrf/braf deltas resolve to 0x213c4c / 0x213bf4
 *   / 0x213ba0. */
#include "dh_band21.h"

void mtx_rotate_x(s32 angle);   /* 0x02213ba0 */
void mtx_rotate_y(s32 angle);   /* 0x02213bf4 */
void mtx_rotate_z(s32 angle);   /* 0x02213c4c */

void mtx_rotate_zyx_regs(s32 ax, s32 ay, s32 az) {
    mtx_rotate_z((u16)az);
    mtx_rotate_y((u16)ay);
    mtx_rotate_x((u16)ax);
}
