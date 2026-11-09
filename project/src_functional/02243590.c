/* actor_spawn_falling_pickup [0x02243590]: Spawns 02232526 item, copies slot, chooses flags and seeds launch magnitude. */
#include "dh.h"
/* 0x02243590  spawn a single falling item/pickup for actor arg1 (arg2 = magnitude).
 * e = func_0220ff28(arg0, (void*)0x02232526)->unk8;   (alloc from template)
 *   e->unk30 = 0x477;
 *   if (!(arg1->unk5C & 3)) e->unk3C = 1;
 *   e->unk68 = 0; e->unk5F = 0; e->unk60 = 0; e->unk64 = 0x3333;
 *   e->unk34 = (s8)arg1->unk34; e->unk38 = 0x200000; e->unk62 = 0x1C;
 *   e->unk8/C/10 = arg1->unk40/44/48;        (position copied from actor's unk40..)
 *   e->unk14 = 0; e->unk18 = 0; e->unk1C = -arg2;
 *   e->unk4C/50/54 mirror unk8/C/10.
 * fidelity: HIGH - template ptr resolved via mova+add (0x02232526); widths from asm
 *   (unk30/60/62 halfword; unk3C/64/38 and position/velocity 32-bit). */
void *task_spawn_list_work7c(s32 a0, void *template);   /* 0x0220ff28 allocator */

void actor_spawn_falling_pickup(s32 arg0, void *arg1, s32 arg2) {
    u8 *e = MP(task_spawn_list_work7c(arg0, (void *)0x06036526 /* PC-relative pointer in HWRAM */), 8);
    M16(e, 0x30) = 0x477;
    if (!(M32(arg1, 0x5C) & 3))
        M32(e, 0x3C) = 1;
    M32(e, 0x68) = 0;
    M8(e, 0x5F) = 0;
    M16(e, 0x60) = 0;
    M32(e, 0x64) = 0x3333;
    M8(e, 0x34) = (s8)M8(arg1, 0x34);
    M32(e, 0x38) = 0x200000;
    M16(e, 0x62) = 0x1C;
    M32(e, 0x08) = M32(arg1, 0x40);
    M32(e, 0x0C) = M32(arg1, 0x44);
    M32(e, 0x10) = M32(arg1, 0x48);
    M32(e, 0x14) = 0;
    M32(e, 0x18) = 0;
    M32(e, 0x1C) = -arg2;
    M32(e, 0x4C) = M32(e, 0x08);
    M32(e, 0x50) = M32(e, 0x0C);
    M32(e, 0x54) = M32(e, 0x10);
}
