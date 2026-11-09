/* scene_tick_waypoint_model_intro [0x0221f9c4]: Scene task switch selects spawn branches; explicit same-frame fallthrough retained. */
#include "dh.h"

extern void scene_reload_graphics(u8 *task);
extern void scene_reset_stage(s32 arg);
extern void scene_load_layer_bank2(u8 *task);
extern u8 *task_spawn_current_work6c(u8 *task, u32 arg1);
extern u8 *task_spawn_child_work88(u8 *task, u32 arg1);
extern void audio_queue_scene_sound(void *obj);
extern void audio_queue_scene_gated_id(s16 id);
extern void stage_draw_visible_cells(void);
extern void sound_queue_push(s32 req);
extern void spawn_entry_list(void *task);
extern void vdp2_pack_rotation_addresses(s32 which, void *arg1);
extern void player_refresh_ready_mask(void);
extern void task_cancel_below_depth(u8 depth);

void scene_tick_waypoint_model_intro(u8 *task)
{
    u32 t = (u32)(uintptr_t)task;
    u32 sp0 = MU32(t, 8);
    u32 rec = 0x060752EC + (s32)M8(MU32(DH_SCENE_DESCRIPTOR_ADDR, 0), 0x29) * 0xC;
    s8 state = (s8)M8(t, 16);

    switch (state) {
    case 0:
        scene_reload_graphics((u8 *)(uintptr_t)t);
        M8(t, 16) = (s8)((s8)M8(t, 16) + 1);
        return;

    case 1: {
        u8 *obj;
        u32 sub, r8;
        scene_reset_stage((s32)t);
        scene_load_layer_bank2((u8 *)(uintptr_t)t);

        sub = MU32(DH_VIEW_TASK_ADDR, 0);
        M32(sub, 0) = 0x0221FED4;
        M8(sub, 16) = 0;

        {
            u32 v = MU32(DH_VDP2_REGISTER_SHADOW_ADDR, 0);
            M16(v, 0x114) = (s16)0xFF01;
            M16(v, 0x116) = (s16)0xFF01;
            M16(v, 0x118) = (s16)0xFF01;
        }

        obj = task_spawn_current_work6c((u8 *)(uintptr_t)t, 0x0221FEC8);
        M32(MU32((u32)(uintptr_t)obj, 8), 0x50) = (s32)(s16)M16(rec, 8);
        audio_queue_scene_sound(obj);

        spawn_entry_list((void *)(uintptr_t)t);

        r8 = 0;
        do {
            u8 *a = task_spawn_child_work88((u8 *)(uintptr_t)t, 0x022202CA);
            u8 *b;
            M8((u32)(uintptr_t)a, 15) = (s8)r8;
            r8++;
            b = task_spawn_child_work88((u8 *)(uintptr_t)t, 0x022202CA);
            M8((u32)(uintptr_t)b, 15) = (s8)r8;
            r8++;
        } while (r8 < 4);

        task_spawn_child_work88((u8 *)(uintptr_t)t, 0x0221FFB0);

        if (MU32(DH_SCENE_INDEX_ADDR, 0) == DH_SCENE_SEGMENTED_MODEL_INTRO) {
            u8 *f = task_spawn_child_work88((u8 *)(uintptr_t)t, 0x0221FE00);
            M32(sp0, 100) = (u32)(uintptr_t)f;
            sub = MU32((u32)(uintptr_t)f, 8);
            M32(sub, 8) = 0x00200000;
            M32(sub, 12) = 0;
            M32(sub, 16) = 0x00200000;
            M32(sp0, 88) = 0x40;
            M8(t, 16) = 2;
        } else {
            u32 r9 = MU32(rec, 4);
            while ((s16)M16(r9, 0) >= 0) {
                u8 *f = task_spawn_child_work88((u8 *)(uintptr_t)t, 0x0221FD7A);
                sub = MU32((u32)(uintptr_t)f, 8);
                M32(sub, 8) = 0x00200000;
                M32(sub, 12) = 0;
                M32(sub, 16) = 0x00200000;
                M8((u32)(uintptr_t)f, 15) = (s8)(s16)M16(r9, 0);
                r9 += 2;
            }
            M8(t, 16) = 3;
        }
        M32(sp0, 84) = 16;
        return;
    }

    case 2: {
        s32 v = M32(sp0, 88) - 1;
        M32(sp0, 88) = v;
        if (v == 0) {
            u32 r9;
            u32 vdpp;
            vdp2_pack_rotation_addresses(0, (void *)0x06072254);
            vdpp = MU32(DH_VDP2_REGISTER_SHADOW_ADDR, 0);
            M16(vdpp, 0xF8) = 0x600;
            M16(vdpp, 0xFA) = 0x700;
            M16(vdpp, 0xFC) = 4;
            M8(MU32(sp0, 100), 18) = 1;
            r9 = MU32(rec, 4);
            while ((s16)M16(r9, 0) >= 0) {
                u8 *f = task_spawn_child_work88((u8 *)(uintptr_t)t, 0x0221FD7A);
                u32 sub = MU32((u32)(uintptr_t)f, 8);
                M32(sub, 8) = 0x00200000;
                M32(sub, 12) = 0;
                M32(sub, 16) = 0x00200000;
                M8((u32)(uintptr_t)f, 15) = (s8)M8(r9, 1);
                r9 += 2;
            }
            M8(t, 16) = 3;
        }
        /* fall through */
    }
    case 3:
    common_tail: {
        player_refresh_ready_mask();
        if (MU32(0x06098664, 0) == 1) {
            u8 *obj = task_spawn_current_work6c((u8 *)(uintptr_t)t, 0x022201CE);
            s32 v = (s16)M16(rec, 10);
            M32(MU32((u32)(uintptr_t)obj, 8), 0x50) = (s32)(s16)M16(rec, 10);
            audio_queue_scene_gated_id((s16)((0x80 - (v >= 0 ? v : -v)) >> 1));
            M8(t, 16) = 4;
            return;
        } else {
            s32 c = M32(sp0, 84);
            M32(sp0, 84) = c - 1;
            if (c >= 0)
                return;
            if (!(((u16)MU16(DH_PLAYER_INPUT_RECORDS_ADDR, 0) & DH_PAD_ACTIONS_DEC) && (MU32(DH_READY_PLAYER_MASK_ADDR, 0) & 1)) &&
                !(((u16)MU16(0x0609874C, 0) & DH_PAD_ACTIONS_DEC) && (MU32(DH_READY_PLAYER_MASK_ADDR, 0) & 2)))
                return;
            if (MU32(DH_PLAY_SESSION_MODE_ADDR, 0) != 1)
                return;
            {
                u8 *obj = task_spawn_current_work6c((u8 *)(uintptr_t)t, 0x022201CE);
                s32 v = (s16)M16(rec, 10);
                M32(MU32((u32)(uintptr_t)obj, 8), 0x50) = (s32)(s16)M16(rec, 10);
                audio_queue_scene_gated_id((s16)((0x80 - (v >= 0 ? v : -v)) >> 1));
            }
            M32(0x06096C90, 0) = 0;
            M8(t, 16) = 4;
            return;
        }
    }

    case 4: {
        u32 sub;
        u32 v;
        stage_draw_visible_cells();
        if (!(MU32(sp0, 60) & 2))
            return;
        sub = MU32(MU32(t, 20), 8);
        M32(sub, 60) = M32(sub, 60) | 1;
        v = MU32(DH_VDP2_REGISTER_SHADOW_ADDR, 0);
        M16(VDP2_TVMD_ADDR, 0) = (s16)(M16(v, 0) & 0x7FFF);
        M32(0x06099578, 0) = 0;
        M32(0x060BACAC, 0) = M32(0x060BACAC, 0) & ~1;
        M32(0x060BACB0, 0) = M32(0x060BACB0, 0) & ~1;
        M32(0x060BACB4, 0) = M32(0x060BACB4, 0) & ~1;
        sound_queue_push(DH_SOUND_TRANSITION_DONE_COMMAND);
        M32(0x060986B8, 0) = 0;
        M8(0x0609861C, 0) = 0;
        M32(0x06098664, 0) = 0;
        task_cancel_below_depth((u8)((s8)M8(t, 14) + 0xFF));
        return;
    }

    default:
        return;
    }
}
