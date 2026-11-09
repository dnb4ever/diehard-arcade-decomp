/* effect_spawn_rising_flash [0x0223edce]: Spawns 0223eef0 child, copies facing/duration and emits 0xa01406ff. */
#include "dh.h"
/* func_0223edce(arg0, src, arg2): spawn a child via func_0220ff28(arg0, template
 * func_0223eef0); oe = obj->unk8. Set oe->unk34 = src->unk34, oe->unk38 = 0x200000,
 * oe->unk70 = arg2, oe->unk64 = (s8)src->unk44. If g_060E0DDC is set, push sound
 * 0xA01406FF.
 * fidelity: HIGH - offsets verified; func_0220ff28=spawn_obj_0x7c, func_02215f6c=
 *   sound_queue_push resolved via braf; template func_0223eef0 via mova+add.
 *   Signature matches notes/symbols.txt: void(s32 arg0, void* obj, s32 arg2). */
extern void *task_spawn_list_work7c(s32 arg0, void *template);
extern void  sound_queue_push_entry(void *cmd);
extern u8    g_sound_effects_enabled_legacy[];

void effect_spawn_rising_flash(s32 arg0, void *src, s32 arg2) {
    void *obj = task_spawn_list_work7c(arg0, (void *)0x06042EF0 /* HWRAM handler func_0223eef0 */);
    void *oe  = MP(obj, 8);

    M8(oe, 0x34)  = M8(src, 0x34);
    M32(oe, 0x38) = 0x200000;
    M32(oe, 0x70) = arg2;
    M32(oe, 0x64) = (s8)M8(src, 0x44);
    if (M32(g_sound_effects_enabled_legacy, 0) != 0) {
        sound_queue_push_entry((void *)DH_SOUND_EFFECT_VARIANT_COMMAND);
    }
}
