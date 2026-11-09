/* sprite_emit_or_queue [0x02215964]: Validates the model ID and emits immediately or queues a sprite based on FIFO fill. */
/* 0x02215964 (emit_or_queue_sprite) - dispatch for one display object arg0.
 * Looks up a validity code = *(s16*)(0x0609ebd0 + arg0->unk30*2); if out of range
 * [0, 0x618] it does nothing. Otherwise it checks how full the command FIFO is:
 * q = sdiv32(fifo_write_ptr(0x060e0dd4) - fifo_base(0x260e0dd0 mirror), 4), i.e. the
 * pending entry count. If q >= 4 the FIFO is busy, so it takes the heavy path
 * (matrix_copy_reverse matrix setup + func_02215cb4 model submit) and bumps the
 * busy counter *0x06098824. Otherwise it fast-pokes a sprite command: into the
 * display-list slot *0x060da0c4 write unkA4 at +0x6C and the s16 fields
 * unk30/unk32/unk20 at +0x70/+0x74/+0x78, run matrix_copy_reverse_to on it, advance the slot
 * by 0x80, push opcode 6 into the FIFO, and bump the fast counter *0x06098820.
 * ABI: r4 = arg0; no return.
 * fidelity: HIGH - reconstructed from fn.py. The sdiv32 result lands in r0 via the
 *   r0/r1 helper ABI (divisor 4 placed in the bsrf delay slot); expressed as the
 *   existing sdiv32(dividend, divisor) extern. bsrf targets resolved (braf_resolve):
 *   0x24a06c sdiv32, 0x2134d8 matrix_copy_reverse_to, 0x213474 matrix_copy_reverse (matrix_copy_
 *   reverse). 0x260e0dd0 and 0x060e0dd4 are kept as the exact (mirror/base) addresses
 *   the SH-2 subtracts. */
#include "dh_band21.h"

extern s32  sdiv32(s32 dividend, s32 divisor);       /* 0x24a06c (r1/r0 ABI) */
extern void matrix_copy_reverse(void);                     /* matrix_copy_reverse */
extern void matrix_copy_reverse_to(u32 slot);
extern void model_build_quad_list(u32 obj, s32 flag);

void sprite_emit_or_queue(u32 arg0)
{
    s32 code = MEM16(0x0609ebd0 + MEM16(arg0 + 0x30) * 2);

    if (code < 0 || code > 0x618)
        return;

    if (sdiv32(MEM32(0x060e0dd4) - MEM32(0x260e0dd0), 4) >= 4) {
        matrix_copy_reverse();
        model_build_quad_list(arg0, MEM32(arg0 + 0xa4));
        MEM32(0x06098824) += 1;
    } else {
        u32 slot = MEMU32(0x060da0c4);
        MEM32(slot + 0x6c) = MEM32(arg0 + 0xa4);
        MEM32(MEMU32(0x060da0c4) + 0x70) = MEM16(arg0 + 0x30);
        MEM32(MEMU32(0x060da0c4) + 0x74) = MEM16(arg0 + 0x32);
        MEM32(MEMU32(0x060da0c4) + 0x78) = MEM16(arg0 + 0x20);
        matrix_copy_reverse_to(MEMU32(0x060da0c4));
        MEM32(0x060da0c4) += 0x80;
        {
            u32 wp = MEMU32(0x060e0dd4);
            MEM32(0x060e0dd4) = wp + 4;
            MEM32(wp) = 6;
        }
        MEM32(0x06098820) += 1;
    }
}
