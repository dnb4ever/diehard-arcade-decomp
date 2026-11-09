/* background_tick_perspective_strips [0x02208c44]: Three task states emit 0x2d0-word transfers and sine-driven strip geometry. */
#include "dh_work.h"
#include "dh.h"
/* 0x02208c44 func_02208c44 (role: perspective_strip_builder) - task handler, sibling of screen_fx_builder_task
 * (0x02207bcc): it builds VDP1 display/command lists via the write cursor g_06099510 and runs a
 * 3-state machine on the byte task->unk10. It is one of the handlers referenced by the 0x200-band
 * loader/camera-init.
 *
 * ABI: arg0 = task; entity = task->unk8 (work struct: unk8/unkC/unk18 = s32 driver fields,
 * unk32 = u16 frame/row index). g_06099510 is the display-list write cursor (a pointer that is
 * post-incremented by one 32-bit word per *g_06099510++).
 *
 * State 0 (build): opens a 0x2D0-word DMA (func_0221632e mode 2 -> 0x25E37000) then fills a
 *   command/coefficient table in three bands, each column emitting a (scale, position, 1/depth)
 *   triple computed with a 16.16 perspective divide and a fixed-point multiply:
 *     cols 0..39   : X = 0x18000 - 0x333*i ; scale = fxmul(0x1600000, (X-0x10000)/X) ;
 *                    pos = ((i-40)/2 + 40)<<16 ; depth = 0x10000/X
 *     cols 40..191 : 0 ; i<<16 ; 0x10000
 *     cols 0..47   : X = 0x555*j + 0x10000 ; scale = fxmul(0x1600000,(X-0x10000)/X) ;
 *                    pos = (j/2 + 0xC0)<<16 ; depth = 0x10000/X
 *   then seeds entity (unkC=0, unk18=0xB333, unk32=0), pokes the VDP2 shadow block (*0x06098F08:
 *   +0x108=0x800, +0xF8 = (&0xFF)|0x700), sets state 1 and FALLS THROUGH into state 1.
 * State 1 (gate): if (*g_06098580 & 0x80000): clear that bit and advance to state 2; else idle.
 * State 2 (animate): integrate unkC by unk18 (zero unk18 once unk32>=0x46), plot via
 *   func_022213a8(0, unkC); only when the integer part of unkC crossed ((unk8^unkC)&0xFFF00000)
 *   does it rebuild two VDP1 cell rows from the ROM glyph table 0x0604F1DC + (unk32<<6): 16 words
 *   to 0x25E78000-band then 10 words to 0x25E78800-band, each word packing two bytes +0x700;
 *   finally unk32++.
 *
 * Resolved calls (jsr @reg / bsrf, hand-computed PIC via mova+add; m2c dropped/mis-typed them):
 *   func_0221632e   = mova 0x208c60 + 0xD6CE  -> 0x21632e (FIXED 4-arg: mode,cur,dst,count)
 *   fixmul_16_16    = mova 0x208ca0 + 0xA528  -> 0x2131c8 ((a*b)>>16)
 *   sh2_fxdiv_16_16 = mova 0x208ca8 + 0xA58C  -> 0x213234 (((s64)num<<16)/den via DIVU; the
 *                     notes/symbols "sh2_hw_divide num/den" is imprecise - it is a 16.16 divide)
 *   func_022213a8   = bsrf 0x208ecc + 0x184D8 -> 0x2213a8 (plot into *0x06098F08)
 *
 * NOTE: unlike screen_fx_builder_task, state 2 passes entity->unkC to func_022213a8 directly
 * (r4=0, r5=unkC) - no negate, no <<16. Confirmed in asm (@(12,r12) -> r5, mov #0,r4 in the
 * bsrf delay slot). The compiler unrolled the first/third build bands by 2 (re-rolled here; the
 * step counts are even) and the state-2 row loops by 2.
 *
 * fidelity: HIGH - hand-traced vs fn.py disasm (0x208c44..0x20903c), all pools read from cart.
 * m2c's output for this TU is heavily confused (stack slots and scalars mistyped as pointers),
 * so every store, cursor bump and call arg was reconstructed from the raw SH-2. */

#define A8(a)   (*(s8  *)(uintptr_t)(a))
#define A32(a)  (*(s32 *)(uintptr_t)(a))
#define AP(a)   (*(void **)(uintptr_t)(a))
#define VRAM(a) ((void *)(uintptr_t)(a))

extern s32 *g_display_transfer_cursor;    /* display-list write cursor */

extern void vdp2_queue_attribute(s32 mode, s32 cur, void *dst, s32 count);  /* 0x21632e */
extern s32  fixmul_16_16(s32 a, s32 b);                              /* 0x2131c8: (a*b)>>16 */
extern s32  sh2_fxdiv_16_16(s32 num, s32 den);                       /* 0x213234: (num<<16)/den */
extern void vdp2_set_layer_scroll(s32 rot, s32 pos);                        /* 0x2213a8 */

/* named func_02208c44 to match the extern in 02208b98.c (spawned via func_0220ff54) */
void background_tick_perspective_strips(void *task)
{
    void *entity = TASK_PTR(task, work);
    s32 i, j, k, X;
    u8 *src;

    switch (TASK_S8(task, state)) {
    case 0:
        vdp2_queue_attribute(2, (s32)(uintptr_t)g_display_transfer_cursor, VRAM(0x25e37000), 0x2d0);

        for (i = 0; i < 40; i++) {
            X = fixmul_16_16(-0x333, i << 16) + 0x18000;
            *g_display_transfer_cursor++ = fixmul_16_16(0x1600000, sh2_fxdiv_16_16(X - 0x10000, X));
            *g_display_transfer_cursor++ = (((i - 40) / 2) + 40) << 16;
            *g_display_transfer_cursor++ = sh2_fxdiv_16_16(0x10000, X);
        }
        for (i = 40; i < 0xC0; i++) {
            *g_display_transfer_cursor++ = 0;
            *g_display_transfer_cursor++ = i << 16;
            *g_display_transfer_cursor++ = 0x10000;
        }
        for (j = 0; j < 48; j++) {
            X = fixmul_16_16(0x555, j << 16) + 0x10000;
            *g_display_transfer_cursor++ = fixmul_16_16(0x1600000, sh2_fxdiv_16_16(X - 0x10000, X));
            *g_display_transfer_cursor++ = ((j / 2) + 0xC0) << 16;
            *g_display_transfer_cursor++ = sh2_fxdiv_16_16(0x10000, X);
        }

        M32(entity, 0xC) = 0;
        M32(entity, 0x18) = 0xB333;
        M16(entity, 0x32) = 0;
        M16(AP(DH_VDP2_REGISTER_SHADOW_ADDR), 0x108) = 0x800;
        M16(AP(DH_VDP2_REGISTER_SHADOW_ADDR), 0xF8) = (s16)(M16(AP(DH_VDP2_REGISTER_SHADOW_ADDR), 0xF8) & 0xFF);
        M16(AP(DH_VDP2_REGISTER_SHADOW_ADDR), 0xF8) = (s16)(M16(AP(DH_VDP2_REGISTER_SHADOW_ADDR), 0xF8) | 0x700);
        TASK_S8(task, state) = 1;
        /* fallthrough */

    case 1:
        if (!(A32(DH_SCENE_EVENT_FLAGS_ADDR) & 0x80000))
            return;
        A32(DH_SCENE_EVENT_FLAGS_ADDR) &= 0xFFF7FFFF;
        TASK_S8(task, state) = 2;
        return;

    case 2:
        M32(entity, 8) = M32(entity, 0xC);
        M32(entity, 0xC) += M32(entity, 0x18);
        if (M16(entity, 0x32) >= 0x46)
            M32(entity, 0x18) = 0;
        A8(0x060b3d50) = 1;
        vdp2_set_layer_scroll(0, M32(entity, 0xC));
        if (!((M32(entity, 8) ^ M32(entity, 0xC)) & 0xFFF00000))
            return;

        vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor,
                      VRAM(0x25e78000 + (((M16(entity, 0x32) + 0x10) & 0x1F) << 6)), 0x10);
        src = (u8 *)(uintptr_t)(0x0604f1dc + (M16(entity, 0x32) << 6));
        for (k = 0; k < 0x20; k += 2)
        {
            s32 *word = g_display_transfer_cursor;
            *word = (src[k] + 0x700) << 16;
            g_display_transfer_cursor = word + 1;
            *word |= src[k + 1] + 0x700;
        }

        vdp2_queue_attribute(12, (s32)(uintptr_t)g_display_transfer_cursor,
                      VRAM(0x25e78800 + (((M16(entity, 0x32) + 0x10) & 0x1F) << 6)), 0xA);
        for (k = 0x20; k < 0x34; k += 2)
        {
            s32 *word = g_display_transfer_cursor;
            *word = (src[k] + 0x700) << 16;
            g_display_transfer_cursor = word + 1;
            *word |= src[k + 1] + 0x700;
        }

        M16(entity, 0x32) = (s16)(M16(entity, 0x32) + 1);
        return;
    }
}
