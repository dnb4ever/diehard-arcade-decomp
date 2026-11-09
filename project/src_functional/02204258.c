/* scene_sprite_spin_task [0x02204258]: Runs timed spin phases for the scene sprite and draws its billboard. */
#include "dh_work.h"
#include "dh.h"

/* 0x02204258 - scene_handler_204258: func_0220feec task handler, 3 states,
 * then tail-calls func_022154fc(entity, 0x10000, 0x10000, 4) every frame.
 *   r4 = task; task->unk10 (byte) = state; task->unk8 = entity.
 *   state 0: init entity fields (unk30=14, clear 32/34/36/38/40, vel 8=0,
 *            accel C=-0xBC00 (0xffff4400), 16.16 unk10=0x20000), go state 2.
 *   state 1: unk26 += 0x800 (s16 wrap), decrement timer at 0x64; at 0 -> state 2.
 *   state 2: when (*0x060987BC & 0x7F) == 0, timer = 32, go state 1.
 */

extern void sprite_draw_billboard(u32 arg0, s32 arg1, s32 arg2, s32 arg3); /* 0x2154fc (tail) */

void scene_sprite_spin_task(void *task)
{
	u32 ent = TASK_S32(task, work);
	s32 state = TASK_U8(task, state);

	if (state == 0) {
		M16(ent, 48) = 14;
		M16(ent, 50) = 0;
		M16(ent, 34) = 0;
		M32(ent, 8) = 0;
		M32(ent, 12) = (s32)0xffff4400;
		M32(ent, 16) = 0x00020000;
		M16(ent, 36) = 0;
		M16(ent, 38) = 0;
		M16(ent, 40) = 0;
		TASK_U8(task, state) = 2;
	} else if (state == 1) {
		s32 c;
		M16(ent, 38) = (s16)(M16(ent, 38) + 0x800);
		c = M32(ent, 100) - 1;
		M32(ent, 100) = c;
		if (c == 0)
			TASK_U8(task, state) = 2;
	} else if (state == 2) {
		if ((M32(DH_FRAME_TICK_ADDR, 0) & 0x7f) == 0) {
			M32(ent, 100) = 32;
			TASK_U8(task, state) = 1;
		}
	}

	sprite_draw_billboard(ent, 0x00010000, 0x00010000, 4);
}

/* fidelity: HIGH. cmp/eq state ladder with bt/bt/bra; braf at 0x2042d8 resolves
 * to func_022154fc (braf_resolve), args r4=entity, r5=r6=0x10000, r7=4. mov.w pool
 * loads 0x800 (positive) and 0xffff4400 modeled with sign extension; 16-bit store
 * at 0x64 uses signed 16-bit read for the += 0x800 wrap. */
