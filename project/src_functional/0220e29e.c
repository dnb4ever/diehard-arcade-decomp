/* debug_initialize_world_pose_task [0x0220e29e]: Seeds the debug object's position and angles once and rebuilds its matrices. */
#include "dh_work.h"
#include "dh.h"

/* 0x0220e29e - small object-init task handler (sibling of func_0220e1e2).
 * r4 = task; task->unk16 (byte) = state; task->unk8 = entity.
 *   state 0: clear entity->{8,12} (16.16), set entity->16 = 0xfffe0000,
 *            clear entity->{36,38,40} (u16), advance the state byte.
 *   any state: tail-call func_02214a7a(entity) (mtx_build_fwd_inv).
 * Real extent 0x20e29e..0x20e2d8 (func_0220e1e2 ends at 0x20e29c); m2c
 * over-swallowed the following sibling handlers (0x20e2d8 et al.).
 */

extern void mtx_build_fwd_inv_entry(void *entity); /* 0x214a7a mtx_build_fwd_inv (tail) */

void debug_initialize_world_pose_task(void *task)
{
	void *ent = TASK_PTR(task, work);
	s32 state = TASK_U8(task, state);

	if (state == 0) {
		M32(ent, 8) = 0;
		M32(ent, 12) = 0;
		M32(ent, 16) = (s32)0xfffe0000;
		MU16(ent, 36) = 0;
		MU16(ent, 38) = 0;
		MU16(ent, 40) = 0;
		TASK_U8(task, state) = (u8)(TASK_U8(task, state) + 1);
	}
	mtx_build_fwd_inv_entry(ent);
}

/* fidelity: HIGH. cmp/eq state ladder (states 1 and default both skip straight
 * to the tail-call); braf at 0x20e2d4 resolved to func_02214a7a (braf_resolve). */
