/* vdp2_upload_pattern_groups [0x02215228]: Uploads size-classed pattern groups, builds the ID map and queues display command three. */
#include "dh_band21.h"
/* 0x02215228 - scene texture/pattern VRAM upload. Clears the 256-entry id->offset
 * map at 0x060d5ccc (s16, to -1), then walks the s16 id list at arg0 (terminated by
 * a negative id). For each id, entry = 0x22700aa0 + id*8; entry->unk4 selects a size
 * class (0/16/32). Each class uploads entry->unk0 (data) to the running VRAM cursor
 * (from 0x25f00000) via func_02214678(data,dest,units) - or func_02214328(data,dest)
 * when entry->unk6==1 - records the current byte offset in the map, and advances the
 * offset and VRAM cursor by the class stride. Finally waits for the VDP1 ready bit
 * (0x260e0dd8 & 0x80), pushes FIFO command 3 (*0x060e0dd4), and waits again (bounded).
 * fidelity: HIGH except the second spin-wait timeout counter, which the original
 *   reads from an uninitialized register (r7); modelled as starting at 0 (behaviour
 *   is a bounded busy-wait on the same ready bit). bsrf targets resolved to 0x214328;
 *   jsr target 0x214678 (mova+add). */

extern s32 sh2_dmac_copy_aligned_blocks(u32 data, u32 dest, s32 units);
extern s32 lz_decompress_buffer(u32 data, u32 dest);

void vdp2_upload_pattern_groups(u32 list)
{
    u32 dest = 0x25f00000;
    u32 off  = 0;
    s32 id;
    s32 i;

    for (i = 0; i < 0x100; i++) {
        MEM16(0x060d5ccc + i * 2) = -1;
    }

    id = MEM16(list);
    list += 2;

    while (id >= 0) {
        u32 ent = 0x22700aa0 + (u32)id * 8;
        s16 type = MEM16(ent + 4);

        if (type == 0) {
            if (MEM16(ent + 6) == 1) lz_decompress_buffer(MEM32(ent), dest);
            else                     sh2_dmac_copy_aligned_blocks(MEM32(ent), dest, 2);
            MEM16(0x060d5ccc + id * 2) = (s16)off;
            off  += 16;
            dest += 32;
        } else if (type == 16) {
            if (MEM16(ent + 6) == 1) lz_decompress_buffer(MEM32(ent), dest);
            else                     sh2_dmac_copy_aligned_blocks(MEM32(ent), dest, 8);
            MEM16(0x060d5ccc + id * 2) = (s16)off;
            off  += 64;
            dest += 0x80;
        } else if (type == 32) {
            if (MEM16(ent + 6) == 1) lz_decompress_buffer(MEM32(ent), dest);
            else                     sh2_dmac_copy_aligned_blocks(MEM32(ent), dest, 32);
            MEM16(0x060d5ccc + id * 2) = (s16)off;
            dest += 0x200;
            off  += 0x100;
        }

        id = MEM16(list);
        list += 2;
    }

    while ((MEM8(0x260e0dd8) & 0x80) != 0x80) { }
    MEM8(0x060e0dd8) = 0;

    {
        u32 w = MEM32(0x060e0dd4);
        MEM32(0x060e0dd4) = w + 4;
        MEM32(w) = 3;
    }

    {
        s32 timeout = 0;   /* original uses an uninitialized register here */
        while ((MEM8(0x260e0dd8) & 0x80) != 0x80) {
            if (++timeout > 0x2bf20) break;
        }
    }
    MEM8(0x060e0dd8) = 0;
}
