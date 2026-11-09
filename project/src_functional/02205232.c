/* Round 4: Original PIC anchors in 060092xx install runtime HWRAM handlers; cart callback addresses were incorrect. Evidence: work/decomp4/fix3.asm. */
/* round_dual_countdown_task [0x02205232]: Drives both player countdowns and installs live play when they expire. */
#include "dh_work.h"
#include "dh.h"
/* 0x02205232 (round_controller_dual) - the "free-play / both-players" morph target of
 * attract_handler and of round_controller_join (port unblock #2b). Self-morphing task driving the
 * two player countdowns (entity->unk40 = P1, entity->unk44 = P2) toward the start of PLAY, then
 * overwriting *task to pick the next state:
 *
 *   - a player's countdown expires (<= 0)  -> *task = scene_run_handler      (0x02205812)  == ENTER PLAY
 *   - at tail, some player is in state 2:
 *         a countdown still running         -> *task = round_controller_join (0x02204afa)
 *         both idle                         -> *task = attract_handler       (0x02204868)
 *
 * Reaching state "a countdown hit 0 -> *task = scene_run_handler" is THE play entry: from the next
 * frame the task is scene_run_handler (0x02205812), which steps the live scene g_0609867c. The
 * matching LATE-JOIN PLAYER SPAWN (same as round_controller_join) seeds actor->unkB0/unkB2 with
 * func_02210020 (rng_next) via (*g_0609882c)->unk8; join==4 and the final hand-off instead ZERO
 * those spawn fields.
 *
 * NO ROM JUMP TABLE here either (the deferred "table-dispatch" was pool-data misread by capstone at
 * 2052e4/205474/205620/20573c). All PIC hand-resolved: morphs mova+add -> 0x205812 / 0x204afa /
 * 0x204868; func_02238176 (0x238176, loaded into r11 and jsr'd), func_02238134 (0x238134), func_0220aca8
 * (0x20aca8 resident_cmd_issue_108, arg *(s16*)(*g_0609866c+40)), func_02210020 (0x210020 rng_next),
 * func_02216066 (0x216066 sound_reemit_latched). fidelity: HIGH - structure from m2c, every
 * call/branch/morph/spawn hand-resolved and checked vs m2c_fn --asm. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

typedef void (*fn_v)(void);
typedef void (*fn_i)(s32);
#define CALL(a, T) (*(T *)(uintptr_t)(a))

extern void attract_service_credit_join(void *buf); /* 0x022045f8 */
extern void hud_draw_status_panel(s32 a);          /* 0x02238134 */
extern void hud_draw_status_digit(s32 a, s32 b);   /* 0x02238176 HUD/countdown display (r11) */
extern void resident_increment_counter108(s32 cmd);   /* 0x0220aca8 resident_cmd_issue_108 */
extern s32  game_random_next(void);      /* 0x02210020 rng_next */
extern void audio_reemit_latched(void);       /* 0x02216066 sound_reemit_latched */

static void hud_print(s32 color, s32 mode, s32 str) {
    R32A(DH_UI_CURSOR_COLUMN_ADDR) = color;
    R32A(DH_UI_CURSOR_ROW_ADDR) = mode;
    CALL(0x0609029c, fn_i)(str);
}

void round_dual_countdown_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *actor;

    attract_service_credit_join((void *)0x06098794);
    CALL(0x0609867c, fn_v)();
    CALL(0x06000660, fn_i)(0x060987d8);
    R16A(0x0609880e) = 1;
    R16A(0x06098812) = 1;

    if ((R32A(DH_FRAME_TICK_ADDR) & 0xf) == 0) {
        if ((R32A(DH_FRAME_TICK_ADDR) & 0x1f) == 0) {
            hud_print(3, 1, R32A(0x06090304 + (s32)RU8A(0x060987db) * 4));
            hud_print(23, 1, R32A(0x06090304 + (s32)RU8A(0x060987df) * 4));
        } else {
            hud_print(3, 1, 0x06050d84);
            hud_print(23, 1, 0x06050d84);
        }
    }
    if (RU8A(0x060987db) > RU8A(0x0609868c)) {
        hud_print(3, 1, R32A(0x06090304 + (s32)RU8A(0x060987db) * 4));
        hud_print(23, 1, R32A(0x06090304 + (s32)RU8A(0x060987df) * 4));
    }
    R8A(0x0609868c) = R8A(0x060987db);

    if (M32(entity, 0x40) == 0)
        goto p2_service;

    /* ---- P1 active ---- */
    if (RU16A(0x06098794 + 4) & 3) {
        M32(entity, 0x40) = 0x27f;
        hud_draw_status_digit(7, 9);
    }
    if ((RU16A(DH_PLAYER_INPUT_RECORDS_ADDR + 4) & 7) && M32(entity, 0x40) < 0x200) {
        M32(entity, 0x40) -= 0x40;
        if (M32(entity, 0x40) < 0)
            M32(entity, 0x40) = 0;
        if (M32(entity, 0x44) != 0) {
            M32(entity, 0x44) -= 0x40;
            if (M32(entity, 0x44) < 0)
                M32(entity, 0x44) = 0;
        }
        hud_draw_status_digit(7, M32(entity, 0x40) >> 6);
    }
    if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 4) {               /* P1 join confirmed */
        R8A(0x0609868a) = 1;
        RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 2;
        M32(entity, 0x40) = 0;
        if (R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 0) {
            actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
            R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 0;
            M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(actor, 0xae) = 0;
            M16(actor, 0xb0) = 0;
            M16(actor, 0xb2) = 0;
        }
        goto hud_tail;                          /* func_02238176(7,-1) then morph decision */
    }
    M32(entity, 0x40) -= 1;
    if (M32(entity, 0x40) <= 0) {
        R8A(0x0609868a) = -1;
        RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 0;
        R8A(0x060987d4) = 0;
        CALL(0x0600066c, fn_i)(0x060987d4);
        M32(entity, 0x40) = 0;
        resident_increment_counter108((s32)M16(PA(DH_SCENE_DESCRIPTOR_ADDR), 40));
        *(void **)task = (void *)(uintptr_t)0x06009812;   /* -> scene_run_handler (ENTER PLAY) */
        goto hud_tail;
    }
    hud_draw_status_panel(1);
    if ((M32(entity, 0x40) & 0x3f) == 0)
        hud_draw_status_digit(7, (M32(entity, 0x40) - 1) >> 6);
    if ((u8)(M32(entity, 0x40) + 0x82) == 0 &&
        R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 4) {
        actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
        R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 4;
        M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
        M16(actor, 0xae) = 1;
        M16(actor, 0xb0) = (s16)game_random_next();
        M16(actor, 0xb2) = (s16)game_random_next();
    }

p2_service:
    if (M32(entity, 0x44) != 0) {               /* ---- P2 active ---- */
        if (RU16A(0x06098794 + 4) & 3) {
            M32(entity, 0x44) = 0x27f;
            hud_draw_status_digit(7, 9);
        }
        if ((RU16A(0x0609874c + 4) & 7) && M32(entity, 0x44) < 0x200) {
            M32(entity, 0x44) -= 0x40;
            if (M32(entity, 0x44) < 0)
                M32(entity, 0x44) = 0;
            if (M32(entity, 0x40) != 0) {
                M32(entity, 0x40) -= 0x40;
                if (M32(entity, 0x40) < 0)
                    M32(entity, 0x40) = 0;
            }
            hud_draw_status_digit(7, M32(entity, 0x44) >> 6);
        }
        if (RU8A(0x06098675) == 4) {           /* P2 join confirmed */
            R8A(0x0609868b) = 1;
            RU8A(0x06098675) = 2;
            M32(entity, 0x44) = 0;
            if (R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 0) {
                actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
                R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 0;
                M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
                M16(actor, 0xae) = 0;
                M16(actor, 0xb0) = 0;
                M16(actor, 0xb2) = 0;
            }
            goto hud_tail;
        }
        M32(entity, 0x44) -= 1;
        if (M32(entity, 0x44) <= 0) {
            R8A(0x0609868b) = -1;
            RU8A(0x06098675) = 0;
            R8A(0x060987d5) = 0;
            M32(entity, 0x44) = 0;
            CALL(0x0600066c, fn_i)(0x060987d4);
            hud_draw_status_digit(7, -1);
            resident_increment_counter108((s32)M16(PA(DH_SCENE_DESCRIPTOR_ADDR), 40));
            *(void **)task = (void *)(uintptr_t)0x06009812;   /* -> scene_run_handler (ENTER PLAY) */
        } else {
            hud_draw_status_panel(1);
            if ((M32(entity, 0x44) & 0x3f) == 0)
                hud_draw_status_digit(7, (M32(entity, 0x44) - 1) >> 6);
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
    }
    goto tail_morph;

hud_tail:
    hud_draw_status_digit(7, -1);

tail_morph:
    if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 2 || RU8A(0x06098675) == 2) {
        if (M32(entity, 0x40) != 0 || M32(entity, 0x44) != 0)
            *(void **)task = (void *)(uintptr_t)0x06008afa;   /* -> round_controller_join */
        else
            *(void **)task = (void *)(uintptr_t)0x06008868;   /* -> attract_handler */
        audio_reemit_latched();
        if (R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 0) {
            actor = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
            R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 0;
            M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(actor, 0xae) = 0;
            M16(actor, 0xb0) = 0;
            M16(actor, 0xb2) = 0;
        }
        hud_draw_status_digit(7, -1);
        if (R16A(DH_ACTOR_POSE_SET_ADDR) == 0)
            hud_draw_status_panel(0);
        hud_print(3, 1, 0x06050d84);
        hud_print(23, 1, 0x06050d84);
    }
}
