#include "dh.h"
/* 0x0222d0e8  draw_actor_hier_model
 *
 * Draw one articulated actor model as a hierarchy of 15 transformed
 * segments. Unrolled (no loop); every per-segment transform/submit is
 * emitted inline, with an irregular matrix push/pop pattern that walks the
 * segment tree.  arg0 = entity.
 *
 * Three per-actor source arrays are set up from the entity:
 *   vec_src = 0x0607976C + (s16)entity->unk5E * 180   (per-segment translation,
 *             consumed 12 bytes/segment by mtx_apply_vec_at0)
 *   rot_src = 0x06096328 + (s16)((s8)entity->unk34 * 108)  (per-segment Euler
 *             angles, consumed 6 bytes/segment by mtx_rotate_zyx)
 *   id_tab  = 0x06096248 + (s8)entity->unk34 * 32    (per-segment u16 model id)
 *
 * The matrix-stack top pointer lives at *0x06098878; +0x30 reserves/pushes a
 * matrix cell, -0x30 pops one.  mtx_stack_push (0x02214a2e) advances it by 0x30
 * and duplicates the current cell.  For each segment the model id is latched
 * into entity->unk30 and the segment is submitted via submit_model (0x02215b34,
 * sibling of submit_model_culled/0x02215a2c) with the actor texture param
 * entity->unkA4.
 *
 * Computed branches: 33 bsrf + 2 mova+add jsr, ALL resolved via braf_resolve.py
 * (and the two PIC jsr by hand: 0x0222d144+0xFFFE78EA=0x02214a2e,
 *  0x0222d184+0xFFFE89B0=0x02215b34) into the draw cluster:
 *   0x022149aa mtx_load_abc          0x02214a40 mtx_apply_vec_at8
 *   0x02214a06 mtx_rotate_zyx_at24   0x02213ca4 vec3_array_matmul
 *   0x022149e4 mtx_rotate_zyx        0x02214a50 mtx_apply_vec_at0
 *   0x02214a2e mtx_stack_push        0x02215b34 submit_model
 *
 * fidelity: HIGH - the full call sequence, the per-segment pointer strides
 * (12/6/2), the two "no-increment" segment groups, the batched +24/+12 pointer
 * bump, and the 1x / 3x / 9x matrix-stack pops are transcribed 1:1 from the SH-2
 * asm; delay-slot stores (pop-before-push) preserved in order.  m2c corroborates
 * the non-call structure (unk30 = *id_tab++, g_matrix_stack_cursor +-0x30 bookkeeping).
 */
extern s32  g_06096c58[];     /* per-actor matmul-input table (idx unk34) */
extern void mtx_load_abc(void);              /* mtx_load_abc (ignores arg) */
extern void mtx_apply_vec_at8(void *entity);      /* mtx_apply_vec_at8 */
extern void mtx_rotate_zyx_at24(void *entity);      /* mtx_rotate_zyx_at24 */
extern void matrix_translate_current(s32 x, s32 y, s32 z); /* vec3_array_matmul (in place) */
extern void mtx_stack_push_dup(void);              /* mtx_stack_push (*p+=0x30; dup) */
extern void mtx_rotate_zyx(void *angles);      /* mtx_rotate_zyx (u16[3]) */
extern void mtx_apply_vec_at0(void *vec);         /* mtx_apply_vec_at0 (s32[3]) */
extern void model_emit_or_queue(void *entity, s32 tex); /* submit_model */

#define MTX_TOP (*(volatile s32 *)(uintptr_t)DH_MATRIX_STACK_CURSOR_ADDR)

void draw_actor_hier_model(void *entity) {
    u8  *vec_src = (u8 *)(uintptr_t)(0x0607976Cu + (s16)M16(entity, 0x5E) * 180);
    u8  *rot_src = (u8 *)(uintptr_t)(0x06096328u
                        + (s16)((s8)M8(entity, ACTOR_SLOT_ID) * 108));
    u16 *id_tab  = (u16 *)(uintptr_t)(0x06096248u + (s8)M8(entity, ACTOR_SLOT_ID) * 32);

    /* --- base transform for the whole actor --- */
    MTX_TOP += 0x30;
    mtx_load_abc();                 /* load base matrix */
    mtx_apply_vec_at8(entity);           /* translate by entity vec @+8 */
    mtx_rotate_zyx_at24(entity);           /* rotate by entity angles @+0x24 */
    mtx_stack_push_dup();                 /* push */
    matrix_translate_current(0, g_06096c58[(s8)M8(entity, ACTOR_SLOT_ID)], 0);

    /* --- segment 0/1: root node + child --- */
    mtx_rotate_zyx(rot_src); rot_src += 6;
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));

    /* --- shared node carrying two sibling parts --- */
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src);          /* no stride (shared node) */
    mtx_rotate_zyx(rot_src);          /* no stride (shared node) */
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    vec_src += 24; rot_src += 12;     /* batched advance over the two parts */
    MTX_TOP -= 0x30; mtx_stack_push_dup();

    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));

    MTX_TOP -= 0x90; mtx_stack_push_dup();        /* pop 3, push (new limb chain) */
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));

    mtx_stack_push_dup();                 /* second limb chain has 6 segments in ROM (C had 5) */
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));

    MTX_TOP -= 0x90; mtx_stack_push_dup();        /* pop 3, push (final limb chain) */
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src); vec_src += 12;
    mtx_rotate_zyx(rot_src); rot_src += 6;
    M16(entity, 0x30) = (s16)*id_tab++; model_emit_or_queue(entity, M32(entity, 0xA4));

    /* --- last shared node: one part, no stride, terminal read --- */
    mtx_stack_push_dup();
    mtx_apply_vec_at0(vec_src);          /* no stride */
    mtx_rotate_zyx(rot_src);          /* no stride */
    M16(entity, 0x30) = (s16)*id_tab; model_emit_or_queue(entity, M32(entity, 0xA4));

    MTX_TOP -= 0x1B0;                 /* pop 9 (unwind whole hierarchy) */
}
