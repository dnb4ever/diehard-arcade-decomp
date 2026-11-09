/* Round 4: Original PIC task handlers are runtime HWRAM addresses; restore attract/join/dual transitions. Evidence: work/decomp4/live_calls.asm. */
/* round_join_countdown_task [0x02204afa]: Drives credit-present player join countdowns and installs the next round controller. */
#include "dh_work.h"
#include "dh.h"
/* 0x02204afa (round_controller_join) - the "credits present" morph target of attract_handler
 * (port unblock #2a). Self-morphing task: once attract_handler writes 0x02204afa into *task, THIS
 * runs each frame. It drives the two per-player PRESS-START / join countdowns (entity->unk40 = P1,
 * entity->unk44 = P2) and decides the next task state by overwriting *task:
 *
 *   - a player's join byte reaches 4 (confirmed)     -> *task = attract_handler       (0x02204868)
 *   - a player's countdown expires (<= 0)            -> *task = attract_handler       (0x02204868)
 *   - the OTHER player presses start mid-countdown   -> *task = round_controller_dual (0x02205232)
 *
 * While counting it reads the peripheral (*g_06000660), prints the countdown digit (HUD thunks
 * *g_0609029c / *g_060902a0, string table g_06090304), ramps the timer on the frame-tick bits of
 * g_06098794/g_06098734, and - when g_0609979a==1 and (unk+0x82)&0xff==0 and the scene is armed
 * (g_06098670==1 && g_06098830>=4) - performs the LATE-JOIN PLAYER SPAWN: it grabs the player
 * actor (*g_0609882c)->unk8, marks it (g_06098830=4, (*g_0609882c)->unkD=1, actor->unkAE=1/2) and
 * randomizes its spawn coords actor->unkB0/unkB2 = func_02210020() (rng_next). On a countdown
 * expiry it also issues the resident scene command func_0220aca8(*(s16*)(*g_0609866c + 40)).
 *
 * NO ROM JUMP TABLE: the "32-way dispatch" this was deferred as is a plain two-player countdown
 * state machine; every computed branch capstone reported inside the literal pools (204cd0/204cd4/
 * 204e10/204f78/204fa4/204fa8/205164) is pool DATA, not a dispatch. All real PIC hand-resolved
 * (ptr_resolve/braf_resolve): morphs mova+add -> 0x204868 / 0x205232; bsrf -> func_02238134 (0x238134),
 * func_02238176 (0x238176), player_join_spawn (0x228168), func_0220aca8 (0x20aca8, resident_cmd_issue_108),
 * func_02210020 (0x210020, rng_next), sound_queue_push (0x215f6c). fidelity: HIGH - structure from
 * m2c, every call/branch/morph/spawn hand-resolved and checked vs m2c_fn --asm. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

typedef void (*fn_v)(void);
typedef void (*fn_i)(s32);
typedef void (*fn_ii)(s32, s32);
#define CALL(a, T) (*(T *)(uintptr_t)(a))

extern void attract_service_credit_join(void *buf); /* 0x022045f8 */
extern void hud_draw_status_panel(s32 a);          /* 0x02238134 */
extern void hud_draw_status_digit(s32 a, s32 b);   /* 0x02238176 HUD/countdown display */
extern void player_join_spawn(s32 p);      /* 0x02228168 */
extern void resident_increment_counter108(s32 cmd);   /* 0x0220aca8 resident_cmd_issue_108 */
extern s32  game_random_next(void);      /* 0x02210020 rng_next */
extern void sound_queue_push(s32 req);/* 0x02215f6c */

static void hud_print(s32 color, s32 mode, s32 str) {
    R32A(DH_UI_CURSOR_COLUMN_ADDR) = color;
    R32A(DH_UI_CURSOR_ROW_ADDR) = mode;
    CALL(0x0609029c, fn_i)(str);
}
static void hud_print_num(s32 color, s32 mode, s32 str, s32 num) {
    R32A(DH_UI_CURSOR_COLUMN_ADDR) = color;
    R32A(DH_UI_CURSOR_ROW_ADDR) = mode;
    CALL(0x060902a0, fn_ii)(str, num);
}

void round_join_countdown_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *actor;

    attract_service_credit_join((void *)0x06098794);
    CALL(0x0609867c, fn_v)();

    if (M32(entity, 0x40) != 0) {                      /* ---- P1 countdown active ---- */
        if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 4) {                  /* P1 join confirmed */
            hud_print(3, 1, 0x06050d84);
            hud_print(3, 2, 0x06050d84);
            *(void **)task = (void *)(uintptr_t)0x06008868;   /* -> attract_handler */
            M32(entity, 0x40) = 0;
            if (R8A(0x0609868e) == 1) {
                if (R32A(0x0609977c) & 1) {
                    RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 2;
                    R8A(0x0609868a) = 1;
                } else {
                    player_join_spawn(0);
                }
            } else {
                RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 2;
                hud_print(3, 1, 0x06050d98);
            }
            if (R16A(DH_ACTOR_POSE_SET_ADDR) != 1)
                return;
            hud_draw_status_panel(0);
            hud_draw_status_digit(7, -1);
            return;
        }

        CALL(0x06000660, fn_i)(0x060987d8);
        hud_print_num(3, 2, 0x06050dac, M32(entity, 0x40) >> 6);
        if ((R32A(DH_FRAME_TICK_ADDR) & 0xf) == 0) {
            if ((R32A(DH_FRAME_TICK_ADDR) & 0x1f) == 0)
                hud_print(3, 1, R32A(0x06090304 + (s32)RU8A(0x060987db) * 4));
            else
                hud_print(3, 1, 0x06050d84);
        }
        if (RU16A(0x06098794 + 4) & 3)
            M32(entity, 0x40) = 0x27f;
        if ((RU16A(DH_PLAYER_INPUT_RECORDS_ADDR + 4) & 7) && M32(entity, 0x40) < 0x200)
            M32(entity, 0x40) -= 0x40;
        if (R16A(DH_ACTOR_POSE_SET_ADDR) == 1) {
            hud_draw_status_digit(7, M32(entity, 0x40) >> 6);
            if ((u8)(M32(entity, 0x40) + 0x82) == 0 &&
                R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 4) {
                actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
                R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 4;
                M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
                M16(actor, 0xae) = 1;
                M16(actor, 0xb0) = (s16)game_random_next();
                M16(actor, 0xb2) = (s16)game_random_next();
            }
        }
        M32(entity, 0x40) -= 1;
        if (M32(entity, 0x40) <= 0) {
            hud_print(3, 1, 0x06050dc0);
            hud_print(3, 2, 0x06050d84);
            R8A(0x0609868a) = -1;
            RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 0;
            R8A(0x060987d4) = 0;
            R16A(0x0609880e) = 0;
            CALL(0x0600066c, fn_i)(0x060987d4);
            M32(entity, 0x40) = 0;
            resident_increment_counter108((s32)M16(PA(DH_SCENE_DESCRIPTOR_ADDR), 40));
            *(void **)task = (void *)(uintptr_t)0x06008868;   /* -> attract_handler */
            return;
        }
        R16A(0x0609880e) = 1;
        if (R8A(0x06098689) != 0) {                   /* P2 pressed start -> go dual */
            R16A(0x06098812) = 0;
            RU8A(0x06098675) = 3;
            R8A(0x060987d5) = 1;
            R8A(0x06098689) = 0;
            R8A(0x0609868b) = 0;
            M32(entity, 0x44) = 0x27f;
            M32(entity, 0x40) = 0x27f;
            hud_print(3, 1, 0x06050d84);
            hud_print(3, 2, 0x06050d84);
            hud_print(23, 2, 0x06050d84);
            *(void **)task = (void *)(uintptr_t)0x06009232;   /* -> round_controller_dual */
            hud_draw_status_digit(7, 9);
            if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
                sound_queue_push((s32)DH_SOUND_LATCH_BASE);
            CALL(0x06000688, fn_i)(0x21);
            return;
        }
    }

    /* ---- P2 countdown service (reached when P1 idle, or P1 counting w/o P2 start) ---- */
    if (M32(entity, 0x44) == 0)
        return;

    if (RU8A(0x06098675) == 4) {                       /* P2 join confirmed */
        hud_print(23, 1, 0x06050d84);
        hud_print(23, 2, 0x06050d84);
        *(void **)task = (void *)(uintptr_t)0x06008868;   /* -> attract_handler */
        M32(entity, 0x44) = 0;
        if (R8A(0x0609868e) == 1) {
            if (R32A(0x0609977c) & 2) {
                RU8A(0x06098675) = 2;
                R8A(0x0609868b) = 1;
            } else {
                player_join_spawn(0);
            }
        } else {
            RU8A(0x06098675) = 2;
            hud_print(23, 1, 0x06050d98);
        }
        if (R16A(DH_ACTOR_POSE_SET_ADDR) != 1)
            return;
        hud_draw_status_panel(0);
        hud_draw_status_digit(7, -1);
        return;
    }

    CALL(0x06000660, fn_i)(0x060987d8);
    {
        s32 cs = (RU8A(0x060987e8) & 4) ? 0x060987dc : 0x060987d8;
        hud_print_num(23, 2, 0x06050dac, M32(entity, 0x44) >> 6);
        if ((R32A(DH_FRAME_TICK_ADDR) & 0xf) == 0) {
            if ((R32A(DH_FRAME_TICK_ADDR) & 0x1f) == 0)
                hud_print(23, 1, R32A(0x06090304 + (s32)RU8A(cs + 3) * 4));
            else
                hud_print(23, 1, 0x06050d84);
        }
    }
    if (RU16A(0x06098794 + 4) & 3)
        M32(entity, 0x44) = 0x27f;
    if ((RU16A(0x0609874c + 4) & 7) && M32(entity, 0x44) < 0x200)
        M32(entity, 0x44) -= 0x40;
    if (R16A(DH_ACTOR_POSE_SET_ADDR) == 1) {
        hud_draw_status_digit(7, M32(entity, 0x44) >> 6);
        if ((u8)(M32(entity, 0x44) + 0x82) == 0 &&
            R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 4) {
            actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
            R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 4;
            M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(actor, 0xae) = 2;
            M16(actor, 0xb0) = (s16)game_random_next();
            M16(actor, 0xb2) = (s16)game_random_next();
        }
    }
    M32(entity, 0x44) -= 1;
    if (M32(entity, 0x44) <= 0) {
        hud_print(23, 1, 0x06050dc0);
        hud_print(23, 2, 0x06050d84);
        R8A(0x0609868b) = -1;
        RU8A(0x06098675) = 0;
        R8A(0x060987d5) = 0;
        R16A(0x06098812) = 0;
        CALL(0x0600066c, fn_i)(0x060987d4);
        M32(entity, 0x44) = 0;
        resident_increment_counter108((s32)M16(PA(DH_SCENE_DESCRIPTOR_ADDR), 40));
        *(void **)task = (void *)(uintptr_t)0x06008868;   /* -> attract_handler */
        return;
    }
    R16A(0x06098812) = 1;
    if (R8A(0x06098688) != 0) {                        /* P1 pressed start -> go dual */
        R16A(0x0609880e) = 0;
        RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 3;
        R8A(0x060987d4) = 1;
        R8A(0x06098688) = 0;
        R8A(0x0609868a) = 0;
        M32(entity, 0x40) = 0x27f;
        M32(entity, 0x44) = 0x27f;
        hud_print(23, 1, 0x06050d84);
        hud_print(23, 2, 0x06050d84);
        hud_print(3, 2, 0x06050d84);
        *(void **)task = (void *)(uintptr_t)0x06009232;   /* -> round_controller_dual */
        hud_draw_status_digit(7, 9);
        if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push((s32)DH_SOUND_LATCH_BASE);
        CALL(0x06000688, fn_i)(0x20);
    }
}
