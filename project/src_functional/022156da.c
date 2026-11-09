/* Round 4: ROM 06019760/06019884 MOV.B sign-extends descriptor control before OR #2 and MOV.W; retain negative byte high bits. Evidence: work/decomp4/pointer_shapes.asm. */
#include "dh_work.h"
/* vdp1_build_sprite_commands [0x022156da]: Converts forward deferred sprite records into thirty-six-byte command storage entries. */
/* 0x022156da  build_vdp1_sprites: walk the deferred-sprite list at 0x060dc648
 * (stride 0x30, terminated by +0x20 == -1) and emit a 0x24-byte VDP1 command
 * per entry into the command buffer pointed to by *0x060cf0c4, advancing it.
 *   desc = entry[0x28]; attr = desc[0x0E];
 *   chr  = 0x060d28cc + (attr_tab[attr] << 4)   (attr_tab @0x060cf0cc, s16)
 *   cmd[0x20] = entry[0x20] + 0x200000;  cmd[0x00] = (s8)desc[6] | 2;
 *   cmd[0x04] = chr[8] | entry[0x2C];    cmd[0x06] = 0;
 *   cmd[0x08] = chr[0]; cmd[0x0A] = chr[2];
 *   cmd[0x0C..0x18] = entry[0x00..0x0C];
 *   counter @0x0609881c incremented (read via uncached mirror 0x2609881c).
 * fidelity: faithful - command fields, char-table *16 index, 0x24-byte stride.
 *   No PIC; sibling of the dispatch-emitter at 0x02215386. */
#include "dh_band21.h"

void vdp1_build_sprite_commands(void) {
    u32 src = 0x060dc648;

    while (SPRITE_DRAW_MEM32(src, depth) != -1) {
        u32 cmd  = MEM32(0x060cf0c4);
        u32 desc = SPRITE_DRAW_MEM32(src, descriptor);
        s32 attr = MEM16(desc + 0x0e);
        u32 chr  = 0x060d28cc + ((u32)(MEM16(0x060cf0cc + attr * 2)) << 4);

        VDP1_COMMAND_MEM32(cmd, sort_key) = SPRITE_DRAW_MEM32(src, depth) + 0x200000;
        VDP1_COMMAND_MEM16(cmd, control) = (s16)((s8)MEM8(desc + 6) | 2);
        VDP1_COMMAND_MEM16(cmd, draw_mode) = (s16)(SPRITE_CHARACTER_MEMU16(chr, draw_mode) | SPRITE_DRAW_MEMU16(src, mode_bits));
        VDP1_COMMAND_MEM16(cmd, colour) = 0;
        VDP1_COMMAND_MEM16(cmd, character_address) = SPRITE_CHARACTER_MEM16(chr, character_address);
        VDP1_COMMAND_MEM16(cmd, character_size) = SPRITE_CHARACTER_MEM16(chr, character_size);
        VDP1_COMMAND_MEM32(cmd, vertex_a) = SPRITE_DRAW_MEM32(src, vertex_a);
        VDP1_COMMAND_MEM32(cmd, vertex_b) = SPRITE_DRAW_MEM32(src, vertex_b);
        VDP1_COMMAND_MEM32(cmd, vertex_c) = SPRITE_DRAW_MEM32(src, vertex_c);
        VDP1_COMMAND_MEM32(cmd, vertex_d) = SPRITE_DRAW_MEM32(src, vertex_d);

        MEM32(0x0609881c) = MEM32(0x2609881c) + 1;
        MEM32(0x060cf0c4) = cmd + 0x24;
        src += 0x30;
    }
}
