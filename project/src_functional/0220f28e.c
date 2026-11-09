/* camera_update_rotated_source [0x0220f28e]: Updates the selected source-relative position and rotation each frame. */
#include "dh.h"
/* 0x0220f28e - per-frame world-transform update for a 3D object `obj`, driven by the active
 * stage source record selected off the global stage block g_06099584 (same selector as
 * func_0220f0ec: sel = g->unk10 + g->unk14; 0 -> no-op, 1/2 -> *(g->unk30/0x34)->unk8 with
 * flag 1, 3 -> *(g->unk30)->unk8 with flag 2).
 *
 * First init (obj->unk68 == 1): seed world position obj->{B8,BC,C0} from arg1->{0,4,8}
 * (with +0x15000 on BC), seed velocity obj->{14,18,1C} = arg2->{0,4,8} >> *g_06096c90
 * (arithmetic, via func_0224a420/ashr_var), clear obj->unk68, and if flag != 0 snap the
 * target obj->{90,94,98} to src->{8,C,10}.
 *
 * Steady state: ease obj->{90,94,98} a quarter of the way toward src->{8,C,10}
 * (delta/4 via sdiv32); apply the src rotation (src+0x24) to the position obj+0xB8 through
 * mtx_push_rotate_apply, writing the transformed point to g_06098B1C, then set
 * obj->{8,C,10} = g_06098B1C[0,4,8] + src->{8,C,10}; compute the look-at angles
 * obj->{24,26} from the point (obj->{8,C,10}) toward (obj->90, obj->94+0x15000, obj->98)
 * via func_0221055c; and integrate world position obj->{B8,BC,C0} += velocity obj->{14,18,1C}.
 *
 * fidelity: HIGH - transcribed from asm. m2c could not lower the 8 PIC calls; all resolved
 *   via braf_resolve and the callees' pinned ABIs: func_0224a420 (ashr_var, r0=value,
 *   r1=count -> value>>count), sdiv32 (r1/r0 -> quotient), mtx_push_rotate_apply(vec,angles),
 *   func_0221055c(x,y,z,bx,by,bz,a6,a7). Note m2c's "unk14 = arg2->unk0" dropped the shift
 *   and "unk90 += " dropped the divide; both are restored here. */
s32 shc_shift_right_signed(s32 value, s32 count);                /* ashr_var 0x0224a420 */
s32 sdiv32(s32 dividend, s32 divisor);                  /* 0x0224a06c */
void mtx_push_rotate_apply(s32 *vec, u16 *angles);      /* 0x0221080a */
void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz, s16 *a6, s16 *a7);

void camera_update_rotated_source(void *obj, void *arg1, void *arg2) {
    void *G = (void *)DH_ACTOR_SLOT_REGISTRY_ADDR;
    s32 sel = M32(G, 0x10) + M32(G, 0x14);
    void *src = 0;
    s32 flag;

    if (sel == 0)
        return;
    if (sel == 1) {
        src = MP(MP(G, 0x30), 8);
        flag = 1;
    } else if (sel == 2) {
        src = MP(MP(G, 0x34), 8);
        flag = 1;
    } else if (sel == 3) {
        src = MP(MP(G, 0x30), 8);
        flag = 2;
    } else {
        flag = sel;                 /* default: src stays NULL (dead path in valid states) */
    }

    if (M32(obj, 0x68) == 1) {
        s32 cnt = M32((void *)0x06096c90, 0);
        M32(obj, 0xB8) = M32(arg1, 0);
        M32(obj, 0xBC) = M32(arg1, 4) + 0x15000;
        M32(obj, 0xC0) = M32(arg1, 8);
        M32(obj, 0x14) = shc_shift_right_signed(M32(arg2, 0), cnt);
        M32(obj, 0x18) = shc_shift_right_signed(M32(arg2, 4), cnt);
        M32(obj, 0x1C) = shc_shift_right_signed(M32(arg2, 8), cnt);
        M32(obj, 0x68) = 0;
        if (flag != 0) {
            M32(obj, 0x90) = M32(src, 8);
            M32(obj, 0x94) = M32(src, 0xC);
            M32(obj, 0x98) = M32(src, 0x10);
        }
    } else {
        void *mat = (void *)DH_TRANSFORMED_VECTOR_ADDR;
        M32(obj, 0x90) += sdiv32(M32(src, 8)    - M32(obj, 0x90), 4);
        M32(obj, 0x94) += sdiv32(M32(src, 0xC)  - M32(obj, 0x94), 4);
        M32(obj, 0x98) += sdiv32(M32(src, 0x10) - M32(obj, 0x98), 4);
        mtx_push_rotate_apply((s32 *)((char *)obj + 0xB8), (u16 *)((char *)src + 0x24));
        M32(obj, 0x08) = M32(mat, 0) + M32(src, 8);
        M32(obj, 0x0C) = M32(mat, 4) + M32(src, 0xC);
        M32(obj, 0x10) = M32(mat, 8) + M32(src, 0x10);
        camera_compute_lookat_angles(M32(obj, 8), M32(obj, 0xC), M32(obj, 0x10),
                      M32(obj, 0x90), M32(obj, 0x94) + 0x15000, M32(obj, 0x98),
                      (s16 *)((char *)obj + 0x24), (s16 *)((char *)obj + 0x26));
        M32(obj, 0xB8) += M32(obj, 0x14);
        M32(obj, 0xBC) += M32(obj, 0x18);
        M32(obj, 0xC0) += M32(obj, 0x1C);
    }
}
