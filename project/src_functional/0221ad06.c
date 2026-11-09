/* sound_tick_timeline_cues [0x0221ad06]: Packed threshold/ID pairs emit (id<<8)|0xa00000ff as frame counter advances. */
/* 0x0221ad06 - timeline sound-cue player child task (spawned by evt_actor_spawn_child
 * 0x0221f004). entity = task->unk8. entity->unk64 is a cursor into a packed cue table of
 * {s16 frame_threshold, s16 sound_id} entries; entity->unk40 is the running frame counter.
 * Each frame: increment entity->unk40, read the next threshold (advancing a local cursor).
 * If the pre-increment counter has reached the threshold, and the global cue gate
 * 0x060E0DDC is set, read the sound id (advancing the cursor again) and push it to the
 * sound queue as ((id << 8) | 0xA00000FF). Then peek the following threshold: if > 0 commit
 * the advanced cursor back to entity->unk64 (more cues pending); otherwise (<= 0 sentinel)
 * terminate the task by zeroing *task.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. braf_resolve: sound_queue_push 0x215f6c.
 *   Note the sound-id halfword is only consumed when the gate 0x060E0DDC is nonzero (as the
 *   mov.w @r14+ sits inside that branch); this is reproduced exactly. */
#include "dh_band21.h"

extern void sound_queue_push(u32 cmd);   /* func_02215f6c */

void sound_tick_timeline_cues(u32 task)
{
    u32 entity = MEMU32(task + 8);
    u32 p = MEMU32(entity + 0x64);
    s32 cnt;
    s32 thresh;

    cnt = MEM32(entity + 0x40) + 1;
    MEM32(entity + 0x40) = cnt;

    thresh = MEM16(p);
    p += 2;

    if ((cnt - 1) >= thresh) {
        if (MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0) {
            s32 id = MEM16(p);
            p += 2;
            sound_queue_push((u32)((id << 8) | DH_SOUND_CUE_BASE));
        }
        if (MEM16(p) > 0)
            MEM32(entity + 0x64) = p;
        else
            MEM32(task) = 0;
    }
}
