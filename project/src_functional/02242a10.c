/* actor_tick_motion_modes [0x02242a10]: Five task states delegate ten-way motion helper and update physics/timers. */
#include "dh.h"

/* 0x02242a10 - per-object state machine, switch(*(s8)(ctx + 0x10)) states 0..4.
 * Args: ctx (r4->r13). obj = *(void **)(ctx + 8) (r14). Operates on obj's
 * physics/timer fields and on a few HWRAM globals.
 *
 * The census "680 insn" covers TWO functions. This is the real func_02242a10
 * ([0x242a10, 0x242caa)); a literal pool and then the local helper func_02242cb8
 * ([0x242cb8, ...)) follow it. m2c fails on the whole range ("Cannot find branch
 * target .L242cd6") because func_02242cb8 contains an inline braf jump table; that
 * helper is decompiled/deferred separately. func_02242a10 itself was decoded from
 * the clean disassembly + pool_read + braf_resolve:
 *   0x242a82 bsrf -> func_02228106   (takes NO args; scans a table -> count)
 *   0x242ab0 bsrf -> func_02245b48   (arg = obj+8)
 *   0x242ca6 braf -> func_0221c368   (the shared tail call, r4 = obj)
 *   bsr .L242cb8 -> func_02242cb8(ctx, obj)   (PC-relative local helper, x3)
 *   jsr @r11, r11 = mova(0x242a30)+0xFFFD353C = sound_queue_push   (PIC trap;
 *     called with command codes 0xA013xxFF, gated on *(0x060e0ddc)).
 *
 * Every exit path falls through to the shared epilogue, which restores the
 * saved registers and tail-calls func_0221c368(obj) (braf). Modelled, like the
 * other state handlers in this band, as a void function ending in that call.
 *
 * Globals (HWRAM): 0x060987bc g (read once into r9); 0x060e0ddc "effect enable"
 * flag; 0x06097410 abort flag; 0x06097418; 0x06098670; 0x06098830 counter;
 * 0x0609882c record ptr (deref, +8 -> inner, +0x0D flag).
 * obj fields: +0x34 s8, +0x58..+0x5C timers/counters, +0x60 s32, +0x68/+0x6C/
 * +0x70 s32 pos/vel/accel, +0x80 s32 flags, +0x84 s32 flags(res), +0xAE/B0/B2
 * s16 (inner). ctx fields: +0x08 obj ptr, +0x10 s8 STATE, +0x13 s8 substate.
 */

s32   actor_allocate_effect_slot(void);                               /* extern (no args) */
void *enemy_select_mode_reference(void *p);                            /* extern */
void  actor_spawn_falling_pickup(void *ctx, void *obj, s32 arg);      /* extern (02243590.c) */
void  sound_queue_push(s32 code);                           /* extern (PIC) */
void  actor_dispatch_motion_mode(void *ctx, void *obj);               /* extern (local helper) */
void  model_draw_title_composite(void *obj);                          /* extern (tail call) */

void actor_tick_motion_modes(void *ctx)
{
    void *obj   = MP(ctx, 8);
    s32   g     = M32((void *)(uintptr_t)DH_FRAME_TICK_ADDR, 0);
    s8    state = M8(ctx, 0x10);

    switch (state) {
    case 0:                             /* state 0: prelude, then run state 1 */
    case 1:
        if (state == 0) {
            M8(obj, 0x34) = (s8)actor_allocate_effect_slot();
            M8(ctx, 0x10)++;
        }
        if (M32((void *)(uintptr_t)0x06097410, 0) != 0) {
            M8(ctx, 0x10) = 3;
            break;
        }
        actor_dispatch_motion_mode(ctx, obj);
        if ((g & 7) == 0) {
            void *res = enemy_select_mode_reference((u8 *)obj + 8);
            if (res != 0) {
                g -= M32(obj, 0x60);
                if (g > 150 &&
                    (M32(res, 0x84) & 0xA00E3800) == 0 &&
                    (M32(obj, 0x80) & 2) == 0) {
                    M32(obj, 0x68) = 0x1000;
                    M32(obj, 0x6C) = 0;
                    M32(obj, 0x70) = 0x800;
                    M32(obj, 0x58) = 0;
                    M32(obj, 0x5C) = 0;
                    M8(ctx, 0x13)  = 4;
                    M32((void *)(uintptr_t)0x06097418, 0) = 1;
                    if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01313FF);
                    if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01314FF);
                    if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01316FF);
                    M8(ctx, 0x10) = 2;
                }
            }
        }
        break;
    case 2:
        if (M32((void *)(uintptr_t)0x06097410, 0) != 0) {
            M8(ctx, 0x10) = 3;
            break;
        }
        actor_dispatch_motion_mode(ctx, obj);
        {
            s32 t = M32(obj, 0x58);
            M32(obj, 0x58) = t - 1;
            if (t > 0)
                break;
        }
        M32(obj, 0x58) = 1;
        if (M32(obj, 0x68) >= 0x1000) {
            s32 v;
            M32(obj, 0x68) += M32(obj, 0x6C);
            v = M32(obj, 0x6C) + M32(obj, 0x70);
            M32(obj, 0x6C) = v;
            if (v > 0x14000) {
                M32(obj, 0x6C) = 0;
                M32(obj, 0x70) = (s16)0xFE00;
            }
            if (M32(obj, 0x68) > 0xA000)
                M32(obj, 0x68) = 0xA000;
            else if (M32(obj, 0x68) < 0x1000)
                M32(obj, 0x68) = 0xFFF;
            actor_spawn_falling_pickup(ctx, obj, M32(obj, 0x68));
            M32(obj, 0x5C)++;
            if (M32((void *)(uintptr_t)DH_PLAY_SESSION_MODE_ADDR, 0) == 1 &&
                M32((void *)(uintptr_t)DH_VIEW_REQUEST_PRIORITY_ADDR, 0) >= 9) {
                void *rec   = MP((void *)(uintptr_t)DH_VIEW_TASK_ADDR, 0);
                void *inner = MP(rec, 8);
                M32((void *)(uintptr_t)DH_VIEW_REQUEST_PRIORITY_ADDR, 0) = 9;
                M8(rec, 0x0D) = 1;
                M16(inner, 0xAE) = 0;
                M16(inner, 0xB0) = 0;
                M16(inner, 0xB2) = 0;
            }
        } else {
            M32(obj, 0x60) = g;
            M32((void *)(uintptr_t)0x06097418, 0) = 0;
            if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01315FF);
            if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01317FF);
            M8(ctx, 0x10) = 1;
        }
        break;
    case 3:                             /* state 3: prelude, then run state 4 */
    case 4:
        if (state == 3) {
            M8(ctx, 0x13) = 8;
            if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01315FF);
            if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0)) sound_queue_push(0xA01317FF);
            M8(ctx, 0x10)++;
        }
        actor_dispatch_motion_mode(ctx, obj);
        break;
    default:
        break;
    }

    model_draw_title_composite(obj);
}
