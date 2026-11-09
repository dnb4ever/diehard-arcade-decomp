#include "dh_work.h"
/* actor_select_effect_action [0x02239ce8]: Selects and commits an effect-linked action record and returns its action state code. */
#include "dh.h"
/* 0x02239ce8  actor_state_select  (my census slot 0x02239ce6 was a +2
 * misalignment; this is the real entry).
 *
 * s32 actor_state_select(void *ent, s32 arg1, s32 arg2).  arg1 (r5) is unused.
 * Chooses/commits an actor action-state record and returns a state code
 * (0 / 2 / 3).  Part of the actor_state_gen_main (0x02239fc6) family; performs
 * the saved-anim swap (g_060E11E8 <-> g_06096248, with g_060E1150 lookup) noted
 * in symbols.txt for the hit-reaction family.
 *
 * sel  = g_060e0f0c[arg2]                       (selector)
 * attr = g_0604ebe4[sel] & 0x0003F000           (record attribute bits)
 * base = *(void**)0x06091678 + sel*8            (default 8-byte record)
 *
 * Primary split on (ent->unk8C & 0x00FFFFFF):
 *   == 0 : pick rec = base (if ent->unk38==1) else g_060916c0[ent->unkE4]+sel*8;
 *          attr==0 -> unk88=79,ret 0 ; else unk88=82,ret 3.
 *   != 0 : rec5 = g_06091678[(s8)ent->unk6D] + sel*8; branch on ent->unk6A and
 *          (s8)ent->unk6D==1 and sel/attr into the 80/81/82 state writes, the
 *          anim swap (ret 2) and the func_02239f78 / func_0223a806 / func_0223a9d0
 *          side-effect chains (ret 0/3).
 *
 * Common tail: unk84=0x400000; unk5C=*(s16*)rec; unk90=(u16)*(s16*)(rec+4);
 *   unk41=1; unk94=arg2; g_060e0f2c[arg2]=2; return state code.
 *
 * No computed branches (braf_resolve's "239dcc bsrf r0" is a misdecode of the
 * literal-pool word .Lpool_239dcc = 0x0003f000, not code).  The three bsr calls
 * (func_02239f78/func_0223a806/func_0223a9d0) are direct PC-relative.  All shifts are
 * fixed-count (<<2/<<1); no shld/shad helpers involved.
 */
extern void actor_restore_primary_channel(void *ent);
extern void actor_aim_at_target_record(void *ent);
extern void actor_commit_effect_handoffs(void *ent, void *a1);

s32 actor_select_effect_action(void *ent, s32 arg1, s32 arg2) {
    s32 sel  = M32(0x060e0f0c, arg2 << 2);
    s32 attr = M32(DH_ACTOR_SLOT_MASKS_ADDR, sel << 2) & 0x0003F000;
    s32 base = M32(0x06091678, 0) + (sel << 3);
    s32 rec;
    s32 ret;

    (void)arg1;

    if ((ACTOR_M32(ent, interaction_flags) & 0x00FFFFFF) == 0) {
        /* ---- path A ---- */
        if (ACTOR_M32(ent, render_flags) == 1)
            rec = base;
        else
            rec = M32(0x060916c0, ACTOR_M32(ent, state_index) << 2) + (sel << 3);

        if (attr == 0) {
            ACTOR_M32(ent, action_state) = 79;
            ret = 0;
        } else {
            ACTOR_M32(ent, action_state) = 82;
            ret = 3;
        }
    } else {
        /* ---- path B ---- */
        s32 kind = (s8)ACTOR_M8(ent, secondary_effect_type);
        s32 rec5 = M32(0x06091678, kind << 2) + (sel << 3);

        if (ACTOR_M8(ent, saved_primary_effect_type) != 0) {
            rec = (attr != 0) ? rec5 : base;
            if (attr != 0) {
                ACTOR_M32(ent, action_state) = 82;
                ret = 3;
                if (ACTOR_M8(ent, primary_effect_type) == 12)
                    actor_restore_primary_channel(ent);
            } else {
                ACTOR_M32(ent, action_state) = 80;
                ret = 0;
                if (ACTOR_M8(ent, primary_effect_type) == 12)
                    actor_restore_primary_channel(ent);
                actor_aim_at_target_record(ent);
                actor_commit_effect_handoffs(ent, (void *)3);
            }
        } else if (kind == 1) {
            rec = rec5;
            if (sel == 1 || attr == 0) {
                s32 a = (s8)ACTOR_M8(ent, slot_id);
                M16(0x060e11e8, a << 2)            = M16(0x06096248, (a << 5) + 8);
                M16(0x06096248, (a << 5) + 8)      = M16(0x060e1150, MU16(ent, 0x68) << 1);
                M16(0x06096248, (a << 5) + 14)     = M16(0x060e11e8, (a << 2) + 2);
                ACTOR_M8(ent, primary_effect_type) = ACTOR_M8(ent, secondary_effect_type);
                ACTOR_M8(ent, primary_effect_slot) = ACTOR_M8(ent, secondary_effect_slot);
                ACTOR_M8(ent, secondary_effect_type) = 0;
                ACTOR_M8(ent, secondary_effect_slot) = 0;
                ACTOR_M32(ent, action_state) = 81;
                ret = 2;
            } else {
                ACTOR_M32(ent, action_state) = 82;
                ret = 3;
            }
        } else {
            if (attr != 0 || sel == 1) {
                rec = rec5;
                ACTOR_M32(ent, action_state) = 82;
                ret = 3;
                if (ACTOR_M8(ent, primary_effect_type) == 12)
                    actor_restore_primary_channel(ent);
            } else {
                rec = base;
                ACTOR_M32(ent, action_state) = 80;
                ret = 0;
                if (ACTOR_M8(ent, primary_effect_type) == 12)
                    actor_restore_primary_channel(ent);
                actor_aim_at_target_record(ent);
                actor_commit_effect_handoffs(ent, (void *)3);
            }
        }
    }

    /* ---- common tail (L239f38) ---- */
    ACTOR_M32(ent, action_category) = 0x00400000;
    ACTOR_M16(ent, animation_requested) = M16(rec, 0);
    ACTOR_M32(ent, action_ticks) = (u16)M16(rec, 4);
    ACTOR_M8(ent, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M32(ent, selected_effect_slot) = arg2;
    M32(0x060e0f2c, arg2 << 2) = 2;
    return ret;
}
/* fidelity: HIGH. Full 656-byte function transcribed branch-for-branch from asm;
   all three callees are direct bsr (named externs), no computed branches, all
   shifts fixed-count. Caveat: arg1 (r5) is unused in the body (kept for the
   documented 3-arg ABI); table globals (0x060e0f0c/0x06091678/0x060916c0/
   0x0604ebe4/0x060e0f2c and the swap tables 0x060e11e8/0x06096248/0x060e1150)
   addressed absolutely per project convention. */
