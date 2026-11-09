/* callback_publish_noop [0x02226134]: Publishes pointer at 060996d0 and installs adjacent no-op entry. */
#include "dh.h"
/* 0x02226134  publish arg0 to 0x060996D0 and set *arg0 = &func_02226144 (mova+8 -> 0x02226144).
 * fidelity: HIGH - exact; the mova-derived constant is the entry of func_02226144. */
extern void *g_060996D0;
extern void published_callback_noop(void);
void callback_publish_noop(void *arg0) {
    g_060996D0 = arg0;
    MP(arg0, 0x0) = (void *)published_callback_noop;
}
