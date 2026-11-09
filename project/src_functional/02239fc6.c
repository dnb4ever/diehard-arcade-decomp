#include "dh_work.h"
/* actor_apply_effect_action [0x02239fc6]: Swaps effect-linked animation channels and updates attachment and collision flags for actions 0x4f through 0x53. */
#include "dh.h"
/* 0x02239fc6 - "apply action" for a fighter slot. pid = arg0->unk94 selects the
 * action record g_060e0f0c[pid]; the action code arg0->unk88 (ASCII 'O'..'S' =
 * 0x4F..0x53) decides how the upper/lower animation channels of the sprite
 * record g_06096248[unk34] are swapped with the saved state g_060e11e8[unk34]
 * and refilled from the animation tables g_060e10a8 / g_060e1148, and which
 * collision/state bits in unk8C are set via the mask table g_0604ebe4. The
 * shared tail (block_19) handles the throw->grab (7) promotion, pushes the move
 * through func_0223b254, and mirrors the move id into g_06096c74 for the first
 * three slots via func_022260c8. Returns the new animation frame code (1 or 4,
 * or act*8 on the unknown-code path).
 * NOTE: func_0223b254 receives, per entry path, r5 = unk34 for 'O' else unk94,
 * and r7 = an absolute g_0604ebe4 pointer for 'O'/'P'/'Q' but a plain index
 * otherwise; both are reproduced exactly here (its ABI is not yet pinned).
 * branches resolved: bsrf @0x23a1ba=func_0223e17c, @0x23a37a=func_022260c8.
 * fidelity: HIGH (func_0223b254 arg interpretation assumed). */
extern s32  g_060e0f0c[];
extern s32  g_060e0f2c[];
extern u8   g_060e11e8[];   /* saved anim state, 4-byte records */
extern u8   g_06096248[];   /* sprite records, 0x20-byte stride */
extern s16  g_060e1148[];
extern s16  g_060e10a8[];
extern s16  g_060e11a8[];
extern u8   g_06091678[];   /* move-descriptor pointer table */
extern s32  g_actor_slot_masks[];   /* collision/state bit masks */
extern s16  g_0608ce74[];
extern s32  g_06096c74[];
extern u8   g_060e0e58[];   /* per-slot scratch, 0x10 stride */
extern void actor_load_part_offset(void *a, s8 pidx, void *masktab, s32 maskarg);
extern void hud_rebuild_player_counters(void *a);
extern void actor_fill_pose_history(void *a);

s32 actor_apply_effect_action(void *arg0) {
    s32 pid = ACTOR_M32(arg0, selected_effect_slot);
    s32 act = g_060e0f0c[pid];
    s32 id  = ACTOR_MU8(arg0, slot_id);
    s32 off = act * 8;              /* byte offset into the anim tables */
    s32 ret = off;                  /* default return (unknown-code path) */
    s32 b254_pidx = pid;
    s32 b254_mask = act * 4;        /* r7: plain index, or absolute below */
    void *spr = &g_06096248[id * 0x20];
    void *sav = &g_060e11e8[id * 4];
    s32 fo = MU16(arg0, 0x68) * 2;  /* frame offset within the anim table */

    g_060e0f2c[pid] = 0;

    switch (ACTOR_M32(arg0, action_state)) {
    case DH_EFFECT_ACTION_SET_PRIMARY:
        M16(sav, 0) = M16(spr, 8);
        M16(spr, 8) = M16(g_060e1148, off + fo);
        ACTOR_M8(arg0, primary_effect_type) = (s8)act;
        ACTOR_M8(arg0, primary_effect_slot) = (s8)pid;
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[act];
        if (act == 0xC && ACTOR_M8(arg0, secondary_effect_type) == 7) {
            ACTOR_M32(arg0, interaction_flags) &= ~0x80;
            ACTOR_M32(arg0, interaction_flags) |= 0x100;
            ACTOR_M8(arg0, secondary_effect_type) = 8;
            M8(arg0, 0x45) = 8;
            return 1;
        }
        ACTOR_M32(arg0, action_ticks) = MU16((u8 *)MP(g_06091678, ACTOR_M8(arg0, secondary_effect_type) * 4) + off, 6);
        return 4;

    case DH_EFFECT_ACTION_SET_SECONDARY:
        M16(sav, 2) = M16(spr, 0xE);
        M16(spr, 0xE) = M16(g_060e10a8, off + fo);
        ACTOR_M8(arg0, secondary_effect_type) = (s8)act;
        ACTOR_M8(arg0, secondary_effect_slot) = (s8)pid;
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[act];
        ACTOR_M32(arg0, action_ticks) = MU16((u8 *)MP(g_06091678, 0) + off, 6);
        return 4;

    case DH_EFFECT_ACTION_ENTER_SECONDARY:
        b254_mask = (s32)(uintptr_t)&g_actor_slot_masks[act];
        ACTOR_M8(arg0, secondary_effect_type) = (s8)act;
        ACTOR_M8(arg0, secondary_effect_slot) = (s8)pid;
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[act];
        b254_pidx = id;
        M16(sav, 2) = M16(spr, 0xE);
        M16(spr, 0xE) = M16(g_060e10a8, off + fo);
        ret = 1;
        M8(arg0, 0x45) = (s8)act;
        break;

    case DH_EFFECT_ACTION_REFRESH_SECONDARY:
        b254_mask = (s32)(uintptr_t)&g_actor_slot_masks[act];
        M16(sav, 2) = M16(spr, 0xE);
        M16(spr, 0xE) = M16(g_060e10a8, off + fo);
        ACTOR_M8(arg0, secondary_effect_type) = (s8)act;
        ACTOR_M8(arg0, secondary_effect_slot) = (s8)pid;
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[act];
        M8(arg0, 0x45) = (s8)act;
        ret = 1;
        break;

    case DH_EFFECT_ACTION_SWAP_CHANNELS:
        M16(spr, 8) = M16(sav, 0);
        M16(sav, 2) = M16(spr, 0xE);
        b254_mask = (s32)(uintptr_t)&g_actor_slot_masks[act];
        M16(spr, 0xE) = M16(g_060e10a8, off + fo);
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[ACTOR_M8(arg0, primary_effect_type) + 0x18];
        ACTOR_M32(arg0, interaction_flags) &= ~g_actor_slot_masks[ACTOR_M8(arg0, primary_effect_type)];
        ACTOR_M32(arg0, interaction_flags) |= g_actor_slot_masks[act];
        ACTOR_M8(arg0, saved_primary_effect_type) = ACTOR_M8(arg0, primary_effect_type);
        ACTOR_M8(arg0, saved_primary_effect_slot) = ACTOR_M8(arg0, primary_effect_slot);
        ACTOR_M8(arg0, primary_effect_type) = 0;
        ACTOR_M8(arg0, primary_effect_slot) = 0;
        ACTOR_M8(arg0, secondary_effect_type) = (s8)act;
        ACTOR_M8(arg0, secondary_effect_slot) = (s8)pid;
        M8(arg0, 0x45) = (s8)act;
        ret = 1;
        if (id < 3) {
            void *p = &g_060e0e58[(id - 1) * 0x10];
            hud_rebuild_player_counters(arg0);
            M16(p, 0xE) = 0;
            M16(p, 0xC) = 0x10;
        }
        break;

    default:
        break;
    }

    /* block_19: shared tail */
    if (ACTOR_M8(arg0, secondary_effect_type) == 7 && ACTOR_M8(arg0, primary_handoff_pending) != 0) {
        ACTOR_M32(arg0, interaction_flags) &= (s32)0xFEFFFF7F;
        ACTOR_M32(arg0, interaction_flags) |= 0x1100;
        g_060e0f0c[pid] = 8;
        M8(arg0, 0x45) = 8;
        ACTOR_M8(arg0, secondary_effect_type) = 8;
        M16(sav, 0) = M16(spr, 8);
        M16(spr, 8) = M16(g_060e11a8, fo);
        ACTOR_M8(arg0, primary_effect_type) = 0xC;
        ACTOR_M8(arg0, primary_effect_slot) = ACTOR_M8(arg0, primary_handoff_slot);
        ACTOR_M8(arg0, primary_handoff_pending) = 0;
        ACTOR_M8(arg0, primary_handoff_slot) = 0;
    }

    actor_load_part_offset(arg0, (s8)b254_pidx, g_actor_slot_masks, b254_mask);

    if (id < 3) {
        s16 t = g_0608ce74[ACTOR_M8(arg0, secondary_effect_type)];
        g_06096c74[id] = t;
        ACTOR_M16(arg0, animation_requested) = t;
        actor_fill_pose_history(arg0);
    }

    return ret;
}
