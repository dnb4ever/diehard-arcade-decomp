/* vdp1_upload_texture_groups [0x022150d4]: Builds character table, DMA uploads texture groups and queues display command 2. */
/* 0x022150d4  upload_texture_groups: build the VDP1 character table and DMA the
 * referenced texture data into VDP1 VRAM for a neg-terminated list of group ids.
 *   - clear 0x1c00 entries of the char-index table @0x060cf0cc to -1.
 *   - per group id g: descriptor @0x22700000 + g*0x14 =
 *         {char_defs(+0), dma_src(+4), base_id(+8), count(+0xC), dma_size(+0x10)}
 *     for each of count chars: copy 5 shorts to the char-entry table @0x060d28cc
 *       (first short offset by the running V-coord), record the global sprite id
 *       in idxtab[base_id]; then DMA dma_size bytes (dma_src -> VRAM) and advance
 *       VRAM, recomputing the V-coord as (VRAM-0x25c00000)/8.
 *   - wait for status bit7 @0x260e0dd8, push command 2 to the queue @0x060e0dd4,
 *     wait again (bounded to 0x2bf20 spins), clear the status byte.
 * fidelity: faithful - bsrf resolved to dma_ch0_transfer (0x2145d0); status read
 *   via uncached mirror 0x260e0dd8, written to cached 0x060e0dd8. */
#include "dh.h"

extern s32 dma_ch0_transfer(s32 src, s32 dst, s32 count);

void vdp1_upload_texture_groups(s16 *list) {
    s16 *idxtab = (s16 *)(uintptr_t)0x060cf0cc;
    u32 chr  = 0x060d28cc;
    u32 vram = 0x25c10000;
    s32 coord = 0x2000;
    s32 sprite_id = 0;
    s32 i, grp;

    for (i = 0; i < 0x1c00; i++) {
        idxtab[i] = -1;
    }

    grp = *list++;
    while (grp >= 0) {
        u32 g       = 0x22700000 + grp * 0x14;
        u32 src     = M32(g, 0x00);
        s32 dma_src = M32(g, 0x04);
        s32 base_id = M32(g, 0x08);
        s32 count   = M32(g, 0x0c);
        s32 dma_size = M32(g, 0x10);
        s32 k;

        for (k = 0; k < count; k++) {
            M16(chr, 0x00) = (s16)(M16(src, 0) + coord);
            M16(chr, 0x02) = M16(src, 2);
            M16(chr, 0x04) = M16(src, 4);
            M16(chr, 0x06) = M16(src, 6);
            M16(chr, 0x08) = M16(src, 8);
            sprite_id++;
            idxtab[base_id] = (s16)(sprite_id - 1);
            base_id++;
            chr += 16;
            src += 16;
        }

        if (dma_size != 0) {
            dma_ch0_transfer(dma_src, (s32)vram, dma_size >> 2);
            vram += dma_size;
        }

        {
            s32 t = (s32)(vram - 0x25c00000);
            if (t < 0) {
                t += 7;
            }
            coord = t >> 3;
        }

        grp = *list++;
    }

    while ((MU8(0x260e0dd8, 0) & 0x80) != 0x80) {
    }
    MU8(0x060e0dd8, 0) = 0;

    {
        u32 q = M32(0x060e0dd4, 0) + 4;
        M32(0x060e0dd4, 0) = q;
        M32(q - 4, 0) = 2;
    }

    while ((MU8(0x260e0dd8, 0) & 0x80) != 0x80) {
        if (++sprite_id > 0x2bf20) {
            break;
        }
    }
    MU8(0x060e0dd8, 0) = 0;
}
