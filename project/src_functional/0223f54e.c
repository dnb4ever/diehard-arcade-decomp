/* scene_waypoint_actor_task [0x0223f54e]: Spawns a child and advances timed velocity segments from a waypoint table. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223f54e - func_0223f54e(task): scripted-waypoint mover FSM (task->0x10 is
 * the state, task->8 = entity). It spawns a child actor, then walks a 16-byte
 * waypoint table (base cached in entity->96, index entity->100) applying a
 * per-segment velocity (entity->20/24/28) to the position (entity->8/0xC/0x10)
 * for a per-segment duration (entity->88). Every path tail-calls func_02246282
 * (the per-actor draw).
 *   state 0: entity->48 = 0x0601; entity->96 = 0x0608d3fc;
 *            s = alloc_slot();                      // func_02228106
 *            if (s > 0) {
 *              o = spawn_obj_a(task, &handler_0223f700);
 *              entity->104 = *(o+8); (*(o+8))->52 = s;
 *            }
 *            entity->0x10++;                         // -> falls through to state 1
 *   state 1: if ((g_06098580 & 0x10) == 0) break;   // wait for gate
 *            i = entity->100++; rec = entity->96 + i*16;
 *            entity->88 = rec[12]; entity->92 = 16;
 *            entity->20 = rec[0]; entity->24 = rec[4]; entity->28 = rec[8];
 *            (entity->104)->0x80 &= ~1;
 *            entity->0x10++;
 *   state 2: c = entity->88; entity->88 = c - 1;
 *            if (c != 0) { entity->8 += entity->20; 0xC += 24; 0x10 += 28; }
 *            else {                                  // segment finished
 *              i = entity->100++; rec = entity->96 + i*16; v = rec[12];
 *              if (v > 0) { entity->88 = v; entity->20=rec[0];24=rec[4];28=rec[8]; }
 *              else if ((~v & 8) == 0) {             // loop/hold code
 *                entity->0x80 |= ~v; entity->88 = ~v; entity->0x10 = 3;
 *              } else {                              // terminate code
 *                g_06098580 &= ~0x10; entity->100 = 0;
 *                (entity->104)->0x80 |= 1; entity->0x10 = 1;
 *              }
 *            }
 *   state 3: if ((entity->88 & entity->0x80) == 0) {   // wait flag
 *              i = entity->100++; rec = entity->96 + i*16;
 *              entity->88 = rec[12]; entity->20=rec[0];24=rec[4];28=rec[8];
 *              entity->0x10 = 2;
 *            }
 *   func_02246282(entity);                          // tail (braf)
 * fidelity: HIGH - 3 computed targets resolve (func_02228106 alloc, spawn_obj_a,
 * and the 0x246282 tail). State 0 FALLS THROUGH into state 1 in the same tick
 * (no branch between them). r6 is the constant 0. The spawn descriptor/handler
 * passed to spawn_obj_a is the ROM address 0x0223f700 (mova 0x23f59c + 0x164).
 * g_06098580 is a 32-bit gate word (bit 0x10). rec[12] in state 2 is a signed
 * control value: (~v & 8)==0 means bit 3 of v is set. Waypoint index uses the
 * pre-increment value (idx then idx+1 stored). entity->96 base holds a ROM
 * pointer (0x0608d3fc). */
extern u8   actor_allocate_effect_slot(void);                 /* 0x02228106 alloc HUD/actor slot */
extern void *task_spawn_child_work88(void *task, void *desc); /* 0x0220feec spawn_obj_a */
extern void actor_draw_transformed(void *entity);         /* 0x02246282 per-actor draw */
extern u32  g_scene_event_flags;                          /* gate word (bit 0x10) */

void scene_waypoint_actor_task(void *task)
{
    void *entity = TASK_PTR(task, work);
    s32 state = TASK_S8(task, state);

    switch (state) {
    case 0: {
        s32 s;
        M16(entity, 48) = 0x0601;
        M32(entity, 96) = (s32)0x0608d3fc;
        M16(entity, 38) = 0;
        s = (s8)actor_allocate_effect_slot();
        if (s > 0) {
            void *o = task_spawn_child_work88(task, (void *)(uintptr_t)0x0223f700U);
            void *child = MP(o, 8);
            M32(entity, 104) = (s32)(uintptr_t)child;
            M8(child, 52) = (s8)s;
        }
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    /* fallthrough */
    case 1: {
        u8 *rec;
        s32 i;
        void *child;
        if ((g_scene_event_flags & 0x10) != 0) {
            i = M32(entity, 100);
            M32(entity, 100) = i + 1;
            rec = (u8 *)(uintptr_t)M32(entity, 96) + i * 16;
            M32(entity, 88) = M32(rec, 12);
            M32(entity, 92) = 16;
            M32(entity, 20) = M32(rec, 0);
            M32(entity, 24) = M32(rec, 4);
            M32(entity, 28) = M32(rec, 8);
            child = (void *)(uintptr_t)M32(entity, 104);
            M32(child, 0x80) &= ~1;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    case 2: {
        s32 c = M32(entity, 88);
        M32(entity, 88) = c - 1;
        if (c != 0) {
            M32(entity, 8)  += M32(entity, 20);
            M32(entity, 0xC) += M32(entity, 24);
            M32(entity, 0x10) += M32(entity, 28);
        } else {
            s32 i = M32(entity, 100);
            u8 *rec;
            s32 v;
            M32(entity, 100) = i + 1;
            rec = (u8 *)(uintptr_t)M32(entity, 96) + i * 16;
            v = M32(rec, 12);
            if (v > 0) {
                M32(entity, 88) = v;
                M32(entity, 20) = M32(rec, 0);
                M32(entity, 24) = M32(rec, 4);
                M32(entity, 28) = M32(rec, 8);
            } else if ((~v & 8) == 0) {
                M32(entity, 0x80) |= ~v;
                M32(entity, 88) = ~v;
                TASK_S8(task, state) = 3;
            } else {
                void *child;
                g_scene_event_flags &= ~0x10u;
                M32(entity, 100) = 0;
                child = (void *)(uintptr_t)M32(entity, 104);
                M32(child, 0x80) |= 1;
                TASK_S8(task, state) = 1;
            }
        }
        break;
    }
    case 3:
        if ((M32(entity, 88) & M32(entity, 0x80)) == 0) {
            s32 i = M32(entity, 100);
            u8 *rec;
            M32(entity, 100) = i + 1;
            rec = (u8 *)(uintptr_t)M32(entity, 96) + i * 16;
            M32(entity, 88) = M32(rec, 12);
            M32(entity, 20) = M32(rec, 0);
            M32(entity, 24) = M32(rec, 4);
            M32(entity, 28) = M32(rec, 8);
            TASK_S8(task, state) = 2;
        }
        break;
    default:
        break;
    }

    actor_draw_transformed(entity);
}
