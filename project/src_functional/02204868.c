/* Round 4: original PIC task handlers use HWRAM addresses; evidence work/decomp4/live_calls.asm. */
/* attract_update_task [0x02204868]: Services credit/join edges and scene stepping before selecting the round controller. */
#include "dh_work.h"
#include "dh.h"
/* 0x02204868 (attract_handler) - THE attract->play TRANSITION PIVOT (port unblock #1).
 *
 * Installed as the game task's handler (task->unk0) by the scene bootstrap func_022058a8
 * (*g_06098678). Runs once per frame while the machine is in attract / coin-wait:
 *   1. func_022045f8(task)            - services the "credit just inserted / join" edges.
 *   2. (*g_0609867c)()              - steps the currently-selected demo/attract scene.
 *   3. reads the two START edge-flags g_06098688 (P1) / g_06098689 (P2).
 *
 * MORPH: if EITHER start flag is set (sum != 0) the handler OVERWRITES ITS OWN TASK's
 * handler pointer *(void**)task with the ROUND CONTROLLER, then returns; from the next
 * frame the task IS the round controller (self-morph, no task alloc/free):
 *     credits present (g_06099598 + g_06099594 != 0) -> *task = round_controller_join  0x02204afa
 *     else (free-play / no coin)                     -> *task = round_controller_dual  0x02205232
 * After writing the morph pointer it does the per-player "START pressed" bring-up:
 * join state g_06098674/75 = 3, clears the start flag, blanks the HUD lines, arms the
 * player countdown entity->unk40/unk44 = 0x27f, and notifies the system task (*g_06000688)(0x20/0x21).
 *
 * If NEITHER start flag is set it stays in attract and, for each player not yet joined
 * (g_06098674/75 == 0), reads the peripheral (*g_06000660) and prints the "insert coin /
 * credits" countdown digit from the string table g_06090304[idx*4], where idx is the
 * per-player countdown byte at g_060987d8+3 (P1) / g_060987dc+3 (P2); it reprints only when
 * the digit changes (compared against the latched g_0609868c / g_0609868d).
 *
 * All computed branches hand-resolved (ptr_resolve/braf_resolve): morph mova+add 0x2048d0+0x22a
 * ->0x204afa and 0x20490c+0x926->0x205232; bsrf->func_02238134 (0x238134), func_02238176 (0x238176),
 * sound_queue_push (0x215f6c). The two stray "bsrf" capstone emitted at 0x2048fc/0x2049f8 are
 * literal-pool words (0x0003386c / 0x00033860), NOT instructions. No ROM jump table here.
 * HUD print idiom: g_060997a4=color, g_060997a8=mode, (*g_0609029c)(str). fidelity: HIGH,
 * hand-traced vs m2c_fn --asm instruction-for-instruction. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))

typedef void (*fn_v)(void);
typedef void (*fn_i)(s32);
#define CALL(a, T) (*(T *)(uintptr_t)(a))

extern void attract_service_credit_join(void *buf);     /* 0x022045f8 credit/join edge service */
extern void hud_draw_status_panel(s32 a);              /* 0x02238134 */
extern void hud_draw_status_digit(s32 a, s32 b);       /* 0x02238176 HUD/countdown display */
extern void sound_queue_push(s32 req);    /* 0x02215f6c */

static void hud_print(s32 color, s32 mode, s32 str) {
    R32A(DH_UI_CURSOR_COLUMN_ADDR) = color;
    R32A(DH_UI_CURSOR_ROW_ADDR) = mode;
    CALL(0x0609029c, fn_i)(str);
}

void attract_update_task(void *task) {
    void *entity = TASK_PTR(task, work);

    attract_service_credit_join(task);
    CALL(0x0609867c, fn_v)();                       /* step current attract scene */

    if ((R8A(0x06098688) + R8A(0x06098689)) != 0) {
        /* ---- a START flag is set: morph this task into the round controller ---- */
        if ((R32A(0x06099598) + R32A(0x06099594)) != 0) {
            if (R16A(DH_ACTOR_POSE_SET_ADDR) == 1)
                hud_draw_status_panel(1);
            *(void **)task = (void *)(uintptr_t)0x06008afa;   /* round_controller_join */
        } else {
            *(void **)task = (void *)(uintptr_t)0x06009232;   /* round_controller_dual */
            hud_draw_status_digit(7, 9);
            if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                sound_queue_push((s32)DH_SOUND_LATCH_BASE);
        }

        if (R8A(0x06098688) != 0) {                 /* P1 start edge */
            R16A(0x0609880e) = 0;
            RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 3;
            RU8A(0x06098688) = 0;
            RU8A(0x0609868a) = 0;
            hud_print(3, 1, 0x06050d84);
            hud_print(23, 1, 0x06050d84);
            M32(entity, 0x40) = 0x27f;
            CALL(0x06000688, fn_i)(0x20);
        }
        if (R8A(0x06098689) != 0) {                 /* P2 start edge */
            R16A(0x06098812) = 0;
            RU8A(0x06098675) = 3;
            RU8A(0x06098689) = 0;
            RU8A(0x0609868b) = 0;
            hud_print(3, 1, 0x06050d84);
            hud_print(23, 1, 0x06050d84);
            M32(entity, 0x44) = 0x27f;
            CALL(0x06000688, fn_i)(0x21);
        }
        return;
    }

    /* ---- no start pressed: attract "insert coin" countdown servicing ---- */
    if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 0) {                     /* P1 not joined */
        CALL(0x06000660, fn_i)(0x060987d8);
        if ((R32A(DH_FRAME_TICK_ADDR) & 0xf) == 0) {
            if ((R32A(DH_FRAME_TICK_ADDR) & 0x1f) == 0)
                hud_print(3, 1, R32A(0x06090304 + (s32)RU8A(0x060987db) * 4));
            else
                hud_print(3, 1, 0x06050d84);
        }
        if (RU8A(0x060987db) > RU8A(0x0609868c))
            hud_print(3, 1, R32A(0x06090304 + (s32)RU8A(0x060987db) * 4));
        RU8A(0x0609868c) = RU8A(0x060987db);
    }
    if (RU8A(0x06098675) == 0) {                     /* P2 not joined */
        CALL(0x06000660, fn_i)(0x060987d8);
        if ((R32A(DH_FRAME_TICK_ADDR) & 0xf) == 0) {
            if ((R32A(DH_FRAME_TICK_ADDR) & 0x1f) == 0)
                hud_print(23, 1, R32A(0x06090304 + (s32)RU8A(0x060987df) * 4));
            else
                hud_print(23, 1, 0x06050d84);
        }
        if (RU8A(0x060987df) > RU8A(0x0609868d))
            hud_print(23, 1, R32A(0x06090304 + (s32)RU8A(0x060987df) * 4));
        RU8A(0x0609868d) = RU8A(0x060987df);
    }
}
