/* game_boot_and_run [0x02201020]: Initialises BIOS/interrupts, task pool and audio; contains the endless task/collision/render frame loop. */
#include "dh.h"
/* 0x02201020 - func_02201020: system init / boot sequence.
 * Fidelity: HIGH - hand-checked vs asm 0x201020..0x201100 and 0x2013c4..0x20158e
 * (return tail bra 0x2013c4 with delay-slot r14 pop; the 0x2013c4 block is
 * inlined and never returns). bsrf/jsr targets resolved from pools/registers.
 * SR manipulation (IMASK=0xF0 set, then cleared) is noted in comments. */

extern void game_reset_globals(void);         /* 0x02201958 */
extern void input_reset_records(void);         /* 0x0220f990 */
extern void smpc_mask_port_shadow(void);         /* 0x02214970 */
extern void smpc_stop_slave_cpu(void);         /* 0x02214918 */
extern void video_reset_buffers(s32 mode);     /* 0x02216154 */
extern void vdp1_begin_frame(void);         /* 0x02214cdc */
extern void sound_queue_reset(void);     /* 0x02215f24 */
extern void display_reset_command_stream(s32 a, s32 b); /* 0x02201102 */
extern void frame_measure_pulse_timing(void);         /* 0x02223c74 */
extern void audio_load_driver(void);         /* 0x02215da8 */
extern void mtx_stack_reset(void);       /* 0x022149a0 */
extern void task_pool_reset(void);         /* 0x0220fc18 */
extern void task_spawn_root_list(void **defs);  /* 0x0220fe48 */
extern void smpc_start_slave_cpu(void);         /* 0x02214944 */
extern void task_run_frame(void);         /* 0x0220fde4 */
extern void collision_sweep_grid(void);         /* 0x0222ece4 */
extern void display_commit_channel_accumulator(void);         /* 0x022386c6 */
extern void view_build_display_transform(s32 mode);     /* 0x022225f6 */
extern void displist_priority_fill(s32 mode); /* 0x022227c4 */
extern void collision_reset_frame(void);         /* 0x0222ecc4 */
extern void actor_clear_frame_pair_slots(void);         /* 0x022148ca */

typedef void (*fnv_t)(void);
typedef void (*fni1_t)(s32);
typedef void (*fni2p_t)(s32, void *);

void game_boot_and_run(void)
{
    s32 base;
    s32 ptr;
    s32 loc0, loc1, loc2;

    game_reset_globals();
    input_reset_records();
    /* stc sr / or #0xF0 / ldc sr : IMASK = 0xF0 (interrupts masked) */

    (*(fnv_t *)(uintptr_t)MU32((void *)(uintptr_t)0x06000628U, 0))();

    (*(fni1_t *)(uintptr_t)MU32((void *)(uintptr_t)0x06000340U, 0))(-1);

    ptr = (s32)MU32((void *)(uintptr_t)0x06000300U, 0);
    (*(fni2p_t)(uintptr_t)ptr)(64, (void *)(uintptr_t)0x223AF0U);
    ptr = (s32)MU32((void *)(uintptr_t)0x06000300U, 0);
    (*(fni2p_t)(uintptr_t)ptr)(65, (void *)(uintptr_t)0x223D98U);
    ptr = (s32)MU32((void *)(uintptr_t)0x06000300U, 0);
    (*(fni2p_t)(uintptr_t)ptr)(67, (void *)(uintptr_t)0x223DCAU);

    M32((void *)(uintptr_t)SCU_T1MD_ADDR_U, 0) = 1;
    M32((void *)(uintptr_t)SCU_T0C_ADDR_U, 0) = -1;

    smpc_mask_port_shadow();
    smpc_stop_slave_cpu();

    (*(fni2p_t *)(uintptr_t)MU32((void *)(uintptr_t)0x06000344U, 0))(-12, 0);

    if (M32((void *)(uintptr_t)0x06000324U, 0) != 1)
        (*(fni1_t *)(uintptr_t)MU32((void *)(uintptr_t)0x06000320U, 0))(1);

    video_reset_buffers(-1);
    vdp1_begin_frame();

    base = (s32)MU32((void *)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR_U, 0);
    M16((void *)(uintptr_t)(u32)(base + 0xF0), 0) = 0x505;
    base = (s32)MU32((void *)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR_U, 0);
    M16((void *)(uintptr_t)(u32)(base + 0xF2), 0) = 0x505;
    base = (s32)MU32((void *)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR_U, 0);
    M16((void *)(uintptr_t)(u32)(base + 0xF4), 0) = 0x505;
    base = (s32)MU32((void *)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR_U, 0);
    M16((void *)(uintptr_t)(u32)(base + 0xF6), 0) = 0x505;

    sound_queue_reset();
    /* stc sr / and ~0xF0 / ldc sr : IMASK cleared (interrupts enabled) */

    display_reset_command_stream(0x505, DH_VDP2_REGISTER_SHADOW_ADDR);

    (*(fni2p_t *)(uintptr_t)MU32((void *)(uintptr_t)0x06000310U, 0))(0x94,
        (void *)(uintptr_t)0x2011B0U);

    M8((void *)(uintptr_t)0xFFFFFE10U, 0) = 1; /* delay slot of bsrf */
    frame_measure_pulse_timing();

    audio_load_driver();

    M32((void *)(uintptr_t)0x06099578U, 0) = 1;
    /* rts tail bra 0x2013c4: pop pr/r14, fall through into the block below */

    M32((void *)(uintptr_t)0x06092230U, 0) = 0x060E1330;
    M32((void *)(uintptr_t)0x06092234U, 0) = 0;
    M32((void *)(uintptr_t)0x060E1330U, 0) = 0;
    M32((void *)(uintptr_t)0x060E1334U, 0) = 0x1CCD0; /* delay slot */
    mtx_stack_reset();

    task_pool_reset();

    task_spawn_root_list((void **)(uintptr_t)0x0604ebc0U);

    ptr = (s32)MU32((void *)(uintptr_t)0x060E0DD0U, 0);
    M32((void *)(uintptr_t)(u32)ptr, 0) = 0;
    M32((void *)(uintptr_t)0x060CF0C4U, 0) = 0x060C3CC4;
    M32((void *)(uintptr_t)0x060DA0C0U, 0) = 0x060D6040;
    M32((void *)(uintptr_t)0x060E0DD0U, 0) = 0x060E0BD0;
    M32((void *)(uintptr_t)0x060E0DD4U, 0) = 0x060E0BD0; /* delay slot */
    smpc_start_slave_cpu();

    M32((void *)(uintptr_t)0x060986B4U, 0) = 0;
    M32((void *)(uintptr_t)SCU_T0C_ADDR_U, 0) = 0;         /* delay slot */
    frame_measure_pulse_timing();

    do {
        base = (s32)MU8((void *)(uintptr_t)0x260E0DD8U, 0);
        base &= 0x80;
    } while (base != 0x80);
    M8((void *)(uintptr_t)0x060E0DD8U, 0) = 0;

    loc0 = 0x060D6040;
    loc1 = 0x060CF0A0;
    loc2 = 0x060E0BD0;

loop_440:
    M32((void *)(uintptr_t)0x06098818U, 0) = 0;
    M32((void *)(uintptr_t)0x0609881CU, 0) = 0;
    M32((void *)(uintptr_t)0x06098820U, 0) = 0;
    M32((void *)(uintptr_t)0x06098824U, 0) = 0;

    ptr = M32((void *)(uintptr_t)0x060E0DD4U, 0);
    M32((void *)(uintptr_t)0x060E0DD4U, 0) = ptr + 4;
    M32((void *)(uintptr_t)(u32)(ptr + 4 - 4), 0) = 5;

    mtx_stack_reset();

    collision_reset_frame();

    actor_clear_frame_pair_slots();

    M32((void *)(uintptr_t)0x060DA0C4U, 0) = loc0;
    M32((void *)(uintptr_t)0x060CF0C8U, 0) = loc1;
    M32((void *)(uintptr_t)0x060E0DD4U, 0) = loc2;

    do {
        base = (s32)MU8((void *)(uintptr_t)0x260E0DD8U, 0);
        base &= 0x80;
    } while (base != 0x80);
    M8((void *)(uintptr_t)0x060E0DD8U, 0) = 0;          /* delay slot */
    task_run_frame();

    ptr = M32((void *)(uintptr_t)0x060E0DD4U, 0);
    M32((void *)(uintptr_t)0x060E0DD4U, 0) = ptr + 4;
    M32((void *)(uintptr_t)(u32)(ptr + 4 - 4), 0) = 7;  /* delay slot */
    collision_sweep_grid();

    display_commit_channel_accumulator();

    view_build_display_transform(0);
    view_build_display_transform(1);
    displist_priority_fill(2);

    do {
        base = (s32)MU8((void *)(uintptr_t)0x260E0DD8U, 0);
        base &= 0x80;
    } while (base != 0x80);
    M8((void *)(uintptr_t)0x060E0DD8U, 0) = 0;

loop_582:
    frame_measure_pulse_timing();

    if (M32((void *)(uintptr_t)0x06098828U, 0) != 0) {
loop_568:
        if (((u32)MU16((void *)(uintptr_t)0x06098798U, 0) & 0x4000U) != 0)
            M32((void *)(uintptr_t)0x06098828U, 0) = 0;
        if (((u32)MU16((void *)(uintptr_t)0x06098798U, 0) & 0x10U) != 0) {
            M16((void *)(uintptr_t)0x06098798U, 0) = 0;
            goto loop_440;
        }
        goto loop_582;
    }
    goto loop_440;
}
