/* 0x0223579c: emit fifteen part transforms, draw hooks and matrix rewinds.
 * ROM keeps the final ID store; only nonzero rewind entries write the cursor.
 * Part/mesh/shift fields retain their proven byte offsets. See builders.asm.
 */
#include "dh_band21.h"

void mtx_load_abc(void);                    /* 0x022149aa */
void mtx_apply_vec_at8(u8 *obj);            /* 0x02214a40 */
void mtx_rotate_zyx_at24(u8 *obj);          /* 0x02214a06 */
void mtx_stack_push_dup(void);              /* 0x02214a2e */
void matrix_copy_reverse(void);             /* 0x02213474 */
void mtx_rotate_zyx(u16 *angles);           /* 0x022149e4  (jsr @r9)              */
void matrix_translate_current(s32 x, s32 y, s32 z);    /* 0x02213ca4  vec3_array_matmul (r8)  */
s32  shc_shift_left(s32 value, s32 count);   /* 0x0224a360  ashl_var: value<<count  */
void model_emit_mesh_part(u8 *obj);                  /* 0x02235dc0  sibling per-block hook   */

#define MATRIX_CURSOR DH_MATRIX_STACK_CURSOR_ADDR_U   /* matrix-stack write pointer */

void vdp_colour_build_c(u8 *obj) {
    s32  *vtab = PTR(s32, (u32)0x0607976C + (u32)(MEM16(obj + 0x5E) * 0xB4));
    u16  *atab = PTR(u16, (u32)0x06096328 + (u32)(s16)((s8)obj[0x34] * 108));
    u16  *itab = PTR(u16, (u32)0x06096248 + (u32)((s8)obj[0x34] * 32));
    s32   i;

    /* bytes to rewind the write pointer before each block's push (block 15
     * rewinds 0x1B0 and does not push - the function returns there). */
    static const s32 rewind[15] = {
        0, 0, 0x60, 0, 0, 0x90, 0, 0, 0, 0, 0, 0x90, 0, 0, 0x1B0
    };

    /* prologue: seed the matrix stack and emit the part's base transform */
    MEMU32(MATRIX_CURSOR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(obj);
    mtx_rotate_zyx_at24(obj);
    mtx_stack_push_dup();
    matrix_translate_current(0, *PTR(s32, (u32)0x0609638C + (u32)(s16)((s8)obj[0x34] * 108)), 0);
    mtx_rotate_zyx(atab);
    mtx_stack_push_dup();

    for (i = 0; i < 15; i++) {
        s32 shift_count = (s8)obj[0x4C];

        MEMU16(obj + 0x30) = *itab++;
        atab += 3;                              /* +0x06: next euler triple */
        matrix_translate_current(shc_shift_left(vtab[0], shift_count),
                      shc_shift_left(vtab[1], shift_count),
                      shc_shift_left(vtab[2], shift_count));
        mtx_rotate_zyx(atab);
        matrix_copy_reverse();
        model_emit_mesh_part(obj);
        if (rewind[i])
            MEMU32(MATRIX_CURSOR) -= (u32)rewind[i];
        if (i != 14) {
            mtx_stack_push_dup();
            vtab += 3;                          /* +0x0C: next vec3 row */
        }
    }
}
