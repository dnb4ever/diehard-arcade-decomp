/* scene_bootstrap [0x022058a8]: Initializes scene selection and resident configuration and installs the attract controller. */
#include "dh.h"
/* 0x022058a8 (scene_bootstrap) - KICKS OFF the whole attract->play state machine (port unblock #0).
 *
 * One-shot scene/attract initializer. Brings up the scene system, selects the per-frame scene
 * step function g_0609867c, builds the scene descriptor pointer g_0609866c, and INSTALLS THE
 * ATTRACT HANDLER (0x06008868) into the game task (*g_06098678)->unk0 so that attract_handler
 * then runs every frame (whence it can morph into the round controller -> scene_run_handler).
 *
 * Flow:
 *   g_06099578=0; sound_queue_reset(); g_060e0ddc=1 (sound on); func_0220f990(); func_0223dcc2(1);
 *   func_0223dcc2(2); g_0609957c = g_060987bc (seed); (*g_06000634)(g_060987ec) (read config);
 *   g_06099794 = g_060511cc[((g_060987ec[g_06098808] >> 3) & 7)] (language); (*g_06000660)(..) (peripheral);
 *   g_0609867c = (g_060987e8 & 4) ? scene_step_A(0x204370) : scene_step_B(0x2044cc); g_06098680=0;
 *   if (((*g_06000798 >> 5) & 1) == 1)   // test/service mode
 *       player_slot_reset(0/1); g_06098668 = 0;   // scene index 0
 *   else if ((g_06000654 & 0x100) == 0) // auto-start dip gate
 *       if (g_06000654 & 0x1000) coin-force P1: (*g_06000664)(0), join=1, timer=30, entity->unk40=0;
 *       if (g_06000654 & 0x2000) coin-force P2: (*g_06000664)(1), join=1, timer=30, entity->unk44=0;
 *       player_slot_reset(0/1); g_06099578=0; VDP2 TVMD(0x25f80000)=*g_06098f08 & 0x7fff (display on);
 *       entity->unk40=unk44=0; g_06098670=1; g_06098668=0;
 *       *(void**)(*g_06098678) = attract_handler (0x06008868);   // <-- installs the attract task
 *   else g_06098668 = 1;
 *   g_0609866c = 0x06072660 + (s16)(g_06098668 * 0x50);   // scene descriptor (stride 0x50)
 *   arg0->unkD = 0; clear g_06096c90/g_0609856c/g_06098570/g_0609979a/g_0609868e/g_06098812/
 *   g_0609880e/g_06098690/g_06098694; g_060e0de0 = 1.
 *
 * All PIC hand-resolved (ptr_resolve/braf_resolve, pools @0x205958..0x205b1c): bsrf->0x215f24,
 * 0x20f990, 0x23dcc2; scene steppers mova+add->0x204370 / 0x2044cc; task handler mova+add->0x204868.
 * The capstone "bsrf" at 0x205968/0x20596c are the pool deltas themselves, not instructions.
 * (*g_06098678) is the resident game object (obj_install_noop_handler), its ->unk8 the player entity.
 * fidelity: HIGH, hand-traced vs m2c_fn --asm. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))

typedef void (*fn_i)(s32);
#define CALL(a, T) (*(T *)(uintptr_t)(a))

extern void sound_queue_reset(void);     /* 0x0215f24 */
extern void input_reset_records(void);    /* 0x0220f990 scene/display init */
extern void player_clear_saved_action(s32 p);   /* 0x0223dcc2 */
extern void player_reset_join_slot(s32 p);   /* 0x0220581a player_slot_reset */

void scene_bootstrap(void *arg0) {
    u32 cfg;
    void *entity;

    R32A(0x06099578) = 0;
    sound_queue_reset();
    R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) = 1;                     /* sound enable */
    input_reset_records();
    player_clear_saved_action(1);
    player_clear_saved_action(2);
    R32A(DH_RANDOM_SEED_ADDR) = R32A(DH_FRAME_TICK_ADDR);      /* RNG seed <- frame timer */
    CALL(0x06000634, fn_i)(DH_BIOS_STATUS_BUFFER_ADDR);       /* BIOS read config block */

    cfg = (u8)R8A(DH_BIOS_STATUS_BUFFER_ADDR + R32A(0x06098808));
    R16A(0x06099794) = R16A(0x060511cc + ((cfg >> 3) & 7) * 2);   /* language */

    CALL(0x06000660, fn_i)(0x060987d8);       /* BIOS peripheral read */

    if (RU8A(0x060987e8) & 4)
        PA(0x0609867c) = (void *)(uintptr_t)0x06008370;   /* scene step A */
    else
        PA(0x0609867c) = (void *)(uintptr_t)0x060084cc;   /* scene step B */

    R32A(0x06098680) = 0;

    if (((R32A(0x06000798) >> 5) & 1) == 1) {             /* test / service mode */
        player_reset_join_slot(0);
        player_reset_join_slot(1);
        R32A(DH_SCENE_INDEX_ADDR) = DH_SCENE_SELECTOR;
    } else if (((u16)R16A(0x06000654) & 0x100) == 0) {    /* auto-start dip gate */
        if ((u16)R16A(0x06000654) & 0x1000) {             /* force-join P1 */
            CALL(0x06000664, fn_i)(0);
            R8A(0x060987d4) = 1;
            R8A(DH_PLAYER_JOIN_STATES_ADDR) = 1;
            R32A(0x0609975c) = 30;
            entity = MP(PA(0x06098678), 8);
            M32(entity, 0x40) = 0;
        }
        if ((u16)R16A(0x06000654) & 0x2000) {             /* force-join P2 */
            CALL(0x06000664, fn_i)(1);
            R8A(0x060987d5) = 1;
            R8A(0x06098675) = 1;
            R32A(0x06099760) = 30;
            entity = MP(PA(0x06098678), 8);
            M32(entity, 0x44) = 0;
        }
        player_reset_join_slot(0);
        player_reset_join_slot(1);
        R32A(0x06099578) = 0;
        R16A(VDP2_TVMD_ADDR) = R16A(PA(DH_VDP2_REGISTER_SHADOW_ADDR)) & 0x7fff; /* VDP2 TVMD: display on */
        entity = MP(PA(0x06098678), 8);
        M32(entity, 0x40) = 0;
        M32(entity, 0x44) = 0;
        R32A(DH_PLAY_SESSION_MODE_ADDR) = 1;
        R32A(DH_SCENE_INDEX_ADDR) = DH_SCENE_SELECTOR;
        *(void **)PA(0x06098678) = (void *)(uintptr_t)0x06008868;  /* install attract_handler */
    } else {
        R32A(DH_SCENE_INDEX_ADDR) = 1;
    }

    R32A(DH_SCENE_DESCRIPTOR_ADDR) = (s16)(R32A(DH_SCENE_INDEX_ADDR) * 0x50) + 0x06072660;   /* scene descriptor */

    M8(arg0, 0xd) = 0;
    R32A(0x06096c90) = 0;
    R32A(0x0609856c) = 0;
    R32A(0x06098570) = 0;
    R16A(DH_ACTOR_POSE_SET_ADDR) = 0;
    R8A(0x0609868e) = 0;
    R32A(DH_AUDIO_LATCHED_SOUND_ID_ADDR) = 1;
    R16A(0x06098812) = 0;
    R16A(0x0609880e) = 0;
    R32A(0x06098690) = 0;
    R32A(0x06098694) = 0;
}
