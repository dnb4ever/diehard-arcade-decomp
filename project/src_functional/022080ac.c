#include "dh_work.h"
#include "dh.h"
/* 0x022080ac (line_scroll_fx_sm) - a VDP2 line-scroll distortion effect state
 * machine. It writes a per-line offset pattern into the display work buffer through
 * the cursor g_06099510 and flushes it to the VDP2 line-scroll VRAM (0x25E3FC00)
 * via func_0221632e, with a decaying wobble on intro and a shrinking slide on outro.
 *
 * ABI(task): task = r4 (r10); entity = task->unk8 (r13). The cursor g_06099510 holds
 * an s32 write pointer: each push stores a longword and advances it by 4. Dispatch on
 * task->unk10 (byte), a cmp/eq chain rendered as a C switch.
 *
 *   state 0: func_0221632e(2, cursor, 0x25E3FC00, 0xF0); push 0xF0 longwords of the
 *            alternating wobble (+amp,-amp,... amp = entity->unk8); decay amp to 3/4
 *            (unk8 += -unk8 >> 2). When amp <= 0x10000: task->unk10 = 1,
 *            entity->unk30 = 0x24 (hold timer), re-flush func_0221632e(2, cursor,
 *            0x25E3FC00, 0xF0), then push 0xF0 zero longwords.
 *   state 1: count entity->unk30 down; on expiry task->unk10 = 2, entity->unkC = 0,
 *            entity->unk18 = 0xA (slide step/accel).
 *   state 2: g_060B3D50 = 1; func_022213a8(0, -entity->unkC << 16); unk18 -= 2;
 *            unkC += unk18; when unkC < -0x78: func_02221384(1); task->unk0 = 0 (detach).
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x2080ac..0x20822c). bsrf
 * targets resolved from the pool deltas against braf_resolve (braf_rewrite mislabels
 * the state-2 calls): func_0221632e (0x0221632e, both state-0 calls),
 * func_022213a8 (0x022213a8), func_02221384 (0x02221384). */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))

extern void vdp2_queue_attribute(s32 a, s32 cursor, s32 dst, s32 count); /* 0x0221632e */
extern void vdp2_set_layer_scroll(s32 a, s32 b);                          /* 0x022213a8 */
extern void vdp2_disable_layer_bit(s32 a);                                 /* 0x02221384 */

/* Advance g_06099510, returning the old slot for the following store. */
static s32 *reserve_line(void) {
    s32 *p = *(s32 **)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR;
    /* The ROM publishes the advanced cursor before filling the old slot. */
    *(s32 **)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR = p + 1;
    return p;
}

#define CURSOR_VAL  (*(s32 *)(uintptr_t)DH_DISPLAY_TRANSFER_CURSOR_ADDR)

void line_scroll_fx_sm(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 i;

    switch (TASK_U8(task, state)) {
    case 0: {
        s32 amp;
        vdp2_queue_attribute(2, CURSOR_VAL, 0x25E3FC00, 0xF0);
        i = 0;
        do {
            s32 *dst = reserve_line();
            *dst = M32(entity, 0x8);
            dst = reserve_line();
            *dst = -M32(entity, 0x8);
            dst = reserve_line();
            *dst = M32(entity, 0x8);
            dst = reserve_line();
            *dst = -M32(entity, 0x8);
            dst = reserve_line();
            *dst = M32(entity, 0x8);
            i += 6;
            dst = reserve_line();
            *dst = -M32(entity, 0x8);
        } while (i < 0xF0);

        amp = M32(entity, 0x8) + (-M32(entity, 0x8) >> 2);
        M32(entity, 0x8) = amp;
        if (amp <= 0x10000) {
            TASK_U8(task, state) = 1;
            M16(entity, 0x30) = 0x24;
            vdp2_queue_attribute(2, CURSOR_VAL, 0x25E3FC00, 0xF0);
            i = 0xF0;
            do {
                i -= 3;
                *reserve_line() = 0;
                *reserve_line() = 0;
                *reserve_line() = 0;
            } while (i != 0);
            return;
        }
        break;
    }
    case 1: {
        s16 t = M16(entity, 0x30) - 1;
        M16(entity, 0x30) = t;
        if (t <= 0) {
            TASK_U8(task, state) = 2;
            M32(entity, 0xC)  = 0;
            M32(entity, 0x18) = 0xA;
        }
        return;
    }
    case 2: {
        s32 c;
        RU8A(0x060B3D50) = 1;
        vdp2_set_layer_scroll(0, -M32(entity, 0xC) << 16);
        M32(entity, 0x18) -= 2;
        c = M32(entity, 0xC) + M32(entity, 0x18);
        M32(entity, 0xC) = c;
        if (c < -0x78) {
            vdp2_disable_layer_bit(1);
            TASK_PTR(task, handler) = 0;
        }
        break;
    }
    }
}
