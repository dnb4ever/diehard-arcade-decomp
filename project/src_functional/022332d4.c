/* scene_spawn_effect_list [0x022332d4]: Spawns typed effects from eight-byte packed position records until a zero terminator. */
#include "dh_work.h"
#include "dh.h"
/* 0x022332d4 - particle-list emitter (tail-called by func_02233250). Walks a list
 * of 8-byte nodes starting at arg0; for each node, the low 16 bits of node->unk0 is
 * the effect type and the high 16 bits a parameter. Spawns the effect into the
 * shared pool via effect_pool_spawn(type, 1, &pos) with pos = {node->unk4 << 12,
 * 0x500, node->unk6 << 12}. For types 0xA/0xB the returned task's entity (unk8) gets
 * unk26 set to the high-16 parameter. Stops at the first node whose unk0 is 0.
 * fidelity: HIGH - effect_pool_spawn ptr (mova 0x2332e8 + 0x98C8 -> 0x0223cbb0). */
extern void *effect_pool_spawn(s32 type, s32 flag, void *pos);  /* effect_pool_spawn */

void scene_spawn_effect_list(void *node) {
    while (M32(node, 0) != 0) {
        s32 type = M32(node, 0) & 0xFFFF;
        s32 pos[3];
        void *task;
        pos[0] = M16(node, 4) << 12;
        pos[1] = 0x500;
        pos[2] = M16(node, 6) << 12;
        task = effect_pool_spawn(type, 1, pos);
        if (type == 0xA || type == 0xB)
            M16(TASK_PTR(task, work), 0x26) = (s16)((u32)M32(node, 0) >> 16);
        node = (u8 *)node + 8;
    }
}
