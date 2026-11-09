/* collision_resolve_sweep [0x0221746c]: Probes the movement grid, resolves penetration and commits the actor movement with status flags. */
#include "dh.h"
/* 0x0221746c  collide_resolve_sweep: swept-AABB collision resolution core used by
 * entity_move_dispatch (func_022170ea). rec = collision record, ent = rec->unk8
 * (the physics body: position unk8/C/10, velocity unk14/18/1C). Half-extents are
 * rec->unk18/1C/20/24. The mover callback g_060B889C (one of func_02217888 /
 * 022179b0 / 02217ae4, a spatial-hash neighbour search returning the first
 * overlapping node or NULL) is probed to find and resolve penetration.
 *
 * 1. If rec->unk4 < 8, clamp speed: |vx|>0x20000 or |vz|>0x20000 both zero vx
 *    (the ROM zeroes unk14 in the vz branch too - a faithful quirk).
 * 2. Form the candidate position (pos+vel) in `probe`; if its grid cell is off the
 *    0x20 x 0x10 playfield, commit the move, clear g_060B88A4, return -1.
 * 3. First probe: no hit -> commit move, g_060B88A4 = bit31, return NULL.
 * 4. Otherwise accept the move, then sweep X then Z against neighbours, walking the
 *    nearer free edge until clear or blocked by a solid (node->unk4==0). sp18/sp4
 *    hold the X/Z residual penetration.
 * 5. Resolve on the least-penetrating axis: push the body out (clamped to |vel|+
 *    0x3000) and record the contact direction in g_060B88A4: X = bit0(neg)/bit1(pos),
 *    Z = bit2(neg)/bit3(pos), bit31 = hard stop (push within the speed budget).
 *    Returns the first contacted node.
 *
 * fidelity: MED-HIGH. Transcribed from a clean m2c decode (no M2C_ERROR; the only
 * PIC is the g_060B889C callback). Entry clamp, screen-bounds and field offsets
 * cross-checked against --asm; the two sweep loops and the resolution ladder follow
 * m2c's control flow verbatim. 0x60000 is the "clear" sentinel, 0xFFFA0000 = -0x60000. */

typedef void *(*mover_fn)(void *pos, void *ext);

#define G_STATUS M32(0x060B88A4, 0)

void *collision_resolve_sweep(void *rec) {
    void    *ent   = MP(rec, 8);
    u8      *ext   = (u8 *)rec + 0x18;
    mover_fn mover = (mover_fn)MP(0x060B889C, 0);
    s32   probe[3];
    void *first, *hit;
    s32   sp4, sp8, spC, sp10, sp14, sp18, sp1C, sp20;
    s32   px, pz, pmin, v;
    s16   cellX, cellZ;

    if (M32(rec, 4) < 8) {
        v = M32(ent, 0x14);
        if ((v < 0 ? -v : v) > 0x20000) {
            M32(ent, 0x14) = 0;
        }
        v = M32(ent, 0x1C);
        if ((v < 0 ? -v : v) > 0x20000) {
            M32(ent, 0x14) = 0;          /* quirk: vz-overspeed zeroes vx */
        }
    }

    probe[0] = M32(ent, 0x08) + M32(ent, 0x14);
    probe[1] = M32(ent, 0x0C) + M32(ent, 0x18);
    probe[2] = M32(ent, 0x10) + M32(ent, 0x1C);
    cellX = (s16)((u32)(probe[0] - M32(0x0609742C, 0)) >> 16);
    cellZ = (s16)((u32)(probe[2] - M32(0x0609742C, 8)) >> 16);
    if ((u32)cellX >= 0x20U || (u32)cellZ >= 0x10U) {
        M32(ent, 0x08) = probe[0];
        M32(ent, 0x0C) = probe[1];
        M32(ent, 0x10) = probe[2];
        G_STATUS = 0;
        return (void *)-1;
    }

    first = mover(probe, ext);
    if (first == (void *)0) {
        M32(ent, 0x08) = probe[0];
        M32(ent, 0x0C) = probe[1];
        M32(ent, 0x10) = probe[2];
        G_STATUS = 0x80000000;
        return (void *)0;
    }

    v = M32(ent, 0x14);
    sp20 = (v < 0 ? -v : v) + 0x3000;
    v = M32(ent, 0x1C);
    sp1C = (v < 0 ? -v : v) + 0x3000;
    M32(ent, 0x08) = probe[0];
    M32(ent, 0x0C) = probe[1];
    M32(ent, 0x10) = probe[2];
    sp14 = probe[0];
    sp18 = 0x60000;

    /* --- X sweep --- */
    spC  = (M32(first, 0x0C) - M32(first, 0x18) - M32(rec, 0x18)) - 1;
    sp8  = spC - M32(ent, 0x08);
    sp10 = M32(first, 0x0C) + M32(first, 0x18) + M32(rec, 0x18) + 1;
    sp4  = sp10 - M32(ent, 0x08);
loop_x:
    if ((u32)sp8 > 0xFFFA0000U || sp4 < 0x60000) {
        if (-sp8 < sp4) {
            probe[0] = spC;
            hit = mover(probe, ext);
            if (hit == (void *)0) {
                sp18 = sp8;
                goto after_x;
            }
            if (M32(hit, 4) == 0) {
                sp8 = -0x60000;
            } else {
                spC = (M32(hit, 0x0C) - M32(hit, 0x18) - M32(rec, 0x18)) - 1;
                sp8 = spC - M32(ent, 0x08);
            }
            goto loop_x;
        }
        probe[0] = sp10;
        hit = mover(probe, ext);
        if (hit == (void *)0) {
            sp18 = sp4;
            goto after_x;
        }
        if (M32(hit, 4) == 0) {
            sp4 = 0x60000;
        } else {
            sp10 = M32(hit, 0x0C) + M32(hit, 0x18) + M32(rec, 0x18) + 1;
            sp4 = sp10 - M32(ent, 0x08);
        }
        goto loop_x;
    }
after_x:

    /* --- Z sweep --- */
    probe[0] = sp14;
    sp4  = 0x60000;
    sp14 = (M32(first, 0x14) - M32(first, 0x20) - M32(rec, 0x20)) - 1;
    sp8  = sp14 - M32(ent, 0x10);
    sp10 = M32(first, 0x14) + M32(first, 0x20) + M32(rec, 0x20) + 1;
    spC  = sp10 - M32(ent, 0x10);
loop_z:
    if ((u32)sp8 > 0xFFFA0000U || spC < 0x60000) {
        if (-sp8 < spC) {
            probe[2] = sp14;
            hit = mover(probe, ext);
            if (hit == (void *)0) {
                sp4 = sp8;
                goto after_z;
            }
            if (M32(hit, 4) == 0) {
                sp8 = -0x60000;
            } else {
                sp14 = (M32(hit, 0x14) - M32(hit, 0x20) - M32(rec, 0x20)) - 1;
                sp8 = sp14 - M32(ent, 0x10);
            }
            goto loop_z;
        }
        probe[2] = sp10;
        hit = mover(probe, ext);
        if (hit == (void *)0) {
            sp4 = spC;
            goto after_z;
        }
        if (M32(hit, 4) == 0) {
            spC = 0x60000;
        } else {
            sp10 = M32(hit, 0x14) + M32(hit, 0x20) + M32(rec, 0x20) + 1;
            spC = sp10 - M32(ent, 0x10);
        }
        goto loop_z;
    }
after_z:

    /* --- resolution on the least-penetrating axis --- */
    px = (sp18 < 0) ? -sp18 : sp18;
    pz = (sp4  < 0) ? -sp4  : sp4;
    pmin = (px < pz) ? px : pz;

    if (pmin >= 0x60000) {
        M32(ent, 0x08) -= M32(ent, 0x14);
        M32(ent, 0x10) -= M32(ent, 0x1C);
        G_STATUS = ((M32(ent, 0x14) >= 0) ? 2 : 1) | ((M32(ent, 0x1C) >= 0) ? 8 : 4);
    } else if (px <= pz + 0x2000) {
        if (px < sp20) {
            M32(ent, 0x08) += sp18;
            G_STATUS = (sp18 < 0) ? 0x80000001 : 0x80000002;
        } else if (sp18 < 0) {
            M32(ent, 0x08) -= sp20;
            G_STATUS = 1;
        } else {
            M32(ent, 0x08) += sp20;
            G_STATUS = 2;
        }
    } else if (pz < sp1C) {
        M32(ent, 0x10) += sp4;
        G_STATUS = (sp4 < 0) ? 0x80000004 : 0x80000008;
    } else if (sp4 < 0) {
        M32(ent, 0x10) -= sp1C;
        G_STATUS = 4;
    } else {
        M32(ent, 0x10) += sp1C;
        G_STATUS = 8;
    }

    return first;
}
