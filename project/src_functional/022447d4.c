/* enemy_classify_facing_agreement [0x022447d4]: Range class and requested side combine with facing; raw default residue preserved. */
#include "dh.h"
/* 0x022447d4  facing-agreement flag for a move/dodge decision.
 * cls = func_02243fba(arg0, arg1):
 *   cls 0 or 3 -> base = 1;
 *   cls 1 or 2 -> base = 0;
 *   other      -> base = (s32)arg0 (r4 never rewritten on the default path,
 *                 and the delay-slot mov r4,r0 returns it verbatim).
 * side = (arg2 > 0) ? 1 : 0; if arg0->unk44 (byte at +0x44) != side, base ^= 1.
 * fidelity: HIGH - the default path is reachable under stub returns and must
 * preserve the incoming arg0 value exactly, including the possible xor. */
s32 actor_compare_xz_quadrant(void *a, void *b);   /* 0x02243fba position-compare mask */

s32 enemy_classify_facing_agreement(void *arg0, void *arg1, s16 arg2) {
    s32 cls = actor_compare_xz_quadrant(arg0, arg1);
    s32 base;
    s32 side = (arg2 > 0) ? 1 : 0;

    if (cls == 0 || cls == 3)
        base = 1;
    else if (cls == 1 || cls == 2)
        base = 0;
    else
        base = (s32)arg0;

    if (M8(arg0, 0x44) != side)
        base ^= 1;
    return base;
}
