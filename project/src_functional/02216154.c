/* video_reset_buffers [0x02216154]: Resets double-buffer pointers and optionally clears transform state and video memory. */
/* 0x02216154  video_state_init: reset the render double-buffer bookkeeping and,
 * per the `mode` bitmask, (re)initialise the transform records and clear VRAM.
 *   buffer index @0x06099574 = 0; list pointers seeded at 0x06098f08 / 0x060b8b90
 *   / 0x060b8b94 / 0x060bac98 / 0x060bac9c; counters @0x060bacac..b4 = 0.
 *   mode&1: func_022161dc() + reset_displist_state() + clear_displist_slots().
 *   mode&4: clear 0x80*16 bytes of VDP1 fb @0x25f00000 (func_02213290).
 *   mode&2: clear 0x8000*16 bytes of VDP2 VRAM @0x25e00000 (func_02213290).
 * fidelity: faithful - absolute-address global stores, bsrf/bsr calls resolved
 *   (both bsrf -> func_02213290). */
#include "dh_band21.h"

extern void memory_clear_vec4_blocks(s32 base, s32 count);
extern void view_reset_transform_records(s32 a, s32 b);
extern void reset_displist_state(void);
extern void clear_displist_slots(void);

void video_reset_buffers(s32 mode) {
    MEM32(0x06099574) = 0;
    MEM32(DH_VDP2_REGISTER_SHADOW_ADDR) = 0x06098de8;
    MEM32(0x060b8b90) = 0x060b88f0 + 0xe0;   /* 0x060b89d0 */
    MEM32(0x060b8b94) = 0x060b8a40 + 0xe0;   /* 0x060b8b20 */
    MEM32(0x060bac98) = 0x060b9698;
    MEM32(0x060bac9c) = 0x060ba718;
    MEM32(0x060bacb4) = 0;
    MEM32(0x060bacb0) = 0;
    MEM32(0x060bacac) = 0;

    if (mode & 1) {
        view_reset_transform_records(0, 0x060b8b20);
        reset_displist_state();
        clear_displist_slots();
    }
    if (mode & 4) {
        memory_clear_vec4_blocks(0x25f00000, 0x0080);
    }
    if (mode & 2) {
        memory_clear_vec4_blocks(0x25e00000, 0x8000);
    }
}
