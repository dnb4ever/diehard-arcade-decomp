#include "dh_work.h"
#include "dh.h"
/* 0x0220b0c6 (camera_pose_reset_task) - zero an entity's pose, then rebuild matrices.
 *
 * ABI: task_handler(task). task = r4; entity = task->unk8 (r14).
 * On first entry (task->unk10 == 0) clears position (unk8/C/10) and Euler angles
 * (unk24/26/28, s16) and advances task->unk10 to 1; every frame tail-calls
 * mtx_build_fwd_inv(entity).
 *
 * NOTE: fn.extent over-swallowed this entry together with two siblings that share
 * its literal pool - camera_vel_integrate_task (0x0220b0fc) and camera_pose_init_task
 * (0x0220b1c0); this TU is only 0x0220b0c6's own body (ends at the braf tail 0x20b0fc).
 * m2c correction: its func_02214a7a(entity, var_r5) 2nd arg is a phantom (r5 is the
 * zero-source register in the init block); mtx_build_fwd_inv takes entity only.
 * braf tail resolved to the real TARGET 0x02214a7a. fidelity: HIGH. */

extern void mtx_build_fwd_inv(void *entity);   /* 0x02214a7a */

void camera_pose_reset_task(void *task)
{
    void *entity = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        M32(entity, 0x8)  = 0;
        M32(entity, 0xC)  = 0;
        M32(entity, 0x10) = 0;
        M16(entity, 0x24) = 0;
        M16(entity, 0x26) = 0;
        M16(entity, 0x28) = 0;
        TASK_S8(task, state) = 1;
    }
    mtx_build_fwd_inv(entity);
}
