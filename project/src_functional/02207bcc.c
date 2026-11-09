#include "dh_work.h"
#include "dh.h"
/* 0x02207bcc (screen_fx_builder_task) - task handler for a layered full-screen effect that
 * builds VDP command/display lists and animates a palette register across a 6-state machine
 * (task->unk10, byte @ +0x10). Three phases, each a "setup" state that falls through into an
 * "animate" state:
 *   0 -> 1 : phase A  (build gradient + tilemaps, ease unkC, ramp palette index unk32 to 31)
 *   2 -> 3 : phase B  (build gradient, sin-animate the palette register, decay unkC by 3)
 *   4 -> 5 : phase C  (build gradient, sin-animate with a bouncing unkC/unk18 integrator)
 *
 * ABI: arg0 = task; entity = task->unk8 (the effect's work struct: unkC/unk18 = s32 driver
 * fields, unk30/unk32 = u16 frame/index counters). g_06099510 is the display-list write
 * cursor (a pointer); the effect appends 32-bit words to it and DMAs/fills VDP1 VRAM via it.
 *
 * Emit helper func_0221632e is a FIXED 4-arg call (mode, src/cursor, dst, count) - m2c's
 * apparent variadic calls (the trailing "4, 0x203, ..." args) are actually the gradient
 * fill-loop stores to *g_06099510, mis-folded into the call.
 *
 * TRAPS handled: braf_resolve reports two phantom "bsrf" at cart 0x207d66 / 0x207ea4 - both
 * are literal-pool shorts (0x0203) sitting between code blocks, NOT instructions (verified:
 * .Lpool_207d66 / .Lpool_207ea4). The real computed calls are func_0221632e (jsr @r10),
 * fill_vec4_array (0x2132ac), tilemap_copy_wrap_w80 (0x21675e), func_022213a8 (0x2213a8) and
 * sin_lookup (0x213190). The compiler's 3x-unrolled gradient loops were re-rolled
 * (0xF00000/0x10000 = 240 is a multiple of 3, so the single-step loop is equivalent).
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x207bcc..0x207fde) and all literal pools
 * read from cart.bin. Every emit's cursor arg, every VDP/ROM address and every state
 * transition verified against the asm. */

#define A8(a)  (*(s8  *)(uintptr_t)(a))
#define A32(a) (*(s32 *)(uintptr_t)(a))
#define AP(a)  (*(void **)(uintptr_t)(a))
#define VRAM(a) ((void *)(uintptr_t)(a))

extern s32 *g_display_transfer_cursor;    /* display-list write cursor */

extern s32  sin_lookup(s32 angle);                                             /* 0x213190 */
extern void vdp2_queue_attribute(s32 mode, s32 cur, void *dst, s32 count);            /* 0x21632e */
extern void fill_vec4_array(void *base, s32 count, s32 value);                 /* 0x2132ac */
extern void tilemap_copy_wrap_w80(s32 a0, void *dst, void *desc,
                                  s32 a3, s32 a4, s32 a5);                      /* 0x21675e */
extern void vdp2_set_layer_scroll(s32 rot, s32 pos);                                   /* 0x2213a8 */

void screen_fx_builder_task(void *task)
{
    void *entity = TASK_PTR(task, work);
    s32 v, i;
    s16 old;

    switch (TASK_S8(task, state)) {
    case 0:
        vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e78000), 0x1000);
        fill_vec4_array(g_display_transfer_cursor, 0x400, 0x0c000c00);
        tilemap_copy_wrap_w80(0x0c00, g_display_transfer_cursor, VRAM(0x226b1e34), 22, 4, 0x203);
        tilemap_copy_wrap_w80(0x0c00, g_display_transfer_cursor, VRAM(0x226b1e3c),  8, 4, 0x203);
        tilemap_copy_wrap_w80(0x0c00, g_display_transfer_cursor, VRAM(0x226b1e3c), 70, 4, 0x203);
        g_display_transfer_cursor = (s32 *)((u8 *)g_display_transfer_cursor + 0x4000);
        vdp2_queue_attribute(2, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e3fc00), 0x00f0);
        for (v = 0; v < 0xF00000; v += 0x10000)
            *g_display_transfer_cursor++ = v;
        TASK_S8(task, state) = 1;
        M32(entity, 0xC) = 0x00f0;
        M16(entity, 0x32) = 0;
        /* fallthrough */
    case 1:
        vdp2_queue_attribute(14, A32(0x226b1de8 + (A32(DH_FRAME_TICK_ADDR) & 1) * 8),
                      VRAM(0x25f00200), 8);
        A8(0x060b3d50) = 1;
        vdp2_set_layer_scroll(0, (-M32(entity, 0xC)) << 16);
        M32(entity, 0xC) += (4 - M32(entity, 0xC)) >> 3;
        if (M32(entity, 0xC) <= 4 && M16(entity, 0x32) < 31) {
            old = M16(entity, 0x32);
            M16(entity, 0x32) = old + 1;
            *(u16 *)((u8 *)AP(DH_VDP2_REGISTER_SHADOW_ADDR) + 0x108) = (u16)(old << 8);
            if (M16(entity, 0x32) == 31) {
                vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e78000), 0x1000);
                fill_vec4_array(g_display_transfer_cursor, 0x400, 0x0c000c00);
            }
        }
        if (M8(TASK_PTR(task, parent), 0xF) == 0)
            return;
        if (A32(0x0609856c) & 1)
            TASK_S8(task, state) = 2;
        else
            TASK_S8(task, state) = 4;
        return;
    case 2:
        vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e78000), 0x1000);
        vdp2_queue_attribute(14, A32(0x226b1e28), VRAM(0x25f00200), 8);
        fill_vec4_array(g_display_transfer_cursor, 0x400, 0x0c000c00);
        tilemap_copy_wrap_w80(0x0c00, g_display_transfer_cursor, VRAM(0x226b1e44), 22, 4, 0x203);
        g_display_transfer_cursor = (s32 *)((u8 *)g_display_transfer_cursor + 0x4000);
        TASK_S8(task, state) = 3;
        M32(entity, 0xC) = 80;
        M16(entity, 0x30) = 0;
        M16(entity, 0x32) = 12;
        /* fallthrough */
    case 3:
        vdp2_queue_attribute(2, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e3fc00), 0x00f0);
        for (i = 0; i < M32(entity, 0xC); i++)
            *g_display_transfer_cursor++ = M32(entity, 0xC) << 16;
        for (v = i << 16; v < 0xF00000; v += 0x10000)
            *g_display_transfer_cursor++ = v;
        if (M32(entity, 0xC) > 0)
            M32(entity, 0xC) -= 3;
        old = M16(entity, 0x30);
        M16(entity, 0x30) = old + 1;
        *(u16 *)((u8 *)AP(DH_VDP2_REGISTER_SHADOW_ADDR) + 0x108) =
            (u16)(((sin_lookup(old << 13) >> 13) + M16(entity, 0x32)) << 8);
        return;
    case 4:
        vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e78000), 0x1000);
        vdp2_queue_attribute(14, A32(0x226b1e30), VRAM(0x25f00200), 8);
        fill_vec4_array(g_display_transfer_cursor, 0x400, 0x0c000c00);
        tilemap_copy_wrap_w80(0x0c00, g_display_transfer_cursor, VRAM(0x226b1e4c), 24, 4, 0x203);
        g_display_transfer_cursor = (s32 *)((u8 *)g_display_transfer_cursor + 0x4000);
        vdp2_queue_attribute(2, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e3fc00), 0x00f0);
        for (v = 0; v < 0xF00000; v += 0x10000)
            *g_display_transfer_cursor++ = v;
        TASK_S8(task, state) = 5;
        M32(entity, 0xC) = -120;
        M32(entity, 0x18) = 8;
        M16(entity, 0x30) = 0;
        M16(entity, 0x32) = 12;
        /* fallthrough */
    case 5:
        A8(0x060b3d50) = 1;
        vdp2_set_layer_scroll(0, (-M32(entity, 0xC)) << 16);
        M32(entity, 0x18) += 2;
        M32(entity, 0xC) += M32(entity, 0x18);
        if (M32(entity, 0xC) > 4) {
            M32(entity, 0xC) = 4;
            M32(entity, 0x18) = (-M32(entity, 0x18)) >> 2;
        }
        old = M16(entity, 0x30);
        M16(entity, 0x30) = old + 1;
        *(u16 *)((u8 *)AP(DH_VDP2_REGISTER_SHADOW_ADDR) + 0x108) =
            (u16)(((sin_lookup(old << 13) >> 13) + M16(entity, 0x32)) << 8);
        return;
    }
}
