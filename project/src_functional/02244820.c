/* enemy_test_dodge_side [0x02244820]: Determines side permission from the relative XZ quadrant and stored offset. */
#include "dh.h"
/* 0x02244820  move/dodge permission flag keyed on the position-compare class.
 * cls = func_02243fba(arg0, arg1)  (2-bit position-compare mask, 0..3):
 *   cls 0,1 -> return (arg0->unk130 > 0) ? 1 : 0;
 *   cls 2   -> return 0;
 *   cls 3   -> return 1.
 * fidelity: HIGH - func_02243fba returns 0..3, so the asm's dead default (r4
 *   unset) is unreachable; modelled as 0. The cls 2/3 paths still load unk130
 *   (dead cmp/pl) which has no effect and is omitted.
 * NOTE: a prior-wave call site (02243db4.c) declares this with 4 args returning
 *   void*; the actual body uses only (arg0, arg1) and returns an s32 flag. */
s32 actor_compare_xz_quadrant(void *a, void *b);   /* 0x02243fba position-compare mask */

s32 enemy_test_dodge_side(void *arg0, void *arg1) {
    s32 cls = actor_compare_xz_quadrant(arg0, arg1);
    switch (cls) {
    case 0:
    case 1:
        return M32(arg0, 0x130) > 0 ? 1 : 0;
    case 2:
        return 0;
    case 3:
        return 1;
    }
    return 0; /* unreachable: cls is a 2-bit mask */
}
