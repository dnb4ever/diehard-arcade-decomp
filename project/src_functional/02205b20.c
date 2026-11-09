#include "dh_work.h"
/* scene_advance_task [0x02205b20]: Selects the next 0x50-byte scene descriptor and spawns its task. */
#include "dh.h"
/* 0x02205b20 (scene_director_build) - the game-scene descriptor writer the port needs: it selects a
 * scene index and sets g_0609866c = &g_scene_table[idx] (base 0x06072660, stride 0x50). Handler ABI
 * (task): entity = task->unk8. Only runs its main body when entity->unk3c bit0 is set; otherwise it
 * services a small per-frame counter (entity->unk50; at >4 it raises g_06098812/g_0609880e).
 *
 * Main body: func_0220fdc8(0); clear entity->unk3c bit0; g_06099578 = 0; g_0609868f = (u8)g_06098668.
 *   if (g_06098668 == 81): mark entity->unk3c bit0, g_06098670 = 0, sound_queue_push(0xa00001ff), ret.
 *   else: cur = *g_0609866c.
 *     if ((s8)task->unk0D < 0): g_06098668 = 81; *g_0609866c = 0x06073fb0; g_060987d0 = 1;
 *     else: switch on (s8)cur[43] choose g_06098668 from cur[44]/cur[45]/table-indexed-by-g_0609856c
 *           and the player-join bytes g_06098674/75, then *g_0609866c = 0x06072660 + (s16)(idx*0x50).
 *   Finally (both): if (g_06098668 >= 76) reset the join state (g_060987d4/d5, g_06098674/75=0),
 *     install 0x06009812 into *(void**)g_06098678, g_060987d0 = 1, g_06098670 = 0; else entity->unk50 = 0.
 *   g_0609868e = *(u8*)(*g_0609866c + 42); tail func_0220fece(task, *(*g_0609866c + 20)).
 * Branches resolved (braf_resolve): bsrf->0x20fdc8/0x215f6c, braf->0x20fece; morph fn-ptr mova+add->
 * 0x205812 (ptr_resolve). The func_0220fece tail sets r4=task, r5=*(cur+20) (func_0220fece's own file flags
 * those arg registers as inherited/unmodeled). r5=0, r13=1 pinned.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. scene table stride 0x50; selector cur[43] signed. */
#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

extern void  task_cancel_below_depth(u8 arg0);          /* 0x0220fdc8 */
extern void  sound_queue_push(s32 req);   /* 0x0215f6c */
extern void *task_spawn_current_work6c(void *task, void *script);             /* constructor tail: task and selected script */

void scene_advance_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *cur;
    s32 idx;

    if (!(M32(entity, 0x3c) & 1)) {
        M32(entity, 0x50) += 1;
        if (M32(entity, 0x50) > 4) {
            RU16A(0x06098812) = 1;
            RU16A(0x0609880e) = 1;
        }
        return;
    }

    task_cancel_below_depth(0);
    M32(entity, 0x3c) &= ~1;
    R32A(0x06099578) = 0;
    RU8A(0x0609868f) = (u8)R32A(DH_SCENE_INDEX_ADDR);
    if (R32A(DH_SCENE_INDEX_ADDR) == DH_SCENE_STOP) {
        M32(entity, 0x3c) |= 1;
        R32A(DH_PLAY_SESSION_MODE_ADDR) = 0;
        sound_queue_push((s32)DH_SOUND_SCENE_END_COMMAND);
        return;
    }

    cur = PA(DH_SCENE_DESCRIPTOR_ADDR);
    if (M8(task, 0xd) < 0) {
        R32A(DH_SCENE_INDEX_ADDR) = DH_SCENE_STOP;
        PA(DH_SCENE_DESCRIPTOR_ADDR) = (void *)(uintptr_t)0x06073fb0;
        R32A(0x060987d0) = 1;
    } else {
        s32 g = R32A(0x0609856c);
        s32 s44 = M8(cur, 44);
        s32 s45 = M8(cur, 45);
        switch (M8(cur, 43)) {
        case -1: R32A(DH_SCENE_INDEX_ADDR) = DH_SCENE_PLAY_ENTRY; break;
        case 0:  R32A(DH_SCENE_INDEX_ADDR) = s44; break;
        case 1:  R32A(DH_SCENE_INDEX_ADDR) = M8(cur, 44 + (g & 1)); break;
        case 2:  R32A(DH_SCENE_INDEX_ADDR) = M8(cur, 44 + (g & 3)); break;
        case 3:
            R32A(DH_SCENE_INDEX_ADDR) = (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 2 && RU8A(0x06098675) == 2) ? s45 : s44;
            break;
        case 4:
            R32A(DH_SCENE_INDEX_ADDR) = R32A(DH_PLAY_SESSION_MODE_ADDR) != 0 ? s45 : s44;
            break;
        default: break;
        }
        idx = R32A(DH_SCENE_INDEX_ADDR);
        PA(DH_SCENE_DESCRIPTOR_ADDR) = (void *)(uintptr_t)(0x06072660 + (s16)(idx * 0x50));
    }

    if (R32A(DH_SCENE_INDEX_ADDR) >= DH_SCENE_NONPLAY_FIRST) {
        RU8A(0x060987d5) = 0;
        RU8A(0x060987d4) = 0;
        RU8A(0x06098675) = 0;
        RU8A(DH_PLAYER_JOIN_STATES_ADDR) = 0;
        *(void **)PA(0x06098678) = (void *)(uintptr_t)0x06009812;
        R32A(0x060987d0) = 1;
        R32A(DH_PLAY_SESSION_MODE_ADDR) = 0;
    } else {
        M32(entity, 0x50) = 0;
    }

    RU8A(0x0609868e) = MU8(PA(DH_SCENE_DESCRIPTOR_ADDR), 42);
    task_spawn_current_work6c(task, MP(PA(DH_SCENE_DESCRIPTOR_ADDR), 20));
}
