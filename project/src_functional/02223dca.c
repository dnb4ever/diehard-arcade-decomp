/* isr_timer0_entry [0x02223dca]: Advances the display page, updates colour control and services the sound command queue. */
#include "dh.h"
/* 0x02223dca  isr_timer0 (SCU Timer-0 vec 0x43): cycle a 4-entry display-page index, push the
 * selected value to SCU T0C (0x25FE0090), and - when the enable flag @0x060986B8 is set -
 * toggle VDP2 reg 0x25F800EC (bit3 on even page / clear on odd, always set bit1). Then tail-
 * branches (braf) to sound_queue_service @0x02215f36 to service the sound ring this tick.
 * fidelity: HIGH - widths from the loads (idx/T0C/table 32-bit, VDP2 reg 16-bit); braf target
 * resolved via tools/decomp/braf_resolve.py (delta 0xffff212e -> 0x02215f36). */
extern s32 g_060986B4;       /* display-page index (0..3) */
extern s32 g_060986B8;       /* VDP2-toggle enable flag */
extern s32 g_06075708[];     /* 4-entry display-page value table */
extern volatile s32 reg_scu_t0c;  /* SCU Timer-0 compare (T0C) */
extern volatile s16 reg_vdp2_ccctl;  /* VDP2 register */
extern void sound_queue_service(void);

void isr_timer0(void) {
    s32 idx = (g_060986B4 + 1) & 3;
    g_060986B4 = idx;
    reg_scu_t0c = g_06075708[idx];
    if (g_060986B8 != 0) {
        s16 v = reg_vdp2_ccctl;
        if ((idx & 1) != 0)
            v &= (s16)0xFFF7;
        else
            v |= 8;
        reg_vdp2_ccctl = v | 2;
    }
    sound_queue_service();  /* braf tail-branch */
}
