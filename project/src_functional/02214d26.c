/* vdp1_trigger_draw [0x02214d26]: Updates VDP1 plot/erase setup and queues draw command. */
#include "dh_band21.h"
/* 0x02214d26 (vdp1_frame_swap) - hidden sibling of vdp1_frame_begin (merged into
 * member 0x02214cdc by fn.extent; recovered as an extra). Re-programs the VDP1
 * TV-mode / FBCR registers and local-coordinate words, pushes a '4' token onto the
 * command ring at *0x060E0DD4, kicks func_0223964e, and waits (twice) on the VBlank
 * ready bit polled through the cache-through mirror 0x260E0DD8 bit7.
 *   - MEMU16(0x060E0E30)=1; VDP1 regs 0x25D00004=2, 06=0, 08=0, 0A=0x58F0
 *   - fb=*0x06098F08; fb+0xF0/F2/F4/F6 = 0x0505; MEMU16(0x060C3CC0)=0x8000
 *   - wait (0x260E0DD8 & 0x80)==0x80; clear 0x060E0DD8
 *   - q=*0x060E0DD4; *0x060E0DD4=q+1; *q=4   (ring push)
 *   - func_0223964e()
 *   - wait again; clear 0x060E0DD8
 * fidelity: HIGH - read via uncached mirror, write via cached addr (as in asm). */
void debug_reset_model_preview(void);

void vdp1_trigger_draw(void) {
    u16 *fb;
    s32 **qp;
    s32 *q;

    MEMU16(0x060E0E30) = 1;
    MEMU16(VDP1_PTMR_ADDR) = 2;        /* PTMR */
    MEMU16(VDP1_EWDR_ADDR) = 0;
    MEMU16(VDP1_EWLR_ADDR) = 0;        /* EWLR */
    MEMU16(VDP1_EWRR_ADDR) = 0x58F0;   /* EWRR */

    fb = PTR(u16, *PTR(u32, DH_VDP2_REGISTER_SHADOW_ADDR));
    fb[0xF0 / 2] = 0x0505;
    fb[0xF2 / 2] = 0x0505;
    fb[0xF4 / 2] = 0x0505;
    fb[0xF6 / 2] = 0x0505;
    MEMU16(0x060C3CC0) = 0x8000;

    while ((MEM8(0x260E0DD8) & 0x80) != 0x80)
        ;
    MEM8(0x060E0DD8) = 0;

    qp = PTR(s32 *, 0x060E0DD4);
    q = *qp;
    *qp = q + 1;
    *q = 4;

    debug_reset_model_preview();

    while ((MEM8(0x260E0DD8) & 0x80) != 0x80)
        ;
    MEM8(0x060E0DD8) = 0;
}
