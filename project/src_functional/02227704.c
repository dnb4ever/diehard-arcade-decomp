#include "dh_work.h"
/* actor_resolve_facing [0x02227704]: Transforms actor slot pose and chooses yaw 0x4000/0xc000 and facing byte from the resulting sign. */
#include "dh.h"
/* 0x02227704 (actor facing resolve) - shared callee. Pushes a matrix frame (g_06098878
 * stack ptr += 0x30), runs the per-actor transform at g_06096328[unk34] (0x6C-byte stride):
 * load_matrix_default(), mtx_rotate_zyx(rec), mtx_rotate_zyx(rec+6), func_02213538(0,0,0x10000);
 * reads the transformed result g_06098B24, pops the frame (g_06098878 -= 0x30), then sets
 * the facing: if currently facing right (unk26==0x4000) keep facing right when result>=0 else
 * flip to left (unk44=0, unk26=0xC000); if not currently 0x4000, flip to left when result>0
 * else face right (unk44=1, unk26=0x4000).
 * fidelity: HIGH - unk34 signed byte, product sign-truncated to word ((s16)(unk34*0x6C));
 * unk26 word (unsigned eq-test), unk44 byte; result sign tests are signed (cmp/pz, cmp/pl);
 * computed calls resolved via braf_rewrite.py (bsrf -> load_matrix_default/022149e4(x2)/02213538). */
extern void load_matrix_default(void);
extern void mtx_rotate_zyx(void *rec);
extern void matrix_transform_to_scratch(s32 a, s32 b, s32 c);
extern s32  g_06096328[];   /* per-actor transform records, 0x6C-byte stride (base) */
extern s32  g_matrix_stack_cursor;     /* matrix/transform stack pointer */
extern s32  g_06098B24;     /* transformed X result */

void actor_resolve_facing(void *entity) {
    void *rec = (char *)g_06096328 + (s16)(ACTOR_WORK(entity)->slot_id * 0x6C);
    s32 result;

    g_matrix_stack_cursor += 0x30;
    load_matrix_default();
    mtx_rotate_zyx(rec);
    mtx_rotate_zyx((char *)rec + 6);
    matrix_transform_to_scratch(0, 0, 0x10000);
    result = g_06098B24;
    g_matrix_stack_cursor -= 0x30;

    if (ACTOR_U16(entity, angle_y) == 0x4000) {
        if (result >= 0) {
            ACTOR_WORK(entity)->facing_right   = 1;
            ACTOR_U16(entity, angle_y) = 0x4000;
            return;
        }
    } else if (result <= 0) {
        ACTOR_WORK(entity)->facing_right   = 1;
        ACTOR_U16(entity, angle_y) = 0x4000;
        return;
    }
    ACTOR_WORK(entity)->facing_right   = 0;
    ACTOR_U16(entity, angle_y) = 0xC000;
}
