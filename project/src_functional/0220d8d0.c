#include "dh_work.h"
/* camera_tick_request_hold [0x0220d8d0]: Applies request-selected camera parameters and restores default priority when the hold timer expires. */
#include "dh.h"
/* 0x0220d8d0 - per-player entity motion/post-hit updater (arg0 = controller/task,
 * arg1 = player index). entity = arg0->unk8. g_06098830 is a global mode/phase
 * word shared with the lookup tables below.
 *
 * If the "pending" byte arg0->unkD is set: clear it, then load entity->unk64 from
 * table 0x06052018 (stride 0x28 per player, +g_06098830*4) and entity->unkA8 from
 * halfword table 0x06052090 (indexed by g_06098830). When g_06098830 == 0, clamp
 * the two signed velocity fields entity->unk24 to [-0x3000,+0x3000] and
 * entity->unk26 to [-0x3000,+0x2000].
 *
 * Then if the countdown entity->unkA8 > 0: decrement it; while still non-zero
 * re-load unk64 from the same 0x06052018 table and return; when it reaches 0 set
 * g_06098830 = 0xA and load unk64 from the per-player table 0x06051e6c.
 *
 * fidelity: HIGH - hand-verified against m2c_fn --asm (fn.py 0x20d8d0). Field
 * names unk*; (s8) truncation on the 0x28 player stride kept faithful. */
#define GU8(a)  (*(u8  *)(uintptr_t)(a))
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

void camera_tick_request_hold(void *arg0, s32 arg1) {
    void *entity = MP(arg0, 8);
    s16 v;

    if (MU8(arg0, 0xD) != 0) {
        MU8(arg0, 0xD) = 0;
        CAMERA_M32(entity, update_handler) = G32(0x06052018 + (s8)(arg1 * 0x28) + G32(DH_VIEW_REQUEST_PRIORITY_ADDR) * 4);
        CAMERA_M16(entity, request_hold_ticks) = G16(0x06052090 + G32(DH_VIEW_REQUEST_PRIORITY_ADDR) * 2);
        if (G32(DH_VIEW_REQUEST_PRIORITY_ADDR) == 0) {
            v = CAMERA_M16(entity, angle_x);
            if (v < -0x3000)      CAMERA_M16(entity, angle_x) = -0x3000;
            else if (v > 0x3000)  CAMERA_M16(entity, angle_x) = 0x3000;
            v = CAMERA_M16(entity, angle_y);
            if (v < -0x3000)      CAMERA_M16(entity, angle_y) = -0x3000;
            else if (v > 0x2000)  CAMERA_M16(entity, angle_y) = 0x2000;
        }
    }

    if (CAMERA_M16(entity, request_hold_ticks) > 0) {
        CAMERA_M16(entity, request_hold_ticks) = (s16)(CAMERA_M16(entity, request_hold_ticks) - 1);
        if (CAMERA_M16(entity, request_hold_ticks) != 0) {
            CAMERA_M32(entity, update_handler) = G32(0x06052018 + (s8)(arg1 * 0x28) + G32(DH_VIEW_REQUEST_PRIORITY_ADDR) * 4);
            return;
        }
        G32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 0xA;
        CAMERA_M32(entity, update_handler) = G32(0x06051e6c + arg1 * 4);
    }
}
