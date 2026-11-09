#include "dh_work.h"
/* process_spawn_list [0x0222a5d0]: Consumes twenty-four-byte spawn records and initializes dynamic or fixed actor slots. */
#include "dh.h"
/* 0x0222a5d0  process_spawn_list - walk a spawn descriptor list and spawn one actor per entry.
 * arg0->unk30 points at the current 24-byte descriptor; each entry is processed, then the
 * cursor is advanced and written back to arg0->unk30.
 *
 * Per entry (cur):
 *   type = (s8)cur->unk0.  Spawn the actor task:
 *     type == 0 -> func_02227b60(&func_0222a300)   (dynamic-id, arm-state handler)
 *     type == 7 -> func_02227b60(&func_0222a970)   (dynamic-id, anim-init handler)
 *     else      -> func_02227bc4(type - 1)         (fixed-id spawner at slot type-1)
 *   (in the fixed-id path the compiler also materialises &func_0222a300 in r5, but the fixed-id
 *    spawner ignores it - it uses its own template - so it is not passed here.)
 *   obj = result->unk8 (the spawned entity). Copy descriptor fields into obj:
 *     obj->unkD0 = cur->unk14 (anim/config record), obj->unk8 = cur->unk8,
 *     obj->unkC = cur->unkC, obj->unk10 = cur->unk10,
 *     obj->unk5E = (s8)cur->unk1 (as 16-bit), obj->unk4F = (s8)cur->unk2,
 *     obj->unk98 = (s16)((u32)cur->unk4 >> 16).
 *   func_0222ca54(obj);
 *   if ((s8)obj->unk34 >= 3)  { if ((s8)cur->unk3 != 0) func_0223e3d0(obj, (s8)cur->unk3); }
 *   else {
 *     idx = obj->unk34 - 1;
 *     obj->unk56 = g_06099788[idx];                       (seed HP/ready word)
 *     if (g_060E0E38[idx*16] == 1 && g_06098668 != 52) func_0223dd96((s8)obj->unk34);
 *   }
 *   cur += 24; loop while (cur->unk4 & 0xFFFF) == 0 and cur->unk14 != 0.
 *
 * fidelity: HIGH - two indirect spawn calls resolved to fixed fps (ptr_resolve: r8=func_02227bc4,
 * r12=func_02227b60); three bsrf tails resolved (0x22ca54 / 0x23dd96 / 0x23e3d0). unk34/4F byte,
 * unk5E/56/98 word, unk8/C/10/D0 long; g_06099788 s16[], g_060E0E38 16-byte records. */
extern void *actor_spawn_dynamic_slot(void *state_handler);   /* dynamic-id spawn registrar */
extern void *actor_spawn_fixed_slot(s32 fixed_idx);         /* fixed-id spawn registrar   */
extern void  actor_spawn_arm_task(void *task);            /* actor arm-state handler    */
extern void  actor_anim_init_task(void *task);            /* actor anim-init handler    */
extern void  actor_load_part_ids(void *obj);
extern void  player_restore_saved_action(s32 slot);
extern void  aim_effect_spawn(void *obj, s32 arg);
extern s16   g_06099788[];                          /* per-slot HP/ready seed words */
extern u8    g_060E0E38[];                           /* 16-byte per-slot status records */
extern s32   g_scene_index;

void process_spawn_list(void *arg0) {
    void *cur = SEQUENCE_MP(arg0, spawn_cursor);
    while (M32(cur, 20) != 0) {
        s8 type = M8(cur, 0);
        void *res;
        void *obj;
        s32 idx;

        if (type == 0)
            res = actor_spawn_dynamic_slot((void *)actor_spawn_arm_task);
        else if (type == 7)
            res = actor_spawn_dynamic_slot((void *)actor_anim_init_task);
        else
            res = actor_spawn_fixed_slot(type - 1);

        obj = MP(res, 8);
        ACTOR_M32(obj, animation_script) = M32(cur, 0x14);
        ACTOR_M32(obj, position_x)  = M32(cur, 0x8);
        ACTOR_M32(obj, position_y)  = M32(cur, 0xC);
        ACTOR_M32(obj, position_z) = M32(cur, 0x10);
        ACTOR_M16(obj, model_index) = (s8)M8(cur, 1);
        ACTOR_M8(obj, model_variant)  = (s8)M8(cur, 2);
        M32(obj, 0x98) = (s16)(MU32(cur, 4) >> 16);

        actor_load_part_ids(obj);

        if ((s8)ACTOR_M8(obj, slot_id) >= 3) {
            s8 u3 = M8(cur, 3);
            if (u3 != 0)
                aim_effect_spawn(obj, u3);
        } else {
            idx = (s8)ACTOR_M8(obj, slot_id) - 1;
            ACTOR_M16(obj, health) = g_06099788[idx];
            if (g_060E0E38[idx * 16] == 1 && g_scene_index != DH_SCENE_SAVED_ACTION_BYPASS)
                player_restore_saved_action((s8)ACTOR_M8(obj, slot_id));
        }

        cur = (u8 *)cur + 24;
        if ((MU32(cur, 4) & 0xFFFF) != 0)
            break;
    }
    SEQUENCE_MP(arg0, spawn_cursor) = cur;
}
