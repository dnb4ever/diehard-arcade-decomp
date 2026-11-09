/* model_emit_or_queue [0x02215b34]: Validates the model ID and emits immediately or queues the display object based on FIFO fill. */
#include "dh_band21.h"
/* 0x02215b34 (emit_or_queue_sprite sibling) - gate an object through the id map at
 * 0x0609ebd0 (indexed by obj->unk30, s16); if the valid-entry is >=0, check the
 * command FIFO fill level (wptr 0x060e0dd4 minus base mirror 0x260e0dd0, rounded
 * toward zero and >>2). If fewer than 2 entries are pending, take the fast path:
 * fill the record at the write cursor 0x060da0c4 (fields +0x6c=sel, +0x70/74/78 =
 * obj->unk30/unk32/unk20), submit via func_022134d8, advance the cursor by 0x80,
 * push opcode 6 onto the FIFO and bump the success counter 0x06098820. Otherwise
 * run func_02213474 + func_02215cb4(obj,sel) and bump the fail counter 0x06098824.
 * fidelity: HIGH - bsrf targets resolved (0x022134d8, 0x02213474); FIFO rounding
 *   (add #3 when negative, arithmetic >>2), s16 field widenings, cursor stride. */

extern void matrix_copy_reverse_to_raw_entry(u32 rec);
extern void matrix_copy_reverse_raw_entry(void);
extern void model_build_quad_list(u32 obj, s32 sel);

void model_emit_or_queue(u32 obj, s32 sel)
{
    if (MEM16(0x0609ebd0 + (MEM16(obj + 0x30) << 1)) >= 0) {
        s32 diff = MEM32(0x060e0dd4) - MEM32(0x260e0dd0);
        if (diff < 0) {
            diff += 3;
        }
        if ((diff >> 2) < 2) {
            u32 rec = MEMU32(0x060da0c4);
            MEM32(rec + 0x6c) = sel;
            MEM32(rec + 0x70) = MEM16(obj + 0x30);
            MEM32(rec + 0x74) = MEM16(obj + 0x32);
            MEM32(rec + 0x78) = MEM16(obj + 0x20);
            matrix_copy_reverse_to_raw_entry(MEMU32(0x060da0c4));
            MEMU32(0x060da0c4) = MEMU32(0x060da0c4) + 0x80;
            {
                u32 w = MEMU32(0x060e0dd4);
                MEMU32(0x060e0dd4) = w + 4;
                MEM32(w) = 6;
            }
            MEM32(0x06098820) += 1;
        } else {
            matrix_copy_reverse_raw_entry();
            model_build_quad_list(obj, sel);
            MEM32(0x06098824) += 1;
        }
    }
}
