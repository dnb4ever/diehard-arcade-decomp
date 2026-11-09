/* interaction_append_slot_records [0x02225fac]: Appends up to two payloads to per-slot interaction stream; sets overflow at actor+52. */
#include "dh.h"
/* 0x02225fac  append up to two per-slot interaction/shot records.
 * arg0 (r4): actor; +0x34 = slot index (s8), +0x52 = overflow flag (set on full).
 * arg1 (r5): source descriptor (+0x00 id, +0x02 misc, +0x04 / +0x06 two payload words,
 *            +0x08 / +0x0A two "next" indices).
 * arg2 (r6): base of the descriptor array (stride 0xC) used to re-target the 2nd write.
 * arg3 (r7): u16 flag bits (bit0 -> write first payload, bit1 -> write second).
 * Per-slot state: g_06096E90[slot] = running count, g_06096EB0[slot] = capacity-left
 * (0 when consumed), g_06096C74[slot] = attribute word, g_06096EA0[slot] = last next-index.
 * Record buffer g_06096CB0 is partitioned 0x3C (=slot*60) bytes per slot, 6-byte entries.
 * If capacity-left already exceeds the source's misc field, set overflow and bail.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm; no PIC branches. The (s8) cast on the
 * entry offset (count*6 truncated to a byte then sign-extended) is preserved verbatim. */

extern u8 g_06096C74[];   /* s32 per slot */
extern u8 g_06096CB0[];   /* 0x3C-byte region per slot, 6-byte entries */
extern u8 g_06096E90[];   /* s16 per slot: entry count */
extern u8 g_06096EA0[];   /* s16 per slot */
extern u8 g_06096EB0[];   /* s16 per slot: capacity consumed */

void interaction_append_slot_records(void *arg0, void *arg1, s32 arg2, u16 arg3) {
    void *src = arg1;
    s32 slot    = M8(arg0, 0x34);
    s32 i2      = slot * 2;
    s32 i4      = slot * 4;
    s32 off     = (s16)(slot * 60);

    if (M16(g_06096EB0, i2) > (s32)MU16(src, 0x02)) {
        MU16(arg0, 0x52) = 1;
        return;
    }

    if ((arg3 & 1) && M16(src, 0x04) != 0) {
        u16 c = (u16)(MU16(g_06096E90, i2) + 1);
        u8 *base = g_06096CB0 + off;
        u8 *e0   = base + (s8)((u32)c * 6);
        MU16(g_06096E90, i2) = c;
        M16(e0, 0x00) = M16(src, 0x04);
        M16(base + (s8)(((u32)c + 1) * 6), 0x00) = (s16)M32(g_06096C74, i4);
        M16(e0, 0x02) = M16(src, 0x00);
        M16(g_06096EA0, i2) = M16(src, 0x08);
        src = (void *)(uintptr_t)(u32)(arg2 + (s32)MU16(src, 0x08) * 0xC);
        M16(g_06096EB0, i2) = 0;
    }

    if ((arg3 & 2) && M16(src, 0x06) != 0) {
        u16 c = (u16)(MU16(g_06096E90, i2) + 1);
        u8 *base = g_06096CB0 + off;
        u8 *e0   = base + (s8)((u32)c * 6);
        MU16(g_06096E90, i2) = c;
        M16(e0, 0x00) = M16(src, 0x06);
        M16(base + (s8)(((u32)c + 1) * 6), 0x00) = (s16)M32(g_06096C74, i4);
        M16(e0, 0x02) = M16(src, 0x00);
        M16(g_06096EA0, i2) = M16(src, 0x0A);
        M16(g_06096EB0, i2) = 0;
    }
}
