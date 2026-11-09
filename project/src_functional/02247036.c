/* enemy_dispatch_script [0x02247036]: Invokes the actor's script index through the global state-handler table. */
#include "dh.h"
/* 0x02247036 - thin dispatcher (func_02247036): invokes the per-object state
 * handler selected by obj->0x100 from the function-pointer table at 0x06090230.
 *   (*(void (**)(void *))(0x06090230 + obj->0x100 * 4))(obj);
 * The one-slot stack juggle in the prologue is a scratch spill of the loaded
 * pointer and has no observable effect.
 * fidelity: HIGH - exact; handler return value is discarded (handlers are void,
 * consistent with the identical computed call in func_02246ab8). */
void enemy_dispatch_script(void *obj)
{
    (*(void (**)(void *))(uintptr_t)(0x06090230 + M32(obj, 0x100) * 4))(obj);
}
