/* text_clear_work_buffer [0x02222ecc]: Clears 0x400-word text working buffer at 060997b0. */
#include "dh.h"
/* 0x02222ecc  draw-callback thunk: clear the 0x400-word text working buffer at 0x060997B0.
 * First entry of a small tail-call thunk block (0x02222ecc..0x02223140); the sibling entries
 * (0x02222ed6, 0x02222ee2, 0x02222efc ...) are installed as the per-mode draw callbacks by
 * func_02222d3c / func_02222e44.
 * fidelity: HIGH - single resolved tail call. */
extern void memory_clear_vec4_blocks(s32 handle, s32 count);
void text_clear_work_buffer(void) {
    memory_clear_vec4_blocks(0x060997B0, 0x400);
}
