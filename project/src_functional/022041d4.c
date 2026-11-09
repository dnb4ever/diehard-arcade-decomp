/* scene_sprite_acceleration_task [0x022041d4]: Initializes a timed sprite acceleration phase and draws its billboard each frame. */
#include "dh_work.h"
#include "dh.h"

/* 0x022041d4 - scene_handler_2041d4: func_0220feec task handler, 3 states,
 * then tail-calls func_022154fc(entity, 0x10000, 0x10000, 4) every frame.
 *   r4 = task; task->unk10 (byte) = state; task->unk8 = entity.
 *   state 0: init entity vel/accel fields (two variants gated on task->unk15),
 *            set timer entity->unk100 = 32, advance to state 2.
 *   state 2: entity->unk16 -= 0x1000, decrement timer; when it hits 0, state 1.
 *   state 1 (and any other): just the tail call.
 * NOTE: m2c over-swallowed siblings 0x204258 and 0x204308 after this body; the
 * real function ends at the braf tail-jump at 0x204254.
 */

extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c); /* 0x2154fc (tail) */

void scene_sprite_acceleration_task(void *task)
{
	void *ent = TASK_PTR(task, work);
	s32 state = TASK_U8(task, state);

	if (state == 0) {
		if (MU8(task, 15) != 0) {
			M32(ent, 8) = 0;
			M32(ent, 12) = (s32)0xffff6000;
			M32(ent, 16) = 0x00050000;
			MU16(ent, 48) = 11;
		} else {
			M32(ent, 8) = 0;
			M32(ent, 12) = (s16)0xe000;   /* sign-extended -> 0xffffe000 */
			M32(ent, 16) = 0x00040000;
			MU16(ent, 48) = 12;
		}
		MU16(ent, 50) = 0;
		MU16(ent, 36) = 0;
		MU16(ent, 38) = 0;
		MU16(ent, 40) = 0;
		M32(ent, 100) = 32;
		TASK_U8(task, state) = 2;
	} else if (state == 2) {
		s32 c;
		M32(ent, 16) = M32(ent, 16) - 0x1000;
		c = M32(ent, 100) - 1;
		M32(ent, 100) = c;
		if (c == 0)
			TASK_U8(task, state) = 1;
	}

	sprite_draw_billboard(ent, 0x00010000, 0x00010000, 4);
}

/* fidelity: HIGH. State ladder + tail-call resolved via braf_resolve (all three
 * braf land on func_022154fc). mov.w pool loads modeled with s16 sign-extension. */
