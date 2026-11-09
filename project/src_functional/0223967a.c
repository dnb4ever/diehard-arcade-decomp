/* draw_rot_sprite_source_alias [0x0223967a]: Existing map corroborated by source: draw a scaled sprite rotated by angle. func_02214a2e(obj) push/setup; func_02213ca4(obj->unk8,0,obj->unk10) position; func_02213ce4(|sin(angle)|+0x10000,0x10000,0x10000) scale (sin=func_02213190, called once to test sign + once to use, as compiled); func_02215a2c(&g_06098624,0) render; g_06098878-=0x30 pop. bsrf/jsr resolved. */
#include "dh.h"
/* 0x0223967a (callC) - draw a shadow/scaled sprite rotated by an angle.
 * mtx_stack_push_dup(obj) sets up / pushes the draw matrix; func_02213ca4 positions
 * at (obj->unk8, 0, obj->unk10); scale the matrix columns by
 * func_02213ce4(|sin(angle)| + 0x10000, 0x10000, 0x10000) where sin is the
 * fixed-point trig sin_lookup; render the sprite via func_02215a2c(&g_06098624,
 * 0); finally pop the matrix stack (g_06098878 -= 0x30).
 * fidelity: HIGH. bsrf/jsr targets resolved: mtx_stack_push_dup, func_02213ca4,
 * sin_lookup, func_02213ce4, func_02215a2c. The magnitude is formed by
 * calling sin_lookup(angle) once to test its sign then once more to use it,
 * exactly as compiled. */
extern u8   g_06098624;
extern s32  g_matrix_stack_cursor;
extern void mtx_stack_push_dup(void *obj);
extern void matrix_translate_current(s32 x, s32 y, s32 z);
extern s32  sin_lookup(s32 angle);
extern void matrix_scale_columns(s32 x, s32 y, s32 z);
extern void model_submit_culled(void *actor, s32 arg);

void draw_rot_sprite_source_alias(void *obj, s32 angle) {
    s32 mag;

    mtx_stack_push_dup(obj);
    matrix_translate_current(M32(obj, 0x8), 0, M32(obj, 0x10));
    if (sin_lookup(angle) >= 0)
        mag = sin_lookup(angle);
    else
        mag = -sin_lookup(angle);
    matrix_scale_columns(mag + 0x10000, 0x10000, 0x10000);
    model_submit_culled(&g_06098624, 0);
    g_matrix_stack_cursor -= 0x30;
}
