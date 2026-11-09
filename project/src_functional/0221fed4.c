/* cam_aim_update [0x0221fed4]: Existing map corroborated by source: (u32 task) weighted_accum3 -> X/Z offset +0x200000, look-at builder, phase accum base->unk64 (wrap >0x70000), mtx_build_fwd_inv (src symbols_band210b.txt) */
/* 0x0221fed4 - per-frame camera/aim update task. base = task->unk8.
 * On first entry (task->unk10 == 0): seed base->unkD4 from
 * (*0x0609866C)->unk10, base->unk64 = 0, base->unk68 = 0x400, bump the phase.
 * Every frame: run weighted_accum3 (func_02214194) over base->unkD4 selected by
 * base->unk64 into a 3-long scratch, then offset X/Z by 0x200000 into
 * base->unk8/unkC/unk10. Feed those plus base->unkB8/BC/C0 through the look-at
 * builder func_0221055c, writing elevation/azimuth to base->unk24/unk26. Advance
 * the phase accumulator base->unk64 += base->unk68, wrapping to 0 past 0x70000.
 * Finish with the matrix builder mtx_build_fwd_inv(base).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - m2c lost the stack out-params; reconstructed from fn.py.
 *   3 bsrf resolved (braf_resolve): 0x214194 weighted_accum3, 0x21055c look-at,
 *   0x214a7a mtx_build_fwd_inv. 8-arg call to func_0221055c verified against its
 *   existing signature (6 s32 + 2 s16* out). */
#include "dh_band21.h"

extern void vector3_blend_four(void *rows, s32 sel, void *out3);  /* weighted_accum3 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *elev, s16 *azim);             /* look-at angles */
extern void mtx_build_fwd_inv(u32 obj);                          /* mtx_build_fwd_inv */

void cam_aim_update(u32 task)
{
    u32 base = MEMU32(task + 8);
    s32 out[3];

    if (MEM8(task + 16) == 0) {
        MEM32(base + 0xd4) = MEM32(MEMU32(DH_SCENE_DESCRIPTOR_ADDR) + 0x10);
        MEM32(base + 0x64) = 0;
        MEM32(base + 0x68) = 0x400;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
    }

    vector3_blend_four(PTR(void, MEMU32(base + 0xd4)), MEM32(base + 0x64), out);
    MEM32(base + 0x08) = out[0] + 0x200000;
    MEM32(base + 0x0c) = out[1];
    MEM32(base + 0x10) = out[2] + 0x200000;

    camera_compute_lookat_angles(MEM32(base + 0x08), MEM32(base + 0x0c), MEM32(base + 0x10),
                  MEM32(base + 0xb8), MEM32(base + 0xbc), MEM32(base + 0xc0),
                  PTR(s16, base + 0x24), PTR(s16, base + 0x26));

    MEM32(base + 0x64) += MEM32(base + 0x68);
    if (MEM32(base + 0x64) > 0x70000)
        MEM32(base + 0x64) = 0;

    mtx_build_fwd_inv(base);
}
