/* players_select_health_leader [0x0222a04e]: Compares two active fighters' signed +56 health words; tie returns -1. */
#include "dh.h"
/* 0x0222a04e - compare the signature stat (field 0x56, s16) of the two active fighters
 * referenced by the global pair at g_060995B0 (slot +4 and slot +8, each a holder whose
 * +8 points at the fighter struct). If the two stats are equal, return -1 (tie, no change).
 * Otherwise, when both stats are nonzero, the fighter with the smaller stat is knocked into
 * a pinned/stunned state: its field 0x84 (16.16) is set to 0x20000000 and its state byte
 * 0x43 is set to 43 (0x2B). Returns 0.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. cmp/ge a,b picks the >= side to spare; the
 *   losing (strictly smaller) side is the one written. s16 stat loaded sign-extended. */
extern u8 g_actor_slot_tasks[];

s32 players_select_health_leader(void)
{
    void *p5 = MP(MP(g_actor_slot_tasks, 4), 8);
    void *p4 = MP(MP(g_actor_slot_tasks, 8), 8);
    s16 a = M16(p5, 0x56);
    s16 b = M16(p4, 0x56);

    if (a == b)
        return -1;
    if (a != 0 && b != 0) {
        if (a < b) {
            M32(p5, 0x84) = 0x20000000;
            M8(p5, 0x43) = 43;
        } else {
            M32(p4, 0x84) = 0x20000000;
            M8(p4, 0x43) = 43;
        }
    }
    return 0;
}
