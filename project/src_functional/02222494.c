/* vdp2_apply_packed_commands [0x02222494]: Zero-terminated command/value stream updates packed shadow words +3a/+b2/+b4. */
#include "dh.h"
/* 0x02222494  display_apply_cmdlist: walk a {command, value} list (32-bit each, terminated by a
 * zero command) and splice each value into a packed bitfield of the display object *0x06098F08.
 * Three 16-bit field words are touched: 0x3A, 0xB2 and 0xB4; each command clears one sub-field
 * (mask) and ORs in (value << shift). Unknown commands consume their value and are ignored.
 * fidelity: HIGH - every mask/shift verified against the ROM dispatch. */
extern void *g_vdp2_register_shadow;
void vdp2_apply_packed_commands(const s32 *cmds) {
    void *o = g_vdp2_register_shadow;
    s32 cmd;
    while ((cmd = *cmds++) != 0) {
        u16 f3A = MU16(o, 0x3A);
        u16 fB2 = MU16(o, 0xB2);
        u16 fB4 = MU16(o, 0xB4);
        s32 val = *cmds++;
        switch (cmd) {
        case 12: M16(o, 0x3A) = (s16)((f3A & 0xCFFF) | (val << 12)); break;
        case 13: M16(o, 0x3A) = (s16)((f3A & 0x3FFF) | (val << 14)); break;
        case 24: M16(o, 0xB2) = (s16)((fB2 & 0xFEFF) | (val << 8));  break;
        case 25: M16(o, 0xB2) = (s16)((fB2 & 0xFDFF) | (val << 9));  break;
        case 26: M16(o, 0xB2) = (s16)((fB2 & 0xFBFF) | (val << 10)); break;
        case 28: M16(o, 0xB4) = (s16)((fB4 & 0xEFFF) | (val << 12)); break;
        case 29: M16(o, 0xB4) = (s16)((fB4 & 0xF3FF) | (val << 10)); break;
        case 30: M16(o, 0xB4) = (s16)((fB4 & 0xFDFF) | (val << 9));  break;
        case 31: M16(o, 0xB4) = (s16)((fB4 & 0xFEFF) | (val << 8));  break;
        }
    }
}
