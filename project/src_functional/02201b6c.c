/* game_install_director_task [0x02201b6c]: Allocates director work, publishes its task and installs the empty default callback. */
#include "dh.h"
/* 0x02201b6c - small init helper: pool-allocate/zero a 0x6c-byte block attached to the
 * handler object arg0 (alloc_array_zeroed(arg0, 1, 0x6c)), publish arg0 into the global
 * slot g_06098678, and install the shared no-op handler (noop_handler @0x02201b8e, formed
 * by mova .Lpool_201b84 + 0xA) as arg0's first (function-pointer) field.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x02214bb8 (alloc_array_zeroed,
 * r4=arg0,r5=1,r6=0x6c; return discarded); mova+0xA resolves to 0x02201b8e. Returns void
 * (r0 left holding the mova base, not a meaningful result). */
#define GP(a)  (*(void **)(uintptr_t)(a))

extern void *alloc_array_zeroed(void *pool, s32 count, s32 size); /* 0x02214bb8 */
extern void  noop_handler(void);                                  /* 0x02201b8e */

void game_install_director_task(void *arg0) {
    alloc_array_zeroed(arg0, 1, 0x6c);
    GP(0x06098678) = arg0;
    *(void **)arg0 = (void *)noop_handler;
}
