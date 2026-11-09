/* scene_transition_timeline_task [0x022463c8]: Advances the active transition timeline and spawns matured actor records. */
#include "dh_work.h"
#include "dh.h"
/* func_022463c8 (cart 0x2463c8, true prologue entry 0x2463b8 / rt 0x0604a3c8) - the scene
 * TRANSITION-TIMELINE interpreter. arg0 = task; task->unk8 = content entity. Walks the active
 * timeline script (cursor record *g_060973F0; column cursor g_060973E0, step width g_060973E4,
 * row limit g_060973E8), gated per-frame by the player-channel word g_06098564 and driven by the
 * entity's unk40 frame-timer. For each matured record it spawns a timeline object (func_02227b60)
 * and copies the record's fields into it, then advances to the next record (cursor += 0x14). When
 * the next record's unk4 (link) is 0 the script is finished: it rebinds the task handler
 * task->unk0 = func_022465fc, whose next tick sets the content-entity unk3C bit3 (the bit
 * func_02232610 state 2 waits on). See notes/port/scene_content_completion.md sec 1d.
 *
 * Confidence: HIGH. find_entry reports 0x2463c8 but the register saves start at 0x2463b8 (r12 is
 * the live constant 7 set at 0x2463be); decoded from the true entry. The lone computed call
 * (func_02227b60) and both mova+add data pointers resolved (braf_resolve/ptr_resolve); every
 * offset/width hand-checked vs capstone disasm 0x2463b8-0x2465fa. g_06098564 case 0 early-returns
 * (pure-attract gate == 0) - the exact scene-3 stall condition.
 */
extern s32   g_060973E0, g_060973E4, g_060973E8, g_060973EC;
extern void *g_060973F0;      /* -> current timeline script record (cursor) */
extern s32   g_ready_player_mask;      /* player-channel gate (0 => timeline frozen) */
extern s32   g_06098808;      /* brightness-table index */
extern u8    g_0609196C[];    /* object-class table, entry stride 4 (-> class struct, unk18=width) */
extern u8    g_bios_status_buffer[];    /* per-channel brightness table */
extern s16   g_0608DDC4[];    /* record-delay table (s16) */
extern s16   g_060E1214;
extern void *actor_spawn_dynamic_slot(void *desc);   /* spawn timeline object, returns the entity */

void scene_transition_timeline_task(void *task) {
    for (;;) {
        void *entity;
        u8   *cursor;
        s32   t;

        if (g_060973E0 > g_060973E8)
            return;
        if (g_060973E0 + g_060973E4 > 40)
            return;

        entity = TASK_PTR(task, work);
        cursor = (u8 *)g_060973F0;

        t = M32(entity, 0x40);                  /* per-record frame timer */
        M32(entity, 0x40) = t - 1;
        if (t > 0)
            return;

        switch (g_ready_player_mask) {
        case 0:
            return;                             /* channels idle: timeline frozen */

        case 1:
        case 2:
            if (MU8(cursor, 9) & 0x20) {        /* record is a pure "wait/advance" (no spawn) */
                u8 *nx = cursor + 0x14;
                if (M32(nx, 4) == 0) {
                    TASK_PTR(task, handler) = (void *)0x0604A5FCUL;   /* -> func_022465fc (timeline done) */
                    return;
                }
                M32(entity, 0x40) = MU16(nx, 0x10);
                g_060973E8 = MU8(nx, 0xB);
                g_060973E4 = M32(MP(g_0609196C, MU8(nx, 8) * 4), 0x18);
                g_060973F0 = nx;
                return;
            }
            /* fall through */

        default: {                              /* case 3 (and any other): spawn a timeline object */
            void *sp  = actor_spawn_dynamic_slot((void *)0x0604A678UL);   /* spawn descriptor */
            void *sub = MP(sp, 8);
            s8    va;
            s32   v;

            M32(sub, 8)     = M16(cursor, 0) << 12;
            M32(sub, 0xC)   = 0;
            M32(sub, 0x10)  = M16(cursor, 2) << 12;
            M32(sub, 0x118) = M32(cursor, 4);
            M16(sub, 0x5E)  = (s16)(M16(cursor, 0xC) & 0xFF);
            M8(sub, 0x4F)   = (s8)((M16(cursor, 0xC) & 0xFF00) >> 8);
            M32(sub, 0xE4)  = MU8(cursor, 8);

            v = (MU8(g_bios_status_buffer, g_06098808) & 7) - 3 + (MU8(cursor, 9) & 7);
            if (v <= 0)
                v = 0;
            else if (v >= 8)
                v = 7;
            M32(sub, 0xE8)  = v;

            M16(sub, 0x144) = (s16)((MU8(cursor, 9) >> 6) & 3);
            M16(sub, 0x56)  = M16(cursor, 0xE);
            M32(sub, 0x100) = M8(cursor, 0x12);
            M32(sub, 0x104) = M8(cursor, 0x13);
            M8(sp, 0xF)     = M8(cursor, 9);
            M8(sp, 0x12)    = (s8)M16(cursor, 0xC);

            va = M8(cursor, 0xA);
            if (va != 0 && (M16(g_0608DDC4, va * 2) + g_060E1214) > g_060973EC)
                va = 0;
            M8(sub, 0x45)   = va;

            cursor += 0x14;                     /* advance to the next record */
            M32(sub, 0xF8)  = g_060973E4;
            g_060973E0     += g_060973E4;

            if (M32(cursor, 4) == 0) {
                TASK_PTR(task, handler) = (void *)0x0604A5FCUL;   /* -> func_022465fc (timeline done) */
                return;
            }
            M32(entity, 0x40) = MU16(cursor, 0x10);
            g_060973E8 = MU8(cursor, 0xB);
            g_060973E4 = M32(MP(g_0609196C, MU8(cursor, 8) * 4), 0x18);
            g_060973F0 = cursor;
            continue;                           /* process the next record */
        }
        }
    }
}
