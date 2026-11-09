/* decompressor_noop [0x022143b8]: Empty return leaf adjacent to decompression cluster. */
/* 0x022143b8  empty leaf (rts; nop). A do-nothing stub reached as a unit entry;
 * present so the pool after it is addressable. No observable effect.
 * fidelity: exact. */
#include "dh.h"

void decompressor_noop(void) {
}
