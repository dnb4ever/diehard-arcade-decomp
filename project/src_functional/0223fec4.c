/* effect_spawn_template_children [0x0223fec4]: Spawns the specified count of template children and seeds their source-relative position and velocity. */
#include "dh.h"
/* 0x0223fec4 - spawn arg2->unk0 particle/child objects from a template list.
 * arg0 = owner/context passed to the allocator; arg1 = source entity (position
 * unk8/C/10, velocity-sign fields unk14/18/1C, unk34 group); arg2 = spawn spec
 * { unk0 = count, unk4 = template array (16-byte entries), unk8, unkC }.
 * For each of count entries: obj = func_0220ff28(arg0, (void*)0x0603636c);
 *   p = obj->unk8; then fill p:
 *   unk3C = 1; unk68 = arg2->unk8; unk5F = 0; unk62 = 29;
 *   unk60 = (s16)arg2->unkC; unk64 = tmpl->unkC; unk34 = arg1->unk34;
 *   unk38 = 0x200000; unk14/18/1C = tmpl->unk0/4/8 with sign copied from
 *   arg1->unk14/18/1C; advance tmpl by 16; unk8/C/10 = arg1 pos + arg1 vel;
 *   unk4C/50/54 = copy of unk8/C/10.
 * bsrf resolved: 0x20ff28. fidelity: HIGH. */
extern void *task_spawn_list_work7c(void *ctx, void *tmpl);

void effect_spawn_template_children(void *arg0, void *arg1, void *arg2) {
    void *tmpl = MP(arg2, 4);
    s32 count = M32(arg2, 0);
    void *obj;
    void *p;

    while (count-- != 0) {
        obj = task_spawn_list_work7c(arg0, (void *)0x0603636c);
        p = MP(obj, 8);

        M32(p, 0x3C) = 1;
        M32(p, 0x68) = M32(arg2, 8);
        M8(p, 0x5F) = 0;
        M16(p, 0x62) = 29;
        M16(p, 0x60) = (s16)M32(arg2, 0xC);
        M32(p, 0x64) = M32(tmpl, 0xC);
        M8(p, 0x34) = M8(arg1, 0x34);
        M32(p, 0x38) = 0x00200000;

        M32(p, 0x14) = (M32(arg1, 0x14) < 0) ? -M32(tmpl, 0) : M32(tmpl, 0);
        M32(p, 0x18) = (M32(arg1, 0x18) < 0) ? -M32(tmpl, 4) : M32(tmpl, 4);
        M32(p, 0x1C) = (M32(arg1, 0x1C) < 0) ? -M32(tmpl, 8) : M32(tmpl, 8);
        tmpl = (u8 *)tmpl + 16;

        M32(p, 8)    = M32(arg1, 8)   + M32(arg1, 0x14);
        M32(p, 0xC)  = M32(arg1, 0xC) + M32(arg1, 0x18);
        M32(p, 0x10) = M32(arg1, 0x10) + M32(arg1, 0x1C);
        M32(p, 0x4C) = M32(p, 8);
        M32(p, 0x50) = M32(p, 0xC);
        M32(p, 0x54) = M32(p, 0x10);
    }
}
