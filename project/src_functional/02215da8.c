/* audio_load_driver [0x02215da8]: Loads sound driver into sound RAM and initialises SCSP control. */
#include "dh_band21.h"
/* 0x02215da8 (sound_init) - bring up the SCSP sound system. Set SCSP ctrl *(u16*)0x25B00400
 * = 0x0300; clear two sound-RAM regions via func_02213290; copy the sound driver program
 * (0x1000 longs @0x23000000 -> 0x25A00000) and its data (0x1B000 longs @0x23004000 ->
 * 0x25A04000); kick the driver (func_02214980); spin a warm-up delay; if sound is enabled
 * (g_060E0DDC) push command 0xA00200FF; mark initialised (g_060E0DE4 = 1).
 * fidelity: HIGH - copy counts from the unrolled loops (0x1000 / 0x1B000 longs). */
extern void memory_clear_vec4_blocks(s32 addr, s32 count);
extern void smpc_clear_port_shadow_bit(void);
extern void sound_queue_push(s32 cmd);   /* sound_queue_push */
extern s32  g_sound_effects_enabled_legacy;               /* sound enable */
extern s32  g_audio_command_ready;               /* sound-init done flag */

void audio_load_driver(void) {
    volatile u32 *s, *d;
    s32 i;

    MEMU16(SCSP_COMMON_CONTROL_ADDR) = 0x0300;
    memory_clear_vec4_blocks(0x25A70000, 0x1000);
    memory_clear_vec4_blocks(0x25A00000, 0x0400);

    s = PTR(volatile u32, 0x23000000);
    d = PTR(volatile u32, 0x25A00000);
    for (i = 0; i < 0x1000; i++)
        d[i] = s[i];

    s = PTR(volatile u32, 0x23004000);
    d = PTR(volatile u32, 0x25A04000);
    for (i = 0; i < 0x1B000; i++)
        d[i] = s[i];

    smpc_clear_port_shadow_bit();

    for (i = 10000; i > 0; i -= 2) { }   /* warm-up delay */

    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push((s32)0xA00200FF);
    g_audio_command_ready = 1;
}
