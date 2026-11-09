/* effect_spawn_eight_frame_flash [0x0223ed94]: Spawns 0223efa0 child, copies actor slot and emits 0xa01402ff. */
#include "dh.h"
/* func_0223ed94(arg0, src): spawn a child via func_0220ff28(arg0, template func_0223efa0),
 * oe = obj->unk8; copy oe->unk34 = src->unk34, set oe->unk38 = 0x200000. If g_060E0DDC
 * is set, push sound 0xA01402FF.
 * fidelity: HIGH - offsets verified; func_0220ff28=spawn_obj_0x7c, func_02215f6c=
 *   sound_queue_push resolved via braf; template func_0223efa0 via mova+add.
 *   Signature matches notes/symbols.txt: void(s32 arg0, void* obj). */
extern void *task_spawn_list_work7c(s32 arg0, void *template);
extern void  sound_queue_push_entry(void *cmd);
extern u8    g_sound_effects_enabled_legacy[];

void effect_spawn_eight_frame_flash(s32 arg0, void *src) {
    void *obj = task_spawn_list_work7c(arg0, (void *)0x06042FA0 /* HWRAM handler func_0223efa0 */);
    void *oe  = MP(obj, 8);

    M8(oe, 0x34)  = M8(src, 0x34);
    M32(oe, 0x38) = 0x200000;
    if (M32(g_sound_effects_enabled_legacy, 0) != 0) {
        sound_queue_push_entry((void *)DH_SOUND_EFFECT_SPAWN_COMMAND);
    }
}
