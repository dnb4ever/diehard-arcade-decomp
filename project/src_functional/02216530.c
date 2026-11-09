/* display_flush_and_toggle_bank [0x02216530]: Runs display flush helpers then XORs parity word 06099574 with one. */
/* 0x02216530  toggle_after_init: run three init/update subroutines in order,
 * then flip the s32 flag @0x06099574 (0<->1).
 * fidelity: faithful - three bsr calls, mov.l/xor #1/mov.l on the flag (rts
 *   delay slot stores the result). */
#include "dh_band21.h"

extern void display_flush_transfer_banks(void);
extern void vdp2_flush_shadow_banks(void);
extern void flush_dirty_dma_records(void);

void display_flush_and_toggle_bank(void) {
    display_flush_transfer_banks();
    vdp2_flush_shadow_banks();
    flush_dirty_dma_records();
    MEM32(0x06099574) ^= 1;
}
