/* prompt_tick_random_accept [0x02219c44]: RNG chooses acceptance column; three action bits classify prompt selection. */
/* 0x0219c44 - rotating prompt HUD + 3-button input poll with RNG-seeded accept key.
 * r4 = task, ent = task->unk8. Variant of func_02219f26: the "correct" button is chosen
 * at random (rng_next & 4 selects a column), and on each press the matched button is
 * classified against that threshold (ent->unk60) to set/clear selection flags.
 *   0 (init): lane = (g_060511DC[(*0x06000248>>12)&0xF]==0)?1:0 -> ent->unk7C;
 *      v = rng_next() & 4; *0x0609856C = v; ent->unk6C = v;
 *      ent->unk60 = g_06054300[((*0x0609856C>>1)+unk7C)].field2 (accept-key threshold);
 *      unk70=unk74=0; func_0221a5f6(ent); state++; fall into 1.
 *   1 (run): ctrl=task->unk14->unk8; if (ctrl->unk8 & 1){ ctrl->unk8=0; state=2; } else {
 *      ent->unk10=0x10000; sprite A 0x0605431C func_022153f6(ent,0x10000,4); sprite B
 *      0x06054328 func_022153f6(ent,0x18000,4); orbiting element: model
 *      g_06054300[((unk6C>>1)+unk7C)], pos 0x06054334, func_022154fc(ent,
 *      0x10000+(sin(unk74)>>3), 0x10000+(cos(unk74)>>3), 4); when (*0x060987BC&3) extra
 *      sprite 0x06054340 model 0x06054318[unk7C] func_022153f6(ent,0x18000,4).
 *      inA=(u16)*ent->unk64: if (inA&7){ if ((inA&7)==unk60) *0x0609856C|=1; else if
 *        (inA&1) *0x0609856C&=~4; else if (inA&2) *0x0609856C|=4; spawn h (o->unk60=0,
 *        o->unk64=inA); state=2; } else { inB=(u16)*ent->unk68: if (inB&7){ *0x0609856C|=2;
 *        if ((inB&7)==unk60) *0x0609856C|=1; else if (inB&1) *=~4; else if (inB&2) *|=4;
 *        spawn h (o->unk60=1, o->unk64=inB); state=2; } else if ((unk70&7)==0 &&
 *        *0x060E0DDC) SFX 0xA01101FF; } }  (inA and inB mutually exclusive, A wins.)
 *   2: if (task->unk14->unk8->unk8 & 1) state=3.
 *   3: task->unk0 = 0.  other: no-op.
 * Always (tail): ent->unk70 += 1; ent->unk74 += 0x2000.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py/m2c. bsrf resolved (braf_resolve): rng_next
 *   0x210020, draw_child_sprite 0x2153f6 x3, cos_lookup 0x2131a8, sin_lookup 0x213190,
 *   draw_billboard_quad 0x2154fc, constructor 0x20feec x2, sound_queue_push 0x215f6c;
 *   direct bsr 0x21a5f6. mova+add handler arg (ptr_resolve): 0x21a6a8 both sites. The
 *   rng_next return (not 0x7C) is masked with 4 - m2c miscomputed this as a constant. */
#include "dh_band21.h"

extern void  hud_select_player_sources(u32 obj);                          /* menu_set_text_ptrs */
extern void  sprite_draw_child(u32 obj, s32 scale, s32 flags);    /* draw_child_sprite */
extern void  sprite_draw_billboard(u32 obj, s32 sx, s32 sy, s32 mode);/* draw_billboard_quad */
extern void  sound_queue_push(u32 cmd);                          /* sound_queue_push */
extern s32   sin_lookup(s32 angle);                           /* sin_lookup */
extern s32   cos_lookup(s32 angle);                           /* cos_lookup */
extern void *task_spawn_child_work88(void *parent, void *tmpl);         /* 0x88-record constructor */
extern void  hud_recoil_sprite_task(void);                             /* reaction-object handler */
extern u32   game_random_next(void);                             /* rng_next */

void prompt_tick_random_accept(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0: {
        s32 lane = (MEM32(0x060511dc + ((MEMU32(0x06000248) >> 12) & 0xf) * 4) == 0) ? 1 : 0;
        s32 v = (s32)(game_random_next() & 4);
        MEM32(ent + 0x7c) = lane;
        MEM32(0x0609856c) = v;
        MEM32(ent + 0x6c) = v;
        MEM32(ent + 0x60) = MEM16(0x06054300 + ((MEM32(0x0609856c) >> 1) + MEM32(ent + 0x7c)) * 4 + 2);
        MEM32(ent + 0x70) = 0;
        MEM32(ent + 0x74) = 0;
        hud_select_player_sources(ent);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
    }
    /* fallthrough */
    case 1: {
        u32 inA_addr = MEMU32(ent + 0x64);
        u32 inB_addr = MEMU32(ent + 0x68);
        u16 inA;

        if (MEM32(MEMU32(MEMU32(task + 0x14) + 8) + 8) & 1) {
            MEM32(MEMU32(MEMU32(task + 0x14) + 8) + 8) = 0;
            MEM8(task + 16) = 2;
            break;
        }

        MEM32(ent + 0x10) = 0x10000;

        MEM32(ent + 0x08) = MEM32(0x0605431c);
        MEM32(ent + 0x0c) = MEM32(0x06054320);
        MEM16(ent + 0x30) = (s16)MEM32(0x06054324);
        sprite_draw_child(ent, 0x10000, 4);

        MEM32(ent + 0x08) = MEM32(0x06054328);
        MEM32(ent + 0x0c) = MEM32(0x0605432c);
        MEM16(ent + 0x30) = (s16)MEM32(0x06054330);
        sprite_draw_child(ent, 0x18000, 4);

        MEM16(ent + 0x30) = MEM16(0x06054300 + ((MEM32(ent + 0x6c) >> 1) + MEM32(ent + 0x7c)) * 4);
        MEM32(ent + 0x08) = MEM32(0x06054334);
        MEM32(ent + 0x0c) = MEM32(0x06054338);
        sprite_draw_billboard(ent,
                      0x10000 + (sin_lookup(MEM32(ent + 0x74)) >> 3),
                      0x10000 + (cos_lookup(MEM32(ent + 0x74)) >> 3),
                      4);

        if (MEM32(DH_FRAME_TICK_ADDR) & 3) {
            MEM32(ent + 0x08) = MEM32(0x06054340);
            MEM32(ent + 0x0c) = MEM32(0x06054344);
            MEM16(ent + 0x30) = MEM16(0x06054318 + MEM32(ent + 0x7c) * 2);
            sprite_draw_child(ent, 0x18000, 4);
        }

        inA = MEMU16(inA_addr);
        if (inA & DH_PAD_ACTIONS_DEC) {
            u32 obj;
            if ((s32)(inA & DH_PAD_ACTIONS_DEC) == MEM32(ent + 0x60))
                MEM32(0x0609856c) |= 1;
            else if (inA & 1)
                MEM32(0x0609856c) &= ~4;
            else if (inA & 2)
                MEM32(0x0609856c) |= 4;
            obj = MEMU32((u32)(uintptr_t)task_spawn_child_work88((void *)(uintptr_t)task,
                                                       (void *)hud_recoil_sprite_task) + 8);
            MEM32(obj + 0x60) = 0;
            MEM32(obj + 0x64) = MEMU16(inA_addr);
            MEM8(task + 16) = 2;
        } else {
            u16 inB = MEMU16(inB_addr);
            if (inB & DH_PAD_ACTIONS_DEC) {
                u32 obj;
                MEM32(0x0609856c) |= 2;
                if ((s32)(inB & DH_PAD_ACTIONS_DEC) == MEM32(ent + 0x60))
                    MEM32(0x0609856c) |= 1;
                else if (inB & 1)
                    MEM32(0x0609856c) &= ~4;
                else if (inB & 2)
                    MEM32(0x0609856c) |= 4;
                obj = MEMU32((u32)(uintptr_t)task_spawn_child_work88((void *)(uintptr_t)task,
                                                           (void *)hud_recoil_sprite_task) + 8);
                MEM32(obj + 0x60) = 1;
                MEM32(obj + 0x64) = MEMU16(inB_addr);
                MEM8(task + 16) = 2;
            } else if ((MEM32(ent + 0x70) & DH_PAD_ACTIONS_DEC) == 0 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) {
                sound_queue_push(0xa01101ff);
            }
        }
        break;
    }
    case 2:
        if (MEM32(MEMU32(MEMU32(task + 0x14) + 8) + 8) & 1)
            MEM8(task + 16) = 3;
        break;
    case 3:
        MEM32(task + 0) = 0;
        break;
    default:
        break;
    }

    MEM32(ent + 0x70) += 1;
    MEM32(ent + 0x74) += 0x2000;
}
