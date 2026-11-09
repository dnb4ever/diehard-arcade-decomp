#include "dh_work.h"
/* sprite_draw_child [0x022153f6]: Transforms/culls child sprite and emits depth-sorted VDP1 command. */
/* 0x022153f6  draw_child_sprite: push a child matrix frame, transform the object,
 * and (if visible and in depth range) emit a VDP1 sprite command.
 *   chr = 0x060d28cc + attrtab[obj[0x30]] * 0x10;     (attrtab @0x060cf0cc)
 *   *0x06098878 += 0x30;                              (push active-matrix frame)
 *   matrix_builder[flags & 1](obj);                   (table @0x06090770)
 *   mtx_apply_vec_at8(obj);
 *   if (clip_test()) { *0x06098878 -= 0x30; return; } (off-screen cull)
 *   project_sprite_quad((u16)chr[4], (u16)chr[6], scale);
 *   *0x06098878 -= 0x30;
 *   depth = g_06098b1c[1]; if in [0x10000,0x1FBFFF] emit command to *0x060cf0c8
 *     (buffer filled backwards, ptr -= 0x24): command built from chr + obj + the
 *     projected result at 0x06098b1c. depth offset by 0x200000 unless flags&4.
 * fidelity: faithful - two bsrf resolved (0x214a40 mtx_apply_vec_at8, 0x2139b8
 *   project_sprite_quad), table-dispatch matrix builder, matrix push/pop. */
#include "dh.h"

extern void mtx_apply_vec_at8(void *obj);
extern void project_sprite_quad(s32 w, s32 h, s32 scale);
extern s32 clip_test(void);

void sprite_draw_child(void *obj, s32 scale, s32 flags) {
    u32 chr = 0x060d28cc + ((u32)M16(0x060cf0cc, M16(obj, 0x30) * 2) << 4);

    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) += 0x30;
    ((void (**)(void *))(uintptr_t)0x06090770)[flags & 1](obj);
    mtx_apply_vec_at8(obj);

    if (clip_test() != 0) {
        M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) -= 0x30;
        return;
    }

    project_sprite_quad((u16)SPRITE_CHARACTER_M16(chr, width), (u16)SPRITE_CHARACTER_M16(chr, height), scale);
    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) -= 0x30;

    {
        s32 depth = M32(DH_TRANSFORMED_VECTOR_ADDR, 4);
        if (depth >= 0x10000 && depth <= 0x1fbfff) {
            u32 cmd = M32(0x060cf0c8, 0);
            VDP1_COMMAND_M32(cmd, sort_key) = (flags & 4) ? depth : depth + 0x200000;
            VDP1_COMMAND_M16(cmd, control) = (s16)(M16(obj, 0x22) | 0x0a01);
            VDP1_COMMAND_M16(cmd, draw_mode) = (s16)(SPRITE_CHARACTER_M16(chr, draw_mode) | M16(obj, 0x20));
            VDP1_COMMAND_M16(cmd, colour) = 0;
            VDP1_COMMAND_M16(cmd, character_address) = SPRITE_CHARACTER_M16(chr, character_address);
            VDP1_COMMAND_M16(cmd, character_size) = SPRITE_CHARACTER_M16(chr, character_size);
            VDP1_COMMAND_M32(cmd, vertex_a) = M32(DH_TRANSFORMED_VECTOR_ADDR, 0);
            VDP1_COMMAND_M32(cmd, vertex_b) = M32(DH_TRANSFORMED_VECTOR_ADDR, 8);
            M32(0x0609881c, 0) = M32(0x2609881c, 0) + 1;
            M32(0x060cf0c8, 0) = cmd - 0x24;
        }
    }
}
