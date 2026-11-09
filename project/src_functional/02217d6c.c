#include "dh_work.h"
/* scene_reload_graphics [0x02217d6c]: Sets scene display state and reloads changed texture, pattern and model resource lists. */
#include "dh_band21.h"
/* 0x02217d6c - room/stage init + resource swap. Brings up the VDP sprite-attribute
 * block spr = *0x06098f08 (writes control words at +0xf8/0xfa/0xfc, bitfields at
 * +0xe/+0xe0/+0, and longs at +0x10/0x14/0x18/0x1c), runs a chain of init helpers
 * (func_02216154(5), func_02222e44(7), the handler at *0x06090294, func_0223820c,
 * func_02214d26, func_02216b14(0x060542ec)), then, for the current room
 * (idx=*0x0609868f, table=0x06072660+idx*0x50, ctx=*0x0609866c), reloads any of the
 * three resource slots ctx->unk0/4/8 that differ from the room table via
 * func_022150d4 / func_02215228 / func_02223ac0. Finishes with func_02227a84 and a
 * tail-call to func_0223c95c.
 * fidelity: HIGH - all bsrf/braf targets resolved (the bytes after the 0x217ea6 tail
 *   braf are a data table, not code); bitfield masks and the three slot compares
 *   modelled. The *0x06090294 call is indirect; the no-arg tail helpers take their
 *   args from globals (nop delay slots). */

extern void video_reset_buffers(s32 a);
extern void hud_setup_large_font(s32 a);
extern void hud_upload_shared_assets(void);
extern void vdp1_trigger_draw(void);
extern void vdp2_apply_sprite_commands(u32 stream);
extern void vdp1_upload_texture_groups(s32 a);
extern void vdp2_upload_pattern_groups(u32 list);
extern void model_load_scene_records(s32 a);
extern void actor_reset_slot_registry(void);
extern void effect_pool_reset_scene(void);

void scene_reload_graphics(void)
{
    u32 spr;
    u32 ctx;
    u32 idx;

    video_reset_buffers(5);

    spr = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
    MEM16(spr + 0xf8) = 0x0603;
    MEM16(spr + 0xfa) = 0x0700;
    MEM16(spr + 0xfc) = 4;
    MEM16(spr + 0xe0) = (s16)(MEMU16(spr + 0xe0) & 0xffef);
    MEM16(spr + 0xe)  = (s16)((MEMU16(spr + 0xe) & 0xfcff) | 0x0300);
    MEM16(spr + 0xe)  = (s16)((MEMU16(spr + 0xe) & 0xff00) | 0x23);
    MEM32(spr + 0x10) = -1;
    MEM32(spr + 0x14) = 0x454fffff;
    MEM32(spr + 0x18) = -1;
    MEM32(spr + 0x1c) = 0x0137ffff;
    MEM32(0x060986b8) = 1;

    hud_setup_large_font(7);
    (*(void (**)(void))(uintptr_t)0x06090294)();
    hud_upload_shared_assets();
    vdp1_trigger_draw();

    MEM16(spr + 0) = (s16)((MEMU16(spr + 0) & 0xffcf) | 0x10);
    MEM16(spr + 0) = (s16)((MEMU16(spr + 0) & 0xfff8) | 0x3);

    vdp2_apply_sprite_commands(0x060542ec);

    MEM16(spr + 0xe0) = (s16)((MEMU16(spr + 0xe0) & 0xfff0) | 0xf);

    ctx = MEM32(DH_SCENE_DESCRIPTOR_ADDR);
    idx = MEM8(0x0609868f);
    if (SCENE_MEM32(ctx, texture_groups) != MEM32(0x06072660 + idx * 0x50 + 0)) {
        vdp1_upload_texture_groups(SCENE_MEM32(ctx, texture_groups));
    }
    ctx = MEM32(DH_SCENE_DESCRIPTOR_ADDR);
    idx = MEM8(0x0609868f);
    if (SCENE_MEM32(ctx, pattern_groups) != MEM32(0x06072660 + idx * 0x50 + 4)) {
        vdp2_upload_pattern_groups(SCENE_MEM32(ctx, pattern_groups));
    }
    ctx = MEM32(DH_SCENE_DESCRIPTOR_ADDR);
    idx = MEM8(0x0609868f);
    if (SCENE_MEM32(ctx, model_list) != MEM32(0x06072660 + idx * 0x50 + 8)) {
        model_load_scene_records(SCENE_MEM32(ctx, model_list));
    }

    actor_reset_slot_registry();
    effect_pool_reset_scene();   /* tail call */
}
