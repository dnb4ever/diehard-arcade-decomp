/* prompt_tick_blinking_buttons [0x0221a1a0]: Timed/blinking prompt polls two sources and latches selection flags. */
/* 0x0221a1a0 - HUD prompt / input-poll task handler (r4 = task, ent = task->unk8).
 * Sibling of func_0221a452 (same 2+1 sprite HUD draw) but it also polls two button
 * sources (pointers ent->unk64 / ent->unk68, installed by func_0221a5f6) and latches
 * selection flags into *0x0609856C, then advances state on a press.
 *   0 (init): lane = (g_060511DC[(*0x06000248>>12)&0xF]==0) ? 1 : 0 -> ent->unk7C;
 *      p = *0x060987BC & 1; ent->unk6C = p; *0x0609856C = p*4; ent->unk6C += task->unkF*2;
 *      zero unk70/74/78; func_0221a5f6(ent); state++; fall into 1.
 *   1 (run): ctrl = task->unk14->unk8; if (ctrl->unk8 & 1) { ctrl->unk8 = 0; state=2; ret. }
 *      blink toggle when (*0x060987BC & 3)==0; ent->unk10 = 0x10000;
 *      when (*0x060987BC & 3): backdrop sprite from 0x0605436C, model 0x06054390[unk7C],
 *        func_022153f6(ent,0x18000,4). Digit sprite: model 0xF, pos 0x06054394[unk6C]
 *        (+unk70/74), depth 0x060543D4[unk6C], func_022154fc(ent,0xC000,0xC000,4). Glyph
 *        overlay: pos 0x06054374, model from 0x0605437C by blink/lane/index,
 *        func_022153f6(ent,0x18000,4). Scroll: unk70/74 += 0x060543B4[unk6C]; unk78++ and
 *        wrap all three past 11. Input: inA=(u16)*ent->unk64; if (inA & 0x3C0) { if
 *        (inA & 0x06054364[unk6C]) *0x0609856C|=1; state=2; } else { inB=(u16)*ent->unk68;
 *        if (inB & 0x3C0) { *0x0609856C|=2; if (inB & 0x06054364[unk6C]) *0x0609856C|=1;
 *        state=2; } } Then unk70++ and, every 8th while *0x060E0DDC live, SFX 0xA01101FF.
 *   2: if (task->unk14->unk8->unk8 & 1) state = 3.
 *   3: task->unk0 = 0 (deactivate).  other: no-op.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py/m2c. bsrf resolved (braf_resolve): draw_child_sprite
 *   0x2153f6 x2, draw_billboard_quad 0x2154fc, sound_queue_push 0x215f6c; direct bsr 0x21a5f6.
 *   The input pointers are snapshotted at the top of state 1 (stack slot / r8) before the
 *   draw calls, as the SH-2 does. Button mask 0x3C0; SFX cadence is unk70 & 7 (vs & 3 in
 *   the 0221a452 sibling). */
#include "dh_band21.h"

extern void hud_select_player_sources(u32 obj);                           /* menu_set_text_ptrs */
extern void sprite_draw_child(u32 obj, s32 scale, s32 flags);     /* draw_child_sprite */
extern void sprite_draw_billboard(u32 obj, s32 sx, s32 sy, s32 mode); /* draw_billboard_quad */
extern void sound_queue_push(u32 cmd);                           /* sound_queue_push */

void prompt_tick_blinking_buttons(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0: {
        s32 lane = (MEM32(0x060511dc + ((MEMU32(0x06000248) >> 12) & 0xf) * 4) == 0) ? 1 : 0;
        s32 p = MEM32(DH_FRAME_TICK_ADDR) & 1;
        MEM32(ent + 0x7c) = lane;
        MEM32(ent + 0x6c) = p;
        MEM32(0x0609856c) = p * 4;
        MEM32(ent + 0x6c) += MEM8(task + 15) * 2;
        MEM32(ent + 0x70) = 0;
        MEM32(ent + 0x74) = 0;
        MEM32(ent + 0x78) = 0;
        hud_select_player_sources(ent);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
    }
    /* fallthrough */
    case 1: {
        u32 inA_addr = MEMU32(ent + 0x64);
        u32 inB_addr = MEMU32(ent + 0x68);
        s32 i6c, i7c;
        u16 inA;

        if (MEM32(MEMU32(MEMU32(task + 0x14) + 8) + 8) & 1) {
            MEM32(MEMU32(MEMU32(task + 0x14) + 8) + 8) = 0;
            MEM8(task + 16) = 2;
            return;
        }

        if ((MEM32(DH_FRAME_TICK_ADDR) & 3) == 0)
            MEM16(ent + 0x32) = (s16)((MEM16(ent + 0x32) + 1) & 1);

        MEM32(ent + 0x10) = 0x10000;

        i7c = MEM32(ent + 0x7c);
        if (MEM32(DH_FRAME_TICK_ADDR) & 3) {
            MEM32(ent + 0x08) = MEM32(0x0605436c);
            MEM32(ent + 0x0c) = MEM32(0x06054370);
            MEM16(ent + 0x30) = MEM16(0x06054390 + i7c * 2);
            sprite_draw_child(ent, 0x18000, 4);
        }

        i6c = MEM32(ent + 0x6c);
        MEM16(ent + 0x30) = 0xf;
        MEM32(ent + 0x08) = MEM32(0x06054394 + i6c * 8) + MEM32(ent + 0x70);
        MEM32(ent + 0x0c) = MEM32(0x06054398 + i6c * 8) + MEM32(ent + 0x74);
        MEM16(ent + 0x28) = MEM16(0x060543d4 + i6c * 2);
        sprite_draw_billboard(ent, 0xc000, 0xc000, 4);

        MEM32(ent + 0x08) = MEM32(0x06054374);
        MEM32(ent + 0x0c) = MEM32(0x06054378);
        if (MEM16(ent + 0x32) != 0)
            MEM16(ent + 0x30) = MEM16(0x0605437c + i7c * 2 + 0x10);
        else
            MEM16(ent + 0x30) = MEM16(0x0605437c + i6c * 4 + i7c * 2);
        sprite_draw_child(ent, 0x18000, 4);

        MEM32(ent + 0x70) += MEM32(0x060543b4 + i6c * 8);
        MEM32(ent + 0x74) += MEM32(0x060543b8 + i6c * 8);
        MEM32(ent + 0x78) += 1;
        if (MEM32(ent + 0x78) > 11) {
            MEM32(ent + 0x70) = 0;
            MEM32(ent + 0x74) = 0;
            MEM32(ent + 0x78) = 0;
        }

        inA = MEMU16(inA_addr);
        if (inA & 0x3c0) {
            if (inA & MEMU16(0x06054364 + i6c * 2))
                MEM32(0x0609856c) |= 1;
            MEM8(task + 16) = 2;
        } else {
            u16 inB = MEMU16(inB_addr);
            if (inB & 0x3c0) {
                MEM32(0x0609856c) |= 2;
                if (inB & MEMU16(0x06054364 + i6c * 2))
                    MEM32(0x0609856c) |= 1;
                MEM8(task + 16) = 2;
            }
        }

        MEM32(ent + 0x70) += 1;
        if ((MEM32(ent + 0x70) & DH_PAD_ACTIONS_DEC) == 0 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push(0xa01101ff);
        break;
    }
    case 2: {
        u32 ctrl = MEMU32(MEMU32(task + 0x14) + 8);
        if (MEM32(ctrl + 8) & 1)
            MEM8(task + 16) = 3;
        break;
    }
    case 3:
        MEM32(task + 0) = 0;
        break;
    default:
        break;
    }
}
