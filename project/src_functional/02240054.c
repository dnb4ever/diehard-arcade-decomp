/* actor_dispatch_action_with_effects [0x02240054]: Resets action fields, dispatches its type/action handler and updates requested effect state. */
#include "dh.h"
/* 0x02240054 - func_02240054: per-actor state tick / transition driver.
 * real fn [0x240054, 0x2401d6) (census 0x24003e is pool-contaminated). Prologue
 * pushes r10..r14 + two stack locals (the state fn writes one; the other caches
 * obj->0x34). Returns the s32 the dispatched state function produced.
 *   M8(obj,0x48) = 0; M8(obj,0x41) = 1; M32(obj,0x88) = 0;
 *   idx = M16(obj,0xBA) & 0x3F;
 *   fn  = g_060918a0[idx][M32(obj,0x108)];          // two-level fn-ptr table
 *   fn(obj, &out_val);                              // writes out_val
 *   if (g_060e0ddc != 0) sound_queue_push((g_0608da48[idx] << 8) | 0xA00000FF);
 *   if (M8(obj,0x4F) == 0) {
 *       d = M16(obj,0x56);
 *       if (d < 0x96) {                             // "arrived" handling
 *           saved34 = M8(obj,0x34);
 *           M8(obj,0x6D) = 4; func_02245c88(obj, d); M8(obj,0x6D) = 0;
 *           M8(obj,0x4F) = 1; M32(obj,0xFC) = 0; M8(obj,0x46) = 1;
 *           func_0222ca54(obj); func_0223ed94(arg0, obj);
 *           e = func_0220ff54(arg0, (void*)0x0223EE1A);
 *           M8(M32(e,8),0x34) = M8(obj,0x34);
 *           if (func_0223e4a0(e) != 0) {
 *               q = (void*)M32(func_0223cbb0(5,3,0),8);
 *               rec = (s32*)(0x06097E08 + (saved34 << 8));
 *               M32(q,8)=rec[0]; M32(q,12)=rec[1]; M32(q,16)=rec[2];
 *               func_0223a924(M8(q,0x34), 3, (u8*)q+8, obj);
 *           }
 *       } else if (d < 0xD6) {
 *           func_0223edce(arg0, obj, 7 & ~((d - 0x40) >> 3));
 *       }
 *   }
 *   func_022418fa(arg0, obj, out_val);
 *   func_02241b2c(obj);
 *   M32(obj,0x14) = 0; M32(obj,0x18) = 0; M32(obj,0x1C) = 0;
 *   return out_val;
 * fidelity: HIGH - every instruction traced; all 11 bsrf resolve cleanly (no
 * phantoms). The fn-ptr call reads a table-of-tables at g_060918a0 indexed by
 * (obj->0xBA & 0x3F) then obj->0x108. 0x34/0x56 signed; 0xBA masked to 6 bits;
 * the 0x0223EE1A argument is a fixed ROM data pointer (mova 0x240100 + -0x12E6).
 * The record index (saved34 << 8) uses the SIGNED 0x34 cached before func_02245c88. */
typedef void (*state_fn)(void *obj, s32 *out);

extern void  sound_queue_push(s32 cmd);                      /* 0x02215f6c sound_queue_push */
extern void  enemy_load_action_descriptor(void *obj, s16 arg);           /* 0x02245c88 */
extern void  actor_load_part_ids(void *obj);                    /* 0x0222ca54 */
extern void  effect_spawn_eight_frame_flash(s32 arg0, void *obj);          /* 0x0223ed94 */
extern void *task_spawn_current_work40(s32 arg0, void *rec);          /* 0x0220ff54 */
extern s32   effect_pool_make_room(void *e);                      /* 0x0223e4a0 */
extern void *effect_pool_spawn(s32 a, s32 b, s32 c);          /* 0x0223cbb0 */
extern void  effect_pool_update_pose(s32 kind, s32 b, void *p, void *obj); /* 0x0223a924 */
extern void  effect_spawn_rising_flash(s32 arg0, void *obj, s32 arg2);/* 0x0223edce */
extern void  actor_finish_arrival(s32 arg0, void *obj, s32 v);   /* 0x022418fa */
extern void  actor_latch_action_completion(void *obj);                    /* 0x02241b2c */
extern state_fn *g_060918a0[];                            /* table of per-type state-fn arrays */
extern s16   g_0608da48[];                                /* s16 table, idx (obj->0xBA & 0x3F) */
extern s32   g_sound_effects_enabled;                                  /* s32 gate flag */

s32 actor_dispatch_action_with_effects(s32 arg0, void *obj)
{
    s32 out_val;
    s32 idx;
    state_fn fn;

    M8(obj, 0x48) = 0;
    M8(obj, 0x41) = 1;
    M32(obj, 0x88) = 0;

    idx = M16(obj, 0xBA) & DH_ACTOR_TYPE_MASK;
    fn  = g_060918a0[idx][M32(obj, 0x108)];
    fn(obj, &out_val);

    if (g_sound_effects_enabled != 0)
        sound_queue_push(((s32)g_0608da48[idx] << 8) | DH_SOUND_CUE_BASE);

    if (M8(obj, 0x4F) == 0) {
        s32 d = M16(obj, 0x56);
        if (d < 0x96) {
            s32 saved34 = M8(obj, 0x34);
            M8(obj, 0x6D) = 4;
            enemy_load_action_descriptor(obj, (s16)d);
            M8(obj, 0x6D) = 0;
            M8(obj, 0x4F) = 1;
            M32(obj, 0xFC) = 0;
            M8(obj, 0x46) = 1;
            actor_load_part_ids(obj);
            effect_spawn_eight_frame_flash(arg0, obj);
            {
                void *e = task_spawn_current_work40(arg0, (void *)(uintptr_t)0x0223EE1AU);
                M8((void *)(uintptr_t)M32(e, 8), 0x34) = (s8)M8(obj, 0x34);
                if (effect_pool_make_room(e) != 0) {
                    void *q   = (void *)(uintptr_t)M32(effect_pool_spawn(5, 3, 0), 8);
                    s32  *rec = (s32 *)(uintptr_t)(0x06097E08U + (saved34 << 8));
                    M32(q, 8)  = rec[0];
                    M32(q, 12) = rec[1];
                    M32(q, 16) = rec[2];
                    effect_pool_update_pose(M8(q, 0x34), 3, (u8 *)q + 8, obj);
                }
            }
        } else if (d < 0xD6) {
            effect_spawn_rising_flash(arg0, obj, 7 & ~((d - 0x40) >> 3));
        }
    }

    actor_finish_arrival(arg0, obj, out_val);
    actor_latch_action_completion(obj);
    M32(obj, 0x14) = 0;
    M32(obj, 0x18) = 0;
    M32(obj, 0x1C) = 0;
    return out_val;
}
