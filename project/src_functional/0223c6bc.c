/* actor_draw_proximity_pose [0x0223c6bc]: Selects one of two model poses from player proximity and submits it under the cached matrix. */
#include "dh.h"
/* 0x0223c6bc actor_draw_proximity - pick a pose by player proximity, then draw.
 * arg0 = actor.
 *   saved = arg0->unk30;
 *   rec = &g_060E0F6C[arg0->unk34 * 4];                 (per-actor proximity record)
 *   if ((rec->unk20 < 0xA900 && (g_060995B4->unk8->unk84 & 0xD)) ||
 *       (rec->unk40 < 0xA900 && (g_060995B8->unk8->unk84 & 0xD)))
 *        arg0->unk30 = 0x3A;      else  arg0->unk30 = 0x39;
 *   g_06098878 += 0x30;                                 (push draw priority)
 *   mtx_load_abc(rec);
 *   func_02213ca4(arg0->unk8, 0, arg0->unk10);
 *   func_02215a2c(arg0, 0);
 *   g_06098878 -= 0x30;
 *   arg0->unk30 = saved;                                (restore pose)
 * fidelity: HIGH - bsrf resolved: 0x2149aa (rec), 0x213ca4 (entity,0,arg0->unk10 via
 * r4/r5/r6), 0x215a2c (actor,0). Proximity fields unk20/unk40 are u32 (cmp/hs);
 * tested flag is entity->unk84 & 0xD. g_060995B4/B8 global player-object pointers;
 * g_060E0F6C u8[] record table (stride 4). Matches m2c --asm. */
extern void  mtx_load_abc(void *p);
extern void  matrix_translate_current(void *entity, s32 a, s32 b);
extern void  model_submit_culled(void *actor, s32 arg);
extern s32   g_matrix_stack_cursor;
extern u8    g_060E0F6C[];
extern void *g_060995B4;
extern void *g_060995B8;

void actor_draw_proximity_pose(void *arg0) {
    s16   saved = M16(arg0, 0x30);
    void *rec = (void *)&g_060E0F6C[M8(arg0, 0x34) * 4];

    if ((MU32(rec, 0x20) < 0xA900U && (M32(MP(g_060995B4, 0x8), 0x84) & 0xD)) ||
        (MU32(rec, 0x40) < 0xA900U && (M32(MP(g_060995B8, 0x8), 0x84) & 0xD))) {
        M16(arg0, 0x30) = 0x3A;
    } else {
        M16(arg0, 0x30) = 0x39;
    }
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc(rec);
    matrix_translate_current(MP(arg0, 0x8), 0, M32(arg0, 0x10));
    model_submit_culled(arg0, 0);
    g_matrix_stack_cursor -= 0x30;
    M16(arg0, 0x30) = saved;
}
