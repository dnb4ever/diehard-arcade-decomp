/* results_edit_options [0x02209a8c]: State-4 config cursor/read/write and six-row option labels. */
#include "dh_work.h"
#include "dh.h"

/* 0x02209a8c - func_02209a8c: results task state 4 (option/config editor).
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern s32   g_060997AC;                                 /* cursor font    */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern s32   g_06098808;      /* config read cursor */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* ========================= state 4 (0x209a8c) ========================= */
/* Option/config editor: 6-row label block, then (on action) advance one of the
 * packed config nibbles/fields in g_060987EC[g_06098808..+1]; then redraw the
 * current selection cursor and the five option value glyphs. */
void results_edit_options(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;
    uintptr_t tbl;
    u16   cs;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 6, tbl = 0x06090540; i < 18; i += 6, tbl += 12) {
        g_ui_cursor_column = 6; g_ui_cursor_row = i;     g_0609029C(RMP(tbl + 0));
        g_ui_cursor_column = 6; g_ui_cursor_row = i + 2; g_0609029C(RMP(tbl + 4));
        g_ui_cursor_column = 6; g_ui_cursor_row = i + 4; g_0609029C(RMP(tbl + 8));
    }

    cs = RU16(0x06098794 + 6);
    if (cs & 0x0204) {
        void *cfg  = (void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808);
        s32   sel  = M32(ent, 0x48);
        switch (sel) {
        case 5:                                   /* confirm -> leave */
            TASK_S8(task, state) = 1;
            M32(ent, 0x40) = 4;
            M32(ent, 0x44) = 1;
            break;
        case 0: {                                 /* field 0: bits 0..2 */
            s32 v = MU8(cfg, 0) & 7;
            v = (v < 7) ? v + 1 : 0;
            M8(cfg, 0) = (s8)(MU8(cfg, 0) & 0xF8);
            M8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) =
                (s8)(MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) | (v & 7));
            break;
        }
        case 1: {                                 /* field 1: bits 3..5 */
            s32 v = (MU8(cfg, 0) >> 3) & 7;
            v = (v < 7) ? v + 1 : 0;
            M8(cfg, 0) = (s8)(MU8(cfg, 0) & 0xC7);
            M8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) =
                (s8)(MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) | ((v & 7) << 3));
            break;
        }
        case 2: {                                 /* field 2: bits 6..7 */
            s32 v = (MU8(cfg, 0) >> 6) & 3;
            v = (v < 3) ? v + 1 : 0;
            M8(cfg, 0) = (s8)(MU8(cfg, 0) & 0x3F);
            M8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) =
                (s8)(MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) | ((v & 3) << 6));
            break;
        }
        case 3: {                                 /* field 3: byte+1 bit 0 */
            void *c2 = (void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1);
            s32 v = MU8(c2, 0) & 1;
            v = (v < 1) ? v + 1 : 0;
            M8(c2, 0) = (s8)(MU8(c2, 0) & 0xFE);
            M8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) =
                (s8)(MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) | (v & 1));
            break;
        }
        case 4: {                                 /* field 4: byte+1 bit 1 */
            void *c2 = (void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1);
            s32 v = (MU8(c2, 0) >> 1) & 1;
            v = (v < 1) ? v + 1 : 0;
            M8(c2, 0) = (s8)(MU8(c2, 0) & 0xFD);
            M8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) =
                (s8)(MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) | ((v & 1) << 1));
            break;
        }
        default:
            break;
        }
    } else if (cs & 0x0408) {
        s32 v = M32(ent, 0x48);
        M32(ent, 0x48) = (v < 5) ? v + 1 : 0;
    }

    g_ui_cursor_column = 6;
    g_ui_cursor_row = (M32(ent, 0x48) * 2) + 6;       /* selection cursor */
    g_0609029C((void *)(uintptr_t)0x0605142C);

    /* value row: each glyph re-reads g_06098808 and the config byte after the
     * cursor stores, as the ROM does */
    g_060997AC = 6;
    g_ui_cursor_column = 0x1E; g_ui_cursor_row = 6;
    g_0609029C(RMP(0x06090558 + ((MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) & 7) * 4)));
    g_ui_cursor_column = 0x1E; g_ui_cursor_row = 8;
    g_0609029C(RMP(0x06090578 + (((MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) >> 3) & 7) * 4)));
    g_ui_cursor_column = 0x1E; g_ui_cursor_row = 0xA;
    g_0609029C(RMP(0x06090598 + (((MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808), 0) >> 6) & 3) * 4)));
    g_ui_cursor_column = 0x1E; g_ui_cursor_row = 0xC;
    g_0609029C(RMP(0x060905A8 + ((MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) & 1) * 4)));
    g_ui_cursor_column = 0x1E; g_ui_cursor_row = 0xE;
    g_0609029C(RMP(0x060905B0 + (((MU8((void *)(uintptr_t)(DH_BIOS_STATUS_BUFFER_ADDR + g_06098808 + 1), 0) >> 1) & 1) * 4)));
    g_060997AC = 1;
}
/* fidelity: HIGH - manually verified instruction-by-instruction vs the asm
 * (0x209a8c..0x209d54): header loop, the 0x0204/0x0408 event split (cs read
 * once), all five packed-config field editors (masks 0xF8/0xC7/0x3F/0xFE/0xFD,
 * shifts, wrap limits), case 5 -> state 1, the selection cursor and the value
 * row.  Each field edit is two separate byte stores; the second (OR) store
 * re-reads g_06098808 for its address, as the ROM does.  difftest SKIPs it
 * (indirect thunk calls / raw RAM macros). */
