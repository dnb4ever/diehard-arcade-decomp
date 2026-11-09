/* hud_blink_join_notice [0x022044cc]: Draws the timed player join/configuration notice from resident input events. */
#include "dh.h"
/* 0x022044cc - timed warning/notice HUD blinker (leaf, no args). When an event bit is set
 * (g_0609879a & 3) and the machine is not in the config-bit0 mode (g_060987e8 & 1 == 0):
 *   - BIOS peripheral read (*g_06000660)(0x060987d8);
 *   - dispatch (via *g_0609029c, ctx g_060997a4=0x10, g_060997a8=0xB) the icon record
 *     *(g_06090304 + (player0_state>1?5:4)*4);
 *   - dispatch (ctx 0x18) the glyph-table entry at g_060506f0 + player0_state*4;
 *   - if g_060987d9 != 0, build a 3-byte position record from template 0x06050d71 nudged by
 *     g_060987d9 / g_060987da and dispatch it; else dispatch the default record 0x06050d50;
 *   - arm the countdown g_06098680 = 0x3c.
 * Every frame: if g_06098680 != 0, decrement it; when it reaches 0 dispatch the clear record
 * 0x06050d74 (ctx 0x10).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. player0_state = (u8)g_060987d8; dispatch takes
 * a pointer arg and the pointer is re-fetched each call; countdown stores the decrement then
 * tests for zero; record template bytes are signed. */
#define G8(a)   (*(s8  *)(uintptr_t)(a))
#define GU8(a)  (*(u8  *)(uintptr_t)(a))
#define GU16(a) (*(u16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

void hud_blink_join_notice(void) {
    void (**disp)(void *) = (void (**)(void *))(uintptr_t)0x0609029c;
    s8 rec[4];
    s32 icon;

    if ((GU16(0x0609879a) & 3) && !(GU8(0x060987e8) & 1)) {
        (*(void (**)(void *))(uintptr_t)0x06000660)((void *)(uintptr_t)0x060987d8);

        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0x10;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0x0B;
        icon = (GU8(0x060987d8) > 1) ? 5 : 4;
        (*disp)(*(void **)((uintptr_t)0x06090304 + icon * 4));

        G32(DH_UI_CURSOR_COLUMN_ADDR) = 0x18;
        G32(DH_UI_CURSOR_ROW_ADDR) = 0x0B;
        (*disp)((void *)((uintptr_t)0x060506f0 + GU8(0x060987d8) * 4));

        if (G8(0x060987d9) != 0) {
            rec[0] = G8(0x06050d71);
            rec[1] = G8(0x06050d72);
            rec[2] = G8(0x06050d73);
            rec[0] = (s8)(rec[0] + G8(0x060987d9));
            rec[1] = (s8)(rec[1] + G8(0x060987da));
            (*disp)(rec);
        } else {
            (*disp)((void *)(uintptr_t)0x06050d50);
        }
        G32(0x06098680) = 0x3c;
    }

    if (G32(0x06098680) != 0) {
        if (--G32(0x06098680) == 0) {
            G32(DH_UI_CURSOR_COLUMN_ADDR) = 0x10;
            G32(DH_UI_CURSOR_ROW_ADDR) = 0x0B;
            (*disp)((void *)(uintptr_t)0x06050d74);
        }
    }
}
