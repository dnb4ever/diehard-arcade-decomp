/* 0x022107dc  store_angle_between: *out = -angle(y1-y0, x1-x0).
 * Calls the vector->angle (atan2-style) helper at 0x022130b4 with
 *   r4 = arg3 - arg1 (dy),  r5 = arg2 - arg0 (dx);  negates the result and stores it as s16.
 * fidelity: faithful - stack arg shuffling, two sub (dy/dx, dx in bsrf delay slot), bsrf call,
 *   neg r0, mov.w store to the 5th (stack) arg. arg4 arrives on the stack. */
#include "dh_band21.h"

extern s32 atan2_fixed(s32 dy, s32 dx);   /* vector -> angle (atan2) */

void store_angle_between(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s16 *out) {
    s32 dy = arg3 - arg1;
    s32 dx = arg2 - arg0;
    *out = (s16)(-atan2_fixed(dy, dx));
}
