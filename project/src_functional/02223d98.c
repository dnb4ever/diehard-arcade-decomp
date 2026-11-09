/* isr_vblank_out_entry [0x02223d98]: Increments the VBlank frame count and commits the framebuffer mode once per frame. */
#include "dh.h"
/* 0x02223d98  isr_vblank_out (SCU VBlank-OUT vec 0x41): one-shot per frame; bumps frame counter
 * @0x0609869C, optionally commits VDP1 TVMR (0x25D00000) from 0x060E0E30.
 * fidelity: HIGH - exact; widths taken directly from the global derefs. */
extern s8  g_060986AC;
extern s8  g_060986AD;
extern s8  g_060986AE;
extern s8  g_060986AF;
extern s32 g_0609869C;
extern s16 g_060E0E30;
extern volatile s16 reg_vdp1_tvmr;
void isr_vblank_out(void) {
    if (g_060986AD == 0) {
        g_060986AD = 1;
        g_060986AC = 0;
        g_0609869C += 1;
        if (g_060986AE == 1) {
            reg_vdp1_tvmr = g_060E0E30;
        }
        g_060986AF = 0;
    }
}
