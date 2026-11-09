/* launch_arc_segment [0x0221d6b6]: Existing map corroborated by source: (u32 task) ballistic segment walker (table 0x06074E4C/0x06074E76 stride 6); gravity arc, bounce/advance; tail-calls func_02246282 (src symbols_band210big.txt) */
/* 0x0221d6b6 - scripted ballistic/launch actor, segment state machine (r4 = task, ent =
 * task->unk8). Walks a per-segment table (stride 6) of position/velocity seeds at 0x06074E4C
 * (lin) and 0x06074E76 (ang), integrating a gravity arc each frame; when a segment ends it
 * either bounces (halve & negate velocity) to the next segment or, when the segment counter
 * ent->unk5C expires, advances the task state. After its own step it ALWAYS tail-calls the
 * shared integrate/draw routine func_02246282(ent).
 *   0: ent->unk30 = 0x312; if (*0x060973F8 != 0) state++.
 *   1: g = *0x060973F8; only if (u8)g_06074E48[ent->unk58] == (u16)g->unk58:
 *        d = (s8)(unk58*6); seed unk14/unk18/unk1C from 0x06074E4C[d] (+0/+2/+4),
 *        unk2A/2C/2E from 0x06074E76[d] (+0/+2/+4); unk5C = 3; orig = unk58++;
 *        if (orig >= 2) { unk5C = 4; if sound-on SFX 0xA01201FF; state = 3; }
 *        else { unk5C = 3; state = 2; }
 *   2 (arc, bounce in place): unk8 += unk14; unkC += unk18; unk10 += unk1C;
 *        unk24 += unk2A; unk26 += unk2C; unk28 += unk2E; unk18 -= 0x360;
 *        if (unk18 < 0 && unkC <= 0x10000) { unkC = 0x10000; unk18 = -(unk18 >> 1);
 *          if (--unk5C <= 0) state = 1; }
 *   3 (arc, advance to next segment): same integrate; unk18 -= 0x360 (done early);
 *        if (unk18 < 0 && unkC <= 0) { orig = unk58++; d = (s8)(orig*6); unkC = 0;
 *          unk14 = 0x06074E4C[d]; unk18 = -(unk18 >> 1); unk1C = 0x06074E4C[d]+4;
 *          unk2A/2C/2E = 0x06074E76[d]; if (--unk5C <= 0) { unk24 = 0; unk28 = 0; state = 4. } }
 *   other: no-op.  Always: func_02246282(ent).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - hand-reconstructed from fn.py + braf_resolve (m2c aborts: the trailing
 *   braf tail-jump leaves a literal pool that its linear sweep mis-decodes into a phantom
 *   .L21d816). bsrf resolved: sound_queue_push 0x215f6c; braf tail-jump 0x21d8fc -> 0x246282
 *   (delta 0x28982) modelled as the final call func_02246282(ent). Table stride 6, segment
 *   offset exts.b-truncated (s8)(unk58*6); unk14/18/1C are mov.w loads sign-extended to 32. */
#include "dh_band21.h"

extern void sound_queue_push(u32 cmd);    /* sound_queue_push */
extern void actor_draw_transformed(u32 ent);    /* shared integrate/draw tail */

void launch_arc_segment(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0:
        MEM16(ent + 0x30) = 0x312;
        if (MEM32(0x060973f8) != 0)
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;

    case 1: {
        u32 g = MEMU32(0x060973f8);
        s32 launch_segment_index = MEM32(ent + 0x58);
        if ((u8)MEM8(0x06074e48 + launch_segment_index) == (u16)MEMU16(g + 0x58)) {
            s32 d = (s8)(launch_segment_index * 6);
            u32 a = 0x06074e4c + d;
            u32 b = 0x06074e76 + d;
            s32 orig;
            MEM32(ent + 0x14) = MEM16(a + 0);
            MEM32(ent + 0x18) = MEM16(a + 2);
            MEM32(ent + 0x1c) = MEM16(a + 4);
            MEM16(ent + 0x2a) = MEM16(b + 0);
            MEM16(ent + 0x2c) = MEM16(b + 2);
            MEM16(ent + 0x2e) = MEM16(b + 4);
            MEM32(ent + 0x5c) = 3;
            orig = MEM32(ent + 0x58);
            MEM32(ent + 0x58) = orig + 1;
            if (orig >= 2) {
                MEM32(ent + 0x5c) = 4;
                if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                    sound_queue_push(0xa01201ff);
                MEM8(task + 16) = 3;
            } else {
                MEM32(ent + 0x5c) = 3;
                MEM8(task + 16) = 2;
            }
        }
        break;
    }

    case 2:
        MEM32(ent + 0x08) += MEM32(ent + 0x14);
        MEM32(ent + 0x0c) += MEM32(ent + 0x18);
        MEM32(ent + 0x10) += MEM32(ent + 0x1c);
        MEM16(ent + 0x24) = (s16)(MEM16(ent + 0x24) + MEM16(ent + 0x2a));
        MEM16(ent + 0x26) = (s16)(MEM16(ent + 0x26) + MEM16(ent + 0x2c));
        MEM16(ent + 0x28) = (s16)(MEM16(ent + 0x28) + MEM16(ent + 0x2e));
        MEM32(ent + 0x18) -= 0x360;
        if (MEM32(ent + 0x18) < 0 && MEM32(ent + 0x0c) <= 0x10000) {
            s32 x;
            MEM32(ent + 0x0c) = 0x10000;
            MEM32(ent + 0x18) = -(MEM32(ent + 0x18) >> 1);
            x = MEM32(ent + 0x5c) - 1;
            MEM32(ent + 0x5c) = x;
            if (x <= 0)
                MEM8(task + 16) = 1;
        }
        break;

    case 3:
        MEM32(ent + 0x08) += MEM32(ent + 0x14);
        MEM32(ent + 0x0c) += MEM32(ent + 0x18);
        MEM32(ent + 0x10) += MEM32(ent + 0x1c);
        MEM32(ent + 0x18) -= 0x360;
        MEM16(ent + 0x24) = (s16)(MEM16(ent + 0x24) + MEM16(ent + 0x2a));
        MEM16(ent + 0x26) = (s16)(MEM16(ent + 0x26) + MEM16(ent + 0x2c));
        MEM16(ent + 0x28) = (s16)(MEM16(ent + 0x28) + MEM16(ent + 0x2e));
        if (MEM32(ent + 0x18) < 0 && MEM32(ent + 0x0c) <= 0) {
            s32 orig = MEM32(ent + 0x58);
            s32 d = (s8)(orig * 6);
            s32 x;
            MEM32(ent + 0x58) = orig + 1;
            MEM32(ent + 0x0c) = 0;
            MEM32(ent + 0x14) = MEM16(0x06074e4c + d);
            MEM32(ent + 0x18) = -(MEM32(ent + 0x18) >> 1);
            MEM32(ent + 0x1c) = MEM16(0x06074e4c + d + 4);
            MEM16(ent + 0x2a) = MEM16(0x06074e76 + d);
            MEM16(ent + 0x2c) = MEM16(0x06074e76 + d + 2);
            MEM16(ent + 0x2e) = MEM16(0x06074e76 + d + 4);
            x = MEM32(ent + 0x5c) - 1;
            MEM32(ent + 0x5c) = x;
            if (x <= 0) {
                MEM16(ent + 0x24) = 0;
                MEM16(ent + 0x28) = 0;
                MEM8(task + 16) = 4;
            }
        }
        break;

    default:
        break;
    }

    actor_draw_transformed(ent);
}
