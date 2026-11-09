/* stage_initialize_grids [0x02216c38]: Seeds the stage anchor and builds the stage render and collision grids. */
/* 0x0216c38 - set-piece/stage initialiser dispatcher (arg0 = stage index, arg1 = variant).
 * Latches the two parameters to globals, seeds the shared camera/anchor record at 0x0609742C
 * from the stage table 0x0606080C (stride 0x2C), then hands off to the two workers:
 *   *0x060B3D54 = arg0; *0x060B3D58 = arg1;
 *   rec = 0x0606080C + arg0*0x2C;
 *   *(0x0609742C+0) = rec->unk8  + 0xFFF00000;   (x, biased)
 *   *(0x0609742C+4) = rec->unkC;                  (y)
 *   *(0x0609742C+8) = rec->unk10 + 0xFFF80000;    (z, biased)
 *   func_02216c88(arg0, arg1, 0x0609742C);         (bsr)
 *   func_02216f7a(arg0);                            (tail jump)
 * ABI: r4 = arg0, r5 = arg1; no return.
 * fidelity: HIGH - reconstructed from fn.py (m2c mis-merges this with the bra-tail-jump target
 *   0x216f7a and drops the macl saves). This is the small real entry 0x216c38; the large loop
 *   body the census size (527) implies belongs to the separate real entries 0x216c88 (bsr) and
 *   0x216f7a (bra tail-call), both confirmed by find_entry. The 0x0606080C base is recomputed
 *   per field in the SH-2 (mul @0x060B3D54 by 0x2C each time); folded to one read here as the
 *   global is not modified between them. The two +0xFFF.....0 biases are 32-bit wrap adds. */
#include "dh_band21.h"

extern void stage_build_render_grid(s32 stage, s32 variant, u32 rec);  /* bsr worker */
extern void stage_build_actor_grid(s32 stage);                        /* tail-jump worker */

void stage_initialize_grids(s32 arg0, s32 arg1)
{
    u32 base;

    MEM32(0x060b3d54) = arg0;
    MEM32(0x060b3d58) = arg1;

    base = 0x0606080c + MEM32(0x060b3d54) * 44;
    MEM32(0x0609742c + 0) = MEM32(base + 8) + (s32)0xfff00000;
    MEM32(0x0609742c + 4) = MEM32(base + 12);
    MEM32(0x0609742c + 8) = MEM32(base + 16) + (s32)0xfff80000;

    stage_build_render_grid(MEM32(0x060b3d54), MEM32(0x060b3d58), 0x0609742c);
    stage_build_actor_grid(MEM32(0x060b3d54));
}
