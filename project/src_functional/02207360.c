/* scene_bg_layer_build [0x02207360]: Builds a requested background layer and spawns its per-frame orbit updater. */
#include "dh.h"
/* 0x02207360 (scene_bg_layer_build) - a VDP2 BG-layer / nametable builder. Loads the
 * VDP2 register-config blocks selected by entity->unk40, DMAs the palette and pattern
 * data, clears + fills the nametable, and spawns the per-frame orbit updater
 * aux_obj_orbit_update (0x02207542).
 *
 * ABI (task, entity): task = r4 (saved on stack), entity = r5 (r14). m2c cannot decode
 * this because the per-cell fill is a `bra`-to-bottom do/while loop (branch target
 * .L2074bc sits inside the loop) - there is NO jump table here; rendered as a plain
 * for-loop. The cell/asset descriptor hangs off entity->unk4C.
 *
 *   if (!(entity->unk40 & 1)) return;                      // build requested
 *   func_02220828(0x06050910);                             // base VDP2 reg config
 *   if (unk40 & 2) func_02220828(0x06050964);
 *   if (unk40 & 8) func_02220828(0x06050970);
 *   if (unk40 & 4) func_02220828(0x0605097C);
 *   if (unk40 & 16){ g_060B3D50 = 0; func_0222142c(0x8000, 0x10000); }
 *   d = entity->unk4C;
 *   if (entity->unk4A != 0) {                               // cell count
 *     func_022145d0(d->unkC->unk4, 0x25F00600, (u32)d->unkC->unk0 >> 1);       // palette -> CRAM
 *     func_022145d0(d->unk8->unk4, 0x25E20000, d->unk8->unk0 << ((unk40&8)?3:4)); // pattern -> VRAM
 *     fill_vec4_array(0x25E70000 + (unk48<<13), (s32)(unk4A<<13)>>4 [rounded], 0); // clear nametable
 *     for (i = 0; i < unk4A; i++)
 *        tilemap_copy_wrap_w80(0, 0x25E70000 + ((unk48+i)<<13), d->unk10 + (i<<3), 0, 0, 0x200);
 *     // -> func_0222156c(0, d->unk0->unk0, d->unk0->unk4, d->unk0->unk8, d->unk0->unkC)
 *   } else {
 *     *(u16*)0x25F00600 = unk48; func_02213290(0x25E20000, 4);
 *     fill_vec4_array(0x25E70000, 0x200, 0);
 *     // -> func_0222156c(0, 0x25E70000, 0x25E70000, 0x25E70000, 0x25E70000)
 *   }
 *   s = func_0220ff54(task, aux_obj_orbit_update);          // spawn updater
 *   s->unk8->unk8  = entity->unk44 << 16;
 *   s->unk8->unkC  = entity->unk46 << 16;
 *   s->unk8->unk3C = entity->unk40 & 16;
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x207360..0x20752e). bsrf/ptr
 * resolved: func_02220828 (0x220828), func_0222142c (0x22142c), func_022145d0
 * (0x2145d0, a DMAC copy), fill_vec4_array (0x2132ac), tilemap_copy_wrap_w80 (0x21675e),
 * func_02213290 (0x213290), func_0222156c (0x22156c), func_0220ff54 (0x20ff54 spawn),
 * aux_obj_orbit_update (0x207542). SH-2 immediates were decimal. */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define W16(a,v) (*(volatile u16 *)(uintptr_t)(a) = (u16)(v))

extern void  vdp2_apply_layer0_commands(s32 cfg);                                   /* 0x220828 */
extern void  vdp2_set_layer_zoom(s32 a, s32 b);                              /* 0x22142c */
extern void  sh2_dma0_copy_words(s32 src, s32 dst, s32 count);              /* 0x2145d0 DMAC copy */
extern void  fill_vec4_array(s32 dst, s32 a, s32 b);                     /* 0x2132ac */
extern void  tilemap_copy_wrap_w80(s32 a, s32 dst, s32 cell,
                                   s32 d, s32 e, s32 f);              /* 0x21675e */
extern void  memory_clear_vec4_blocks(s32 dst, s32 count);                     /* 0x213290 */
extern void  vdp2_pack_layer_addresses(s32 a, s32 b, s32 c, s32 d, s32 e);         /* 0x22156c */
extern void *task_spawn_current_work40(void *task, void *handler);                /* 0x20ff54 spawn */

void scene_bg_layer_build(void *task, void *entity) {
    void *d;
    void *s;
    s32   a, b, c, e;   /* func_0222156c args */

    if (!(M32(entity, 0x40) & 1))
        return;

    vdp2_apply_layer0_commands(0x06050910);
    if (M32(entity, 0x40) & 2)  vdp2_apply_layer0_commands(0x06050964);
    if (M32(entity, 0x40) & 8)  vdp2_apply_layer0_commands(0x06050970);
    if (M32(entity, 0x40) & 4)  vdp2_apply_layer0_commands(0x0605097C);
    if (M32(entity, 0x40) & 16) {
        RU8A(0x060B3D50) = 0;
        vdp2_set_layer_zoom(0x8000, 0x10000);
    }

    d = MP(entity, 0x4C);

    if (M16(entity, 0x4A) != 0) {
        void *pal  = MP(d, 0xC);
        void *patt = MP(d, 0x8);
        void *hdr  = MP(d, 0x0);
        s32   nt    = (s32)M16(entity, 0x4A) << 13;
        s32   i;

        sh2_dma0_copy_words(M32(pal, 0x4), 0x25F00600, (s32)((u32)M32(pal, 0x0) >> 1));
        sh2_dma0_copy_words(M32(patt, 0x4), 0x25E20000,
                      M32(patt, 0x0) << ((M32(entity, 0x40) & 8) ? 3 : 4));

        if (nt < 0) nt += 15;
        nt >>= 4;
        fill_vec4_array(0x25E70000 + ((s32)M16(entity, 0x48) << 13), nt, 0);

        for (i = 0; i < (s32)M16(entity, 0x4A); i++)
            tilemap_copy_wrap_w80(0,
                                  0x25E70000 + (((s32)M16(entity, 0x48) + i) << 13),
                                  (s32)(uintptr_t)MP(d, 0x10) + (i << 3),
                                  0, 0, 0x200);

        a = M32(hdr, 0x0);
        b = M32(hdr, 0x4);
        c = M32(hdr, 0x8);
        e = M32(hdr, 0xC);
    } else {
        W16(0x25F00600, M16(entity, 0x48));
        memory_clear_vec4_blocks(0x25E20000, 4);
        fill_vec4_array(0x25E70000, 0x200, 0);
        a = b = c = e = 0x25E70000;
    }

    vdp2_pack_layer_addresses(0, a, b, c, e);

    s = task_spawn_current_work40(task, (void *)(uintptr_t)0x0600b542);
    M32(MP(s, 0x8), 0x8)  = (s32)M16(entity, 0x44) << 16;
    M32(MP(s, 0x8), 0xC)  = (s32)M16(entity, 0x46) << 16;
    M32(MP(s, 0x8), 0x3C) = M32(entity, 0x40) & 16;
}
