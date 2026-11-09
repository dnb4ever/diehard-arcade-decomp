#include "dh_work.h"
/* sprite_draw_billboard [0x022154fc]: Projects billboard corners and emits VDP1 quad under matrix-mode and depth gates. */
/* 0x022154fc (draw_billboard_quad) - transform an object and emit a projected
 * billboard quad into the sprite command list. arg0 = object, arg1/arg2 = quad
 * half-extent scales (x,y) passed to project_quad_corners, arg3 = mode|flags.
 * mode = arg3 & 3 selects how the model matrix is set up:
 *   0: push frame (g_06098878 += 0x30), load_matrix_default, mtx_apply_vec_at8(arg0);
 *      if the frustum test clip_test rejects it, pop and return; else
 *      mtx_rotate_zyx_at24(arg0), commit the matrix (matrix_copy_rowswap), take the depth
 *      z = top_matrix->unk2C.
 *   1: same but with mtx_load_abc instead of load_matrix_default.
 *   2: no matrix push; if clip_test rejects, return (nothing pushed); else commit
 *      (matrix_copy_rowswap) and take z = top_matrix->unk2C.
 *   3: no matrix was set up, so the depth register still holds a stale non-coordinate
 *      value that always fails the range gate below -> this mode is a no-op.
 * Common emit (modes 0-2): look up the quad record rec = g_060D28CC + id*0x10 where
 * id = *(s16*)(0x060CF0CC + arg0->unk30*2). Only when 0x10000 <= z < 0x1FBFFF:
 * project_quad_corners(rec->unk4, rec->unk6, arg1, arg2) fills the 4 screen corners
 * at g_06098B1C[0..3]; then fill the next sprite slot *0x060CF0C8: +0x20 = z (+0x200000
 * unless arg3 & 4), +0 = arg0->unk22 | 2, +4 = rec->unk8 | arg0->unk20, +6 = 0,
 * +8 = rec->unk0, +0xA = rec->unk2, +0xC..+0x18 = the 4 projected corners; bump the
 * sprite counter (*0x0609881C = *0x2609881C + 1) and advance the slot (-0x24).
 * ABI: r4 = arg0, r5 = arg1, r6 = arg2, r7 = arg3; no return.
 * fidelity: HIGH - reconstructed from fn.py. bsrf targets resolved (braf_resolve):
 *   load_matrix_default 0x213fb4, mtx_apply_vec_at8 0x214a40, mtx_rotate_zyx_at24
 *   0x214a06, mtx_load_abc 0x2149aa, project_quad_corners 0x213a70; the jsr @r12 is
 *   the mova+add PIC pointer to matrix_copy_rowswap (matrix commit to g_060DA040).
 *   clip_test and matrix_copy_rowswap read the global matrix stack and ignore r4.
 *   0x2609881C (read) / 0x0609881C (write) kept as the exact mirror/base pair. */
#include "dh_band21.h"

extern void load_matrix_default(void);            /* load_matrix_default */
extern void mtx_load_abc(void);                   /* mtx_load_abc */
extern void mtx_apply_vec_at8(u8 *obj);           /* mtx_apply_vec_at8 */
extern void mtx_rotate_zyx_at24(u8 *obj);         /* mtx_rotate_zyx_at24 */
extern void matrix_copy_rowswap(void);                  /* commit top matrix -> g_060DA040 */
extern s32  clip_test(void);                  /* frustum/back-face cull test */
extern void project_quad_corners(s32 half_w, s32 half_h, s32 scale_x, s32 scale_y); /* 0x213a70 */

void sprite_draw_billboard(u32 arg0, s32 arg1, s32 arg2, s32 arg3)
{
    s32 z;
    u32 rec;
    u32 slot;

    switch (arg3 & 3) {
    case 0:
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
        load_matrix_default();
        mtx_apply_vec_at8((u8 *)(uintptr_t)arg0);
        if (clip_test() != 0) {
            MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
            return;
        }
        mtx_rotate_zyx_at24((u8 *)(uintptr_t)arg0);
        matrix_copy_rowswap();
        z = MEM32(MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        break;
    case 1:
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
        mtx_load_abc();
        mtx_apply_vec_at8((u8 *)(uintptr_t)arg0);
        if (clip_test() != 0) {
            MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
            return;
        }
        mtx_rotate_zyx_at24((u8 *)(uintptr_t)arg0);
        matrix_copy_rowswap();
        z = MEM32(MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        break;
    case 2:
        if (clip_test() != 0)
            return;
        matrix_copy_rowswap();
        z = MEM32(MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        break;
    default:
        return;  /* mode 3: stale depth register always fails the gate (no-op) */
    }

    rec = 0x060d28cc + (s16)MEM16(0x060cf0cc + MEM16(arg0 + 0x30) * 2) * 0x10;

    if (z >= 0x10000 && z < 0x1fbfff) {
        project_quad_corners((u16)SPRITE_CHARACTER_MEM16(rec, width), (u16)SPRITE_CHARACTER_MEM16(rec, height), arg1, arg2);

        slot = MEMU32(0x060cf0c8);
        VDP1_COMMAND_MEM32(slot, sort_key) = (arg3 & 4) ? z : z + 0x200000;
        VDP1_COMMAND_MEM16(slot, control) = (s16)(MEM16(arg0 + 0x22) | 2);
        VDP1_COMMAND_MEM16(slot, draw_mode) = (s16)(SPRITE_CHARACTER_MEM16(rec, draw_mode) | MEM16(arg0 + 0x20));
        VDP1_COMMAND_MEM16(slot, colour) = 0;
        VDP1_COMMAND_MEM16(slot, character_address) = SPRITE_CHARACTER_MEM16(rec, character_address);
        VDP1_COMMAND_MEM16(slot, character_size) = SPRITE_CHARACTER_MEM16(rec, character_size);
        VDP1_COMMAND_MEM32(slot, vertex_a) = MEM32(DH_TRANSFORMED_VECTOR_ADDR);
        VDP1_COMMAND_MEM32(slot, vertex_b) = MEM32(0x06098b20);
        VDP1_COMMAND_MEM32(slot, vertex_c) = MEM32(0x06098b24);
        VDP1_COMMAND_MEM32(slot, vertex_d) = MEM32(0x06098b28);
        MEM32(0x0609881c) = MEM32(0x2609881c) + 1;
        MEM32(0x060cf0c8) -= 0x24;
    }
}
