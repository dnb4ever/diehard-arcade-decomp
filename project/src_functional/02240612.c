/* enemy_action_signal_and_expire [0x02240612]: Signals effect slot 060e0f2c[actor+94] then shared expire/lock-on check. */
#include "dh.h"
/* 0x02240612 - func_02240612: leaf handler (blob sibling of func_022402c0).
 * real fn [0x240612, 0x240622) - no stack frame; forwards its two args to the
 * tail. Reached only by call/branch (the preceding func_02240588 ends in rts).
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_022402c0(obj, out);                 // expire/lock-on check (tail, bra)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte g_060e0f2c
 * table (shll2); both args pass through unchanged to func_022402c0. */
extern void actor_check_expiry_or_lock(void *obj, s32 *out);   /* 0x022402c0 (tail) */
extern s32  g_060e0f2c[];                         /* s32 flag table, idx obj->0x94 */

void enemy_action_signal_and_expire(void *obj, s32 *out)
{
    g_060e0f2c[M32(obj, 0x94)] = 1;
    actor_check_expiry_or_lock(obj, out);
}
