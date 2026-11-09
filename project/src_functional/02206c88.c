/* scene_intro_view_task [0x02206c88]: Advances the stage intro view and derives look-at angles and model matrices. */
#include "dh_work.h"
#include "dh.h"
/* 0x02206c88  func_02206c88  (role: scene81_off16 - the scene-81 "off16" per-frame handler,
 * descriptor off16 => runtime task handler rt 0x0600ac88). Installed when scene 81 (the GAME
 * scene) builds, it runs ALONGSIDE scene81_game_sm (scene81_game_sm). It is NOT the round loop:
 * it is the scene-81 3D view-camera / model-matrix intro-move driver. The two computed calls
 * resolve (braf_resolve, trusted targets) to:
 *     func_022105ca = lookat_angles_xz_delta(dx,dy,dz, &out_a, &out_b)   (aim angle solver)
 *     mtx_build_fwd_inv = mtx_build_fwd_inv(obj)                             (build fwd+inv view mtx)
 * so the "object" it drives (obj = task->unk8) is the camera: its position obj+8/+C/+10 and
 * Euler angles obj+24/+26/+28 are rebuilt every frame into the active forward matrix
 * (g_06098aec) and inverse matrix (g_06098abc) by mtx_build_fwd_inv.
 *
 * ABI: handler(task). task = r4; obj = task->unk8 (r14). r12 = 0x160000 (the X/Z travel limit).
 *
 * 4-state cmp/eq dispatch on task->unk10 (NO jump table -> faithful C switch):
 *   state 0 (seed pose): obj.X(8)=0x160000, obj.Y(C)=0x20000, obj.Z(10)=-0x100000,
 *            obj.angle(24)=0xC000, obj.ang(26)=obj.ang(28)=0, obj.unk64=0x4000; -> state 1.
 *   state 1 (dolly Z in): obj.Z += 0x8000/frame; each frame solve the aim angles via
 *            lookat_angles_xz_delta(0x160000-obj.X, -obj.Y, 0x160000-obj.Z, &obj.24, &obj.26)
 *            (note obj.Z is pre-incremented); then clear obj.26. When obj.Z reaches 0x160000:
 *            snap obj.angle(24)=0xC000, clamp obj.Z=0x160000 -> state 2.
 *   state 2 (crane Y up): obj.Y += 0xE000/frame, obj.Z -= 0x2000/frame; when obj.Y reaches
 *            0x1E0000: clamp obj.Y=0x1E0000 -> state 3.
 *   state 3 (settled; == default): no body. Terminal by design - the camera is parked.
 * Tail (ALL states, every frame): mtx_build_fwd_inv(obj) rebuilds the view matrices.
 *
 * STALL NOTE: state 3 does not wait on any flag/actor/signal - the case is empty and nothing
 * (here or elsewhere) writes task->unk10 beyond 3. Reaching state 3 just means the camera intro
 * (Z-dolly then Y-crane) has finished; the handler then only rebuilds the matrix each frame.
 * Verified in dh-portA (STV_SM81DIAG, hdl=0600AC88, ent=060E13B0): 0->1->2->3 then parked at 3
 * for the whole scene-81 lifetime (ent.3c stays 0; no busy bit involved).
 *
 * In state 1 the C arg `-M32(obj,0x8)+0x160000` == `0x160000 - obj.X` (r4 = r12 - obj[8]); the
 * out-params land in obj+0x24 and obj+0x26 (r7 and the stacked 5th arg).
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (code 0x206c88..0x206d48 + trailing pool
 * 0x206d4a..0x206d8a). find_entry/fn.extent over-run this function into the adjacent
 * scene81_cam_update (scene81_cam_update), which begins at the real prologue right after the pool
 * and is a SEPARATE function (decompiled elsewhere); this file is only func_02206c88. */

extern void camera_angles_from_delta(s32 dx, s32 dy, s32 dz, void *out_a, void *out_b); /* lookat_angles_xz_delta */
extern void mtx_build_fwd_inv(void *obj);                                        /* mtx_build_fwd_inv */

void scene_intro_view_task(void *task) {
    void *obj = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0:
        M32(obj, 0x8)  = 0x160000;
        M32(obj, 0xC)  = 0x20000;
        M32(obj, 0x10) = -0x100000;
        M16(obj, 0x24) = (s16)0xC000;
        M16(obj, 0x26) = 0;
        M16(obj, 0x28) = 0;
        M32(obj, 0x64) = 0x4000;
        TASK_U8(task, state) = 1;
        /* fallthrough */
    case 1:
        M32(obj, 0x10) += 0x8000;
        camera_angles_from_delta(-M32(obj, 0x8) + 0x160000, -M32(obj, 0xC),
                      -M32(obj, 0x10) + 0x160000,
                      (char *)obj + 0x24, (char *)obj + 0x26);
        M16(obj, 0x26) = 0;
        if (M32(obj, 0x10) >= 0x160000) {
            M16(obj, 0x24) = (s16)0xC000;
            M32(obj, 0x10) = 0x160000;
            TASK_U8(task, state) = 2;
        }
        break;
    case 2:
        M32(obj, 0xC)  += 0xE000;
        M32(obj, 0x10) -= 0x2000;
        if (M32(obj, 0xC) >= 0x1E0000) {
            M32(obj, 0xC) = 0x1E0000;
            TASK_U8(task, state) = 3;
        }
        break;
    case 3:
    default:
        break;
    }

    mtx_build_fwd_inv(obj);
}
