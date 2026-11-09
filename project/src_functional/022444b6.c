/* enemy_sweep_to_anchor [0x022444b6]: Resolves reachability and step sequence toward the actor's stored anchor. */
#include "dh.h"

/* 0x022444b6 - per-actor "sweep toward target" collision / path resolver.
 * Census entry 0x2444b6 is already the real prologue (clean, phantom-free).
 * idx = actor->unk34 selects two parallel per-player scratch tables:
 *   tbl10 = 0x06097128 + idx*0x24  (unk0 = step-sequence selector word;
 *                                   +0x10.. = up to 3 found-target pointers)
 *   tbl13 = 0x060B6D9C + idx*0x48  (collision box: +0x18,+0x1C,+0x20,+0x24)
 * Calls (all resolved, none stubbed):
 *   func_022448d0(actor, tgt, &actor->unk8)   -> reachability test (0 = blocked)
 *   atan2_fixed(dy, dx)                      -> angle (atan2-like; r4=dy r5=dx)
 *       bsrf @0x244568, target 0x022130b4 via braf_resolve.py
 *   dispatch_cmd(&pos, &box, 0, idx)          -> hit object or NULL
 *       jsr @r8, r8 = mova(0x0224458c) + 0xFFFD32AC = 0x02217838 (hand-resolved)
 * HWRAM step tables: 0x0608DBD4 (seq pointer by selector), 0x0608DB6C
 * ({dx,dy} s32 pairs indexed by step), 0x0608DBF4 (status word by count).
 * fidelity: HIGH - hand-traced against m2c_fn --asm; m2c dropped the bsrf/jsr
 * calls (M2C_ERROR) so this was reconstructed by hand. */

s32   aabb_overlap_box_b(void *actor, void *tgt, void *pos);      /* extern */
s32   atan2_fixed(s32 dy, s32 dx);                         /* extern */
void *dispatch_cmd(void *pos, void *box, s32 z, s32 idx);   /* extern */

s32 enemy_sweep_to_anchor(void *actor)
{
    s8    idx   = M8(actor, 0x34);
    void *tgt   = MP(actor, 0x124);
    void *p158  = MP(actor, 0x158);
    void *tbl10 = (u8 *)(uintptr_t)0x06097128 + (s16)(idx * 0x24);
    void *tbl13 = (u8 *)(uintptr_t)DH_ACTOR_MOVEMENT_RECORDS_ADDR + (s16)(idx * 0x48);
    s32   fr[9];                 /* 36-byte call frame (offsets 0x00..0x20) */
    s32   count;
    u32   step;

    if (aabb_overlap_box_b(actor, tgt, (u8 *)actor + 8) == 0) {
        if (M8(actor, 0x44) != 0) {
            if (M32(actor, 8) < M32(tgt, 0)) {
                return 2;
            }
            return 0;
        }
        if (M32(actor, 8) > M32(tgt, 0)) {
            return 2;
        }
        return 0;
    }

    M32(fr, 0x08) = M32(tbl13, 0x18);
    M32(fr, 0x0C) = M32(tbl13, 0x1C);
    M32(fr, 0x10) = M32(tbl13, 0x20);
    M32(fr, 0x14) = M32(tbl13, 0x24);
    M16(fr, 0x00) = (s16)M32(tbl10, 0x00);
    M32(fr, 0x1C) = 0;

    step = ((u16)(-atan2_fixed(M32(tgt, 8) - M32(actor, 0x10),
                                 M32(tgt, 0) - M32(actor, 8))) >> 0xC) + 1;

    MP(fr, 0x00) = MP(0x0608DBD4 + (u32)(u16)M16(fr, 0x00) * 4, 0);

    count = 0;
    do {
        void *sp0 = MP(fr, 0x00);
        void *hit;
        s32   pairoff;

        MP(fr, 0x00) = (u8 *)sp0 + 2;
        step    = 0xE & (M16(sp0, 0) + step);
        pairoff = (s32)step * 4;
        M32(fr, 0x18) = M32(actor, 8)    + M32(0x0608DB6C + pairoff, 0);
        M32(fr, 0x20) = M32(actor, 0x10) + M32(0x0608DB6C + pairoff, 4);

        hit = dispatch_cmd((u8 *)fr + 0x18, (u8 *)fr + 8, 0, idx);
        if (hit == 0) {
            break;
        }
        {
            void *hobj = MP(hit, 8);
            if (M8(hobj, 0x34) == 0 && (step == 4 || step == 0xC)) {
                return 0;
            }
            MP((u8 *)tbl10 + 0x10 + count * 4, 0) = hobj;
            count++;
        }
    } while (count < 3);

    if (M32(0x0608DBF4 + count * 4, 0) == 2) {
        return 0;
    }

    step &= 0xE;
    if (M8(actor, 0x44) != 0) {
        if (M32(actor, 8) < M32(tgt, 0)) {
            return 2;
        }
        step += 8;
    } else if (M32(actor, 8) > M32(tgt, 0)) {
        return 2;
    }

    {
        void *dirtab = MP(p158, 0);
        s16   dir    = M16(dirtab, ((u32)(step >> 1) & 7) * 2);
        if ((u16)M16(actor, 0x5A) != (u16)dir) {
            M8(actor, 0x41)  = 1;
            M16(actor, 0x5C) = dir;
        }
    }
    return 1;
}
