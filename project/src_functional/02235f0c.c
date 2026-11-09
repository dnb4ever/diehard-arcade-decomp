/* actor_draw_11seg [0x02235f0c]: Existing map corroborated by source: obj. Draw 11 body parts: pose g_06097d58[unk34*0x100] stride (src symbols_band23mid.txt) */
#include "dh.h"
/* 0x02235f0c - multi-segment actor draw (11 body parts). obj->unk30/unk32 are reset
 * to 1/0. The per-part depth table is g_0607c424[obj->unk5E * 0x10] and the pose
 * record is g_06097d58[obj->unk34 * 0x100] (stride 0x10 per part). If the hit-flash
 * trigger obj->unk48 is set, the flash counter obj->unkA0 is armed to 15 and the
 * trigger cleared. For each of the 11 parts it temporarily swaps the object's
 * position (unk8/C/10) to the part's pose position, chooses palette state unk30 (9
 * on the flashing part while unkA0 lasts, else 1), sets the draw-flag bit unk20|0x100,
 * submits the sprite at depth (g_0607c424 entry << 9) >> 4, then restores unk20 and
 * the position.
 * fidelity: HIGH - all 5 sprite_submit1 (0x022153f6) calls confirmed; the compiler
 * unrolled the single 11-iteration loop by 4 (identical rolled here). */
extern s32  g_0607c424[];                               /* per-unk5E part depth table, stride 0x40 */
extern u8   g_06097d58[];                                /* per-actor pose record, stride 0x100 */
extern void sprite_draw_child(void *obj, s32 depth, s32 flag);  /* sprite_submit1 */

void actor_draw_11seg(void *obj) {
    s32 save[3];
    s32 *depth_tab = (s32 *)((u8 *)g_0607c424 + (M16(obj, 0x5E) << 6));
    u8  *pose = g_06097d58 + (M8(obj, 0x34) << 8);
    s32 i;

    M16(obj, 0x30) = 1;
    M16(obj, 0x32) = 0;
    if (M8(obj, 0x48) != 0) {
        M32(obj, 0xA0) = 15;
        M8(obj, 0x48) = 0;
    }
    for (i = 0; i < 11; i++) {
        s32 depth;
        s32 saved20;
        save[0] = M32(obj, 8);
        save[1] = M32(obj, 0xC);
        save[2] = M32(obj, 0x10);
        M32(obj, 8)    = M32(pose, 0);
        M32(obj, 0xC)  = M32(pose, 4);
        M32(obj, 0x10) = M32(pose, 8);
        if (M32(obj, 0xA0) != 0 && (u8)M8(obj, 0x49) == i) {
            M32(obj, 0xA0) -= 1;
            M16(obj, 0x30) = 9;
        } else {
            M16(obj, 0x30) = 1;
        }
        saved20 = M16(obj, 0x20);
        M16(obj, 0x20) |= 0x100;
        depth = depth_tab[i] << 9;
        if (depth < 0)
            depth += 15;
        depth >>= 4;
        sprite_draw_child(obj, depth, 1);
        M16(obj, 0x20) = (s16)saved20;
        M32(obj, 8)    = save[0];
        M32(obj, 0xC)  = save[1];
        M32(obj, 0x10) = save[2];
        pose += 0x10;
    }
}
