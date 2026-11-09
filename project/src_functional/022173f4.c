/* collision_sweep_pose [0x022173f4]: Stages a temporary collision pose and selects the overlap callback around the sweep. */
#include "dh_band21.h"
/* 0x022173f4 - stage a model/pose into the shared collision record at 0x060b88a8
 * and run the sweep via func_0221746c, with a per-mode mover hook written to
 * 0x060b889c. Copies arg0 into rec+8 and the 4-long pose (arg1[0..3]) into
 * rec+{0x18,0x1c,0x20,0x24}. mode 0: plain sweep. mode 1: hook=func_022179b0,
 * sweep, restore hook=func_02217888. mode 2: hook=func_02217ae4, sweep, restore
 * hook=func_02217888. default: no sweep, returns arg0 unchanged (r4 passthrough).
 *
 * Repair: the hook slots must hold the real (host-resolved) function pointers so
 * func_0221746c dispatches into the actual movers (which themselves touch
 * 0x060b889c during the sweep loops); the previous numeric constants (0x02217ae4
 * etc.) were inert stub targets and produced too few writes at 0x060b889c
 * (candidate 2 vs reference 6 in mode 2). Pointer values follow the same
 * convention as the sibling entity_move_dispatch (func_022170ea).
 * fidelity: HIGH. */

extern void *collision_resolve_sweep(u32 rec);              /* collide_resolve_sweep     */
extern void *grid_find_overlap_any(void *pos, void *ext); /* default mover callback    */
extern void *grid_find_overlap_static(void *pos, void *ext); /* push mover callback       */
extern void *grid_find_overlap_type_zero(void *pos, void *ext); /* slide mover callback      */

s32 collision_sweep_pose(s32 arg0, u32 pose, s32 mode)
{
    s32 ret = arg0;

    MEM32(0x060b88a8 + 0x08) = arg0;
    MEM32(0x060b88a8 + 0x18) = MEM32(pose + 0x0);
    MEM32(0x060b88a8 + 0x1c) = MEM32(pose + 0x4);
    MEM32(0x060b88a8 + 0x20) = MEM32(pose + 0x8);
    MEM32(0x060b88a8 + 0x24) = MEM32(pose + 0xc);

    switch (mode) {
    case 0:
        ret = (s32)collision_resolve_sweep(0x060b88a8);
        break;
    case 1:
        MEM32(0x060b889c) = (s32)grid_find_overlap_static;
        ret = (s32)collision_resolve_sweep(0x060b88a8);
        MEM32(0x060b889c) = (s32)grid_find_overlap_any;
        break;
    case 2:
        MEM32(0x060b889c) = (s32)grid_find_overlap_type_zero;
        ret = (s32)collision_resolve_sweep(0x060b88a8);
        MEM32(0x060b889c) = (s32)grid_find_overlap_any;
        break;
    }
    return ret;
}
