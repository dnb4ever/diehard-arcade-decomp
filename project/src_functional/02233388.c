/* scene_spawn_record_list [0x02233388]: Spawns tasks from twenty-byte records and copies position, yaw and task flags. */
#include "dh.h"
/* 0x02233388 - spawn-record list builder (called by func_02233250). Walks a list of
 * 20-byte records starting at arg1; each record->unkC is both the object template and
 * the list terminator (stop when it is 0). record->unk0 selects a spawn variant:
 *   0 -> func_0220feec   1 -> func_0220fece   2 -> func_0220ff28
 *   3 -> func_0220ff0a   4 -> func_0220ff90   5 -> func_0220ff54
 * each spawn(parent, record->unkC) returns the new object; its entity (unk8) is
 * seeded with world position (unk8/C/10 = record->unk4/6/8 << 12) and unk26 =
 * record->unkA. The object itself gets unkF/unk12/unk13 = record->unk1/2/3.
 * fidelity: HIGH - 5 jsr targets resolved from mova+add, 1 bsrf confirmed. Record
 * type > 5 is a dead path in the original (stale object pointer); modelled as a
 * no-op spawn here. */
extern void *task_spawn_child_work88(void *parent, void *template);
extern void *task_spawn_current_work6c(void *parent, void *template);
extern void *task_spawn_list_work7c(void *parent, void *template);
extern void *task_spawn_actor_work(void *parent, void *template);
extern void *task_spawn_current_workdc(void *parent, void *template);
extern void *task_spawn_current_work40(void *parent, void *template);

void scene_spawn_record_list(void *parent, void *record) {
    while (M32(record, 0xC) != 0) {
        void *tmpl = MP(record, 0xC);
        void *obj;
        switch (MU8(record, 0)) {
        case 0: obj = task_spawn_child_work88(parent, tmpl); break;
        case 1: obj = task_spawn_current_work6c(parent, tmpl); break;
        case 2: obj = task_spawn_list_work7c(parent, tmpl); break;
        case 3: obj = task_spawn_actor_work(parent, tmpl); break;
        case 4: obj = task_spawn_current_workdc(parent, tmpl); break;
        case 5: obj = task_spawn_current_work40(parent, tmpl); break;
        default: obj = parent; goto bytes;  /* dead path: original keeps stale obj */
        }
        {
            void *entity = MP(obj, 8);
            M32(entity, 8)    = M16(record, 4) << 12;
            M32(entity, 0xC)  = M16(record, 6) << 12;
            M32(entity, 0x10) = M16(record, 8) << 12;
            M16(entity, 0x26) = (s16)M16(record, 0xA);
        }
    bytes:
        M8(obj, 0xF)  = M8(record, 1);
        M8(obj, 0x12) = M8(record, 2);
        M8(obj, 0x13) = M8(record, 3);
        record = (u8 *)record + 20;
    }
}
