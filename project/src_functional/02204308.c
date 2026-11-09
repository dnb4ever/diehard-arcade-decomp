/* scene_sprite_ease_task [0x02204308]: Eases the scene sprite depth toward its target and draws its billboard. */
#include "dh_work.h"
#include "dh.h"

/* 0x02204308 - scene_handler_204308: func_0220feec task handler.
 * r4 = task; task->unk10 (byte) = state; task->unk8 = entity.
 *   state 0: zero entity fields 0x20/0x22/0x24/0x26/0x28/0x32, go to state 1.
 *   state 1: ease entity->unk10 (16.16) toward 0x20000 by 1/4 of the gap,
 *            then tail-call func_022154fc(entity, 0x10000, 0x10000, 4).
 */

extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c); /* 0x2154fc (tail) */

void scene_sprite_ease_task(void *task)
{
	void *ent = TASK_PTR(task, work);
	s32 state = TASK_U8(task, state);

	if (state == 0) {
		M16(ent, 36) = 0;
		M16(ent, 38) = 0;
		M16(ent, 40) = 0;
		M16(ent, 50) = 0;
		M16(ent, 34) = 0;
		M16(ent, 32) = 0;
		TASK_U8(task, state) = 1;
		return;
	}
	if (state == 1) {
		s32 v = M32(ent, 16);
		M32(ent, 16) = v + ((0x00020000 - v) >> 2);
		sprite_draw_billboard(ent, 0x00010000, 0x00010000, 4);
	}
}

/* fidelity: HIGH. Plain cmp/eq state ladder; braf at 0x20435a is the resolved
 * tail-jump to func_022154fc (braf_resolve). shar;shar modeled as arithmetic
 * >>2 on the signed gap. */
