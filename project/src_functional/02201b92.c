/* boot_read_region_input [0x02201b92]: Resets input/audio and selects the initial scene from region and peripheral configuration. */
#include "dh.h"
/* 0x0201b92 (boot input/region config) - sibling of boot_globals_init (0x02201958), called from
 * the boot state machine func_022016ac. Initialises sound (sound_queue_reset + func_0220f990), then:
 *  - region gate: if the region table g_060511DC[(g_06000248>>12)&0xF] != 1, set g_06098668 = 1;
 *    otherwise clear it and read the peripheral/SMPC buffer via the BIOS callback *0x06000660
 *    into g_060987D8, then flag g_06098668 = 1 for specific controller types (status byte unk10
 *    bits 0/2, and peripheral-id bytes unk3/unk7 == 2);
 *  - clears a batch of state globals (g_060987D0 gated by config bit 5, g_060E0DE0, g_06098580,
 *    g_0609856C, g_06098570, g_0609979A, ctx->unkD);
 *  - 4 read-modify-writes of the config byte g_060987EC[g_06098808] (&0xF8,|3,&0xC7,|0x18);
 *  - language g_06099794 = region default g_060511D2; zeroes two 2-byte state blocks
 *    (g_060987D4, g_06098674); sound-enable g_060E0DDC = (g_06000798 & 0x40) ? 1 : 0.
 * fidelity: HIGH control flow / widths from the stores (region table long, g_0609868F/config
 * bytes byte, g_0609979A/language word, flags long); computed calls resolved via
 * braf_rewrite.py (bsrf -> sound_queue_reset / func_0220f990). The two init calls' argument arity
 * is MED (arg0 is spilled to stack but not re-passed). */
extern void sound_queue_reset(void);
extern void input_reset_records(void);
extern void (*g_06000660)(void *);  /* BIOS peripheral-read callback slot */
extern u32  g_06000248;             /* BIOS config word */
extern u32  g_06000798;             /* BIOS dip/config word */
extern s32  g_060511DC[];           /* region stride table */
extern s16  g_060511D2;             /* region default language */
extern u8   g_060987D8[];           /* peripheral/SMPC data buffer */
extern u8   g_bios_status_buffer[];           /* config byte array (indexed by g_06098808) */
extern u8   g_060987D4[];           /* 2-byte state block */
extern u8   g_player_join_states[];           /* 2-byte state block */
extern s32  g_scene_index, g_060987D0, g_audio_latched_sound_id, g_scene_event_flags, g_0609856C, g_06098570;
extern s32  g_06098808, g_sound_effects_enabled_legacy;
extern s8   g_0609868F;
extern s16  g_actor_pose_set, g_06099794;

void boot_read_region_input(void *ctx) {
    sound_queue_reset();
    input_reset_records();
    if (g_060511DC[(g_06000248 >> 0xC) & 0xF] != 1) {
        g_scene_index = 1;
    } else {
        g_scene_index = DH_SCENE_SELECTOR;
        g_06000660(g_060987D8);
        if (!(MU8(g_060987D8, 0x10) & 1)) {
            if (MU8(g_060987D8, 0x10) & 4) {
                if (MU8(g_060987D8, 3) == 2 || MU8(g_060987D8, 7) == 2)
                    g_scene_index = 1;
            } else if (MU8(g_060987D8, 3) == 2) {
                g_scene_index = 1;
            }
        }
    }
    g_0609868F = 2;
    if (((g_06000798 >> 5) & 1) == 1)
        g_060987D0 = 0;
    g_audio_latched_sound_id = 0;
    g_scene_event_flags = 0;
    g_0609856C = 0;
    g_06098570 = 0;
    g_actor_pose_set = 0;
    M8(ctx, 0xD) = 0;
    g_bios_status_buffer[g_06098808] = (u8)(g_bios_status_buffer[g_06098808] & 0xF8);
    g_bios_status_buffer[g_06098808] = (u8)(g_bios_status_buffer[g_06098808] | 3);
    g_bios_status_buffer[g_06098808] = (u8)(g_bios_status_buffer[g_06098808] & 0xC7);
    g_bios_status_buffer[g_06098808] = (u8)(g_bios_status_buffer[g_06098808] | 0x18);
    g_06099794 = g_060511D2;
    M8(g_060987D4, 1) = 0;
    M8(g_060987D4, 0) = 0;
    M8(g_player_join_states, 1) = 0;
    M8(g_player_join_states, 0) = 0;
    g_sound_effects_enabled_legacy = (g_06000798 & 0x40) ? 1 : 0;
}
