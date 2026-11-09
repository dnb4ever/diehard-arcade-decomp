/* scene_load_tiled_layer_with_task [0x022078aa]: Palette/tiles from 226aeaf8, 8x8 tile grid and command block 06050a30. */
#include "dh.h"

/* 0x022078aa - scene/layer loader (spawn variant). r4 = task/parent.
 * Member of the scene_layer_loader family (cf. 0x0220825c, 0x02207772):
 *   - DMAC a palette blob (records at 0x226aeb00) into VDP2 CRAM 0x25f00200.
 *   - DMAC a VRAM blob (src 0x226aeaf8, count (*0x226aeaf4 << 5) >> 2) into
 *     VDP2 VRAM 0x25e38000.
 *   - fill an 8x8 (step-8) grid of the plane at VRAM 0x25e78000 via
 *     func_0221675e (character/tilemap writer), tile source 0x226aeb04.
 *   - init the command-list struct at 0x06050a30 (func_02220ade).
 *   - patch VDP2 shadow struct *0x06098f08: field 0xf8 &= 0xff, |= 0x400.
 *   - submit the plane (func_0222156c(1, vram,vram,vram,vram)).
 *   - spawn a child with template 0x022079d0 (func_0220ff54) and clear
 *     child->unk8->unk8.
 */

#define G32(a)  (*(s32  *)(uintptr_t)(a))
#define GU32(a) (*(u32  *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32   sh2_dma0_copy_words(u32 src, u32 dst, s32 count);               /* 0x2145d0 DMAC ch0 */
extern void  tilemap_copy_wrap_wide(s32 a, u32 b, u32 c, s32 d, s32 e, s32 f);  /* 0x21675e */
extern void  vdp2_apply_layer1_commands(u32 arg);                                   /* 0x220ade */
extern void  vdp2_pack_layer_addresses(s32 a, u32 b, u32 c, u32 d, u32 e);         /* 0x22156c */
extern void *task_spawn_current_work40(void *parent, u32 template);               /* 0x20ff54 spawn */

void scene_load_tiled_layer_with_task(void *task)
{
	u32 vram = 0x25e78000u;
	void *s;
	void *child;
	s32 d, e;

	sh2_dma0_copy_words(GU32(0x226aeb00), 0x25f00200u, 8);
	sh2_dma0_copy_words(GU32(0x226aeaf8), 0x25e38000u, (G32(0x226aeaf4) << 5) >> 2);

	for (e = 8; e < 64; e += 8)
		for (d = 0; d < 64; d += 8)
			tilemap_copy_wrap_wide(0x0c00, vram, 0x226aeb04u, d, e, 3);

	vdp2_apply_layer1_commands(0x06050a30u);

	s = GP(DH_VDP2_REGISTER_SHADOW_ADDR);
	MU16(s, 0xf8) = (u16)(MU16(s, 0xf8) & 0xff);  /* mask reuses offset+7 == 0xff */
	MU16(s, 0xf8) = (u16)(MU16(s, 0xf8) | 0x400);

	vdp2_pack_layer_addresses(1, vram, vram, vram, vram);

	child = task_spawn_current_work40(task, 0x0600b9d0u /* PC-relative pointer in HWRAM */);
	M32(MP(child, 8), 8) = 0;
}

/* fidelity: HIGH. All bsrf resolved (func_022145d0 x2, func_0221675e,
 * func_02220ade, func_0222156c, func_0220ff54). The inner character-writer loop
 * is a 2x-unrolled step-8 double loop in the asm; rewritten as the equivalent
 * simple nested for (bounds 8..56 / 0..56, both exact). DMA count = signed
 * (*0x226aeaf4 << 5) >> 2, matching the sibling loaders. */
