/* vdp2_flush_shadow_banks [0x022165c6]: Flushes current 0x120-byte bank then refreshes opposite bank from shared shadow. */
#include "dh_band21.h"
/* 0x022165c6 - double-buffered blit pair for the two VDP work banks. The parity
 * flag at 0x06099574 selects the "current" buffer; other = current^1 (computed but
 * NOT written back here). Each parity indexes a per-buffer record table at
 * 0x06098ba8 with stride 0x120 (288). Flushes the current buffer to VDP RAM
 * 0x25f80000 via func_02216582, then tail-calls func_0221654a for the other buffer
 * using the pointer stored at 0x06098f08.
 * fidelity: HIGH - parity read twice (current for 02216582, other for 0221654a),
 *   stride *0x120 sign-extended to s16, tail-call result forwarded. */

extern s32 vdp2_commit_shadow_bank(u32 vdp, u32 rec);
extern s32 vdp2_copy_shadow_bank(u32 rec, u32 arg);

s32 vdp2_flush_shadow_banks(void)
{
    u32 current = MEMU32(0x06099574);
    u32 other   = current ^ 1;

    vdp2_commit_shadow_bank(VDP2_TVMD_ADDR, 0x06098ba8 + (u32)(s16)(current * 0x120));
    return vdp2_copy_shadow_bank(0x06098ba8 + (u32)(s16)(other * 0x120), MEMU32(DH_VDP2_REGISTER_SHADOW_ADDR));
}
