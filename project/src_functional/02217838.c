/* 0x02217838  dispatch_cmd: write `ctx` to the context global g_060b88a0, dispatch on
 * `sel` to one of three handlers (passing a0,a1 through), then reset g_060b88a0 to -1
 * and return the handler result. sel 0 -> func_02217888, 1 -> func_022179b0,
 * 2 -> func_02217ae4; any other sel returns sel unchanged (handlers not called).
 * fidelity: HIGH - the "*g = ctx" store is in the bt/s delay slot (always runs);
 *   cmp/eq ladder dispatch, r0->r6 result, "*g = -1" tail, return via r6. */
#include "dh_band21.h"

extern s32 grid_find_overlap_any(s32 a0, s32 a1);
extern s32 grid_find_overlap_static(s32 a0, s32 a1);
extern s32 grid_find_overlap_type_zero(s32 a0, s32 a1);

s32 dispatch_cmd(s32 a0, s32 a1, s32 sel, s32 ctx) {
    s32 *g = PTR(s32, 0x060b88a0);
    s32 result = sel;

    *g = ctx;
    switch (sel) {
    case 0: result = grid_find_overlap_any(a0, a1); break;
    case 1: result = grid_find_overlap_static(a0, a1); break;
    case 2: result = grid_find_overlap_type_zero(a0, a1); break;
    default: break;        /* sel unchanged, no handler */
    }
    *g = -1;
    return result;
}
