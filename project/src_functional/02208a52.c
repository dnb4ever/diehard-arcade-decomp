#include "dh_work.h"
#include "dh.h"
/* 0x02208a52 (disp_shutter_sm) - a small display state machine driven by the
 * global flag word 0x06098580 (bit 0x40000) that toggles the VDP2 display shadow
 * field unkF8 and then runs a decaying open/settle timer.
 *
 * ABI(task): task = r4; entity = task->unk8. disp = *0x06098F08 (game display
 * shadow). Dispatch on task->unk10 (byte), a cmp/eq chain rendered as a C switch.
 *
 *   state 0: disp->unkF8 &= 0xFF; task->unk10 = 1; (fallthrough)
 *   state 1: if (0x06098580 & 0x40000) { clear that bit; task->unk10 = 2; }  return.
 *   state 2: disp->unkF8 &= 0xFF; disp->unkF8 |= 0x600; task->unk10 = 3; (fallthrough)
 *   state 3: ease entity->unk10 toward 0x10000 (unk10 += (0x10000 - unk10) >> 4),
 *            then func_022214d2(0x160, 16, 0, 0, unk10, unk10) (a draw/transform call).
 *            On an even frame (!(0x060987BC & 1)):
 *              if (entity->unk32 > 0): while unk10 < 0x30000, decrement unk32 and set
 *                 disp->unk108 = (0x20 - unk32_orig) << 8; return.
 *              else (unk32 <= 0): func_02221384(1); task->unk0 = 0 (detach handler).
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x208a52..0x208b8c). bsrf
 * resolved: func_022214d2 (0x022214d2), func_02221384 (0x02221384). */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern void vdp2_configure_layer1_interpolation(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f); /* 0x022214d2 */
extern void vdp2_disable_layer_bit(s32 a);                                   /* 0x02221384 */

void disp_shutter_sm(void *task) {
    void *entity = TASK_PTR(task, work);
    void *disp   = PA(DH_VDP2_REGISTER_SHADOW_ADDR);

    switch (TASK_U8(task, state)) {
    case 0:
        M16(disp, 0xF8) = (s16)(M16(disp, 0xF8) & 0xFF);
        TASK_U8(task, state) = 1;
        /* fallthrough */
    case 1:
        if (R32A(DH_SCENE_EVENT_FLAGS_ADDR) & 0x40000) {
            R32A(DH_SCENE_EVENT_FLAGS_ADDR) &= (s32)0xFFFBFFFF;
            TASK_U8(task, state) = 2;
        }
        return;
    case 2:
        M16(disp, 0xF8) = (s16)(M16(disp, 0xF8) & 0xFF);
        disp = PA(DH_VDP2_REGISTER_SHADOW_ADDR);
        M16(disp, 0xF8) = (s16)(M16(disp, 0xF8) | 0x600);
        TASK_U8(task, state) = 3;
        /* fallthrough */
    case 3: {
        s32 v = M32(entity, 0x10) + ((0x10000 - M32(entity, 0x10)) >> 4);
        M32(entity, 0x10) = v;
        vdp2_configure_layer1_interpolation(0x160, 16, 0, 0, v, v);
        if (!(R32A(DH_FRAME_TICK_ADDR) & 1)) {
            if (M16(entity, 0x32) > 0) {
                if (M32(entity, 0x10) < 0x30000) {
                    s16 n = M16(entity, 0x32) - 1;
                    M16(entity, 0x32) = n;
                    M16(disp, 0x108) = (s16)((-(s16)(n + 1) + 0x20) << 8);
                    return;
                }
            } else {
                vdp2_disable_layer_bit(1);
                TASK_PTR(task, handler) = 0;
            }
        }
        break;
    }
    }
}
