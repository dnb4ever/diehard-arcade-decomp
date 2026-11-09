/* coop_lever_director [0x0221e256]: Existing map corroborated by source: (u32 task) 2-player lever; owner unk44; grab/turn SFX; spawns 0x219984; tail actor_anim_frame_update + func_0222ca98 (src symbols_band210big.txt) */
/* 0x0221e256 - two-player cooperative lever/switch actor director (task handler, r4 = task,
 * ent = task->unk8). Manages a shared lever that both players can grab; tracks which player
 * (ent->unk44) owns it, reacts to the paired player slot, plays grab/turn SFX, and spawns a
 * companion effect. Per-actor config is 0x06074F38[unk34-1] (mask), 0x06098734[unk34-1]
 * (stride 0x18 input word spC), 0x0604EBE4[] (player bit masks), 0x06074EE4[] (model by owner).
 * sp0 = g_06074F38[unk34-1] (event mask); sp4 = sp0 & *0x06098580 (event pending).
 *   0 (init): func_0221707c(ent,0,0,0,0) [slot register]; func_0222db44(ent) [pose load];
 *      unk5C = unk50 = 0x4B6; unk41 = 1; unk44 = unk34-1; state = 3; unk46 = 3;
 *      if (*0x06098568 & 0x0604EBE4[unk34&1]) unk45 = 3.
 *   1: a = (u16)unk58; if (a<2 || a>8) { if (sp4) { *0x06098580 &= ~sp0;
 *      unk8 = 0x06074EE4[unk44]; if ((*0x0609856C & 1)==unk44) { *0x0609856C |=
 *      0x0604EBE4[unk34+15]; func_0221af30(1); SFX 0xA01201FF; SFX (g_06074F40[unk34-1]<<8)|
 *      0xA00000FF; spawn anim_counter_step; unk5C = unk44?0x4BB:0x4BC; unk41=1; state=4; break; } } }
 *      else if (a==5) unk44 ^= 1;  then block_49: if (unk54 & 2) { unk8 = 0x06074EE4[unk44];
 *      unk5C = unk50; state = unk46. }
 *   2,3: if (sp4) { *0x06098580 &= ~sp0; if ((*0x0609856C&1)==unk44) { same grab/SFX/spawn;
 *      unk5C = unk44?0x4BB:0x4BC; unk41=1; state=4; break; } } then: i = (unk34&1)*4; if
 *      (*0x06098568 & 0x0604EBE4[i]) { st = g_060995B4[i]->unk10; if (st!=unk45 && st>=2) {
 *      st==4 -> unk5C=unk50=0x4B7,state=2,unk46=2 else 0x4B6,state=3,unk46=3; unk41=1;
 *      unk45=st; } } sv = g_06098734 input; if ((sv&0x100)&&!unk44) unk5C=0x4B9,unk41=1,state=1;
 *      else if ((sv&0x200)&&unk44) unk5C=0x4BA,unk41=1,state=1.
 *   4: if (unk54 & 2) { *0x06098580 &= ~sp0; unk5C = unk50; state = unk46. }
 *   Always: if (unk34==1 && (*0x06098568 & 2)) unk10 = g_060995B8->unk8->unk10 + 0x18000;
 *      actor_anim_frame_update(task, ent); tail-call func_0222ca98(ent).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py + m2c. bsrf/braf resolved (braf_resolve/ptr_resolve):
 *   func_0221707c (5-arg: r4-r7 + one stack arg, all 0 - r11 reused as 0, add #4,r15 after),
 *   func_0222db44, func_0221af30, sound_queue_push 0x215f6c (indirect via r11 fn-ptr, each call
 *   1 arg - m2c mis-reports 4), func_0220ff72(task, h219984), actor_anim_frame_update 0x22dc2c;
 *   braf tail-jump 0x21e616 -> 0x22ca98 modelled as the final call func_0222ca98(ent). Model
 *   offsets 0x06074EE4[owner] use exts.b of owner*0xC. */
#include "dh_band21.h"

extern void  actor_register_movement_slot(u32 entity, s32 a1, s32 a2, s32 a3, s32 a4); /* slot register */
extern void  actor_load_pose_buffers(u32 entity);                                 /* pose load */
extern void  player_update_aim_limits(s32 arg0);                                   /* crosshair aim step */
extern void  sound_queue_push(u32 cmd);                                    /* sound_queue_push */
extern void *task_spawn_child_work40(void *parent, void *tmpl);                   /* 0x40-record constructor */
extern void  anim_counter_step(void);                                       /* companion effect handler */
extern void  actor_anim_frame_update(u32 arg0, u32 entity);             /* actor_anim_frame_update */
extern void  draw_actor_hier_shadowed_entry(u32 ent);                                    /* shared draw/finalise tail */

void coop_lever_director(u32 task)
{
    u32 ent = MEMU32(task + 8);
    s32 n34 = MEM8(ent + ACTOR_SLOT_ID);
    s32 sp0 = MEM16(0x06074f38 + (n34 - 1) * 2);
    s32 sp4 = sp0 & MEM32(DH_SCENE_EVENT_FLAGS_ADDR);
    u32 spC = DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((n34 - 1) * 0x18);

    switch (MEM8(task + 16)) {
    case 0:
        actor_register_movement_slot(ent, 0, 0, 0, 0);
        actor_load_pose_buffers(ent);
        MEM16(ent + 0x5c) = 0x4b6;
        MEM16(ent + 0x50) = MEM16(ent + 0x5c);
        MEM8(ent + 0x41) = 1;
        MEM8(ent + 0x44) = (s8)(n34 - 1);
        MEM8(task + 16) = 3;
        MEM8(ent + 0x46) = 3;
        if (MEM32(0x06098568) & MEM32(DH_ACTOR_SLOT_MASKS_ADDR + (n34 & 1) * 4))
            MEM8(ent + 0x45) = 3;
        break;

    case 1: {
        s32 a = (u16)MEM16(ent + 0x58);
        if (a < 2 || a > 8) {
            int matched = 0;
            if (sp4 != 0) {
                MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~sp0;
                MEM32(ent + 0x08) = MEM32(0x06074ee4 + (s8)(MEM8(ent + 0x44) * 12));
                matched = ((MEM32(0x0609856c) & 1) == MEM8(ent + 0x44));
            }
            if (matched) {
                MEM32(0x0609856c) |= MEM32(DH_ACTOR_SLOT_MASKS_ADDR + (n34 + 15) * 4);
                player_update_aim_limits(1);
                if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                    sound_queue_push(0xa01201ff);
                if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                    sound_queue_push(((u32)MEM16(0x06074f40 + (n34 - 1) * 2) << 8) | DH_SOUND_CUE_BASE);
                task_spawn_child_work40((void *)(uintptr_t)task, (void *)anim_counter_step);
                MEM16(ent + 0x5c) = (MEM8(ent + 0x44) != 0) ? 0x4bb : 0x4bc;
                MEM8(ent + 0x41) = 1;
                MEM8(task + 16) = 4;
                break;
            }
        } else if (a == 5) {
            MEM8(ent + 0x44) = (s8)(MEM8(ent + 0x44) ^ 1);
        }
        /* block_49 */
        if ((u16)MEM16(ent + 0x54) & 2) {
            MEM32(ent + 0x08) = MEM32(0x06074ee4 + (s8)(MEM8(ent + 0x44) * 12));
            MEM16(ent + 0x5c) = MEM16(ent + 0x50);
            MEM8(task + 16) = (s8)MEM8(ent + 0x46);
        }
        break;
    }

    case 2:
    case 3:
        if (sp4 != 0) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~sp0;
            if ((MEM32(0x0609856c) & 1) == MEM8(ent + 0x44)) {
                MEM32(0x0609856c) |= MEM32(DH_ACTOR_SLOT_MASKS_ADDR + (n34 + 15) * 4);
                player_update_aim_limits(1);
                if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                    sound_queue_push(0xa01201ff);
                if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                    sound_queue_push(((u32)MEM16(0x06074f40 + (n34 - 1) * 2) << 8) | DH_SOUND_CUE_BASE);
                task_spawn_child_work40((void *)(uintptr_t)task, (void *)anim_counter_step);
                MEM16(ent + 0x5c) = (MEM8(ent + 0x44) != 0) ? 0x4bb : 0x4bc;
                MEM8(ent + 0x41) = 1;
                MEM8(task + 16) = 4;
                break;
            }
        }
        {
            s32 i = (n34 & 1) * 4;
            if (MEM32(0x06098568) & MEM32(DH_ACTOR_SLOT_MASKS_ADDR + i)) {
                u32 slot = MEMU32(0x060995b4 + i);
                s32 st = (s8)MEM8(slot + TASK_STATE);
                if (st != MEM8(ent + 0x45) && st >= 2) {
                    if (st == 4) {
                        MEM16(ent + 0x5c) = 0x4b7;
                        MEM16(ent + 0x50) = 0x4b7;
                        MEM8(task + 16) = 2;
                        MEM8(ent + 0x46) = 2;
                    } else {
                        MEM16(ent + 0x5c) = 0x4b6;
                        MEM16(ent + 0x50) = 0x4b6;
                        MEM8(task + 16) = 3;
                        MEM8(ent + 0x46) = 3;
                    }
                    MEM8(ent + 0x41) = 1;
                    MEM8(ent + 0x45) = (s8)MEM8(slot + TASK_STATE);
                }
            }
            {
                u16 sv = (u16)MEM16(spC);
                if ((sv & 0x100) && MEM8(ent + 0x44) == 0) {
                    MEM16(ent + 0x5c) = 0x4b9;
                    MEM8(ent + 0x41) = 1;
                    MEM8(task + 16) = 1;
                } else if ((sv & 0x200) && MEM8(ent + 0x44) != 0) {
                    MEM16(ent + 0x5c) = 0x4ba;
                    MEM8(ent + 0x41) = 1;
                    MEM8(task + 16) = 1;
                }
            }
        }
        break;

    case 4:
        if ((u16)MEM16(ent + 0x54) & 2) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~sp0;
            MEM16(ent + 0x5c) = MEM16(ent + 0x50);
            MEM8(task + 16) = (s8)MEM8(ent + 0x46);
        }
        break;

    default:
        break;
    }

    if (n34 == 1 && (MEM32(0x06098568) & 2))
        MEM32(ent + 0x10) = MEM32(MEMU32(MEMU32(0x060995b8) + 8) + 0x10) + 0x18000;

    actor_anim_frame_update(task, ent);
    draw_actor_hier_shadowed_entry(ent);
}
