/* text_format_context_init [0x02223780]: Initializes formatter context words to 0,1,0. */
#include "dh.h"
/* 0x02223780  init 3-word struct: [0]=0, [1]=1, [2]=0
 * fidelity: HIGH - exact (mov.l #0/#1 to @r4,@(4,r4),@(8,r4)). */
void text_format_context_init(void *p) {
    M32(p, 0x0) = 0;
    M32(p, 0x4) = 1;
    M32(p, 0x8) = 0;
}
