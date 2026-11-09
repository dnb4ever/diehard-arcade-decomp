/* camera_copy_pose_spawn_flagged_child [0x0220f202]: Copies player part pose and creates its flagged angle-seeded child on initialization. */
#include "dh.h"
/* 0x0220f202 - init an object's sprite-attribute block from source record `src`, and on
 * first init spawn a child record (as 0x20f19e, plus sets the child's unkF byte = 1). If
 * obj->unk68 == 1: copy 3 shorts src->{0,2,4} -> obj->{24,26,28}, clear obj->unk68, then
 * ret = func_0220ff72(*g_060996d0, &rec_022199da); ret->unkF = 1; (ret->unk8)->unk18 =
 * (u16)obj->unk26. Always sets obj->{8,C,10} from attribute table g_06097d68 entry [1]
 * (g_06098568 != 2) or [2] (== 2).
 * fidelity: HIGH - transcribed from asm; bsrf resolves to 0x20ff72, data ptr 0x022199da. */
#define G_ATTR ((u8 *)0x06097d68)
void *task_spawn_child_work40(void *a, void *b);

void camera_copy_pose_spawn_flagged_child(void *obj, void *src) {
    u8 *ent;

    if (M32(obj, 0x68) == 1) {
        void *ret;
        M16(obj, 0x24) = M16(src, 0);
        M16(obj, 0x26) = M16(src, 2);
        M16(obj, 0x28) = M16(src, 4);
        M32(obj, 0x68) = 0;
        ret = task_spawn_child_work40(MP((void *)0x060996d0, 0), (void *)0x022199da);
        MU8(ret, 0x0F) = 1;
        M32(MP(ret, 8), 0x18) = MU16(obj, 0x26);
    }

    ent = G_ATTR + ((M32((void *)0x06098568, 0) != 2 ? 1 : 2) << 8);
    M32(obj, 0x08) = M32(ent, 0);
    M32(obj, 0x0C) = M32(ent, 4);
    M32(obj, 0x10) = M32(ent, 8);
}
