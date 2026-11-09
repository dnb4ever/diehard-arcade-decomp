/* vdp2_apply_colour_commands [0x022223c8]: Applies command/value pairs to selected colour and character control bitfields. */
#include "dh.h"
/* 0x022223c8  apply an (opcode,value) list (terminated by opcode 0) to pack bitfields of the
 * global display object *0x06098F08: field 0x3A (ops 12/13), field 0xB4 (ops 28/29/30/31),
 * field 0xB2 (ops 24/25/26). Each list entry is two 32-bit words.
 * fidelity: HIGH - exact; fields re-read each iteration (writes accumulate), all 16-bit. */
extern void *g_vdp2_register_shadow;
void vdp2_apply_colour_commands(s32 *list) {
    void *o = g_vdp2_register_shadow;
    s32 op;
    while ((op = *list++) != 0) {
        s16 f3A = M16(o, 0x3A);
        s16 fB4 = M16(o, 0xB4);
        s16 fB2 = M16(o, 0xB2);
        s32 val = *list++;
        switch (op) {
        case 12: M16(o, 0x3A) = (s16)((f3A & ~0x300) | (val << 8));  break;
        case 13: M16(o, 0x3A) = (s16)((f3A & ~0xC00) | (val << 10)); break;
        case 31: M16(o, 0xB4) = (s16)((fB4 & ~0x1)   | val);         break;
        case 30: M16(o, 0xB4) = (s16)((fB4 & ~0x2)   | (val << 1));  break;
        case 29: M16(o, 0xB4) = (s16)((fB4 & ~0xC)   | (val << 2));  break;
        case 28: M16(o, 0xB4) = (s16)((fB4 & ~0x10)  | (val << 4));  break;
        case 24: M16(o, 0xB2) = (s16)((fB2 & ~0x1)   | val);         break;
        case 25: M16(o, 0xB2) = (s16)((fB2 & ~0x2)   | (val << 1));  break;
        case 26: M16(o, 0xB2) = (s16)((fB2 & ~0x4)   | (val << 2));  break;
        }
    }
}
