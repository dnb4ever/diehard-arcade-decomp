/* hud_tick_timer_icon [0x0221a452]: Two-sprite framed icon uses task digit index, blink phase and scrolling accumulators. */
/* 0x0221a452 (hud_timer_display_update) - task handler drawing a 2-sprite HUD element
 * (a framed digit/icon that scrolls and ticks). task = r4, ent = task->unk8.
 * State at task->unk10:
 *   0 (init): pick lane ent->unk7C = (g_060511DC[(*0x06000248 >> 12) & 0xF] == 0)
 *      ? 1 : 0; ent->unk6C = task->unk15 (digit/frame index); zero the scroll
 *      accumulators unk70/unk74/unk78; advance phase; fall into the draw phase.
 *   1 (run): a global phase *0x060987BC low 2 bits gates two things:
 *      - when (phase & 3) == 0, toggle the blink bit ent->unk32 = (ent->unk32+1)&1.
 *      - when (phase & 3) != 0, draw the backdrop sprite: pos from 0x0605436C,
 *        ent->unk30 = frame[0x06054390 + ent->unk7C*2], render func_022153f6(ent,
 *        0x18000, 4).
 *      Then the digit sprite: ent->unk30 = 0xF, pos = table 0x06054394[ent->unk6C]
 *      (+unk70/unk74 scroll), ent->unk28 = 0x060543D4[ent->unk6C], render via
 *      func_022154fc(ent, 0xC000, 0xC000, 4). Then the glyph overlay: pos from
 *      0x06054374, ent->unk30 picked from 0x0605437C by blink/lane/index, render
 *      func_022153f6(ent, 0x18000, 4). Advance the scroll: unk70/unk74 += table
 *      0x060543B4[ent->unk6C]; unk78++ and wrap all three to 0 past 11; unk70++ and,
 *      every 4th step while *0x060E0DDC is live, push SFX 0xA01101FF.
 *   other: nothing.
 * In states 0/1: if the owning controller task->unk14->unk15 != 0, clear it and deactivate
 *   this task (task->unk0 = 0).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py. bsrf resolved (braf_resolve):
 *   func_022153f6 (draw_child_sprite, 0x2153f6) x2, func_022154fc (draw_billboard_quad,
 *   0x2154fc), sound_queue_push (sound_queue_push, 0x215f6c). unk70 is written twice
 *   (scroll add, then the +1 phase tick) exactly as the SH-2 does. */
#include "dh_band21.h"

extern void sprite_draw_child(u32 obj, s32 scale, s32 flags);         /* draw_child_sprite */
extern void sprite_draw_billboard(u32 obj, s32 sx, s32 sy, s32 mode);     /* draw_billboard_quad */
extern void sound_queue_push(u32 cmd);                               /* sound_queue_push */

void hud_tick_timer_icon(u32 task)
{
    u32 ent = MEMU32(task + 8);
    s32 i6c, i7c;
    u32 ctrl;

    switch (MEM8(task + 16)) {
    case 0:
        MEM32(ent + 0x7c) = (MEM32(0x060511dc + ((MEM32(0x06000248) >> 12) & 0xf) * 4) == 0) ? 1 : 0;
        MEM32(ent + 0x6c) = (s8)MEM8(task + 15);
        MEM32(ent + 0x70) = 0;
        MEM32(ent + 0x74) = 0;
        MEM32(ent + 0x78) = 0;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        /* fallthrough */
    case 1:
        if ((MEM32(DH_FRAME_TICK_ADDR) & 3) == 0)
            MEM16(ent + 0x32) = (s16)((MEM16(ent + 0x32) + 1) & 1);

        MEM32(ent + 0x10) = 0x10000;

        if (MEM32(DH_FRAME_TICK_ADDR) & 3) {
            MEM32(ent + 0x08) = MEM32(0x0605436c);
            MEM32(ent + 0x0c) = MEM32(0x06054370);
            MEM16(ent + 0x30) = MEM16(0x06054390 + MEM32(ent + 0x7c) * 2);
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
        i7c = MEM32(ent + 0x7c);
        if (MEM16(ent + 0x32) != 0)
            MEM16(ent + 0x30) = MEM16(0x0605437c + i7c * 2 + 0x10);
        else
            MEM16(ent + 0x30) = MEM16(0x0605437c + MEM32(ent + 0x6c) * 4 + i7c * 2);
        sprite_draw_child(ent, 0x18000, 4);

        MEM32(ent + 0x70) += MEM32(0x060543b4 + MEM32(ent + 0x6c) * 8);
        MEM32(ent + 0x74) += MEM32(0x060543b8 + MEM32(ent + 0x6c) * 8);
        MEM32(ent + 0x78) += 1;
        if (MEM32(ent + 0x78) > 11) {
            MEM32(ent + 0x70) = 0;
            MEM32(ent + 0x74) = 0;
            MEM32(ent + 0x78) = 0;
        }
        MEM32(ent + 0x70) += 1;
        if ((MEM32(ent + 0x70) & 3) == 0 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push(0xa01101ff);
        break;
    default:
        return; /* ROM 21a46e jumps past the owner-cancel block. */
    }

    ctrl = MEMU32(task + 0x14);
    if (MEM8(ctrl + 15) != 0) {
        MEM8(ctrl + 15) = 0;
        MEM32(task + 0) = 0;
    }
}
