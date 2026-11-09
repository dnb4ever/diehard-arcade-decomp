/* scene81_stage_setup [0x02206e2e]: Loads requested stage layers, palettes and pattern-name data into VDP2. */
#include "dh.h"
/* 0x02206e2e (scene81_stage_setup) - load the character/stage graphics into VDP2 for the
 * GAME scene. Called from scene81_game_sm state 4 as scene81_stage_setup(task, scenedesc); it reads
 * its second argument (ROM 0600ae32: mov r5,r14), here `ctx`, which carries the stage config
 * that the earlier character/stage-intro scenes (75/76) populated:
 *     ctx->unk30 (s32 flags), ctx->unk34 (s32 flags for the 2nd BG layer),
 *     ctx->unk38 (tile base / layer select, ==0 => layer A else layer B),
 *     ctx->unk3A (s16 cell-column count), ctx->unk3C -> stage-asset bundle:
 *         bundle->unk0/unk4 (s32 cfg handed to func_02221bd4),
 *         bundle->unk8 -> {count @0, src @4} pattern-name table,
 *         bundle->unkC -> {count @0, src @4} palette/colour table,
 *         bundle->unk10 -> array of {.,.src@4} cell records (8 bytes each).
 *
 * Body: if ctx->unk30 bit0 clear, do nothing. Else, gated on a "already loaded" cache
 * (0x06098690 for layer A / 0x06098694 for layer B keyed on ctx->unk3C), DMA the pattern
 * table into VDP2 VRAM (0x25E00000 / 0x25E18000 via sh2_dmac_copy) and upload each cell
 * record's tiles to 0x25E40000+ (tilemap_copy_wrap_w80 when unk30 bit8 set, else straight
 * DMA). Then DMA the colour table to CRAM mirror 0x25F00400, clear 0x25E5C000, clear the
 * display DISP bit, and run the VDP2 plane/pattern builders (func_022220be register loads
 * from cfg blocks 0x060507xx/0x0605082x, func_02221f68/02222052 plane setup, func_02221bd4,
 * display_apply_cmdlist). unk30 bit2/3 select a packed 16-colour palette build (src table
 * 0x06050878) written through the CRAM cursor *0x06099510. A second, optional layer is set
 * up the same way when ctx->unk34 bit0 is set (bundle->unk4, VRAM 0x25E5F080/0x25E5F780).
 *
 * Branches resolved via braf_resolve (bsrf PIC calls) + ptr_resolve (sh2_dmac_copy @0x2145d0,
 * func_022220be @0x22220be). fidelity: HIGH - transcribed from m2c_fn output, cross-checked
 * vs --asm (extent 0x206e2e..0x2071b0). */

#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))
#define DISP     (*(void **)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR)
#define CRAM_CUR (*(s32 **)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR)   /* VDP2 CRAM write cursor */

extern void sh2_dmac_copy(void *src, u32 dst, u32 count);         /* 0x02145d0 */
extern void vdp2_apply_rotation_commands(void *cfg);                             /* 0x022220be (VDP2 reg loads) */
extern void tilemap_copy_wrap_w80(s32 a0, u32 dst, void *src,
                                  s32 a3, s32 a4, s32 mask);       /* 0x0221675e */
extern void memory_clear_vec4_blocks(u32 dst, s32 count);                 /* 0x02213290 */
extern void vdp2_apply_colour_commands(s32 arg);                               /* 0x022223c8 */
extern void vdp2_queue_attribute(s32 a0, s32 *cur, u32 dst, s32 count);  /* 0x0221632e */
extern void display_set_priority_origin(u32 dst);                               /* 0x022227aa */
extern void vdp2_setup_rotation_pair(s32 plane, u32 dst);                    /* 0x02221f68 */
extern void vdp2_setup_coefficient_pair(s32 plane, u32 dst, s32 count);         /* 0x02222052 */
extern void vdp2_pack_rotation_addresses(s32 layer, s32 cfg);                    /* 0x02221bd4 */
extern void view_initialize_transform(s32 layer, s32 a, s32 b);              /* 0x02222574 */
extern void display_apply_cmdlist(s32 list);                      /* 0x02222494 */

void scene81_stage_setup(void *scenedesc, void *ctx) {
    void *bundle;
    void *tbl;
    void *cur_rec;
    s32 i;

    (void)scenedesc;                  /* ROM reads configuration through r5. */

    if (!(M32(ctx, 0x30) & 1))
        return;

    bundle = MP(ctx, 0x3C);
    if (M16(ctx, 0x38) == 0) {
        if (bundle != PA(0x06098690)) {
            u32 count;
            PA(0x06098690) = bundle;
            tbl = MP(bundle, 0x8);
            if (M32(ctx, 0x30) & 0x20)
                count = (u32)M32(tbl, 0x0) * 8;
            else
                count = (u32)M32(tbl, 0x0) * 0x10;
            sh2_dmac_copy(MP(tbl, 0x4), 0x25E00000, count);
            for (i = 0; i < M16(ctx, 0x3A); i++) {
                cur_rec = (char *)MP(bundle, 0x10) + (i * 8);
                if (M32(ctx, 0x30) & 0x100)
                    tilemap_copy_wrap_w80(0, (u32)(((M16(ctx, 0x38) + i) << 13) + 0x25E40000),
                                          cur_rec, 0, 0, 0x200);
                else
                    sh2_dmac_copy(MP(cur_rec, 0x4), (u32)((i << 13) + 0x25E40000), 0x800U);
            }
        }
    } else if (bundle != PA(0x06098694)) {
        u32 count;
        PA(0x06098694) = bundle;
        tbl = MP(bundle, 0x8);
        if (M32(ctx, 0x30) & 0x20)
            count = (u32)M32(tbl, 0x0) * 8;
        else
            count = (u32)M32(tbl, 0x0) * 0x10;
        sh2_dmac_copy(MP(tbl, 0x4), 0x25E18000, count);
        for (i = 0; i < M16(ctx, 0x3A); i++) {
            cur_rec = (char *)MP(bundle, 0x10) + (i * 8);
            tilemap_copy_wrap_w80(0xC00, (u32)(((M16(ctx, 0x38) + i) << 13) + 0x25E40000),
                                  cur_rec, 0, 0, 0);
        }
    }

    tbl = MP(bundle, 0xC);
    sh2_dmac_copy(MP(tbl, 0x4), 0x25F00400, (u32)M32(tbl, 0x0) >> 1);
    memory_clear_vec4_blocks(0x25E5C000, 0x200);
    MU16(DISP, 0xE) = (u16)(MU16(DISP, 0xE) & 0x7FFF);

    vdp2_apply_rotation_commands((void *)0x06050770);
    vdp2_apply_colour_commands(0x0605082C);
    if (M32(ctx, 0x30) & 2)
        vdp2_apply_colour_commands(0x06050860);
    if (M32(ctx, 0x30) & 4) {
        vdp2_apply_rotation_commands((void *)0x060507E0);
        if (M32(ctx, 0x30) & 8) {
            s16 *src = (s16 *)(uintptr_t)0x06050878;
            vdp2_queue_attribute(0xE, CRAM_CUR, 0x25F005E0, 8);
            for (i = 0; i < 0x10; i += 2) {
                s32 *w = CRAM_CUR;
                *w = (s32)src[i] << 16;
                CRAM_CUR = w + 1;
                w[0] |= src[i + 1];
            }
            display_set_priority_origin(0x25F005E0);
            R32A(0x060BACB4) |= 1;
        } else {
            s16 *src = (s16 *)(uintptr_t)0x0605088A;
            s32 *w;
            vdp2_apply_colour_commands(0x0605086C);
            vdp2_queue_attribute(0xE, CRAM_CUR, 0x25F005FC, 1);
            w = CRAM_CUR;
            *w = (s32)src[0] << 16;
            CRAM_CUR = w + 1;
            w[0] |= src[0];
            display_set_priority_origin(0x25F005FC);
        }
    }
    if (M32(ctx, 0x30) & 0x20)
        vdp2_apply_rotation_commands((void *)0x060507FC);
    if (M32(ctx, 0x30) & 0x10)
        vdp2_apply_rotation_commands((void *)0x06050808);
    if (M16(ctx, 0x38) != 0)
        vdp2_apply_rotation_commands((void *)0x06050814);
    if (M32(ctx, 0x30) & 0x80)
        vdp2_apply_rotation_commands((void *)0x06050820);

    vdp2_setup_rotation_pair(4, 0x25E5F000);
    vdp2_setup_coefficient_pair(6, 0x25E5F200, 0xF0);
    vdp2_pack_rotation_addresses(0, M32(bundle, 0x0));
    view_initialize_transform(0, M32(ctx, 0x30) & (s32)(s16)0xF000, M32(ctx, 0x30) & 0x40);
    R32A(0x060BACAC) |= 1;

    if (M32(ctx, 0x34) & 1) {
        vdp2_apply_rotation_commands((void *)0x060507D4);
        display_apply_cmdlist(0x0605082C);
        if (M32(ctx, 0x34) & 2)
            display_apply_cmdlist(0x06050860);
        vdp2_setup_rotation_pair(5, 0x25E5F080);
        vdp2_setup_coefficient_pair(7, 0x25E5F780, 0xF0);
        vdp2_pack_rotation_addresses(1, M32(bundle, 0x4));
        view_initialize_transform(1, M32(ctx, 0x34) & (s32)(s16)0xF000, M32(ctx, 0x34) & 0x40);
        R32A(0x060BACB0) |= 1;
    }
}
