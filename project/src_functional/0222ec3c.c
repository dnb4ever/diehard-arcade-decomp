/* animation_cell_upload_frame [0x0222ec3c]: Selects source frame and destination by cell state then copies 0x190 words. */
#include "dh.h"
/* 0x0222ec3c - drawlist block copy wrapper.
 * Reads the current stage/selector record g_06096c94 and computes two pointers:
 *   src = (*(g_06085854 + unk6*8 + 4)) + (((u16)unk2 - 1) * 0x640)
 *   dst = *(g_0609d7d0 + (s16)(g_0609ebd0[unkA] * 0x14) + 4)
 * then tail-calls copy_longs(src, dst, 0x190) to move a 0x640-byte (400-long)
 * block into the active drawlist record.
 * fidelity: HIGH - verified vs m2c_fn.py --asm; the tail braf resolves to
 *   0x0221326c (copy_longs). The (s16) narrowing on (g_0609ebd0[unkA]*0x14)
 *   mirrors the exts.w the compiler emits. MACL save/restore around the mul.l is
 *   a compiler artifact and not represented. */
extern void copy_longs_raw_entry(void *src, void *dst, s32 count);  /* 0x0221326c copy_longs */

extern u8  g_06096c94[];   /* stage/selector record */
extern u8  g_06085854[];   /* per-stage drawlist source table (8-byte entries) */
extern s16 g_0609ebd0[];   /* selector -> record-id map */
extern u8  g_0609d7d0[];   /* drawlist record table (0x14-byte entries) */

void animation_cell_upload_frame(void)
{
    s32 idx1 = M16(g_06096c94, 6);
    s32 idx2 = M16(g_06096c94, 10);
    s32 t    = g_0609ebd0[idx2];
    u8 *src  = (u8 *)MP(g_06085854, idx1 * 8 + 4);
    u8 *dst  = (u8 *)MP(g_0609d7d0, (s16)(t * 0x14) + 4);

    src += ((s32)(u16)M16(g_06096c94, 2) - 1) * 0x640;
    copy_longs_raw_entry(src, dst, 0x190);
}
