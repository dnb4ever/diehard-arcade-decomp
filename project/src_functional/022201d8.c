/* display_tick_strip_upload [0x022201d8]: Task stores strip/data cursors, line index and assembled cell for per-frame upload. */
#include "dh_work.h"
#include "dh.h"
/* 0x022201d8  per-frame scanline/strip upload task (state machine).
 * task   (r4): +0x08 -> work struct, +0x10 = state byte.
 * work       : +0x64 holds the source record pointer on entry, later reused as the
 *              VDP1 VRAM write pointer; +0x68 = data cursor; +0x44 = line count;
 *              +0x40 = line index; +0x3C = word-order flag; +0x60 = assembled 32-bit cell.
 * record (work->unk64 on entry): +0x00 count/word (s16), +0x02 offset+flag (s16),
 *              +0x04 data pointer.
 *   state 0: latch record fields; dest = 0x25F00000 + ((rec.off >> 1) << 2); advance.
 *   state 1: ++index; when index >= count advance.
 *   state 2: build 32-bit cell (hi/lo order per flag), hand to func_0221632e(14, &cell,
 *            dest, 1); stop (advance) at a 0xFFFF terminator, else bump cursor.
 *   state 3/other: nothing.
 * NOTE (faithful): work->unk64 (loaded into r13 every call) is the record in state 0 but
 * the VRAM pointer afterwards; state 2 therefore re-reads rec fields from whatever unk64
 * currently points at - reproduced exactly as the ROM does it.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x21632e. */

extern void vdp2_queue_attribute(s32 a, void *cell, void *dst, s32 n);

void display_tick_strip_upload(void *task) {
    void *work = TASK_PTR(task, work);
    void *rec  = MP(work, 0x64);

    switch (TASK_U8(task, state)) {
    case 0:
        MP(work, 0x68) = MP(rec, 0x04);
        M32(work, 0x44) = M16(rec, 0x00);
        MP(work, 0x64) = (void *)(uintptr_t)(u32)(0x25F00000 + ((M16(rec, 0x02) >> 1) << 2));
        M32(work, 0x3C) = M16(rec, 0x02) & 1;
        TASK_U8(task, state)++;
        /* fallthrough */
    case 1:
        M32(work, 0x40) += 1;
        if (M32(work, 0x40) >= M32(work, 0x44))
            TASK_U8(task, state)++;
        break;
    case 2: {
        void *cur = MP(work, 0x68);
        u16 v = MU16(cur, 0x00);
        if (M32(work, 0x3C) != 0)
            M32(work, 0x60) = (s32)(((u32)MU16(rec, 0x00) << 16) | v);
        else
            M32(work, 0x60) = (s32)(((u32)v << 16) | MU16(rec, 0x02));
        vdp2_queue_attribute(14, (u8 *)work + 0x60, MP(work, 0x64), 1);
        cur = (u8 *)cur + 2;
        if (MU16(cur, 0x00) == 0xFFFF)
            TASK_U8(task, state)++;
        else
            MP(work, 0x68) = cur;
        break;
    }
    default:
        break;
    }
}
