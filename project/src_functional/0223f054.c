/* chain_hit_task [0x0223f054]: Existing map corroborated by source: multi-hit chain vs locked target g_060995b0[slot]; spawn child func_0220feec every 4th count; damage func_022ab18 */
#include "dh_work.h"
#include "dh.h"
/* 0x0223f054 (projectile task handler) - multi-hit chain: periodically spawns a
 * child hit against the locked target until the target is lost, the chain
 * counter expires, or the target changes phase.
 * ent = task->unk8; slot = ent->unk4C (byte).
 * Guards (else -> kill): g_06099590[slot] must be non-NULL; increment chain
 *   counter ent->unkC4 and require its pre-increment value (as u16) < 35;
 *   target = g_060995b0[slot]->unk8 must share ent->unk5A.
 * Kill path: if g_060e0ddc != 0 fire sound sound_queue_push(0xA0140DFF); task->unk0 = 0.
 * Otherwise only every 4th count ((counter & 3) == 0) spawn a child hit:
 *   child = func_0220feec(task, template 0x02239500); child->unkF = counter & 7;
 *   cent = child->unk8; cent->unk34 = slot.
 *   Temporarily set target->unk4A = ent->unk34; if target->unk56 > 4 apply damage
 *   actor_apply_damage(target, 1); restore target->unk4A.
 * fidelity: HIGH. bsrf resolved 0x20feec/0x22ab18/0x215f6c; template r5 =
 * mova(.Lpool_23f0ac)+0xFFFFA454 = 0x02239500. g_06099590/g_060995b0 are the
 * player-manager (g_06099584) sub-tables at +0xC / +0x2C. */
extern s32   g_sound_effects_enabled;
extern void *g_actor_slot_ids[];
extern void *g_actor_slot_tasks_legacy[];
extern void *task_spawn_child_work88(void *parent, void *handler);
extern void  actor_apply_damage(void *actor, s32 amount);
extern void  sound_queue_push(s32 cmd);

void chain_hit_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 slot = M8(ent, 0x4C);
    s32 cnt;
    void *target;
    void *child;
    void *cent;
    s8 saved;

    if (g_actor_slot_ids[slot] == 0)
        goto kill;
    cnt = M32(ent, 0xC4);
    M32(ent, 0xC4) = cnt + 1;
    target = TASK_PTR(g_actor_slot_tasks_legacy[slot], work);
    if ((u16)cnt >= 35)
        goto kill;
    if (M16(ent, 0x5A) != M16(target, 0x5A))
        goto kill;

    if (((u16)cnt & 3) != 0)
        return;

    child = task_spawn_child_work88(task, (void *)0x02239500);
    M8(child, 0xF) = (s8)(cnt & 7);
    cent = MP(child, 8);
    M8(cent, 0x34) = (s8)slot;
    saved = M8(target, 0x4A);
    M8(target, 0x4A) = M8(ent, 0x34);
    if (M16(target, 0x56) > 4)
        actor_apply_damage(target, 1);
    M8(target, 0x4A) = saved;
    return;

kill:
    if (g_sound_effects_enabled != 0)
        sound_queue_push(0xA0140DFF);
    TASK_S32(task, handler) = 0;
}
