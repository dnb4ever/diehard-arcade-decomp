/* effect_tick_frame_relative_sprite [0x02239462]: Copies owner cel XYZ, seeds +14 and advances animated sprite fields. */
#include "dh.h"
/* 0x02239462 - per-frame handler for a sprite/animation sub-object.
 * obj = actor->unk8. On the first frame (actor->unk10 == 0) seed obj->unk14 =
 * 0xD0000 and bump actor->unk10. Each frame: look up a 16-byte cel record in
 * g_06097d58 at [(obj->unk34 << 8) + (actor->unkF << 4)] and copy its first
 * three longs into obj->unk8/C/10; set obj->unk30 = 0x193 + (obj->unk32 & 3);
 * advance the frame counter obj->unk32 and, once it exceeds 30, clear actor->0.
 * Finally slide obj->unk14 down by 0x2000 and tail-call the child-sprite
 * drawer func_022153f6(obj, obj->unk14, 1).
 * m2c failed: braf tail-call (target unresolved) + mid-function literal pool.
 * branch resolved: braf r1 @2394cc -> 0x02153f6 (delta 0xfffdbf26). fidelity HIGH. */
extern u8   g_06097d58[];
extern void sprite_draw_child(void *a, s32 b, s32 c);

void effect_tick_frame_relative_sprite(void *actor) {
    void *obj = MP(actor, 8);
    void *tbl;

    if (M8(actor, 0x10) == 0) {
        M32(obj, 0x14) = 0xD0000;
        M8(actor, 0x10)++;
    }

    tbl = &g_06097d58[(M8(obj, 0x34) << 8) + (M8(actor, 0x0F) << 4)];
    M32(obj, 8)   = M32(tbl, 0);
    M32(obj, 0xC) = M32(tbl, 4);
    M32(obj, 0x10) = M32(tbl, 8);

    M16(obj, 0x30) = 0x193 + (M16(obj, 0x32) & 3);

    if (++M16(obj, 0x32) > 30)
        M32(actor, 0) = 0;

    M32(obj, 0x14) -= 0x2000;
    sprite_draw_child(obj, M32(obj, 0x14), 1);
}
