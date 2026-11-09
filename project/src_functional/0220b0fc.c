#include "dh_work.h"
#include "dh.h"
/* 0x0220b0fc (camera_vel_integrate_task) - velocity/accel integrator for an entity's
 * pose, then rebuild matrices. Sibling of camera_pose_reset_task (0x0220b0c6);
 * exposed by fn.extent's over-swallow of the 0x20b0c6..0x20b354 pool group.
 *
 * ABI: task_handler(task). task = r4; entity = task->unk8 (r14).
 *   state 0: seed position (unk8/C/10=0), velocity (unk14=0, unk18=0, unk1C=-0x8000),
 *     acceleration (unk40=0, unk44=0x40, unk48=-0x180) and angles (unk24=0,
 *     unk26=0x4000, unk28=0); advance to state 1; fall through to run the step
 *     the same frame.
 *   state 1: unless the Z velocity has reached its floor (-unk1C >= 0x30000) add the
 *     acceleration triple to the velocity triple; once floored, instead bleed the
 *     Y velocity down by 0x60 while positive. Then integrate position by velocity
 *     (unk8+=unk14, unkC+=unk18, unk10+=unk1C).
 *   state >= 2: no update.
 * Always tail-calls mtx_build_fwd_inv(entity).
 *
 * m2c correction: func_02214a7a(entity, var_r5) 2nd arg is a phantom (r5 is the
 * zero-source in init); mtx_build_fwd_inv takes entity only. braf tail -> real
 * TARGET 0x02214a7a. Bounded 0x20b0fc..0x20b1c0 (braf tail at 0x20b196 + shared
 * pool). fidelity: HIGH. */

extern void mtx_build_fwd_inv(void *entity);   /* 0x02214a7a */

void camera_vel_integrate_task(void *task)
{
    void *entity = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x8)  = 0;
        M32(entity, 0xC)  = 0;
        M32(entity, 0x10) = 0;
        M32(entity, 0x14) = 0;
        M32(entity, 0x18) = 0;
        M32(entity, 0x1C) = -0x8000;
        M32(entity, 0x40) = 0;
        M32(entity, 0x44) = 0x40;
        M32(entity, 0x48) = -0x180;
        M16(entity, 0x24) = 0;
        M16(entity, 0x26) = 0x4000;
        M16(entity, 0x28) = 0;
        TASK_S8(task, state) = 1;
        /* fall through */
    case 1:
        if (-M32(entity, 0x1C) < 0x30000) {
            M32(entity, 0x14) += M32(entity, 0x40);
            M32(entity, 0x18) += M32(entity, 0x44);
            M32(entity, 0x1C) += M32(entity, 0x48);
        } else if (M32(entity, 0x18) > 0) {
            M32(entity, 0x18) -= 0x60;
        }
        M32(entity, 0x8)  += M32(entity, 0x14);
        M32(entity, 0xC)  += M32(entity, 0x18);
        M32(entity, 0x10) += M32(entity, 0x1C);
        break;
    }
    mtx_build_fwd_inv(entity);
}
