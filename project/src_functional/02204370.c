#include "dh.h"
/* 0x02204370 (scene_step_A) - the attract/demo "press start to join" prompt stepper
 * (the per-frame demo stepper that g_0609867C points to). It renders the two-player
 * join prompt using the SAME join-UI machinery as scene_credit_join_sm (0x0220600e):
 * the RAM join-digit table g_06090304 (indexed by player-ready state, entry [4] if the
 * panel count byte is <=1 else [5]), the ROM player-number glyph table g_060506F0, and
 * the glyph thunk (*0x0609029C) positioned via the HUD cursor globals g_060997A4 (color)
 * / g_060997A8 (mode). Contains NO jump table and NO PIC calls - every call is an
 * indirect through a RAM fn-ptr slot; m2c decoded it cleanly.
 *
 * Gate: only renders when (g_ctrl_struct(0x06098794)->unk6 & 3) and !(g_060987E8 & 1).
 * Then reads the panel (*0x06000660)(0x060987D8) and draws P1 (cols 0xA/0x12) and P2
 * (cols 0x16/0x1E) slots; the name glyph is scratch-built from the ROM 3-byte record
 * 0x06050D4C (P1) / 0x06050D53 (P2) offset by the panel's unk1/unk2, else the default
 * 0x06050D50. It then arms a 0x3C-frame timer g_06098680; when it expires it draws the
 * final glyph 0x06050D58 at cols 0xA/0xB.
 *
 * fidelity: HIGH - direct transcription of m2c (clean decode, no M2C_ERROR). */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))
#define TABP(a)  (*(void **)(uintptr_t)(a))

#define G_COLOR  DH_UI_CURSOR_COLUMN_ADDR
#define G_MODE   DH_UI_CURSOR_ROW_ADDR
#define UI_GLYPH ((void (*)(const void *))PA(0x0609029C))
#define PANEL1   0x060987D8
#define PANEL2   0x060987DC

/* one join slot: ready-digit (g_06090304[idx]), player-number (g_060506F0[count]) and
 * name glyph (scratch from ROM `rec` offset by panel unk1/unk2, else default). The name
 * glyph reuses the number glyph's cursor (col_num / mode 0xB), auto-advanced by the thunk. */
static void draw_slot(void *panelbuf, s32 col_digit, s32 col_num, const void *rec) {
    u8 scratch[3];
    s32 idx = ((s32)MU8(panelbuf, 0) <= 1) ? 4 : 5;

    R32A(G_COLOR) = col_digit; R32A(G_MODE) = 0xB;
    UI_GLYPH(TABP(0x06090304 + idx * 4));

    R32A(G_COLOR) = col_num;   R32A(G_MODE) = 0xB;
    UI_GLYPH((const void *)(uintptr_t)(0x060506F0 + MU8(panelbuf, 0) * 4));

    if (M8(panelbuf, 1) != 0) {
        scratch[0] = MU8(rec, 0) + MU8(panelbuf, 1);
        scratch[1] = MU8(rec, 1) + MU8(panelbuf, 2);
        scratch[2] = MU8(rec, 2);
        UI_GLYPH(scratch);
    } else {
        UI_GLYPH((const void *)0x06050D50);
    }
}

void scene_step_A(void) {
    if ((RU16A(0x06098794 + 6) & 3) && !(RU8A(0x060987E8) & 1)) {
        ((void (*)(void *))PA(0x06000660))((void *)PANEL1);   /* read panel */
        draw_slot((void *)PANEL1, 0xA,  0x12, (const void *)0x06050D4C);
        draw_slot((void *)PANEL2, 0x16, 0x1E, (const void *)0x06050D53);
        R32A(0x06098680) = 0x3C;
    }

    if (R32A(0x06098680) != 0) {
        if (--R32A(0x06098680) == 0) {
            R32A(G_COLOR) = 0xA; R32A(G_MODE) = 0xB;
            UI_GLYPH((const void *)0x06050D58);
        }
    }
}
