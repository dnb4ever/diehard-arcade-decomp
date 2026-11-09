/* player_join_commit_fsm [0x02229c00]: Existing map corroborated by source: (extra) *** SOLE SETTER of the start-present flag g_06098688 *** */
#include "dh_work.h"
#include "dh.h"
/* 0x02229c00 (player_join_commit_fsm) - the player-character "join" state handler and the
 * SOLE setter of the attract->play start-present flag g_06098688 (see notes/port/
 * play_transition_status.md). Handler ABI: (task, entity); entity = r5, task = r4 (its
 * handler slot *task is overwritten on commit = the FSM morph to the next state 0x0229df4).
 *
 * entity[0xc8] is a join countdown. While it is still > 0 after this tick's decrement the
 * handler only runs a blink effect (toggle entity[0xcc] low bit on odd ticks; drive the
 * visible flag word entity[0x20]) and returns. When the countdown reaches 0 it decrements the
 * per-player word counter g_0609978c[idx-1] (idx = entity[0x34], player 1/2):
 *   - counter still != 0  -> RE-ARM: reset timers/fields (including the visible flag word
 *     entity[0x20] = 0), issue the resident scene command (resident_cmd_issue_100) and
 *     register the actor's slot record (actor_slot_record_reg).
 *   - counter == 0        -> COMMIT: raise the start-present flag g_06098688[idx-1] = 1, publish
 *     entity->unk8/C/10 into the 12-byte record g_06099764[idx-1], OR this player's bits
 *     (g_0604ebe4[idx-1]) into g_0609977c, morph *task to the next state (0x0229df4), then
 *     call func_0223dd28(idx), func_0220a9c2(resident_query() - base) and tail resident_cmd_issue_100.
 *
 * Computed branches resolved: bsrf->0x23dd28/0x20a9c2/0x21707c, braf/bsrf->0x20ac0c (braf_resolve);
 * morph fn-ptr mova+add->0x229df4 (ptr_resolve). g_06000640 is the resident "query" fp adjacent to
 * the g_06000644 dispatcher used by resident_cmd_issue_100. r13 is pinned to 0 for the whole body
 * (every r13 store writes 0, incl. the re-arm entity[0x20] = 0 word at 0x229d54 and the
 * g_06099590[idx] clear).
 * The resident_cmd_issue_100 index is loaded with mov.w @(40, r4) (0x229d1e / 0x229d78), i.e. a
 * SIGN-EXTENDED s16 from *(s32*)0x0609866c + 40, not a zero-extended word.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. Callees func_0223dd28/func_0220a9c2 undecompiled
 * but called faithfully (1 s32 arg each). */

#define R8(a)   (*(s8  *)(uintptr_t)(a))
#define RU16(a) (*(u16 *)(uintptr_t)(a))
#define R32(a)  (*(s32 *)(uintptr_t)(a))
#define WB(a,v) (*(u8  *)(uintptr_t)(a) = (u8)(v))
#define WW(a,v) (*(u16 *)(uintptr_t)(a) = (u16)(v))
#define WL(a,v) (*(s32 *)(uintptr_t)(a) = (s32)(v))

extern void player_save_action(s32 idx);              /* 0x0223dd28 (undecompiled) */
extern void stats_add_duration_histogram(s32 arg);              /* 0x0220a9c2 (undecompiled) */
extern s32  stats_increment_indexed_counter(s32 idx);              /* 0x0220ac0c resident_cmd_issue_100 */
extern void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4); /* 0x0221707c actor_slot_record_reg */
extern void player_join_next(void);                  /* 0x0229df4 next FSM state (handler) */
extern s32 (*g_06000640)(void);                  /* resident query fp (adjacent to g_06000644) */

void player_join_commit_fsm(void *task, void *entity) {
    s32 idx;
    s32 cnt;
    void *rec;
    s32 r0;
    s32 base;

    cnt = M32(entity, 0xc8) - 1;
    M32(entity, 0xc8) = cnt;
    if (cnt > 0) {
        /* countdown still running: blink effect, then return */
        if (M32(entity, 0xc8) & 1)
            M16(entity, 0xcc) = (MU16(entity, 0xcc) + 1) & 1;
        M16(entity, 0x20) = MU16(entity, 0xcc) != 0 ? 0x100 : 0;
        return;
    }

    idx = ACTOR_WORK(entity)->slot_id;
    cnt = (s16)(RU16(0x0609978c + (idx - 1) * 2) - 1);
    WW(0x0609978c + (idx - 1) * 2, cnt);
    if (cnt != 0) {
        /* RE-ARM: counter not yet exhausted */
        ACTOR_M8(entity, action_pending) = 0;
        {
            s32 v = RU16(0x06099794);
            if (RU16(DH_ACTOR_POSE_SET_ADDR) == 1)
                v = (s16)v * 2;
            ACTOR_M16(entity, health) = v;
        }
        ACTOR_WORK(entity)->animation_requested = R32(0x06096c74 + idx * 4);
        M16(entity, 0x20) = 0;                   /* 0x229d54: mov.w r13(=0) @(32,r14) */
        M32(entity, 0xc4) = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        M32(entity, 0xbc) = -1;
        ACTOR_WORK(entity)->hit_shake_ticks = 90;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_TIMED_RECOVERY;
        stats_increment_indexed_counter((s16)M16((u32)M32(DH_SCENE_DESCRIPTOR_ADDR, 0), 40));
        actor_register_movement_slot(entity, 0x5999, (s32)0xffffb334, 0x4000, 0x14ccc);
        return;
    }

    /* COMMIT: counter exhausted */
    player_save_action(idx);
    WL(DH_ACTOR_SLOT_IDS_ADDR + idx * 4, 0);
    M16(entity, 0x20) = 0;
    *(void **)task = (void *)player_join_next;      /* morph -> next state */
    WB(0x06098688 + (idx - 1), 1);              /* raise the start-present flag */
    rec = (void *)(uintptr_t)(0x06099764 + (idx - 1) * 12);
    M32(rec, 0) = ACTOR_WORK(entity)->position_x;
    M32(rec, 4) = ACTOR_WORK(entity)->position_y;
    M32(rec, 8) = ACTOR_WORK(entity)->position_z;
    WL(0x0609977c, R32(0x0609977c) | R32(DH_ACTOR_SLOT_MASKS_ADDR + (idx - 1) * 4));
    WB(0x0609868a + (idx - 1), 0);
    r0 = g_06000640();
    base = idx == 1 ? R32(0x060987f8) : R32(0x060987fc);
    stats_add_duration_histogram(r0 - base);
    stats_increment_indexed_counter((s16)M16((u32)M32(DH_SCENE_DESCRIPTOR_ADDR, 0), 40));
}
