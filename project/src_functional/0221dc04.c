/* scene_tick_thirteen_phase_setup [0x0221dc04]: Thirteen-state controller spawns table-selected helper tasks and advances script. */
#include "dh_band21.h"
/* 0x0221dc04 - per-actor state machine (dispatch on byte state @arg+0x10, 13 states).
 * arg is the actor record; w = arg->unk8 is its work/animation sub-record. The state
 * byte selects a handler via a 13-entry jump table (states >= 13 return immediately):
 *   0        : init - run two setup passes (func_0220ff54 over table @0x0600bbcc,
 *              func_0220fece over table @0x0603b828), fetch a timeline via
 *              func_0222a878 into w[0x30], run the spawn list (func_0222a5d0), seed the
 *              step counter w[0x18] from timeline[1], then advance state.
 *   1/3/5/7  : tick-down w[0x18]; on reaching 0, pulse func_0222a708, poke 16 into
 *              (*0x0609882c)->unk8->unk3C, and advance state.
 *   2/4/6    : re-run spawn list, reload w[0x18] from timeline[1], advance state.
 *   8        : re-run spawn list, reload w[0x18], set w[0x1C] = (*0x06098570) << 3,
 *              advance state.
 *   9        : tick-down w[0x1C]; on 0, spawn via func_0220feec (table @0x0601e1a0)
 *              and advance state - then fall into state 10.
 *   10       : tick-down w[0x18]; on 0, pulse func_0222a708, poke 16 into the live
 *              record, compute a difficulty/variant selector from flags @0x0609856c
 *              and the clamped counter @0x06098570, index timeline[0x14] by it, install
 *              the new entry into w[0x3C]/w[0x30], mark arg->unk8(=1)/arg->unkF(=1),
 *              set state 11, clear @0x06096c90.
 *   11       : re-run spawn list, set w[0x1C] = 1, advance state (into the +1 tail).
 *   12       : tick-down w[0x1C]; on 0, if flag bit0 @0x0609856c clear, set bit16 of
 *              @0x06098580, then clear arg[0].
 * fidelity: HIGH - jump table resolved from raw ROM (braf base 0x21dc3c, states 0..12
 *   valid per cmp/hs #13); all callees from braf/ptr_resolve; absolute-addr globals and
 *   16/8-bit accesses faithful. func_0222a878/func_0222a708 take no args (none set at
 *   call sites); their r0 return is kept only where the asm stores it. No register-count
 *   shifts present (all shll/shll2/shar are fixed-count). */

extern void task_spawn_current_work40(u32 actor, u32 table);  /* HWRAM handler, formed by mova + delta */
extern void task_spawn_current_work6c(u32 actor, u32 table);  /* spawn_obj_0x6c - setup over @0x0603b828 */
extern u32  scene_select_player_parameter(void);                  /* fetch timeline/record pointer */
extern void process_spawn_list_entry(u32 w);                 /* process_spawn_list */
extern u32  actor_mark_slots_for_refresh(void);                  /* per-step pulse */
extern void task_spawn_child_work88(u32 actor, u32 table);  /* spawn_obj_a over table @0x0601e1a0 */
extern void player_update_aim_limits(s32 arg);               /* reset helper */

void scene_tick_thirteen_phase_setup(u32 arg) {
    u32 w = MEM32(arg + 8);
    u32 state = MEM8(arg + 16);
    u32 cnt, sel, base, elem, n;

    if (state >= 13)
        return;

    switch (state) {
    case 0:
        task_spawn_current_work40(arg, 0x0600bbcc);
        task_spawn_current_work6c(arg, 0x0603b828);
        MEM32(w + 48) = scene_select_player_parameter();
        process_spawn_list_entry(w);
        MEM32(w + 24) = MEM32(MEM32(w + 48) + 4);
        goto advance_state;

    case 1:
    case 3:
    case 5:
    case 7:
        MEM32(w + 24) -= 1;
        if (MEM32(w + 24) != 0)
            return;
        actor_mark_slots_for_refresh();
        MEM32(MEM32(MEM32(DH_VIEW_TASK_ADDR) + 8) + 60) = 16;
        MEM8(arg + 16) = (u8)(MEM8(arg + 16) + 1);
        return;

    case 2:
    case 4:
    case 6:
        process_spawn_list_entry(w);
        MEM32(w + 24) = MEM32(MEM32(w + 48) + 4);
        goto advance_state;

    case 8:
        process_spawn_list_entry(w);
        MEM32(w + 24) = MEM32(MEM32(w + 48) + 4);
        MEM32(w + 28) = MEM32(0x06098570) << 3;
        MEM8(arg + 16) = (u8)(MEM8(arg + 16) + 1);
        return;

    case 9:
        cnt = MEM32(w + 28);
        MEM32(w + 28) = cnt - 1;
        if (cnt == 0) {
            task_spawn_child_work88(arg, 0x0601e1a0);
            MEM8(arg + 16) = (u8)(MEM8(arg + 16) + 1);
        }
        /* fallthrough */
    case 10:
        MEM32(w + 24) -= 1;
        if (MEM32(w + 24) != 0)
            return;
        actor_mark_slots_for_refresh();
        MEM32(MEM32(MEM32(DH_VIEW_TASK_ADDR) + 8) + 60) = 16;
        sel = ((MEM32(0x0609856c) & 4) >> 1) | (MEM32(0x0609856c) & 1);
        if ((sel & 1) == 0) {
            player_update_aim_limits(0);
            MEM32(0x06098570) = 0;
        } else {
            n = MEM32(0x06098570) + 1;
            MEM32(0x06098570) = n;
            if ((s32)n > 4)
                MEM32(0x06098570) = 4;
        }
        sel |= (MEM32(0x06098568) - 1) << 2;
        base = MEM32(MEM32(w + 48) + 20);
        elem = MEM32(base + (sel << 2));
        MEM32(w + 60) = elem;
        MEM32(w + 48) = elem;
        MEM32(w + 8) = 1;
        MEM32(0x0609856c) &= 1;
        MEM8(arg + 15) = 1;
        MEM8(arg + 16) = 11;
        MEM32(0x06096c90) = 0;
        return;

    case 11:
        process_spawn_list_entry(w);
        MEM32(w + 28) = 1;
        goto advance_state;

    case 12:
        cnt = MEM32(w + 28);
        MEM32(w + 28) = cnt - 1;
        if (cnt != 0)
            return;
        if ((MEM32(0x0609856c) & 1) == 0)
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x00010000;
        MEM32(arg) = 0;
        return;
    }

advance_state:
    MEM8(arg + 16) = (u8)(MEM8(arg + 16) + 1);
}
