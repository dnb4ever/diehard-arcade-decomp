#include "dh_work.h"
/* scene_reset_stage [0x02217f04]: Switches sound resources, resets stage grids and initializes scene background tasks. */
#include "dh_band21.h"
/* 0x02217f04 - stage/scene reset sequence. ctx = *0x0609866c. If ctx->unkC (r13)
 * is non-zero and differs from the stage record for the current room
 * (*0x0607266c[(s8)*0x0609868f * 0x50]), tear it down via func_02215e20(r13,0x6c000).
 * Then raises busy flags (0x060bacac/b0/b4 |= 1), runs func_0222eb72, sets the VDP
 * attribute sign bit at *0x06098f08, marks 0x06099578=1, kicks the loader
 * func_02216c38(ctx->unk20, ctx->unk24), clears a block of scene globals, and runs
 * the init chain func_02206e2e / func_02207360 (both (arg0,ctx)); if ctx->unk2a==2
 * also func_02207b3e(arg0,ctx). Finally, when 0x06098668==0x4a, calls
 * func_0223dcc2(1) and tail-calls func_0223dcc2(2).
 * fidelity: HIGH - all bsrf/braf targets resolved; room-index *0x50, flag bits,
 *   and the two func_0223dcc2 calls modelled. func_0222eb72 takes no args (its
 *   entry overwrites r4). */

extern void audio_send_command_buffer(s32 a, s32 b);
extern void animation_cell_reset(void);
extern void stage_initialize_grids(s32 a, s32 b);
extern void scene81_stage_setup_entry(s32 a, u32 ctx);
extern void scene_bg_layer_build_entry(s32 a, u32 ctx);
extern void scene_load_layer_bank1(s32 a, u32 ctx);
extern void player_clear_saved_action(s32 a);

void scene_reset_stage(s32 arg0)
{
    u32 ctx = MEM32(DH_SCENE_DESCRIPTOR_ADDR);
    s32 cur = SCENE_MEM32(ctx, sound_resource);

    if (cur != 0 &&
        cur != MEM32(0x0607266c + (s16)((s8)MEM8(0x0609868f) * 0x50))) {
        audio_send_command_buffer(cur, 0x6c000);
    }

    MEM32(0x060bacac) |= 1;
    MEM32(0x060bacb0) |= 1;
    MEM32(0x060bacb4) |= 1;
    animation_cell_reset();

    {
        u32 attr = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
        MEM16(attr) = (s16)((MEMU16(attr) & 0x7fff) | 0x8000);
    }
    MEM32(0x06099578) = 1;

    stage_initialize_grids(SCENE_MEM32(ctx, stage_index), SCENE_MEM32(ctx, view_radius));

    MEM32(0x060973f8) = 0;
    MEM32(0x06098664) = 0;
    MEM8(0x0609861c)  = 1;
    MEM32(0x0609856c) = 0;
    MEM32(DH_SCENE_EVENT_FLAGS_ADDR) = 0;

    scene81_stage_setup_entry(arg0, ctx);
    scene_bg_layer_build_entry(arg0, ctx);

    if ((s8)MEM8(ctx + 0x2a) == 2) {
        scene_load_layer_bank1(arg0, ctx);
    }

    if (MEM32(DH_SCENE_INDEX_ADDR) == 0x4a) {
        player_clear_saved_action(1);
        player_clear_saved_action(2);
    }
}
