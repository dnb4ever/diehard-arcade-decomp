/* debug_move_world_pose_task [0x0220e1e2]: Applies player-one direction input to world coordinates and rebuilds the model matrices. */
#include "dh.h"

/* func_0220e1e2 (cart 0x20e1e2) - controller-driven object-transform task.
 * self = task (r4); entity = *(void**)(task+8). State byte at task+0x16
 * (0 = init, 1 = run). Reads the player-1 input word g_06098734 and nudges the
 * entity three 16.16 coords at +8/+12/+16 by +/-0x1000 per d-pad bit, then
 * tail-calls the model-matrix rebuilder func_02214a7a (mtx_build_fwd_inv).
 *
 * BOUNDED: m2c over-swallowed several sibling task handlers after this one.
 * The real extent is 0x20e1e2..0x20e29c, ending at the braf tail-call. The
 * swallowed siblings (0x20e2a0, 0x20e2dc, ...) are separate functions.
 */

extern u16 g_player_input_records;                 /* player-1 input / button word */
extern void mtx_build_fwd_inv_entry(void *obj);  /* mtx_build_fwd_inv */

void debug_move_world_pose_task(void *self)
{
	void *entity;
	s32 st;

	st = M8(self, 16);
	entity = MP(self, 8);

	if (st == 0) {
		M32(entity, 8)  = 0;
		M32(entity, 12) = 0;
		M32(entity, 16) = (s32)0xfffb0000;
		M16(entity, 36) = 0;
		M16(entity, 38) = 0;
		M16(entity, 40) = 0;
		M8(self, 16)++;
	}

	if (st == 0 || st == 1) {
		if (g_player_input_records & DH_PAD_ACTION1) {
			if (g_player_input_records & DH_PAD_Z_POS) M32(entity, 16) += 0x1000;
			if (g_player_input_records & DH_PAD_Z_NEG) M32(entity, 16) -= 0x1000;
		}
		if (g_player_input_records & DH_PAD_ACTION3) {
			if (g_player_input_records & DH_PAD_Z_POS) M32(entity, 12) -= 0x1000;
			if (g_player_input_records & DH_PAD_Z_NEG) M32(entity, 12) += 0x1000;
			if (g_player_input_records & DH_PAD_X_POS) M32(entity, 8)  += 0x1000;
			if (g_player_input_records & DH_PAD_X_NEG) M32(entity, 8)  -= 0x1000;
		}
	}

	mtx_build_fwd_inv_entry(entity);
}
/* fidelity: HIGH - hand-traced vs --asm; bounded to real extent 0x20e1e2..0x20e29c (m2c over-swallowed siblings); input word re-read each test as in asm. */
