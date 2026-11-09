/* audio_send_command_buffer [0x02215e20]: SCSP mailbox handshake, bounded wait, command-buffer copy and timeout recovery. */
#include "dh_band21.h"
/* 0x02215e20 (sound_send_command) - hand a command buffer to the sound driver via the SCSP
 * mailbox. Clear g_060E0DE4; write 0xA00005FF to *(u32*)0x25A03C20; wait (up to 0x7A120
 * polls) for ack *(u16*)0x25A03EFE == 0xFFFF, calling func_02214970 on timeout; copy
 * (len>>2) longs from buf to 0x25A04000; on timeout reset the mailbox (0x25A03C20=0,
 * func_02214980, 0x25A03EFE=0, settle delay) else just clear the ack; set g_060E0DE4 = 1.
 * fidelity: HIGH - len>>2 longs (arithmetic shift); poll limit 0x7A120; PIC targets resolved. */
extern void smpc_mask_port_shadow(void);
extern void smpc_clear_port_shadow_bit(void);
extern s32  g_audio_command_ready;

void audio_send_command_buffer(void *buf, s32 len) {
    volatile u32 *s, *d;
    s32 n, i;
    s32 timeout = 0;

    g_audio_command_ready = 0;
    MEMU32(0x25A03C20) = 0xA00005FF;

    i = 0;
    while (MEMU16(0x25A03EFE) != 0xFFFF) {
        if (++i > 0x7A120) {
            smpc_mask_port_shadow();
            timeout = 1;
            break;
        }
    }

    n = len >> 2;
    s = (volatile u32 *)buf;
    d = PTR(volatile u32, 0x25A04000);
    for (i = 0; i < n; i++)
        d[i] = s[i];

    if (timeout) {
        MEMU32(0x25A03C20) = 0;
        smpc_clear_port_shadow_bit();
        MEMU16(0x25A03EFE) = 0;
        for (i = 200000; i > 0; i -= 2) { }   /* post-reset settle delay */
    } else {
        MEMU16(0x25A03EFE) = 0;
    }
    g_audio_command_ready = 1;
}
