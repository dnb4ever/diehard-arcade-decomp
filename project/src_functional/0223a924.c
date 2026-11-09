/* effect_pool_update_pose [0x0223a924]: Sets a pooled effect mode and copies the supplied pose before the attachment hook. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223a924 (leafC) - dispatch on mode (arg1); idx=arg0, src=arg2, save=arg3.
 * base=0x060E0E88; obj = *( *(base+0xC4 + 4*idx) + 8 ). Store mode into base+0xA4+4*idx.
 *  mode==1: obj[8]=src[0]; obj[16]=src[8]; obj[12] = 0x0608CEAC[ *(base+0x84+4*idx) ].
 *  mode==3: if (*(s8)(save+52) < 3) obj[0x90]--; then fall into copy/tailcall.
 *  mode==4 / fall: obj[8/12/16]=src[0/4/8]; tail-call func_0223a888(obj, save).
 *  else: return.   bra cart 0x23a888 -> RAM 0x0603e888.
 * fidelity: HIGH - pointers/longs 32-bit; field 52 8-bit; table strides 4. */
extern s32 g_060E0E88;
extern s32 g_0608CEAC;
extern void effect_project_source_motion(void *, void *);
void effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save) {
    void *base = &g_060E0E88;
    void *obj = MP(MP(base, 0xC4 + idx * 4), 8);
    M32(base, 0xA4 + idx * 4) = mode;
    if (mode == 1) {
        WORLD_POSE(obj)->position_x  = M32(src, 0);
        WORLD_POSE(obj)->position_z = M32(src, 8);
        WORLD_POSE(obj)->position_y  = (&g_0608CEAC)[ M32(base, 0x84 + idx * 4) ];
        return;
    }
    if (mode == 3) {
        if (M8(save, 52) < 3)
            M32(obj, 0x90) = M32(obj, 0x90) - 1;
    } else if (mode != 4) {
        return;
    }
    WORLD_POSE(obj)->position_x  = M32(src, 0);
    WORLD_POSE(obj)->position_y  = M32(src, 4);
    WORLD_POSE(obj)->position_z = M32(src, 8);
    effect_project_source_motion(obj, save);
}
