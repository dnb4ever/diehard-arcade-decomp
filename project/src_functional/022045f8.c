/* attract_service_credit_join [0x022045f8]: Consumes coin/join edges, updates player readiness and notifies resident BIOS services. */
#include "dh.h"
/* 0x022045f8 (attract_credit_join_service) - per-frame "a credit was inserted / a player
 * accepted" edge handler, called first thing by attract_handler and both round controllers.
 *
 * Two symmetric coin-accept edges (P1 g_0609880c/g_0609880e, P2 g_06098810/g_06098812): when
 * both the "coin pending" and "coin ready" halfword flags are 1 it consumes the edge
 * (clears both, g_06098680=1, marks the join record g_060987d4, increments the per-player join
 * counter g_06098674/75), mirrors region/language (g_0609978c/88 <- g_06099796/94), arms the
 * per-player accept timer g_0609975c/60 = 30, blanks the HUD line, and on counter==1 or ==4
 * registers the player's resident coordinate command (func_0220aa70 / func_0220ab34) and stores
 * the resident-query result (*g_06000640)(n) into g_060987f8/fc, then notifies the join record
 * (*g_0600066c) and the system task (*g_06000688)(0x10/0x11).
 *
 * Two symmetric "join confirm" edges (counter==1): if the controller/config gate g_0609868e != 1
 * it advances the counter to 2, runs func_0223dcc2(player) and prints the join string *g_06090310;
 * otherwise it tail-calls player_join_spawn(player). P2's confirm path is a genuine tail call in asm.
 *
 * bsrf/braf hand-resolved (braf_resolve, pool deltas @0x2047e0..0x204864): 0x20aa70, 0x20ab34,
 * 0x23dcc2, 0x228168. No ROM jump table. fidelity: HIGH, hand-traced vs m2c_fn --asm. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))

typedef void (*fn_i)(s32);
typedef s32  (*fn_i_i)(s32);
#define CALL(a, T) (*(T *)(uintptr_t)(a))

extern void stats_increment_pair_counters70(s32 p);   /* 0x0220aa70 resident_cmd_reg_coords_70 */
extern void stats_increment_pair_counters74(s32 p);   /* 0x0220ab34 resident_cmd_reg_coords_74 */
extern void player_clear_saved_action(s32 p);   /* 0x0223dcc2 */
extern void player_join_spawn(s32 p);    /* 0x02228168 */

static void hud_print(s32 color, s32 mode, s32 str) {
    R32A(DH_UI_CURSOR_COLUMN_ADDR) = color;
    R32A(DH_UI_CURSOR_ROW_ADDR) = mode;
    CALL(0x0609029c, fn_i)(str);
}

void attract_service_credit_join(void *buf) {
    (void)buf;  /* arg passed by callers (r4), unused by this routine */

    /* ---- P1 coin-accept edge ---- */
    if ((u16)R16A(0x0609880c) == 1 && (u16)R16A(0x0609880e) == 1) {
        R16A(0x0609880e) = 0;
        R16A(0x0609880c) = 0;
        R32A(0x06098680) = 1;
        R8A(0x060987d4) = 1;
        RU8A(DH_PLAYER_JOIN_STATES_ADDR) += 1;
        R16A(0x0609978c) = R16A(0x06099796);
        R16A(0x06099788) = R16A(0x06099794);
        R32A(0x0609975c) = 30;
        hud_print(3, 1, 0x06050d84);
        if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 1) {
            stats_increment_pair_counters70(0);
            R32A(0x060987f8) = CALL(0x06000640, fn_i_i)(0);
        } else if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 4) {
            stats_increment_pair_counters74(0);
            R32A(0x060987f8) = CALL(0x06000640, fn_i_i)(0);
        }
        CALL(0x0600066c, fn_i)(0x060987d4);
        CALL(0x06000688, fn_i)(0x10);
    }

    /* ---- P2 coin-accept edge ---- */
    if ((u16)R16A(0x06098810) == 1 && (u16)R16A(0x06098812) == 1) {
        R16A(0x06098812) = 0;
        R16A(0x06098810) = 0;
        R32A(0x06098680) = 1;
        R8A(0x060987d5) = 1;
        RU8A(0x06098675) += 1;
        R16A(0x0609978e) = R16A(0x06099796);
        R16A(0x0609978a) = R16A(0x06099794);
        R32A(0x06099760) = 30;
        hud_print(23, 1, 0x06050d84);
        if (RU8A(0x06098675) == 1) {
            stats_increment_pair_counters70(1);
            R32A(0x060987fc) = CALL(0x06000640, fn_i_i)(1);
        } else if (RU8A(0x06098675) == 4) {
            stats_increment_pair_counters74(1);
            R32A(0x060987fc) = CALL(0x06000640, fn_i_i)(1);
        }
        CALL(0x0600066c, fn_i)(0x060987d4);
        CALL(0x06000688, fn_i)(0x11);
    }

    /* ---- P1 join-confirm edge ---- */
    if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 1) {
        if (R8A(0x0609868e) != 1) {
            R8A(DH_PLAYER_JOIN_STATES_ADDR) = 2;
            player_clear_saved_action(1);
            hud_print(3, 1, R32A(0x06090310));
        } else {
            player_join_spawn(0);
        }
    }

    /* ---- P2 join-confirm edge (tail calls in asm) ---- */
    if (RU8A(0x06098675) == 1) {
        if (R8A(0x0609868e) != 1) {
            R8A(0x06098675) = 2;
            player_clear_saved_action(2);
            hud_print(23, 1, R32A(0x06090310));
        } else {
            player_join_spawn(1);
        }
    }
}
