/* background_tick_view_transform [0x02207884]: Clears 060b3d50 then submits view X+Z and inverted half Y; inherited arguments remain unmodelled. */
#include "dh.h"
/* 0x02207884 - small task handler that clears a one-shot flag and tail-calls the shared
 * draw/transform routine func_022213a8 with geometry derived from the current scene object.
 * e = (*g_0609882c)->unk8; clears g_060b3d50; then tail func_022213a8((e->unk8 + e->unk10) << 2,
 * 0x01000000 - (e->unkC >> 1)). (The larger loop body that fn.units grouped after this entry is
 * a separate function at 0x022078a8, reached only via its own prologue - not part of 0x02207884.)
 * fidelity: HIGH for this stub - hand-traced vs m2c_fn --asm. braf tail -> 0x022213a8; e->unkC
 * is arithmetic-shifted (>>1). func_022213a8's r6/r7 are inherited/unmodeled (undefined on this
 * handler-entry path), so only the two computed arguments are modeled. */

extern void vdp2_set_layer_scroll(s32 a, s32 b);  /* 0x022213a8 (tail) */

void background_tick_view_transform(void) {
    void *e = MP(MP((void *)(uintptr_t)DH_VIEW_TASK_ADDR, 0), 8);

    *(s8 *)(uintptr_t)0x060b3d50 = 0;
    vdp2_set_layer_scroll((M32(e, 0x08) + M32(e, 0x10)) << 2,
                 0x01000000 - (M32(e, 0x0c) >> 1));
}
