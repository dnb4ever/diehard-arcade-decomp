/* vdp2_init_menu_graphics [0x022348cc]: Shadow mode setup, three asset DMAs and plane-pointer packing. */
#include "dh_work.h"
#include "dh.h"
/* 0x022348cc - VDP2 VRAM/CRAM graphics init (task leaf). arg0 = task.
 * Clears the top bit of (*0x06098f08)->unk14, runs two struct setups, two VDP2
 * plane-mode setups (mode 4/5 and 6/7), DMAs three ROM blocks into VDP2 CRAM/VRAM
 * via sh2_dmac_copy (func_022145d0), packs 16 identical bank pointers through the
 * VDP2 shadow packer (func_0222184c), runs a final setup, sets bit0 of g_060bacac,
 * and clears the task handler (task->unk0 = 0).
 *   p = *(void**)0x06098f08; p->unk14 (s16) &= 0x7fff;
 *   func_022220be(0x0608be1c); func_022223c8(0x0608be70);
 *   func_02221f68(4, 0x25e5f000); func_02222052(6, 0x25e5f200, 0xf0);
 *   dmac(*(0x226fd568), 0x25f00400, 0x80);
 *   dmac(*(0x226fd550), 0x25e00000, (*(0x226fd54c)<<6)/4);
 *   dmac(*(0x226fd578), 0x25e40000, 0x800);
 *   func_0222184c(0, 0x25e40000 x16); func_02222574(0,0,0);
 *   *(0x060bacac) |= 1; task->unk0 = 0;
 * fidelity: MED/HIGH - logic is HIGH; the three ROM source blocks are read through
 * the 0x22000000-mirror pool pointers (cart 0x02xxxxxx + bit29) as in the asm; the
 * count math (<<6 then signed /4) exactly matches the cmp/pz;add #3;shar;shar idiom
 * (low 6 bits are zero so the rounding term is inert, = *16). func_0222184c takes
 * which + v0..v15 (v0..v2 in r5-r7, v3..v15 on stack), here all = 0x25e40000. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define G32V(a) (*(volatile s32 *)(uintptr_t)(a))

extern void vdp2_apply_rotation_commands(void *rec);
extern void vdp2_apply_colour_commands(void *rec);
extern void vdp2_setup_rotation_pair(s32 mode, void *vram);          /* VDP2 mode 4/5 setup */
extern void vdp2_setup_coefficient_pair(s32 mode, void *vram, s32 arg); /* VDP2 mode 6/7 setup */
extern void sh2_dma0_copy_words(void *src, void *dst, s32 count); /* sh2_dmac_copy */
extern void vdp2_shadow_pack6_args(s32 which,
                         s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5, s32 v6,
                         s32 v7, s32 v8, s32 v9, s32 v10, s32 v11, s32 v12,
                         s32 v13, s32 v14, s32 v15);       /* vdp2_shadow_pack6_args */
extern void view_initialize_transform(s32 a, s32 b, s32 c);

void vdp2_init_menu_graphics(void *task) {
    void *p = MP((uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR, 0);
    s32 count1;

    M16(p, 14) &= 0x7fff;

    vdp2_apply_rotation_commands((void *)(uintptr_t)0x0608be1c);
    vdp2_apply_colour_commands((void *)(uintptr_t)0x0608be70);

    vdp2_setup_rotation_pair(4, (void *)(uintptr_t)0x25e5f000);
    vdp2_setup_coefficient_pair(6, (void *)(uintptr_t)0x25e5f200, 0xf0);

    sh2_dma0_copy_words((void *)(uintptr_t)G32(0x226fd568),
                  (void *)(uintptr_t)0x25f00400, 0x80);

    count1 = (G32(0x226fd54c) << 6) / 4;
    sh2_dma0_copy_words((void *)(uintptr_t)G32(0x226fd550),
                  (void *)(uintptr_t)0x25e00000, count1);

    sh2_dma0_copy_words((void *)(uintptr_t)G32(0x226fd578),
                  (void *)(uintptr_t)0x25e40000, 0x800);

    vdp2_shadow_pack6_args(0,
                 0x25e40000, 0x25e40000, 0x25e40000, 0x25e40000,
                 0x25e40000, 0x25e40000, 0x25e40000, 0x25e40000,
                 0x25e40000, 0x25e40000, 0x25e40000, 0x25e40000,
                 0x25e40000, 0x25e40000, 0x25e40000, 0x25e40000);

    view_initialize_transform(0, 0, 0);

    G32V(0x060bacac) |= 1;
    TASK_S32(task, handler) = 0;
}
