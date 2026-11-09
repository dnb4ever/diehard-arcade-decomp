/* enemy_dash_from_second_list [0x02247530]: Selects the second descriptor sublist and starts its scripted dash. */
#include "dh.h"
/* 0x02247530  entry-variant wrapper: pick a move descriptor from the second
 * sub-list (*(entity+0x174))->+4 and tail-call func_022473ee.
 * bounds: [0x02247530, 0x247558); pool-inclusive end 0x247580. */

extern u32  rng_next(void);                            /* 0x02210020 */
extern void enemy_begin_scripted_dash(void *entity, void *desc); /* 0x022473ee */

void enemy_dash_from_second_list(void *entity) {
    void *list = MP(MP(entity, 0x174), 0x4);
    enemy_begin_scripted_dash(entity, MP(list, (rng_next() & 1) * 4));
}
