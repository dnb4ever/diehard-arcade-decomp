#include "dh_work.h"
#include "dh.h"
/* 0x02207542 (aux_obj_orbit_update) - a spawned task handler (installed by the scene
 * builder 0x02207360 via func_0220ff54) that positions an object on a sin-driven orbit
 * relative to the aux display object (*0x0609882C)->unk8.
 *
 * ABI(task): task = r4; entity = task->unk8 (r14). aux = (*0x0609882C)->unk8 (r13).
 * It clamps the aux angle aux->unk24 (s16) to [-0x4000, 0x4000], clears g_060B3D50,
 * then computes the object's screen position and tail-calls the transform/plot routine
 * func_022213a8(x, y):
 *   s = sin_lookup(angle);                              (sin_lookup)
 *   y = entity->unkC - (s << 8) - (aux->unkC << 2);
 *   x = entity->unk8 - ((u16)aux->unk26 << k);          k = 11 if entity->unk60 != 0 else 12
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x207542..0x2075d0; the only
 * difference between the two branches is the horizontal scale shift 11 vs 12). bsrf
 * resolved: sin_lookup sin_lookup (0x02213190); tail func_022213a8 (0x022213a8). */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern s32  sin_lookup(s32 angle);   /* sin_lookup */
extern void vdp2_set_layer_scroll(s32 x, s32 y);/* transform/plot (tail) */

void aux_obj_orbit_update(void *task) {
    void *entity = TASK_PTR(task, work);
    void *aux    = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
    s16   ang    = M16(aux, 0x24);
    s32   s, x, y;

    if (ang < -0x4000)
        ang = -0x4000;
    else if (ang > 0x4000)
        ang = 0x4000;

    RU8A(0x060B3D50) = 0;

    s = sin_lookup(ang);
    y = M32(entity, 0xC) - (s << 8) - (M32(aux, 0xC) << 2);
    x = M32(entity, 0x8) - ((s32)MU16(aux, 0x26) << (M32(entity, 0x60) != 0 ? 11 : 12));

    vdp2_set_layer_scroll(x, y);   /* SH-2 braf tail-call */
}
