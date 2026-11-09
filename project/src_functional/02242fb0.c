/* prop_release_to_panel_handler [0x02242fb0]: Reinstalls 02218bec on active grab object, resets task fields and clears global handle. */
#include "dh.h"
/* 0x02242fb0  install handler func_02218bec into the object held in g_060973f8
 * and init several fields, then clear g_060973f8.
 * o = g_060973f8; o->field[0]=&func_02218bec; o->field[0xf]=0; o->field[0x10]=0;
 * o->field[0x12]=1; o->field[0x13]=1; (*(o+8))->field[0x26]=0xC000; g_060973f8=0.
 * The handler address is formed PC-relative (mova + add) = SH2 0x02218bec.
 * fidelity: HIGH - handler stored into a 32-bit field; on the host the pointer
 * is truncated to 32 bits (functional model of "install handler"). */
extern void *g_060973f8;        /* pending-object pointer slot (HWRAM) */
extern void prop_tick_eased_panel(void);
void prop_release_to_panel_handler(void) {
    void *o = g_060973f8;
    M32(o, 0) = (s32)(s64)(void *)prop_tick_eased_panel;
    M8(o, 0x10) = 0;
    M8(o, 0x0F) = 0;
    M8(o, 0x12) = 1;
    M8(o, 0x13) = 1;
    void *sub = MP(o, 8);
    M16(sub, 0x26) = (s16)0xC000;
    g_060973f8 = 0;
}
