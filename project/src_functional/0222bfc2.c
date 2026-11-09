/* player_scan_grab_target [0x0222bfc2]: Six active slots tested for range, eligibility, side and collision before acquisition. */
#include "dh.h"
/* 0x0222bfc2 (func_0222bfc2) - partner/enemy grab-acquisition scan. Walks the
 * 6 tracked actor slots (i=1..6) in the block based at g_06099584; for the first
 * slot that passes every gate (slot active, per-player range value < 0x12100,
 * target is an NPC or 2P enabled, target->unk84 has bit-group 0x40D set and none
 * of 0x237FBB00, not stunned/dead/invuln, |self->unk10 - other->unk10| < 0x4000,
 * and the relative facing from func_022107dc differs from self->unk44) it latches
 * a grab/throw: links the two actors (unk4B/unk4C), seeds the target (unkE0/84/88)
 * and self (unk84/41/43/C4/C8) and selects a move id into self->unk5C. unk45 picks
 * the "front grab" (==0, move 0x92/0x94, type 0x2B) vs "back/running grab" branch
 * (!=0, gated on target->unk38&0x6000, move 0x2E1/0x2E4, type 0x2D). Returns after
 * the first match (or after exhausting all slots).
 *
 * ABI: r4 = self actor. void return. The assigned census address 0x0222bfc2 is the
 * real entry (clean prologue). func_022107dc returns the inter-actor angle via an
 * s16 out-param (&ang); only its sign bit (>>15) is used.
 * fidelity: HIGH - all control flow + offsets verified against --asm; single bsrf
 * resolved to func_022107dc; all shifts are fixed-count (no dropped shld/shad). */

extern void store_angle_between_raw_entry(s32 a0, s32 a1, s32 a2, s32 a3, s16 *out_angle);

extern u8  g_actor_slot_registry[];   /* actor-slot tracking block: +0x0C active, +0x2C ptr[], +0x4C range[] */
extern s16 g_actor_pose_set_legacy;     /* language / 2P-enable flag */

void player_scan_grab_target(void *self)
{
    s32 i;
    s16 ang;

    for (i = 1; i < 7; i++) {
        s32   off = i * 4;
        void *other;
        s32   flags, self_pos, other_pos, dist;

        if (MU32(g_actor_slot_registry, 0x0c + off) == 0)
            continue;
        if (MU32(g_actor_slot_registry, 0x4c + (M8(self, 0x34) << 5) + off) >= 0x12100u)
            continue;

        other = MP(g_actor_slot_registry, 0x2c + off);
        other = MP(other, 0x8);
        if (M8(other, 0x34) <= 2 && g_actor_pose_set_legacy == 0)
            continue;

        flags = M32(other, 0x84);
        if (!(flags & 0x40d))
            continue;
        if (flags & 0x237fbb00)
            continue;
        if (M8(other, 0x48) != 0)
            continue;
        if (M32(other, 0x38) & 0x9e00)
            continue;
        if (M8(other, 0x4e) != 0)
            continue;

        self_pos  = M32(self, 0x10);
        other_pos = M32(other, 0x10);
        dist = self_pos - other_pos;
        if (dist < 0)
            dist = other_pos - self_pos;
        if (dist >= 0x4000)
            continue;

        store_angle_between_raw_entry(M32(self, 0x8), self_pos, M32(other, 0x8), other_pos, &ang);
        if (M8(self, 0x44) == ((u16)ang >> 15))
            continue;

        if (M8(self, 0x45) == 0) {
            M8(self, 0x4b)   = (s8)M8(other, 0x34);
            M8(other, 0x4c)  = (s8)M8(self, 0x34);
            M32(other, 0xe0) = 0x2000;
            M32(other, 0x84) = 0x00040000;
            M32(other, 0x88) = 43;
            M32(self, 0x84)  = 0x00020000;
            M8(self, 0x41)   = 1;
            M8(self, 0x43)   = 18;
            M32(self, 0xc4)  = (M8(self, 0x44) != 0) ? (s32)0xffff667e : 0x00009982;
            M32(self, 0xc8)  = 0;
            if (M8(self, 0x44) == M8(other, 0x44))
                M16(self, 0x5c) = 0x0094;
            else
                M16(self, 0x5c) = 0x0092;
            return;
        }

        if (!(M32(other, 0x38) & 0x6000)) {
            M8(self, 0x4b)   = (s8)M8(other, 0x34);
            M8(other, 0x4c)  = (s8)M8(self, 0x34);
            M32(other, 0xe0) = 0x2000;
            M32(other, 0x84) = 0x00040000;
            M32(other, 0x88) = 45;
            M32(self, 0x84)  = 0x00020000;
            M8(self, 0x41)   = 1;
            M8(self, 0x43)   = 18;
            M32(self, 0xc4)  = (M8(self, 0x44) != 0) ? (s32)0xffff667e : 0x00009982;
            M32(self, 0xc8)  = 0;
            if (M8(self, 0x44) == M8(other, 0x44))
                M16(self, 0x5c) = 0x02e4;
            else
                M16(self, 0x5c) = 0x02e1;
        }
        return;
    }
}
/* fidelity: HIGH - full control flow + field offsets verified vs --asm; bsrf->func_022107dc (angle via out-param, sign bit used); no dropped shifts. */
