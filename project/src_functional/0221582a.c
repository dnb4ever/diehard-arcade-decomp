/* Round 4: ROM 06019760/06019884 MOV.B sign-extends descriptor control before OR #2 and MOV.W; retain negative byte high bits. Evidence: work/decomp4/pointer_shapes.asm. */
#include "dh_work.h"
/* vdp1_build_sprite_commands_reverse [0x0221582a]: Converts deferred sprites into the reverse-growing command buffer. */
/* 0x0221582a  build_vdp1_sprites_rev: identical to build_vdp1_sprites (0x2156da)
 * but for the second draw buffer: source list at 0x060da0c8, command pointer at
 * 0x060cf0c8, and the command pointer is DECREMENTED by 0x24 each entry (the
 * list is filled backwards). Terminated by entry[0x20] == -1.
 * fidelity: faithful - same command layout as 0x2156da; cmd ptr -= 0x24. */
#include "dh_band21.h"

void vdp1_build_sprite_commands_reverse(void) {
    u32 src = 0x060da0c8;

    while (SPRITE_DRAW_MEM32(src, depth) != -1) {
        u32 cmd  = MEM32(0x060cf0c8);
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
        MEM32(0x060cf0c8) = cmd - 0x24;
        src += 0x30;
    }
}
