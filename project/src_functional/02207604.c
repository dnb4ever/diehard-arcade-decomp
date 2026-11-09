/* scene_load_descriptor_layer [0x02207604]: Layer descriptor enable+64, count+74 and source+76 control palette/tile uploads. */
#include "dh.h"

/* 0x02207604 - VDP2 layer/scene loader driven by a descriptor record `rec`
 * (passed in r5; r4/arg0 is unused). Runs only when (rec->unk64 & 1).
 *
 * Two sub-modes keyed on rec->unk74 (character/tile count):
 *   rec->unk74 != 0:
 *     - DMAC a CRAM blob to 0x25f00600 (src/count from rec->unk76->[12]).
 *     - DMAC a VRAM blob to 0x25e20000 (src rec->unk76->[8]->[4]; count
 *       (*src)<<3 or <<4 depending on rec->unk64 bit 8).
 *     - fill the VRAM plane at (rec->unk72<<13)+0x25e70000.
 *     - write rec->unk74 character entries into VRAM via func_0221675e.
 *   rec->unk74 == 0:
 *     - *(u16)0x25f00600 = rec->unk72; clear 4 cells at 0x25e20000; fill 0x200
 *       cells at 0x25e60000.
 * Then load up to four VDP2 register-config blocks (func_02220828) gated by
 * rec->unk64 bits (unconditional 0x06050988, then bit2/bit8/bit4), and submit
 * the plane via func_0222156c using the rec->unk76->[0] vector.
 */

extern s32   sh2_dma0_copy_words(u32 src, u32 dst, s32 count);               /* 0x2145d0 DMAC ch0 */
extern void  fill_vec4_array_raw_entry(u32 dst, s32 count, s32 val);               /* 0x2132ac fill_vec4_array */
extern void  memory_clear_vec4_blocks(u32 dst, s32 count);                        /* 0x213290 clear_vec4_array */
extern void  tilemap_copy_wrap_wide(s32 a, u32 b, u32 c, s32 d, s32 e, s32 f);  /* 0x21675e */
extern void  vdp2_apply_layer0_commands(u32 cfg);                                   /* 0x220828 VDP2 reg-config loader */
extern void  vdp2_pack_layer_addresses(s32 a, u32 b, u32 c, u32 d, u32 e);         /* 0x22156c */

void scene_load_descriptor_layer(s32 arg0, void *rec)
{
	(void)arg0;

	if ((M32(rec, 64) & 1) == 0)
		return;

	if (M16(rec, 74) != 0) {
		void *p2 = MP(MP(rec, 76), 12);
		void *p8;
		s32 cnt, i;

		sh2_dma0_copy_words((u32)M32(p2, 4), 0x25f00600u, (s32)((u32)M32(p2, 0) >> 1));

		p8 = MP(MP(rec, 76), 8);
		cnt = (M32(rec, 64) & 8) ? (M32(p8, 0) << 3) : (M32(p8, 0) << 4);
		sh2_dma0_copy_words((u32)M32(p8, 4), 0x25e20000u, cnt);

		fill_vec4_array_raw_entry((u32)((M16(rec, 72) << 13) + 0x25e70000),
		              ((s32)M16(rec, 74) << 13) / 16, 0);

		for (i = 0; i < M16(rec, 74); i++) {
			u32 vram = (u32)(((M16(rec, 72) + i) << 13) + 0x25e70000);
			u32 data = (u32)M32(MP(rec, 76), 16) + (u32)(i << 3);
			tilemap_copy_wrap_wide(0, vram, data, 0, 0, 0x203);
		}
	} else {
		MU16(0x25f00600, 0) = MU16(rec, 72);
		memory_clear_vec4_blocks(0x25e20000u, 4);
		fill_vec4_array_raw_entry(0x25e60000u, 0x200, 0);
	}

	vdp2_apply_layer0_commands(0x06050988u);
	if (M32(rec, 64) & 2)
		vdp2_apply_layer0_commands(0x06050964u);
	if (M32(rec, 64) & 8)
		vdp2_apply_layer0_commands(0x06050970u);
	if (M32(rec, 64) & 4)
		vdp2_apply_layer0_commands(0x0605097cu);

	{
		void *w = MP(MP(rec, 76), 0);
		vdp2_pack_layer_addresses(0, (u32)M32(w, 0), (u32)M32(w, 4),
		              (u32)M32(w, 8), (u32)M32(w, 12));
	}
}

/* fidelity: HIGH. All PIC calls resolved (DMAC x2, fill/clear_vec4_array,
 * func_0221675e, func_02220828 x1-4, func_0222156c). DMAC #1 count is a LOGICAL
 * >>1; the plane-fill count is signed (unk74<<13)/16 (exact). The lone
 * "UNRESOLVED bsrf" braf_resolve reported at 0x2076de is a false positive - it
 * is the embedded literal .short 0x0203 (loaded into r10), not an instruction.
 * arg0 (r4) is unused by the body. */
