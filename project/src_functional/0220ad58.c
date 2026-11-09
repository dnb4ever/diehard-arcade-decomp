/* debug_draw_camera_pose [0x0220ad58]: Prints the active view position and angle triple through the HUD formatter. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220ad58 (HUD debug text: print the current entity's world position + angles) -
 * entity = (*(void**)0x0609882c)->unk8. Sets the text cursor (g_060997A4 = column
 * `col`, g_060997A8 = row `row`+i) and calls the formatted-print thunk
 * *g_060902A0(fmt, coord, angle) for the X/Y/Z triple:
 *   line 0: header string (no value);
 *   line 1: entity->unk8  (X, 32-bit) + entity->unk24 (angle, u16);
 *   line 2: entity->unkC  (Y)         + entity->unk26;
 *   line 3: entity->unk10 (Z)         + entity->unk28.
 * fidelity: HIGH - coord fields 32-bit, angle fields u16 (extu.w); print thunk takes
 * stack args (modelled as varargs: fmt first). Verified vs asm. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, ...);
extern void *g_view_task;

void debug_draw_camera_pose(u16 col, u16 row) {
    void *ent = TASK_PTR(g_view_task, work);

    g_ui_cursor_column = col;
    g_ui_cursor_row = row;
    g_060902A0(0x0605145C);

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 1;
    g_060902A0(0x0605146C, M32(ent, 0x8), MU16(ent, 0x24));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 2;
    g_060902A0(0x06051478, M32(ent, 0xC), MU16(ent, 0x26));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 3;
    g_060902A0(0x06051484, M32(ent, 0x10), MU16(ent, 0x28));
}
