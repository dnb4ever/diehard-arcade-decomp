/* actor_reset_slot_registry [0x02227a84]: Cancels owned actor tasks, clears slot occupancy and resets the pair relation matrix. */
#include "dh.h"
/* 0x02227a84  reset the dynamic entity system rooted at 0x06099584. For each of the 8 slots:
 * clear the id (+0xC), drop any object pointer (+0x2C) that still belongs to the current context
 * (*0x06099580) - zeroing the object's +0 live flag first - reset that slot's row and column of the
 * 8x8 int32 relation matrix (+0x4C) to -1, clear the per-slot byte flag (base+i) and the per-slot
 * word 0x0609974C[i]. Then clear the live count (+8), the pair state words at 0x06099790, the word
 * at 0x0609977C, and the 24-byte block at 0x060996D4.
 * fidelity: HIGH (equivalence-oriented) - the matrix is filled by clean row/column loops whose final
 * state matches the ROM's interleaved clear; all other stores are exact. */
extern u8   g_actor_slot_registry[];    /* entity system root */
extern s32  g_06099580;      /* current context */
extern u16  g_0609974C[];
extern u16  g_06099790[];
extern s32  g_0609977C;
extern u8   g_effect_slot_occupied[];
void actor_reset_slot_registry(void) {
    s32 i;
    u8 *p;
    for (i = 0; i < 8; i++) {
        void *obj;
        s32 k;
        M32(g_actor_slot_registry, 0xC + i * 4) = 0;
        obj = MP(g_actor_slot_registry, 0x2C + i * 4);
        if (obj != (void *)0 && M32(obj, 0x14) == g_06099580) {
            if (M32(obj, 0) != 0) {
                M32(obj, 0) = 0;
            }
            MP(g_actor_slot_registry, 0x2C + i * 4) = (void *)0;
        }
        for (k = 0; k < 8; k++) {
            M32(g_actor_slot_registry, 0x4C + k * 0x20 + i * 4) = -1;
            M32(g_actor_slot_registry, 0x4C + i * 0x20 + k * 4) = -1;
        }
        M8(g_actor_slot_registry, i) = 0;
        g_0609974C[i] = 0;
    }
    M32(g_actor_slot_registry, 8) = 0;
    /* ROM writes +2 before +0. */
    g_06099790[1] = 0;
    g_06099790[0] = 0;
    g_0609977C = 0;
    p = g_effect_slot_occupied;
    do {
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p += 3;
    } while (p < g_effect_slot_occupied + 0x18);
}
