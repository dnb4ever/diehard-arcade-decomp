/* actor_dispatch_type_completion [0x022418ce]: Dispatches the nonzero type completion hook unless the action phase is six. */
#include "dh.h"
/* 0x022418ce (callC) - table dispatch. If obj->field108 != 6 and the selector byte
 * h = g_0608DA94[obj->field0xBA & 0x3F] is nonzero, call the handler g_0608DA88[h]
 * with (obj, idx, h*4). g_0608DA88 is a dword-stride function-pointer table so the
 * byte h already indexes it directly; h*4 is its byte offset.
 * fidelity: HIGH - field108 32-bit, fieldBA 16-bit masked 0x3F, selector byte s8. */
extern s8 g_0608DA94[];
extern void (*g_0608DA88[])(void *, s32, s32);
void actor_dispatch_type_completion(void *obj) {
    s32 idx;
    s8 h;
    if (M32(obj, 0x108) == 6)
        return;
    idx = M16(obj, 0xBA) & DH_ACTOR_TYPE_MASK;
    h = g_0608DA94[idx];
    if (h != 0)
        g_0608DA88[h](obj, idx, h * 4);
}
