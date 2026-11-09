/* isr_vblank_in [0x02223cf0]: Services VBlank input, frame-change setup, resident callbacks and input polling. */
#include "dh_band21.h"
/* 0x02223cf0 (isr_vblank_in) - the VBlank-IN interrupt handler (SCU vector 0x40,
 * installed by main_init_entry). Guarded by the re-entrancy byte 0x060986AC: if set,
 * returns immediately; otherwise sets it, clears 0x060986AD, sets 0x060986AF.
 * cur = *0x0609869C (frame counter). If drawing is active (*0x06099578 != 0): when
 * cur==0 it calls the BIOS-area callback *0x06090298; else when *0x060E0BC8==1 it
 * issues a VDP1 frame-change (TVMR 0x25D00000 = *0x060E0E30 | 8, FBCR 0x25D00002 = 3),
 * calls func_02216530 and sets 0x060986AE. If drawing is idle and cur >= *0x06098698
 * it sets FBCR = 2 and 0x060986AE. It then runs the two BIOS callbacks *0x06000610
 * and *0x06000668 (whose nonzero return is passed to func_02215f08), and tail-calls
 * func_0220f9d4 when *0x06098698 == *0x0609869C, else func_0220fb58.
 * fidelity: HIGH - control flow, widths and signed frame compare verified. */
typedef void (*vfp)(void);
typedef s32  (*ifp)(void);

extern void display_flush_and_toggle_bank(void);
extern void audio_queue_indexed_cue(s32 arg);
extern void input_poll_records(void);
extern void input_settle_injected_buttons(void);

void isr_vblank_in(void) {
    s32 cur;

    if (MEM8(0x060986AC) != 0)
        return;

    MEM8(0x060986AC) = 1;
    MEM8(0x060986AD) = 0;
    MEM8(0x060986AF) = 1;

    cur = MEM32(0x0609869C);

    if (MEM32(0x06099578) != 0) {
        if (cur == 0) {
            (*(vfp *)(uintptr_t)0x06090298)();
        } else if (MEM32(0x060E0BC8) == 1) {
            MEMU16(VDP1_TVMR_ADDR) = (u16)(MEMU16(0x060E0E30) | 8);
            MEMU16(VDP1_FBCR_ADDR) = 3;
            display_flush_and_toggle_bank();
            MEM8(0x060986AE) = 1;
        }
    } else {
        if (cur >= MEM32(0x06098698)) {
            MEMU16(VDP1_FBCR_ADDR) = 2;
            MEM8(0x060986AE) = 1;
        }
    }

    (*(vfp *)(uintptr_t)0x06000610)();
    {
        s32 r = (*(ifp *)(uintptr_t)0x06000668)();
        if (r != 0)
            audio_queue_indexed_cue(r);
    }

    if (MEM32(0x06098698) == MEM32(0x0609869C))
        input_poll_records();
    else
        input_settle_injected_buttons();
}
