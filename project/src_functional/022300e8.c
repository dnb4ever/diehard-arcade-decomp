/* projectile_sustained_fire_task [0x022300e8]: Emits alternate clones every fourth frame until linked ammunition or owner state ends firing. */
#include "dh_work.h"
#include "dh.h"
/* 0x022300e8 (projectile task handler, sustained-fire state) - periodically
 * reposition/emit while the owner holds fire, until ammo runs out or the owner
 * is hit.  Advances to func_022301ac in every exit case.
 * ent = task->unk8; src = task->unk14->unk8 (owner actor).
 * If src->unk84 & 0x43800 (owner collided/dead): ent->unk5A = 60,
 *   task->unk0 = func_022301ac, return.
 * Decrement ent->unk5A; act only every 4th frame (when (ent->unk5A & 3) == 0),
 *   otherwise return.
 * Reserve ammo via func_0223c74a(src); if it returns != 0 (exhausted):
 *   ent->unk5A = 60, task->unk0 = func_022301ac, return.
 * Otherwise reposition via func_022301c4(task, ent, rec, (ent->unk74+1)&1) where
 *   rec = (u8*)g_06097d58 + src->unk34*0x100 + (u8)ent->unk5F*0x10, and increment
 *   ent->unk74.  If func_022301c4 drove ent->unk5A to 0: ent->unk5A = 60,
 *   task->unk0 = func_022301ac.
 * fidelity: HIGH. m2c failed on branch-into-branch; hand-decoded.
 * bsrf resolved 0x230124 -> func_0223c74a; all three computed next-state
 * pointers (mova+disp) resolve to 0x022301ac. */
extern u8 g_06097d58[];
extern s32 actor_consume_linked_effect_count(void *oent);
extern void effect_spawn_actor_clone(void *task, void *ent, void *rec, s32 flag);
extern void effect_cancel_on_timer5a(void *task);

void projectile_sustained_fire_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);
    void *rec;
    s32 cnt;

    if (M32(src, 0x84) & 0x43800) {
        M16(ent, 0x5A) = 60;
        TASK_PTR(task, handler) = (void *)effect_cancel_on_timer5a;
        return;
    }

    M16(ent, 0x5A) = (s16)(M16(ent, 0x5A) - 1);
    if (M16(ent, 0x5A) & 3)
        return;

    if (actor_consume_linked_effect_count(src) != 0) {
        M16(ent, 0x5A) = 60;
        TASK_PTR(task, handler) = (void *)effect_cancel_on_timer5a;
        return;
    }

    rec = &g_06097d58[(s32)M8(src, 0x34) * 0x100 + (u8)M8(ent, 0x5F) * 0x10];
    cnt = M32(ent, 0x74) + 1;
    M32(ent, 0x74) = cnt;
    effect_spawn_actor_clone(task, ent, rec, cnt & 1);

    if (M16(ent, 0x5A) != 0)
        return;
    M16(ent, 0x5A) = 60;
    TASK_PTR(task, handler) = (void *)effect_cancel_on_timer5a;
}
