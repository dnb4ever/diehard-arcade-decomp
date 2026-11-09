/* actor_spawn_aimed_shot [0x02241a10]: Spawns 0221279c projectile, copies owner pose/slot and aims using direction helpers. */
#include "dh.h"
/* 0x02241a10  spawn an actor's muzzle-flash / shot effect and aim it.
 * e0 = func_0220feec(arg0, (void*)0x0221279C); e0->unk10 = 1; e = e0->unk8.
 * Seed: e->unk30=0x296, e->unk34=actor->unk34, e->unk38=0x80000, e->unk60=0x1999,
 *   e->unk64=2, and position e->unk8/C/10 from table g_06097D68[actor->unk34]
 *   (0x100 stride, 3 words).
 * Aim: v = min(actor->unk74 + 0x1000, 0x3000);
 *   h = fixmul((v>>1) + (actor->unk64<<8), cos(actor->unk60));
 *   e->unk14 = -fixmul(h, sin(actor->unk62));
 *   e->unk18 = max(fixmul(v, sin(actor->unk60)), 0x800);
 *   e->unk1C =  fixmul(h, cos(actor->unk62));
 * Jitter from one rng value r: e->unk2A/2C/2E = (s16)(((r>>k)&0xFFF)+0xF800), k=0,8,16.
 * fidelity: HIGH - sin=0x213190 cos=0x2131a8 fixmul=0x2131c8 (415ee-consistent);
 *   widths from asm; r>>8 arithmetic, r>>16 via shlr16+exts.w; trig/fixmul pure so
 *   argument-evaluation order is immaterial. */
extern u8 g_06097D68[];      /* per-actor muzzle offsets, 0x100 stride */
void *task_spawn_child_work88(s32 arg0, void *template);  /* 0x0220feec allocator */
s32 game_random_next(void);     /* rng 0x02210020 */
s32 sin_lookup(s16 angle);            /* sin_lookup 0x02213190 */
s32 cos_lookup(s16 angle);            /* cos_lookup 0x022131a8 */
s32 fixed_mul16(s32 a, s32 b);         /* 16.16 fixed multiply 0x022131c8 */

void actor_spawn_aimed_shot(s32 arg0, void *actor) {
    s32 r = game_random_next();
    void *e0 = task_spawn_child_work88(arg0, (void *)0x0601679C /* PC-relative pointer in HWRAM */);
    M8(e0, 0x10) = 1;
    {
        u8 *e = MP(e0, 8);
        s32 n = (s32)M8(actor, 0x34) << 8;
        s32 v, h, w;

        M16(e, 0x30) = 0x296;
        M8(e, 0x34) = (s8)M8(actor, 0x34);
        M32(e, 0x38) = 0x80000;
        M32(e, 0x60) = 0x1999;
        M32(e, 0x64) = 2;
        M32(e, 0x08) = M32(&g_06097D68[n], 0);
        M32(e, 0x0C) = M32(&g_06097D68[n], 4);
        M32(e, 0x10) = M32(&g_06097D68[n], 8);

        v = M32(actor, 0x74) + 0x1000;
        if (v > 0x3000)
            v = 0x3000;

        h = fixed_mul16((v >> 1) + ((s32)M16(actor, 0x64) << 8),
                          cos_lookup(M16(actor, 0x60)));
        M32(e, 0x14) = -fixed_mul16(h, sin_lookup(M16(actor, 0x62)));

        w = fixed_mul16(v, sin_lookup(M16(actor, 0x60)));
        if (w < 0x800)
            w = 0x800;
        M32(e, 0x18) = w;

        M32(e, 0x1C) = fixed_mul16(h, cos_lookup(M16(actor, 0x62)));

        M16(e, 0x2A) = (s16)((r & 0xFFF) + 0xF800);
        M16(e, 0x2C) = (s16)(((r >> 8) & 0xFFF) + 0xF800);
        M16(e, 0x2E) = (s16)((((s32)(s16)((u32)r >> 16)) & 0xFFF) + 0xF800);
    }
}
