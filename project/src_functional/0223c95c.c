/* effect_pool_reset_scene [0x0223c95c]: Clears pooled effects and copies the scene-selected pose and effect parameter tables. */
#include "dh.h"
/* 0x0223c95c - stage/effect state initialiser (void). Clears the big effect
 * state block at g_060e0e88 (eight 8-word sub-slots at +0xe4 set to -1, the two
 * parallel slot arrays at +0x84 and +0xa4 zeroed, head word zeroed), copies three
 * fixed config tables (20/80/80 shorts) and one 20-byte table into the live
 * HWRAM working copies, zeroes two small scratch arrays, then selects a pose/
 * layout template from g_06091704[] (index g_06098668, +0x52 unless g_06098670==1)
 * and scatters its 27 shorts + 1 byte into the working tables.
 * The ROM unrolls the table copies 2x/8x; rewritten here as flat loops (identical
 * semantics). branches resolved: none (no computed branches).
 * fidelity: HIGH. */
extern u8    g_060e0e88[];   /* effect state block; +0x84/+0xa4 slot arrays, +0xe4 masks */
extern u8    g_060e0e58[];
extern u8    g_060e0e78[];
extern u8    g_060e1080[];
extern u8    g_060e10a8[];
extern u8    g_060e1148[];
extern u8    g_060e106c[];
extern u8    g_060e1208[];
extern u8    g_060e120c[];
extern u8    g_060e1214[];
extern s16   g_0608d050[];
extern s16   g_0608d078[];
extern s16   g_0608d118[];
extern s8    g_0608d1b8[];
extern s16  *g_06091704[];
extern s32   g_scene_index;
extern s32   g_play_session_mode;

void effect_pool_reset_scene(void) {
    s32 i, j, idx;
    s16 *s;

    /* clear the eight effect slots and their parallel state/substate arrays */
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++)
            M32(g_060e0e88, 0xe4 + i * 32 + j * 4) = -1;
        M32(g_060e0e88, 0x84 + i * 4) = 0;
        M32(g_060e0e88, 0xa4 + i * 4) = 0;
    }
    M32(g_060e0e88, 0) = 0;

    M16(g_060e1214, 0) = 0;
    for (i = 0; i < 20; i++)
        M16(g_060e1080, i * 2) = g_0608d050[i];
    for (i = 0; i < 80; i++)
        M16(g_060e10a8, i * 2) = g_0608d078[i];
    for (i = 0; i < 80; i++)
        M16(g_060e1148, i * 2) = g_0608d118[i];
    for (i = 0; i < 20; i++)
        M8(g_060e106c, i) = g_0608d1b8[i];

    for (i = 0; i < 8; i++)
        M32(g_060e0e58, i * 4) = 0;
    for (i = 0; i < 16; i++)
        M8(g_060e0e78, i) = 0;

    idx = g_scene_index;
    if (g_play_session_mode != 1)
        idx += 0x52;
    s = g_06091704[idx];

    M16(g_060e1080, 0x0a) = s[0];
    M16(g_060e1080, 0x0c) = s[1];
    M16(g_060e10a8, 0x28) = s[2];
    M16(g_060e10a8, 0x2a) = s[3];
    M16(g_060e10a8, 0x2c) = s[4];
    M16(g_060e10a8, 0x2e) = s[5];
    M16(g_060e10a8, 0x30) = s[6];
    M16(g_060e10a8, 0x32) = s[7];
    M16(g_060e10a8, 0x34) = s[8];
    M16(g_060e10a8, 0x36) = s[9];
    M16(g_060e1080, 0x14) = s[10];
    M16(g_060e1080, 0x16) = s[11];
    M16(g_060e10a8, 0x50) = s[12];
    M16(g_060e10a8, 0x52) = s[13];
    M16(g_060e10a8, 0x54) = s[14];
    M16(g_060e10a8, 0x56) = s[15];
    M16(g_060e10a8, 0x58) = s[16];
    M16(g_060e10a8, 0x5a) = s[17];
    M16(g_060e10a8, 0x5c) = s[18];
    M16(g_060e10a8, 0x5e) = s[19];
    M16(g_060e1208, 0x00) = s[20];
    M16(g_060e1208, 0x02) = s[21];
    M32(g_060e120c, 0x00) = (s32)s[22] << 12;
    M32(g_060e120c, 0x04) = (s32)s[23] << 12;
    M8(g_060e106c, 0x05) = (s8)s[24];
    M8(g_060e106c, 0x06) = (s8)s[25];
    M8(g_060e106c, 0x0a) = (s8)s[26];
    M8(g_060e106c, 0x0b) = ((s8 *)s)[55];
}
