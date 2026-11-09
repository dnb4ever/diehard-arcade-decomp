/* interaction_advance_frame_pair [0x02227602]: Input/frame table 060862f4 drives paired animation requests and latch/dwell counters. */
#include "dh.h"
/* 0x02227602  per-entity animation/interaction step. For entity arg1 (id = (arg1+0x34)), looks up
 * its current frame record fr = g_060862F4[g_06096EA0[id]] + g_06096CA0[id]*8 and, while not latched
 * (unk52 == 0) and the dwell counter g_06096EB0[id] is below the frame's length (fr+0, unsigned
 * cmp/hs), latches a transition when the matching input bit of arg0->unk4 is set (bit1 when
 * g_06096EA0[id] != 0, else bit0): it writes the frame's target id (fr+4) into arg1->unk5C /
 * arg2->unk5C (arg2 gets target+1) and sets unk52.
 * On commit (arg1->unk54 & 2): if latched, clears the latch + dwell counter and advances
 * g_06096CA0[id]; otherwise writes the fallback target (fr+6) to arg1->unk5C / arg2->unk5C
 * (target+1) and sets g_06096E90[id] = 1. Finally the dwell counter is incremented and clamped to
 * 0x1F with a SIGNED s16 comparison (cmp/gt): negative counters are incremented and kept.
 * fidelity: HIGH - fixed dwell clamp signedness (was u16 unsigned compare, writing 0x001F where
 * ROM keeps 0xFFFE at 0x06096EB0+id*2); first-diff evidence 0x06096ec2 ref 254 cand 0. */
extern u16   g_06096EA0[];   /* per-entity frame-set selector */
extern u16   g_06096CA0[];   /* per-entity frame index */
extern s16   g_06096EB0[];   /* per-entity dwell counter (signed clamp) */
extern s16   g_06096E90[];   /* per-entity fallback flag */
extern void *g_060862F4[];   /* frame-set data pointers */
void interaction_advance_frame_pair(void *arg0, void *arg1, void *arg2) {
    s32 id = M8(arg1, 0x34);
    u8 *fr = (u8 *)g_060862F4[g_06096EA0[id]] + g_06096CA0[id] * 8;

    if (MU16(arg1, 0x52) == 0 && (u32)(u16)g_06096EB0[id] < (u32)M32(fr, 0)) {
        u16 in = MU16(arg0, 4);
        s16 target = M16(fr, 4);
        if (g_06096EA0[id] != 0) {
            if (in & 2) {
                M16(arg1, 0x5C) = target;
                M16(arg2, 0x5C) = (s16)(MU16(fr, 4) + 1);
                M16(arg1, 0x52) = 1;
            }
        } else if (in & 1) {
            M16(arg1, 0x5C) = target;
            M16(arg2, 0x5C) = (s16)(MU16(fr, 4) + 1);
            M16(arg1, 0x52) = 1;
        }
    }

    if (MU16(arg1, 0x54) & 2) {
        if (MU16(arg1, 0x52) != 0) {
            M16(arg1, 0x52) = 0;
            g_06096EB0[id] = 0;
            g_06096CA0[id] = (u16)(g_06096CA0[id] + 1);
        } else {
            M16(arg1, 0x5C) = M16(fr, 6);
            M16(arg2, 0x5C) = (s16)(MU16(fr, 6) + 1);
            g_06096E90[id] = 1;
        }
    }
    g_06096EB0[id] = (s16)(g_06096EB0[id] + 1);
    if (g_06096EB0[id] > 0x1F) {
        g_06096EB0[id] = 0x1F;
    }
}
