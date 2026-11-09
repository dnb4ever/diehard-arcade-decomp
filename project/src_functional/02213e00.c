/* 0x02213e00  copy_field_back: obj[+32] = *(obj - 0x30 + 0x20) = obj[-4].
 * Reached via the 0x2213d90 jump-table; on entry r8=obj. Modeled with obj (r8) as parameter.
 * fidelity: faithful - r2=obj-0x30, load @(32,r2), store @(32,obj) in rts delay. Leaf. */
#include "dh_band21.h"

void copy_field_back(s32 *obj) {
    obj[8] = obj[-4];   /* +32 = *(obj - 0x10) */
}
