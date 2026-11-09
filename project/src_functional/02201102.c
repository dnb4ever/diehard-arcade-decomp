/* display_reset_command_stream [0x02201102]: Resets both render command cursors and clears the stream storage. */
#include "dh.h"
/* 0x02201102 - func_02201102: reset the 32-bit word stream buffers.
 * Sets both the write cursor (0x060E0DD0) and reset pointer (0x060E0DD4)
 * to the stream base 0x060E0BD0, clears the stage marker at 0x060987C4,
 * then zeroes consecutive word pairs from 0x060E0BD0 up to 0x060E0DD0.
 * fidelity: HIGH - hand-checked vs asm 0x201102..0x201136. */

extern s32 g_060E0DD0;                   /* write cursor of a 32-bit word stream */
extern s32 g_060E0DD4;                   /* stream reset pointer */
extern s32 g_060987C4;                   /* stage/cursor marker */

void display_reset_command_stream(void)
{
    u32 p;

    p = 0x060E0BD0U;
    g_060E0DD4 = (s32)0x060E0BD0;
    g_060E0DD0 = (s32)p;
    g_060987C4 = 0;
    if (p < 0x060E0DD0U) {
        do {
            M32((void *)(uintptr_t)p, 0) = 0;
            p += 4;
            M32((void *)(uintptr_t)p, 0) = 0;
            p += 4;
        } while (p < 0x060E0DD0U);
    }
}
