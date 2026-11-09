/* vdp2_stage_loader [0x02234984]: Existing map corroborated by source: actor_task->unk10 state(0->1). func_02220828 cfg; (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02234984 - VDP2 stage/graphics loader (state machine on actor_task->unk10).
 *
 * Reads the current scene actor via g_curActorObj->unk8, caching its angle field
 * unk24 (s16). Dispatches on the state byte task->unk10:
 *   state 0: load the VDP2 register-config block @0x0608be9c, run two SH-2 DMAC
 *            copies from the out-of-cart stage descriptors (0x22695d64/6c/74) into
 *            VDP2 VRAM (0x25f00600 and 0x25e20000), copy a wrapped tilemap into the
 *            nametable (0x25e70000), commit the VDP2 shadow plane, then advance the
 *            state byte to 1 and fall through into state 1.
 *   state 1: clamp the cached angle to [-0x4000, 0x4000], clear 0x060b3d50, then
 *            tail-call the transform/plot helper func_022213a8 with the rotation
 *            (-(actor->unk26 << 11)) and position (-(sin(angle) << 8) - (actor->unkC
 *            << 2)).
 *   other:   no-op.
 *
 * fidelity: HIGH - all 7 computed branches resolved (braf_resolve); the mid-function
 * literal pool 0x2349b0-0x234a07 is data (not code). Multi-stack-arg calls modelled
 * with their exact push counts (func_0221675e 6 args, func_0222156c 5 args; the
 * three pushed slots are reclaimed by the shared add #12,r15). Tail call via braf. */
extern void *g_curActorObj;                                       /* 0x0609882c */
extern void  vdp2_apply_layer0_commands(void *cfg);                            /* VDP2 register-config block loader */
extern void  sh2_dma0_copy_words(void *src, void *dst, s32 count);      /* SH-2 on-chip DMAC ch0 copy */
extern void  tilemap_copy_wrap_wide(s32 a0, void *dst, void *desc,
                           s32 a3, s32 a4, s32 a5);               /* tilemap_copy_wrap_w80 */
extern void  vdp2_pack_layer_addresses(s32 sel, s32 a1, s32 a2, s32 a3, s32 a4); /* vdp2_shadow_pack4_sel */
extern s32   sin_lookup(s32 angle);                            /* sin_lookup */
extern void  vdp2_set_layer_scroll(s32 rot, s32 pos);                     /* transform/plot */

#define DESC(a) (*(volatile u32 *)(uintptr_t)(a))                 /* out-of-cart descriptor word */

void vdp2_stage_loader(void *task) {
    void *actor = MP(g_curActorObj, 8);
    s16  angle  = M16(actor, 0x24);

    switch (TASK_U8(task, state)) {
    case 0:
        vdp2_apply_layer0_commands((void *)0x0608be9c);
        sh2_dma0_copy_words((void *)(uintptr_t)DESC(0x22695d70), (void *)0x25f00600, DESC(0x22695d6c) >> 1);
        sh2_dma0_copy_words((void *)(uintptr_t)DESC(0x22695d68), (void *)0x25e20000, DESC(0x22695d64) << 4);
        tilemap_copy_wrap_wide(0, (void *)0x25e70000, (void *)0x22695d74, 0, 0, 0);
        vdp2_pack_layer_addresses(0, 0x25e70000, 0x25e70000, 0x25e70000, 0x25e70000);
        TASK_S8(task, state) = 1;
        /* fallthrough */
    case 1:
        if (angle < -0x4000)      angle = -0x4000;
        else if (angle > 0x4000)  angle =  0x4000;
        *(volatile s8 *)0x060b3d50 = 0;
        vdp2_set_layer_scroll(-((s32)MU16(actor, 0x26) << 11),
                      -(sin_lookup(angle) << 8) - (M32(actor, 0xC) << 2));
        break;
    default:
        break;
    }
}
