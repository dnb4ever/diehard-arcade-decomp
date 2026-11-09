/* vdp2_apply_rotation_commands [0x022220be]: Interprets rotation-background command/value pairs into VDP2 shadow fields. */
#include "dh.h"
/* 0x022220be - VDP2 shadow-register command-list interpreter. Walks a {command, value}
 * list (two s32 words each, terminated by command 0) and splices each value into a packed
 * bitfield of the display/VDP2 shadow object *g_06098F08. Sibling of display_apply_cmdlist
 * (0x02222494), which handles a disjoint command range into fields 0x3A/0xB2/0xB4; this one
 * covers commands 1..51 across a wider set of fields. Before the loop it force-sets bit 0x10
 * of field 0x20. Each command clears one sub-field (AND mask) and ORs in (value << shift);
 * commands 40/41 set the sign bit (bit31) of 32-bit fields 0xD8/0xDC from value&1; command 47
 * writes the raw value into field 0x10C. Commands 12,13,18-26,28-32 (and anything >51) are
 * reserved no-ops that just consume their value.
 * fidelity: HIGH - all 51 jump-table handlers (table @0x222174, braf base 0x222170) hand-
 *   decoded and address-anchored vs m2c_fn --asm; masks and shift counts verified per case.
 *   16-bit fields use sign-extended loads then truncating halfword stores (modeled with u16). */
extern void *g_vdp2_register_shadow;

void vdp2_apply_rotation_commands(const s32 *cmds)
{
    void *o = g_vdp2_register_shadow;
    s32 cmd;

    M16(o, 0x20) = (s16)((MU16(o, 0x20) & ~0x10) | 0x10);

    while ((cmd = *cmds++) != 0) {
        s32 val = *cmds++;
        switch (cmd) {
        case 1:  M16(o, 0x20) = (s16)((MU16(o, 0x20) & 0xEFFF) | (val << 12)); break;

        case 2:  M16(o, 0x2A) = (s16)((MU16(o, 0x2A) & 0x8FFF) | (val << 12)); break;
        case 3:  M16(o, 0x2A) = (s16)((MU16(o, 0x2A) & 0xFBFF) | (val << 10)); break;
        case 4:  M16(o, 0x2A) = (s16)((MU16(o, 0x2A) & 0xFDFF) | (val << 9));  break;
        case 5:  M16(o, 0x2A) = (s16)((MU16(o, 0x2A) & 0xFEFF) | (val << 8));  break;

        case 6:  M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0x7FFF) | (val << 15)); break;
        case 7:  M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0xBFFF) | (val << 14)); break;
        case 8:  M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0xFDFF) | (val << 9));  break;
        case 9:  M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0xFEFF) | (val << 8));  break;
        case 10: M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0xFF1F) | (val << 5));  break;
        case 11: M16(o, 0x38) = (s16)((MU16(o, 0x38) & 0xFFE0) | val);         break;

        case 14: M16(o, 0x2E) = (s16)((MU16(o, 0x2E) & 0xFFDF) | (val << 5));  break;
        case 15: M16(o, 0x2E) = (s16)((MU16(o, 0x2E) & 0xFFEF) | (val << 4));  break;
        case 16: M16(o, 0x2E) = (s16)((MU16(o, 0x2E) & 0xFFF8) | val);         break;

        case 17: M16(o, 0x22) = (s16)((MU16(o, 0x22) & 0xFFEF) | (val << 4));  break;

        case 27: M16(o, 0xB0) = (s16)((MU16(o, 0xB0) & 0xFFFC) | val);         break;

        case 33: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFF7F) | (val << 7));  break;
        case 34: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFFD) | (val << 1));  break;
        case 35: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFF7) | (val << 3));  break;
        case 36: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFDF) | (val << 5));  break;
        case 37: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFFE) | val);         break;
        case 38: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFFB) | (val << 2));  break;
        case 39: M16(o, 0xD4) = (s16)((MU16(o, 0xD4) & 0xFFEF) | (val << 4));  break;

        case 40: M32(o, 0xD8) = (M32(o, 0xD8) & 0x7FFFFFFF) | ((u32)(val & 1) << 31); break;
        case 41: M32(o, 0xDC) = (M32(o, 0xDC) & 0x7FFFFFFF) | ((u32)(val & 1) << 31); break;

        case 42: M16(o, 0xE6) = (s16)((MU16(o, 0xE6) & 0xFFF8) | val);         break;
        case 43: M16(o, 0x24) = (s16)((MU16(o, 0x24) & 0xFFEF) | (val << 4));  break;
        case 44: M16(o, 0xEA) = (s16)((MU16(o, 0xEA) & 0xFCFF) | (val << 8));  break;
        case 45: M16(o, 0xE8) = (s16)((MU16(o, 0xE8) & 0xFFEF) | (val << 4));  break;
        case 46: M16(o, 0xEC) = (s16)((MU16(o, 0xEC) & 0xFFEF) | (val << 4));  break;
        case 47: M16(o, 0x10C) = (s16)val;                                     break;
        case 48: M16(o, 0xEE) = (s16)((MU16(o, 0xEE) & 0xFCFF) | (val << 8));  break;
        case 49: M16(o, 0x110) = (s16)((MU16(o, 0x110) & 0xFFEF) | (val << 4)); break;
        case 50: M16(o, 0x112) = (s16)((MU16(o, 0x112) & 0xFFEF) | (val << 4)); break;
        case 51: M16(o, 0xE2) = (s16)((MU16(o, 0xE2) & 0xFFEF) | (val << 4));  break;

        default: break;
        }
    }
}
