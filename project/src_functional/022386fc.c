/* display_accumulate_channel [0x022386fc]: Adds a three-short display channel into the shared frame accumulator. */
#include "dh.h"
/* 0x022386fc copy_size_triple - accumulate a 3-short triple from arg0 (+0,+2,+4)
 * into the global scratch triple at 0x0609861e (size/offset accumulator).
 * fidelity: HIGH - three s16 read-add-store pairs; last store in the rts delay slot. */
#define G16(a) (*(s16 *)(uintptr_t)(a))

void display_accumulate_channel(void *arg0) {
    G16(0x0609861e + 0) = (s16)(G16(0x0609861e + 0) + M16(arg0, 0));
    G16(0x0609861e + 2) = (s16)(G16(0x0609861e + 2) + M16(arg0, 2));
    G16(0x0609861e + 4) = (s16)(G16(0x0609861e + 4) + M16(arg0, 4));
}
