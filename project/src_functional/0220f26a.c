/* camera_spawn_initial_child [0x0220f26a]: Creates the scene child only on the work record's initialization frame. */
#include "dh.h"
/* 0x0220f26a - on first init of an object, spawn a child record. If obj->unk68 == 1,
 * clear obj->unk68 and call func_0220ff72(*g_060996d0, &rec_022199da). Otherwise does
 * nothing. (reduced sibling of 0x20f19e/f202 with no attribute copy.)
 * fidelity: HIGH - transcribed from asm; bsrf resolves to 0x20ff72, data ptr 0x0601d984. */
void *task_spawn_child_work40(void *a, void *b);

void camera_spawn_initial_child(void *obj) {
    if (M32(obj, 0x68) == 1) {
        M32(obj, 0x68) = 0;
        task_spawn_child_work40(MP((void *)0x060996d0, 0), (void *)0x0601d984);
    }
}
