/* enemy_dash_from_third_list [0x02247580]: Selects a random entry from the third descriptor sublist and starts its dash. */
#include "dh.h"
/* 0x02247580  entry-variant wrapper: pick a move descriptor from the third
 * sub-list (offset 0x8) of *(entity+0x174) using rng_next()&1 and tail-call
 * func_022473ee(entity, chosen). See also 0x02247508 / 0x02247530. */

extern u32  rng_next(void);                            /* 0x210020 */
extern void enemy_begin_scripted_dash(void *entity, void *desc); /* 0x2473ee */

void enemy_dash_from_third_list(void *entity) {
    void *list = MP(MP(entity, 0x174), 0x8);
    enemy_begin_scripted_dash(entity, MP(list, (rng_next() & 1) * 4));
}
