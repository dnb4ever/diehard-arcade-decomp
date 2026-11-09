#include "dh_work.h"
#include "dh.h"
/* 0x022079d0 (wave_distort_update) - builds a per-scanline sin-wave line-scroll /
 * rotation-coefficient table for a full-screen distortion effect, driven off the aux
 * display object (*0x0609882C)->unk8. Runs every frame; entity->unk8 is a free-running
 * phase (+= 0x12345) and entity->unk32 a per-call phase counter.
 *
 * ABI(task): task = r4; entity = task->unk8 (r12); aux = (*0x0609882C)->unk8 (r13).
 * The VDP2 coefficient table is written through the cursor g_06099510 (each push stores
 * one longword and advances the cursor by 4), 2 longwords per scanline, 0xF0 scanlines.
 *
 *   entity->unk8  += 0x12345;
 *   entity->unkC   = (u16)aux->unk24 << 8;
 *   entity->unk10  = aux->unkC >> 4;
 *   entity->unk32 += 1;
 *   g_060B3D50 = 1;
 *   func_022213a8(entity->unk8, 0);              // set rotation/phase
 *   func_0222142c(0x10000 - entity->unk10);      // set scale
 *   ang = clamp(aux->unk24, -0x4000, 0x4000);
 *   s   = sin_lookup(ang);
 *   nzero = min(0xF0, 124 - (s16)((u32)(-(s<<8) - (aux->unkC<<2)) >> 16));  // blank lines
 *   func_0221632e(2, cursor, 0x25E3F800, 0x1E0); // open the coefficient DMA (2*0xF0 words)
 *   for (i = 0; i < nzero; i++)       { push 0; push 0; }                    // flat top band
 *   for (; i < 0xF0; i++) {
 *       push sin_lookup(((unk32<<1)+i)<<9) << 4;
 *       push i*0xC000 + (cos_lookup(((unk32+(i<<1))<<9)) << 4) + entity->unkC;
 *   }
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x2079d0..0x207b2a). bsrf/ptr
 * resolved: func_022213a8 (0x2213a8), func_0222142c (0x22142c), sin_lookup sin_lookup
 * (0x213190), cos_lookup (0x2131a8), func_0221632e (0x21632e). Note the two inner
 * sin calls store into the cursor slot (dst), not into a function argument. */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern void vdp2_set_layer_scroll(s32 a, s32 b);                 /* 0x2213a8 */
extern void vdp2_set_layer_zoom(s32 a);                        /* 0x22142c */
extern s32  sin_lookup(s32 angle);                    /* sin_lookup 0x213190 */
extern s32  cos_lookup(s32 angle);                    /* 0x2131a8 */
extern void vdp2_queue_attribute(s32 a, s32 cursor, s32 dst, s32 count); /* 0x21632e */

#define CURSOR_VAL (*(s32 *)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR)

static s32 *reserve_coef(void) {
    s32 *p = *(s32 **)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR;
    /* The ROM publishes the advanced cursor before filling the old slot. */
    *(s32 **)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR = p + 1;
    return p;
}

void wave_distort_update(void *task) {
    void *entity = TASK_PTR(task, work);
    void *aux    = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
    s16   ang    = M16(aux, 0x24);
    s32   s, nzero, tmp, r5v, i;

    M32(entity, 0x8)  += 0x12345;
    M32(entity, 0xC)   = (s32)MU16(aux, 0x24) << 8;
    M32(entity, 0x10)  = M32(aux, 0xC) >> 4;
    M16(entity, 0x32) += 1;
    RU8A(0x060B3D50) = 1;

    vdp2_set_layer_scroll(M32(entity, 0x8), 0);
    vdp2_set_layer_zoom(0x10000 - M32(entity, 0x10));

    if (ang < -0x4000)      ang = -0x4000;
    else if (ang > 0x4000)  ang = 0x4000;

    s   = sin_lookup(ang);
    tmp = -(s << 8) - (M32(aux, 0xC) << 2);
    r5v = 124 - (s16)((u32)tmp >> 16);
    nzero = (r5v < 0xF0) ? r5v : 0xF0;

    vdp2_queue_attribute(2, CURSOR_VAL, 0x25E3F800, 0x1E0);

    i = 0;
    while (i < nzero) {
        *reserve_coef() = 0;
        *reserve_coef() = 0;
        i++;
    }
    while (i < 0xF0) {
        s32 *dst = reserve_coef();
        *dst = sin_lookup(((M16(entity, 0x32) << 1) + i) << 9) << 4;
        dst = reserve_coef();
        *dst = i * 0xC000
             + (cos_lookup((M16(entity, 0x32) + (i << 1)) << 9) << 4)
             + M32(entity, 0xC);
        i++;
    }
}
