/* display_ramp_to_limit [0x022101ce]: Ramps a signed triple to plus or minus 255 and signals completion to the parent task. */
/* 0x022101ce (scene_handler_2101ce) - per-frame slide-and-clamp update of one
 * channel, sibling of func_022100f8 (same over-swallowed unit; this TU models
 * only 0x2101ce's own body, ending at the braf tail-call at 0x21028e).
 * base = task->unk8; speed = base->unk50; state = task->unk10 (s8); the s16
 * triple lives at base[0x34/0x36/0x38]; a "moving" flag (bit value 2) sits at
 * (task->unk14->unk8)->unk3C.
 *   state 0: raise the flag; pick direction (speed>=0 -> state 2, else state 1).
 *            (no seeding of the triple, unlike 0x2100f8.)
 *   state 1: base[0x34] += speed each frame; once it reaches the low clamp
 *            (<= -0xFF) snap all three to -0xFF, drop the flag, zero *task,
 *            else also step base[0x36]/base[0x38].
 *   state 2: symmetric toward the high clamp (>= 0xFF).
 * Always ends by tail-calling func_022386fc(&base[0x34]).
 * fidelity: HIGH - offsets match sibling 0x2100f8; signed s16 wrap via exts.w,
 *   clamp consts 0xff01=-0xFF / 0x00ff, flag bit 2 (|=2 / &=~2), braf tail-call
 *   resolved to 0x022386fc. */
#include "dh_band21.h"

extern void display_accumulate_channel(u32 channel); /* commit channel (tail call) */

void display_ramp_to_limit(u32 task)
{
	u32 base  = MEM32(task + 8);
	s32 speed = MEM32(base + 0x50);
	s32 state = MEM8(task + 0x10);
	s16 v;

	switch (state) {
	case 0:
		MEM32(MEM32(MEM32(task + 0x14) + 8) + 0x3c) |= 2;
		if (speed >= 0)
			MEM8(task + 0x10) = 2;
		else
			MEM8(task + 0x10) = 1;
		break;
	case 1:
		v = (s16)(MEM16(base + 0x34) + speed);
		MEM16(base + 0x34) = v;
		if (v > -0xff) {
			MEM16(base + 0x36) = (s16)(MEM16(base + 0x36) + speed);
			MEM16(base + 0x38) = (s16)(MEM16(base + 0x38) + speed);
		} else {
			MEM16(base + 0x38) = -0xff;
			MEM16(base + 0x36) = -0xff;
			MEM16(base + 0x34) = -0xff;
			MEM32(MEM32(MEM32(task + 0x14) + 8) + 0x3c) &= ~2;
			MEM32(task) = 0;
		}
		break;
	case 2:
		v = (s16)(MEM16(base + 0x34) + speed);
		MEM16(base + 0x34) = v;
		if (v >= 0xff) {
			MEM16(base + 0x38) = 0xff;
			MEM16(base + 0x36) = 0xff;
			MEM16(base + 0x34) = 0xff;
			MEM32(MEM32(MEM32(task + 0x14) + 8) + 0x3c) &= ~2;
			MEM32(task) = 0;
		} else {
			MEM16(base + 0x36) = (s16)(MEM16(base + 0x36) + speed);
			MEM16(base + 0x38) = (s16)(MEM16(base + 0x38) + speed);
		}
		break;
	}

	display_accumulate_channel(base + 0x34);
}
