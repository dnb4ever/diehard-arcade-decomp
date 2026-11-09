/* debug_reset_model_preview [0x0223964e]: Resets the shared model-preview pose and default part ID. */
#include "dh.h"
/* 0x0223964e (leafC) - reset fields of global object 0x06098624.
 * fidelity: HIGH - 0x20/0x22/0x24/0x26/0x28/0x30/0x32 16-bit (mov.w); 0x8/0xC/0x10 32-bit (mov.l). */
extern s16 g_06098624;
void debug_reset_model_preview(void) {
    void *o = &g_06098624;
    M16(o, 0x30) = 66;
    M16(o, 0x32) = 0;
    M16(o, 0x20) = 0x100;
    M16(o, 0x22) = 0;
    M16(o, 0x24) = 0;
    M16(o, 0x26) = 0;
    M16(o, 0x28) = 0;
    M32(o, 0x8) = 0;
    M32(o, 0xC) = 0;
    M32(o, 0x10) = 0;
}
