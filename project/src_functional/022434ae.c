/* projectile_spawn_velocity_burst [0x022434ae]: Descriptor count and 0x10-byte velocity records seed a cohort of child particles. */
#include "dh.h"
/* 0x022434ae  spawn a burst of projectiles/particles described by the arg2 list.
 * count = arg2->unk0; src = arg2->unk4 (array of 0x10-byte velocity vectors).
 * For each of count entries:
 *   e = func_0220ff28(arg0, (void*)0x060364DA)->unk8;   (alloc from template)
 *   e->unk3C = 1; e->unk68 = arg2->unk8; e->unk5F = 0; e->unk60 = (s16)arg2->unkC;
 *   e->unk64 = src->unkC; e->unk34 = (s8)arg1->unk34; e->unk38 = 0x200000;
 *   e->unk14/18/1C = src[0]/src[1]/src[2];                         (velocity)
 *   e->unk8  = arg1->unk8 + arg1->unk14;  e->unkC = arg1->unkC + arg1->unk18;
 *   e->unk10 = arg1->unk10 + arg1->unk1C;                          (position)
 *   e->unk4C/50/54 mirror unk8/C/10; advance src by 0x10.
 * fidelity: HIGH - template ptr resolved via mova+add (0x060364DA); widths from asm
 *   (unk3C/68/64/38 are 32-bit; unk5F byte; unk60 halfword). */
void *task_spawn_list_work7c(s32 a0, void *template);   /* 0x0220ff28 allocator */

void projectile_spawn_velocity_burst(s32 arg0, void *arg1, void *arg2) {
    u8 *src = MP(arg2, 4);
    s32 count = M32(arg2, 0);
    while (count-- != 0) {
        u8 *e = MP(task_spawn_list_work7c(arg0, (void *)0x060364DA), 8);
        M32(e, 0x3C) = 1;
        M32(e, 0x68) = M32(arg2, 8);
        M8(e, 0x5F) = 0;
        M16(e, 0x60) = (s16)M32(arg2, 0xC);
        M32(e, 0x64) = M32(src, 0xC);
        M8(e, 0x34) = (s8)M8(arg1, 0x34);
        M32(e, 0x38) = 0x200000;
        M32(e, 0x14) = M32(src, 0);
        M32(e, 0x18) = M32(src, 4);
        M32(e, 0x1C) = M32(src, 8);
        M32(e, 0x08) = M32(arg1, 8) + M32(arg1, 0x14);
        M32(e, 0x0C) = M32(arg1, 0xC) + M32(arg1, 0x18);
        M32(e, 0x10) = M32(arg1, 0x10) + M32(arg1, 0x1C);
        M32(e, 0x4C) = M32(e, 0x08);
        M32(e, 0x50) = M32(e, 0x0C);
        M32(e, 0x54) = M32(e, 0x10);
        src += 0x10;
    }
}
