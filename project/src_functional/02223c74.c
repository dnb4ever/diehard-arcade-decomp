/* frame_measure_pulse_timing [0x02223c74]: Increments the frame tick and measures the shared low/high pulse widths until the frame gate latches. */
#include "dh.h"
/* 0x02223c74  pulse-width timing calibration: with the busy flag 0x060E0BC8 raised and the frame
 * counter 0x060987BC bumped, repeatedly measure the low/high dwell of the polled signal byte
 * 0x060986AF (gated by 0x060986AE) - counting zero samples into n0 and each high run into
 * 0x060986A4 while summing them into n1 - until the gate latches 1. The grand total (n1 + n0)
 * is stored to 0x060986A0, the gate is cleared, and the busy flag plus 0x0609869C are reset.
 * fidelity: HIGH - exact; polled bytes modelled volatile. */
extern volatile s8 g_060986AF;   /* polled signal level */
extern volatile s8 g_060986AE;   /* measurement gate (latches 1 when done) */
extern s32 g_060E0BC8;           /* busy flag */
extern s32 g_frame_tick;           /* frame counter */
extern s32 g_060986A4;           /* last high-run length */
extern s32 g_060986A0;           /* accumulated total */
extern s32 g_0609869C;
void frame_measure_pulse_timing(void) {
    s32 n0 = 0;
    s32 n1 = 0;
    g_060E0BC8 = 1;
    g_frame_tick += 1;
    do {
        s32 run;
        while (g_060986AF == 0) {
            n0++;
        }
        run = 0;
        while (g_060986AF == 1) {
            run++;
        }
        g_060986A4 = run;
        n1 += run;
    } while (g_060986AE != 1);
    g_060986A0 = n1 + n0;
    g_060986AE = 0;
    g_060E0BC8 = 0;
    g_0609869C = 0;
}
