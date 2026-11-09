/* hud_status_config_task [0x02203c28]: Emits configuration-selected HUD groups from resident peripheral and input state. */
#include "dh_work.h"
#include "dh.h"

/* 0x02203c28 - scene_hud_handler_cfg5: a HUD draw-command emitter task handler.
 * r4 = task; ctx = task->unk8. Issues draw groups through the global dispatch
 * function-pointer slot *0x0609029c, writing the command-block fields
 * 0x060997a4 (type) and 0x060997a8 (code) before each call, exactly like the
 * sibling handlers (cf. 0x02203500).
 *
 * Flow:
 *   - call the BIOS/HWRAM function pointer *0x06000660 with ctx-block 0x060987d8.
 *   - if (ctx->unk80 & g_060987bc) == 0: bump parity counter ctx->unk64; on ODD
 *     emit the full element sequence, on EVEN emit three plain dispatches of
 *     *0x060902f0 (codes 9,10,11).
 *   - always: if (*(u16)0x0609879a & 3) != 0, emit the full element sequence.
 * The full sequence (identical in both places) is factored into the static
 * helper below to avoid transcription drift.
 */

#define G32(a) (*(s32 *)(uintptr_t)(a))
#define G8(a)  (*(u8  *)(uintptr_t)(a))

extern void hud_copy_status_template(s32 arg0);             /* 0x203de8 */
extern void hud_encode_status_item(s32 arg0, u8 *arg1);   /* 0x203e0a */
extern void hud_encode_config_nibble(s32 arg0, s32 arg1);   /* 0x203e5e */

static void hud_emit_sequence(void)
{
	void (**dispatch)(s32) = (void (**)(s32))(uintptr_t)0x0609029c;

	G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
	G32(DH_UI_CURSOR_ROW_ADDR) = 9;
	(*dispatch)(G32(0x060902cc + (G8(0x060987db) << 2)));

	if (G8(0x060987d4) == 0) {
		if ((G8(0x060987e9) & 0xF) > 1) {
			hud_copy_status_template(7);
			hud_encode_config_nibble(1, 1);
			G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
			G32(DH_UI_CURSOR_ROW_ADDR) = 10;
			(*dispatch)((s32)0x0609d7b0);
		}
	} else {
		if ((G8(0x060987e9) >> 4) > 1) {
			hud_copy_status_template(8);
			hud_encode_config_nibble(0, 0);
			G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
			G32(DH_UI_CURSOR_ROW_ADDR) = 10;
			(*dispatch)((s32)0x0609d7b0);
		}
	}

	hud_copy_status_template(G8(0x060987d8) > 1 ? 6 : 5);
	hud_encode_status_item(14, (u8 *)(uintptr_t)0x060987d8);
	G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
	G32(DH_UI_CURSOR_ROW_ADDR) = 11;
	(*dispatch)((s32)0x0609d7b0);
}

void hud_status_config_task(void *task)
{
	void *ctx = TASK_PTR(task, work);
	void (**biosfn)(void *) = (void (**)(void *))(uintptr_t)0x06000660;
	void (**dispatch)(s32) = (void (**)(s32))(uintptr_t)0x0609029c;

	(*biosfn)((void *)(uintptr_t)0x060987d8);

	if ((M32(ctx, 80) & G32(DH_FRAME_TICK_ADDR)) == 0) {
		s32 cnt = M32(ctx, 64) + 1;
		M32(ctx, 64) = cnt;
		if (cnt & 1) {
			hud_emit_sequence();
		} else {
			G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
			G32(DH_UI_CURSOR_ROW_ADDR) = 9;
			(*dispatch)(G32(0x060902f0));
			G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
			G32(DH_UI_CURSOR_ROW_ADDR) = 10;
			(*dispatch)(G32(0x060902f0));
			G32(DH_UI_CURSOR_COLUMN_ADDR) = 11;
			G32(DH_UI_CURSOR_ROW_ADDR) = 11;
			(*dispatch)(G32(0x060902f0));
		}
	}

	if ((MU16(0x06098794, 6) & 3) != 0)
		hud_emit_sequence();
}

/* fidelity: HIGH on control flow and command fields. The dispatch slot
 * *0x0609029c is modeled as void(*)(s32) per the established house convention
 * (02203500 et al.); the asm also presets r5 before some dispatch calls, which
 * the one-arg model drops - correct only if the HWRAM callee ignores r5 (it is
 * not in-cart and cannot be verified here). func_02203de8/e0a/e5e resolved to
 * existing siblings. The ODD/second-half element sequence is identical and
 * shared via hud_emit_sequence(). */
