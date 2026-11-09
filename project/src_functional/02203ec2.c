/* hud_dispatch_status_buffer [0x02203ec2]: Sets the cursor and dispatches the shared encoded status text buffer. */
/* func_02203ec2 (cart 0x203ec2) - leaf (indirect tail-call).
 * Stash arg0/arg1 into globals, then tail-call the function pointer held at
 * 0x0609029C, passing the fixed buffer 0x0609D7B0 as its argument.
 * Confidence: HIGH. Call is through a global function pointer (not a fixed
 * cart target), so it stays an indirect call.
 */
#include "decomp.h"

void hud_dispatch_status_buffer(s32 arg0, s32 arg1) {
    *(s32 *)DH_UI_CURSOR_COLUMN_ADDR = arg0;
    *(s32 *)DH_UI_CURSOR_ROW_ADDR = arg1;
    ((void (*)(void *))*(void **)0x0609029C)((void *)0x0609D7B0);
}
