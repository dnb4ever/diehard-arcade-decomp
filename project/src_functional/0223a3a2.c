/* actor_state_release_gen [0x0223a3a2]: Existing map corroborated by source: paired release/transition state generator (nested switch on unk6C/unk6D); called by 0x22979c */
#include "dh.h"
/* 0x0223a3a2 - "cancel/release action" for a fighter slot; the counterpart of
 * func_02239fc6. Dispatches on the action code arg0->unk88: 0x52 ('R') releases
 * the upper channel (unk6C/unk6E), 0x53 ('S') the lower channel (unk6D/unk6F).
 * The channel's current move id selects the cleanup: 1 sets a hold bit, 12 arms
 * a follow-up (unk70), 13/14 bump the hit counters unk72/unk73 and flag the slot
 * ready (g_060e0f2c=5), and 15/16/17 (only on the upper channel) spawn a thrown
 * body via func_0220feec(g_06099580, func_02227906) seeded from the actor's world
 * position. Afterwards the sprite record g_06096248[unk34] is restored from the
 * saved state g_060e11e8[unk34], the move's collision bits are cleared, and for
 * the first three slots the shadow (g_060e0e58) is reset via func_0223e17c with
 * its frame chosen from g_060e0e78 / g_0608d3e8.
 * branches resolved: bsrf @0x23a4b2=func_0220feec, @0x23a696=func_0223e17c; the
 * spawn's r5 = func_02227906 (constructor callback, mova 0x23a4ac + 0xFFFED45A
 * = 0x227906).
 * NOTE: on the unknown-code path the shared tail reads g_0608d3e8[chan] with chan
 * (r13) never set by the ROM; chan is initialised to 0 here (that path is inert).
 * fidelity: HIGH. */
extern s32  g_060e0f2c[];
extern u8   g_06096248[];   /* sprite records, 0x20 stride */
extern u8   g_060e11e8[];   /* saved anim state, 4-byte records */
extern s16  g_060e11a8[];
extern u8   g_actor_slot_tasks_legacy[];   /* actor pointer table */
extern void *g_06099580;    /* spawn pool/context */
extern s16  g_0608c794[];
extern s32  g_actor_slot_masks[];
extern s8   g_0608d3e8[];
extern s8   g_060e0e78[];
extern u8   g_060e0e58[];    /* per-slot shadow, 0x10 stride */
extern s16  g_0608ce74[];
extern s32  g_06096c74[];
extern void *task_spawn_child_work88(void *ctx, void (*ctor)(void));
extern void effect_handoff_sprite_task(void *task);
extern void hud_rebuild_player_counters(void *a);

void actor_state_release_gen(void *arg0) {
    s32 id = MU8(arg0, 0x34);
    s32 chan = 0;
    void *spr = &g_06096248[id * 0x20];
    void *sav = &g_060e11e8[id * 4];

    switch (M32(arg0, 0x88)) {
    case 0x52:
        chan = MU8(arg0, 0x6C);
        switch (chan) {
        case 1:
            M32(arg0, 0x8C) |= 0x02000000;
            M8(arg0, 0x6A) = (s8)chan;
            M8(arg0, 0x6B) = M8(arg0, 0x6E);
            break;
        case 12:
            M32(arg0, 0x8C) |= 0x01000000;
            M8(arg0, 0x70) = 1;
            M8(arg0, 0x71) = M8(arg0, 0x6E);
            break;
        case 13:
            g_060e0f2c[MU8(arg0, 0x6E)] = 5;
            if (M8(arg0, 0x72) + M8(arg0, 0x73) < 4)
                M8(arg0, 0x72)++;
            break;
        case 14:
            g_060e0f2c[MU8(arg0, 0x6E)] = 5;
            if (M8(arg0, 0x72) + M8(arg0, 0x73) < 4)
                M8(arg0, 0x73)++;
            break;
        case 15:
        case 16:
        case 17: {
            void *ret = task_spawn_child_work88(g_06099580, (void (*)(void))effect_handoff_sprite_task);
            void *child = MP(ret, 8);
            void *src = MP(MP(g_actor_slot_tasks_legacy, id * 4), 8);
            M32(child, 8)    = M32(src, 8);
            M32(child, 0xC)  = M32(src, 0xC);
            M32(child, 0x10) = M32(src, 0x10);
            M32(child, 0x60) = id - 1;
            M32(child, 0x64) = g_0608c794[(chan - 0xF) & 3];
            g_060e0f2c[MU8(arg0, 0x6E)] = 5;
            break;
        }
        default:
            break;
        }
        if (M8(arg0, 0x6D) == 8 && M8(arg0, 0x70) != 0) {
            M16(spr, 8) = g_060e11a8[MU16(arg0, 0x68)];
            M32(arg0, 0x8C) |= 0x1000;
            M32(arg0, 0x8C) &= (s32)0xFEFFFFFF;
            M8(arg0, 0x6E) = M8(arg0, 0x71);
            M8(arg0, 0x6C) = 0xC;
            M8(arg0, 0x70) = 0;
            M8(arg0, 0x71) = 0;
        } else {
            M16(spr, 8) = M16(sav, 0);
            M32(arg0, 0x8C) &= ~g_actor_slot_masks[chan];
            M8(arg0, 0x6C) = 0;
            M8(arg0, 0x6E) = 0;
        }
        break;

    case 0x53:
        chan = MU8(arg0, 0x6D);
        switch (chan) {
        case 1:
            M32(arg0, 0x8C) |= 0x02000000;
            M8(arg0, 0x6A) = (s8)chan;
            M8(arg0, 0x6B) = M8(arg0, 0x6F);
            break;
        case 12:
            M32(arg0, 0x8C) |= 0x01000000;
            M8(arg0, 0x70) = 1;
            M8(arg0, 0x71) = M8(arg0, 0x6F);
            break;
        case 13:
            g_060e0f2c[MU8(arg0, 0x6F)] = 5;
            if (M8(arg0, 0x72) + M8(arg0, 0x73) < 4)
                M8(arg0, 0x72)++;
            break;
        case 14:
            g_060e0f2c[MU8(arg0, 0x6F)] = 5;
            if (M8(arg0, 0x72) + M8(arg0, 0x73) < 4)
                M8(arg0, 0x73)++;
            break;
        default:
            break;
        }
        M16(spr, 0xE) = M16(sav, 2);
        M32(arg0, 0x8C) &= ~g_actor_slot_masks[chan];
        M8(arg0, 0x6D) = 0;
        M8(arg0, 0x6F) = 0;
        break;

    default:
        break;
    }

    if (id < 3) {
        void *shadow = &g_060e0e58[(id - 1) * 0x10];
        s32 j = 0;
        hud_rebuild_player_counters(arg0);
        while (M8(g_060e0e78, (id - 1) * 8 + j) != g_0608d3e8[chan]) {
            j++;
            if (j >= 6)
                break;
        }
        M16(shadow, 0xE) = (s16)j;
        M16(shadow, 0xC) = 0x10;
        g_06096c74[id] = g_0608ce74[M8(arg0, 0x6D)];
    }
}
