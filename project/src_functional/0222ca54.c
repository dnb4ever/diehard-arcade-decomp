#include "dh_work.h"
/* actor_load_part_ids [0x0222ca54]: Copies sixteen part IDs from the model/variant table into the slot's pose output. */
#include "dh.h"
/* 0x0222ca54  copy 16 shorts from palette/anim source
 * *(0x060909C0 + field0x5E*16 + field0x4F*4) into per-id block 0x06096248 + id*32;
 * store the last copied short into field 0x68.
 * fidelity: HIGH - exact. */
extern u8 g_060909C0[];
extern u8 g_06096248[];
void actor_load_part_ids(void *arg0) {
    s16 *src = (s16 *)MP(&g_060909C0[ACTOR_M16(arg0, model_index) * 16 + ACTOR_M8(arg0, model_variant) * 4], 0);
    s16 *dst = (s16 *)&g_06096248[ACTOR_M8(arg0, slot_id) << 5];
    s32 n = 16;
    do {
        n -= 2;
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2;
        dst += 2;
    } while (n != 0);
    M16(arg0, 0x68) = src[-1];
}
