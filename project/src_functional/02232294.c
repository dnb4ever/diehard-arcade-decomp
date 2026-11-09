/* trail_spark_task [0x02232294]: Existing map corroborated by source: camera-locked trail: slot func_02228106, orient template g_06088088 via func_0221080a, world = cam g_06098B1C + oent */
#include "dh_work.h"
#include "dh.h"
/* 0x02232294 (task handler) - camera-locked spark/smoke trail. entity = task->unk8;
 * oent = task->unk14->unk8. First tick (task->unk10==0): reserve slot
 * func_02228106 -> unk34, unk60=8, unk62=0x1D, unk64=0x10000, copy oent pos into
 * unk8/C/10, ++task->unk10. Each tick: ++unk5A; clear unk68 every 16 frames;
 * unk26 = oent->unk26; snapshot pos into unk4C/50/54; orient the velocity template
 * g_06088088[task->unkF] (12B stride) by the entity angle at &entity->unk24 via
 * mtx_push_rotate_apply; set world pos = camera g_06098B1C[] + oent pos. Kill if
 * g_06097410 != 0; else while unk10 < 0x2A0000 tail drawlist_prepend.
 * fidelity: HIGH - unk34 byte, unk5A/unk60/unk62/unk26 word, others long. The
 * template index keeps the compiler's exts.b truncation ((s8)(unkF*12)).
 * bsrf 0x228106/0x21080a; braf 0x22eea2 -> func_0222eea2. */
extern s32  actor_allocate_effect_slot(void);
extern void mtx_push_rotate_apply(void *vel, void *ang);
extern void collision_queue_actor(void *entity);   /* drawlist_prepend */
extern u8   g_06088088[];
extern s32  g_transformed_vector[];
extern s32  g_06097410;

void trail_spark_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    if (TASK_S8(task, state) == 0) {
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        M16(ent, 0x60) = 8;
        M16(ent, 0x62) = 0x1D;
        M32(ent, 0x64) = 0x10000;
        M32(ent, 0x8)  = M32(oent, 0x8);
        M32(ent, 0xC)  = M32(oent, 0xC);
        M32(ent, 0x10) = M32(oent, 0x10);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    M16(ent, 0x5A) = (s16)(M16(ent, 0x5A) + 1);
    if (((s16)M16(ent, 0x5A) & 0xF) == 0)
        M32(ent, 0x68) = 0;
    M16(ent, 0x26) = M16(oent, 0x26);
    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    mtx_push_rotate_apply(&g_06088088[(s8)(M8(task, 0xF) * 12)], (void *)((u8 *)ent + 0x24));
    M32(ent, 0x8)  = g_transformed_vector[0] + M32(oent, 0x8);
    M32(ent, 0xC)  = g_transformed_vector[1] + M32(oent, 0xC);
    M32(ent, 0x10) = g_transformed_vector[2] + M32(oent, 0x10);
    if (g_06097410 != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    if (M32(ent, 0x10) < 0x2A0000)
        collision_queue_actor(ent);
}
