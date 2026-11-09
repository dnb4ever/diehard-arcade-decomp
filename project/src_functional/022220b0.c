/* clear_word_region [0x022220b0]: Tail-calls vector clear with arithmetic count>>2. */
#include "dh.h"
/* 0x022220b0  thin wrapper: tail-call func_02213290(handle, count >> 2) (arithmetic shift).
 * fidelity: HIGH - single resolved braf tail call.
 * NOTE: a separate, unreferenced command-list interpreter shares this unit's literal pool and
 * occupies the rest of the 0x302-byte extent; it has no recognised entry and is left as band
 * remaining. */
extern void memory_clear_vec4_blocks(s32 handle, s32 count);
void clear_word_region(s32 handle, s32 count) {
    memory_clear_vec4_blocks(handle, count >> 2);
}
