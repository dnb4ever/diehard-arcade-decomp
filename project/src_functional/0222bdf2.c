/* player_input_anim_select [0x0222bdf2]: Existing map corroborated by source: walk/turn anim selector driven by per-player input record g_06098734 (stride 0x18); picks unk5C from g_06086308 by directional/attack flags; tail animation_update(g_060995b0[unk34], entity) */
#include "dh.h"
#include "dh_work.h"
/* 0x0222bdf2 (player_input_anim_select) - player-actor walk/turn animation selector driven by the
 * per-player directional-input record at g_06098734 (stride 0x18, indexed by unk34-1). (entity=r4)
 * Reads the record's command word at +0 (facing/attack flags, 'f') and gate word at +4 ('pe4').
 *   pe4 & 7 == 0 (no fresh 8-way command): run down the step timer unkC4 (minus 3 extra while
 *     pe4 & 0x3C0), and once it expires pick the idle/hold anim from g_06086308[unk4D & 1], set
 *     heading unk26 (0x4000 right / 0xC000 left per unk4D bit1), unkCE=0.
 *   else (fresh command): if (unk4D & 2) heading=0x4000 and fold f's 0x300 pair, else heading=0xC000.
 *     Choose a base selector from pe4's low bits (4->1 ; bit0->2/3 by f&0x200 ; bit1->4, or
 *     5/6 by f&0x200 and player #2), then let f's 0x40/0x80 "special" flags override it to 7/8
 *     (swapped by unk4D&2) when f's 0x300 pair is clear. unk5C = g_06086308[sel*2 + (unk4D&1)];
 *     unkCE = sel.
 *   Both command paths set unk43=0xC, unk41=1 and tail-call animation_update(g_060995b0[unk34], entity).
 * Branches resolved (braf_resolve): both braf r3 -> 0x0222dc2c (animation_update, tail). The m2c
 * extent over-runs past the second braf into the literal pool; real body ends at the rts (0x22bfb8).
 * unk34/41/43/4D byte, unk26/5C/CE word, unkC4 long. Cross-checked statement-for-statement vs
 * m2c_fn --asm. fidelity: HIGH. */
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern s32 actor_anim_frame_update(void *a0, void *entity);   /* 0x0222dc2c animation_update (tail) */

void player_input_anim_select(void *entity) {
    u32 pe = (u32)(DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((ACTOR_WORK(entity)->slot_id - 1) * 0x18));
    u16 pe4 = (u16)R16A(pe + 4);

    if ((pe4 & 7) == 0) {
        if (pe4 & 0x3C0)
            M32(entity, 0xC4) -= 3;
        M32(entity, 0xC4) -= 1;
        if (M32(entity, 0xC4) > 0)
            return;
        M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086308 + (ACTOR_M8(entity, direction_code) & 1) * 2);
        if (ACTOR_M8(entity, direction_code) & 2)
            M16(entity, OBJ_ANGLE_Y) = 0x4000;
        else
            M16(entity, OBJ_ANGLE_Y) = (s16)0xC000;
        M16(entity, 0xCE) = 0;
        ACTOR_WORK(entity)->state_handler = 0xC;
        ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
    } else {
        u16 f = (u16)R16A(pe);
        s32 d = ACTOR_M8(entity, direction_code);
        s32 d1 = d & 1;
        s32 sel;

        if (d & 2) {
            if (f & 0x300)
                f ^= 0x300;
            M16(entity, OBJ_ANGLE_Y) = 0x4000;
        } else {
            M16(entity, OBJ_ANGLE_Y) = (s16)0xC000;
        }

        if (pe4 & 4)
            sel = 1;
        else if (pe4 & 1)
            sel = (f & 0x200) ? 3 : 2;
        else
            sel = (f & 0x200) ? ((M8(entity, ACTOR_SLOT_ID) == 2) ? 6 : 5) : 4;

        if ((f & 0x80) && !(f & 0x300))
            sel = (d & 2) ? 7 : 8;
        else if ((f & 0x40) && !(f & 0x300))
            sel = (d & 2) ? 8 : 7;

        M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086308 + (sel * 2 + d1) * 2);
        M16(entity, 0xCE) = (s16)sel;
        ACTOR_WORK(entity)->state_handler = 0xC;
        ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
    }
    actor_anim_frame_update(PA(DH_ACTOR_SLOT_TASKS_ADDR + M8(entity, ACTOR_SLOT_ID) * 4), entity);
}
