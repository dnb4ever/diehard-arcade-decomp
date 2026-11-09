/* actor_sound_emit_dir [0x02241822]: Existing map corroborated by source: emit directional sound (gated g_060E0DDC): id=g_060542B8[((u8)(a+0x49)<<1)+((a+0x64)>16)]; func_02215f6c((id<<8)|0xA00000FF) */
#include "dh.h"
/* 0x02241822 (actor_sound_emit_dir) - emit a directional sound for an actor, gated
 * on the sound-enable flag g_060E0DDC. Picks a sound id from table g_060542B8 indexed
 * by (facing<<1)+closeFlag, facing=(u8)(a+0x49), closeFlag=((a+0x64)>16).
 *   cmd = (id << 8) | 0xA00000FF ; sound_queue_push(cmd)  (sound queue push)
 * fidelity: HIGH - mov.b facing (extu.b), mov.w id (sign-extended), bsrf resolved. */
extern s32 g_sound_effects_enabled_legacy;
extern s16 g_060542B8[];
void sound_queue_push(u32 cmd);

void actor_sound_emit_dir(void *a) {
    s32 facing = MU8(a, 0x49);
    s32 close = (M16(a, 0x64) > 16) ? 1 : 0;
    s32 id;

    if (g_sound_effects_enabled_legacy != 0) {
        id = g_060542B8[(facing << 1) + close];
        sound_queue_push((id << 8) | DH_SOUND_CUE_BASE);
    }
}
