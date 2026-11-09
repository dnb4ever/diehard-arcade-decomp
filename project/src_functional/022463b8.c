/* enemy_wave_spawn_task [0x022463b8]: Advances timed wave records and spawns enemies once their screen triggers are eligible. */
#include "dh_work.h"
#include "dh.h"
/* 0x022463b8  scripted enemy-wave spawner (steady-state handler). task = wave
 * task, ctrl = MP(task,8) is the spawn controller, entry = g_060973f0 is the
 * current 20-byte script record. Globals: g_060973e0 accumulated screen-X,
 * g_060973e4 current record width, g_060973e8 trigger-X, g_06098564 run mode.
 *
 * While the next record is on-screen (g_060973e0 <= g_060973e8 and
 * g_060973e0 + width <= 0x28), tick the controller's inter-spawn delay (unk40);
 * when it reaches 0, dispatch on g_06098564:
 *   0 -> stop. 1/2 -> if this record is a marker (unk9&0x20) just advance to the
 *   next record (or finish via func_022465fc when the chain ends); otherwise fall
 *   through and spawn. 3/default -> spawn: register a new actor
 *   (func_02227b60(func_02246678)), fill its position (unk0,unk2 << 12), id/flags
 *   (unkC split into unk5E/unk4F), type unk8, depth-sorted layer unkE8 (clamped
 *   0..7 from g_060987ec[g_06098808] and the record flags), facing unk144, hp
 *   unk56, script ptrs unk100/unk104, palette unk45 (zeroed past the g_060973ec
 *   budget), and advance the accumulator; then walk to the next record and loop.
 *
 * bounds: [0x022463b8, 0x2465fc); func_02246308 (the init entry) tail-jumps here.
 * notes: func_02227b60 takes only the template; the extra call-site regs (and so
 *   this handler's would-be arg1/arg2) are dead. Field widths from m2c casts /
 *   fn.py --asm. fidelity: HIGH. */
extern s32  g_060973e0;        /* accumulated spawn X */
extern s32  g_060973e4;        /* current record width */
extern s32  g_060973e8;        /* trigger X */
extern s32  g_060973ec;        /* palette budget */
extern void *g_060973f0;       /* current script record */
extern s32  g_ready_player_mask;        /* run mode */
extern void *g_0609196c[];     /* per-type records (->unk18 = width) */
extern u8   g_bios_status_buffer_legacy[];      /* stage depth table */
extern s32  g_06098808;        /* depth-table index */
extern s16  g_0608ddc4[];      /* palette-cost table */
extern s16  g_060e1214;        /* palette base cost */

void *actor_spawn_dynamic_slot(void *state_handler);        /* 0x02227b60 spawn registrar */
void enemy_initialize_spawn_task(void *task);                  /* 0x02246678 spawned-actor handler */
void enemy_wave_test_complete(void *task);                  /* 0x022465fc wave-done handler */

void enemy_wave_spawn_task(void *task) {
    while (g_060973e0 <= g_060973e8 && (g_060973e0 + g_060973e4) <= 0x28) {
        void *ctrl = TASK_PTR(task, work);
        void *entry = g_060973f0;
        s32 cd = M32(ctrl, 0x40) - 1;
        M32(ctrl, 0x40) = cd;
        if ((cd + 1) > 0) {
            return;
        }
        switch (g_ready_player_mask) {
        case 0:
            return;
        case 1:
        case 2:
            if (MU8(entry, 9) & 0x20) {
                void *next = (u8 *)entry + 0x14;
                if (M32(next, 4) == 0) {
                    TASK_PTR(task, handler) = (void *)enemy_wave_test_complete;
                    return;
                }
                M32(ctrl, 0x40) = (u16)MU16(next, 0x10);
                g_060973e8 = (u8)MU8(next, 0xB);
                g_060973e4 = M32(g_0609196c[(u8)MU8(next, 8)], 0x18);
                g_060973f0 = next;
                return;
            }
            /* fall through */
        default:
        case 3: {
            void *obj = actor_spawn_dynamic_slot((void *)enemy_initialize_spawn_task);
            void *data = MP(obj, 8);
            void *next;
            s32 depth;
            s8 pal;

            M32(data, 0x08) = (s32)M16(entry, 0) << 0xC;
            M32(data, 0x0C) = 0;
            M32(data, 0x10) = (s32)M16(entry, 2) << 0xC;
            M32(data, 0x118) = M32(entry, 4);
            M16(data, 0x5E) = (s16)(M16(entry, 0xC) & 0xFF);
            M8(data, 0x4F) = (s8)((M16(entry, 0xC) & 0xFF00) >> 8);
            M32(data, 0xE4) = (u8)MU8(entry, 8);

            depth = (g_bios_status_buffer_legacy[g_06098808] & 7) - 3 + (MU8(entry, 9) & 7);
            if (depth > 0) {
                if (depth >= 8) {
                    depth = 7;
                }
            } else {
                depth = 0;
            }
            M32(data, 0xE8) = depth;

            M16(data, 0x144) = (s16)((MU8(entry, 9) >> 6) & 3);
            M16(data, 0x56) = (s16)M16(entry, 0xE);
            M32(data, 0x100) = (s32)M8(entry, 0x12);
            M32(data, 0x104) = (s32)M8(entry, 0x13);
            M8(obj, 0x0F) = (s8)MU8(entry, 9);
            M8(obj, 0x12) = (s8)M16(entry, 0xC);

            pal = M8(entry, 0xA);
            if (pal != 0 && (g_0608ddc4[pal] + g_060e1214) > g_060973ec) {
                pal = 0;
            }
            M8(data, 0x45) = pal;

            next = (u8 *)entry + 0x14;
            M32(data, 0xF8) = g_060973e4;
            g_060973e0 += g_060973e4;
            if (M32(next, 4) == 0) {
                TASK_PTR(task, handler) = (void *)enemy_wave_test_complete;
                return;
            }
            M32(ctrl, 0x40) = (u16)MU16(next, 0x10);
            g_060973e8 = (u8)MU8(next, 0xB);
            g_060973e4 = M32(g_0609196c[(u8)MU8(next, 8)], 0x18);
            g_060973f0 = next;
            break;
        }
        }
    }
}
