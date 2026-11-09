#include "dh.h"
/* 0x0239ce8 (actor_state_select) - combat-state / anim selector for one actor,
 * sibling of actor_state_gen_main (0x02239fc6). Resolves the queued command at
 * index `sel` into the actor's next combat state (unk88, one of 0x4F/0x50/0x51/0x52)
 * and anim record, and returns a small code (0/2/3; the caller forms state = ret+0x21).
 *
 * ABI: s32 (actor, <r5 unused>, sel). actor=r4; sel=r6.
 *   cmd      = g_060E0F0C[sel]                 (command/tile index, 0x060E0F0C)
 *   rec_off  = cmd * 8                         (stride into the 8-byte anim arrays)
 *   flags    = g_0604EBE4[cmd] & 0x3F000       (command flag mask)
 *   rec_a    = *g_06091678 + rec_off           (default anim record)
 *   rec_b    = g_06091678[actor->unk6D] + rec_off (grab-partner anim record)
 *
 * If the actor has no active flag mask (unk8C & 0xFFFFFF == 0): pick rec_a (or the
 * per-actor record g_060916C0[unkE4] when unk38!=1) and set unk88 = 0x52/0x4F by
 * `flags`. Otherwise branch on the grab state (unk6A, unk6D): grab-release and
 * re-grab paths set unk88 = 0x50/0x52 (calling func_02239f78 when unk6C==0xC, and the
 * release helpers func_0223a806/func_0223a9d0 on the 0x50 path); the unk6D==1 "hold"
 * path either swaps the saved anim words (actor_anim_swap_install, unk88=0x51) or,
 * when `flags` is set, takes the 0x52 branch.
 * Common tail: unk84=0x400000, unk5C=rec->unk0 (s16), unk90=(u16)rec->unk4,
 * unk41=1, unk94=sel, and mark g_060E0F2C[sel]=2.
 *
 * m2c notes: the "computed bsrf" braf_resolve flagged at 0x239dcc is the POOL
 * CONSTANT 0x0003f000 (phantom 0x0003 = bsrf), ignored. No real computed branches,
 * no PIC pointers, no tst #imm8>=0x80. Store widths verified against asm (the anim
 * swaps are mov.w / s16; unk88 is mov.l / s32). Bounded 0x239ce8..0x239f78 (1 rts).
 * fidelity: HIGH. */

#define AI32(a) (*(s32 *)(uintptr_t)(a))
#define AI16(a) (*(s16 *)(uintptr_t)(a))

extern void actor_restore_primary_channel(void *actor, void *rec, s32 cmd, s32 rec_off); /* 0x0239f78 */
extern void actor_aim_at_target_record(void *actor);                                  /* 0x0223a806 */
extern void actor_commit_effect_handoffs(void *actor, s32 arg);                         /* 0x0223a9d0 */

/* swap the saved anim words g_060E11E8[unk34] <-> g_06096248[unk34] and install the
 * queued grab action into unk6C/6E (clearing unk6D/6F); sets state unk88 = 0x51. */
static void actor_anim_swap_install(void *actor)
{
    s32 i = M8(actor, 0x34);
    AI16(0x060E11E8 + i * 4)        = AI16(0x06096248 + i * 32 + 8);
    AI16(0x06096248 + i * 32 + 8)   = AI16(0x060E1150 + (u16)MU16(actor, 0x68) * 2);
    AI16(0x06096248 + i * 32 + 0xE) = AI16(0x060E11E8 + i * 4 + 2);
    M8(actor, 0x6C) = M8(actor, 0x6D);
    M8(actor, 0x6E) = M8(actor, 0x6F);
    M8(actor, 0x6D) = 0;
    M8(actor, 0x6F) = 0;
    M32(actor, 0x88) = 0x51;
}

s32 actor_state_select(void *actor, s32 arg1, s32 sel)
{
    s32 idx4    = sel * 4;
    s32 cmd     = AI32(0x060E0F0C + idx4);
    s32 rec_off = cmd * 8;
    s32 flags   = AI32(DH_ACTOR_SLOT_MASKS_ADDR + cmd * 4) & 0x3F000;
    void *rec_a = (void *)(uintptr_t)(AI32(0x06091678) + rec_off);
    void *var_r13;
    s32 var_r12;

    (void)arg1;

    if (!(M32(actor, 0x8C) & 0xFFFFFF)) {
        if (M32(actor, 0x38) == 1)
            var_r13 = rec_a;
        else
            var_r13 = (void *)(uintptr_t)(AI32(0x060916C0 + M32(actor, 0xE4) * 4) + rec_off);
        if (flags != 0) {
            M32(actor, 0x88) = 0x52;
            var_r12 = 3;
        } else {
            M32(actor, 0x88) = 0x4F;
            var_r12 = 0;
        }
    } else {
        s32 sp0 = M8(actor, 0x6D);
        void *rec_b = (void *)(uintptr_t)(AI32(0x06091678 + sp0 * 4) + rec_off);

        if (M8(actor, 0x6A) != 0) {
            if (flags != 0) {
                var_r13 = rec_b;
                M32(actor, 0x88) = 0x52;
                var_r12 = 3;
                if (M8(actor, 0x6C) == 0xC)
                    actor_restore_primary_channel(actor, rec_b, cmd, rec_off);
            } else {
                var_r13 = rec_a;
                M32(actor, 0x88) = 0x50;
                var_r12 = 0;
                if (M8(actor, 0x6C) == 0xC)
                    actor_restore_primary_channel(actor, rec_b, cmd, rec_off);
                actor_aim_at_target_record(actor);
                actor_commit_effect_handoffs(actor, 3);
            }
        } else if (sp0 != 1) {
            if (flags != 0 || cmd == 1) {
                var_r13 = rec_b;
                M32(actor, 0x88) = 0x52;
                var_r12 = 3;
                if (M8(actor, 0x6C) == 0xC)
                    actor_restore_primary_channel(actor, rec_b, cmd, rec_off);
            } else {
                var_r13 = rec_a;
                M32(actor, 0x88) = 0x50;
                var_r12 = 0;
                if (M8(actor, 0x6C) == 0xC)
                    actor_restore_primary_channel(actor, rec_b, cmd, rec_off);
                actor_aim_at_target_record(actor);
                actor_commit_effect_handoffs(actor, 3);
            }
        } else {
            var_r13 = rec_b;
            if (cmd == 1) {
                actor_anim_swap_install(actor);
                var_r12 = 2;
            } else if (flags != 0) {
                M32(actor, 0x88) = 0x52;
                var_r12 = 3;
            } else {
                actor_anim_swap_install(actor);
                var_r12 = 2;
            }
        }
    }

    M32(actor, 0x84) = 0x400000;
    M16(actor, 0x5C) = M16(var_r13, 0);
    M32(actor, 0x90) = (u16)M16(var_r13, 4);
    M8(actor, 0x41) = 1;
    M32(actor, 0x94) = sel;
    AI32(0x060E0F2C + idx4) = 2;
    return var_r12;
}
