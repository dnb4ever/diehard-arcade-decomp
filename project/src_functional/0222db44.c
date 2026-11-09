/* actor_load_pose_buffers [0x0222db44]: Loads the selected animation's initial pose and primes both interpolation buffers. */
#include "dh.h"
/* 0x0022db44  actor_load_pose_buffers
 *
 * Initialise an actor's per-frame pose state from a variant record.
 *   arg0 = entity.
 *
 * Sets the running flag (unk58=1) and frame counter (unk5A=0), latches the hit
 * animation id (g_0608CE74[unk6D]) into the per-slot anim-id table
 * g_06096C74[unk34] and into entity->unk5C, copies three position longs of the
 * source pose record into the per-actor record g_06096328[unk34]+0x60, derives
 * the scale byte unk40 = (1 << pose->shift) + 0xFF, block-copies the two 108-byte
 * pose halves into the working buffers g_0609661C / g_06096910 (both indexed by
 * unk34*108), and zeroes the 12-byte g_060996EC slot.
 *
 * Previously deferred: three computed branches. braf_resolve.py pins them:
 *   22dbaa bsrf -> 0x24a360 (ashl_var: value<<count, r0/r1 ABI)
 *   22dbc0 bsrf -> 0x24a280 (block copy, r0=count/r1=dst/r2=src, count>64 path)
 *   22dbce bsrf -> 0x24a280 (block copy)
 *
 * fidelity: HIGH - struct offsets, table strides (108, 28, 12), the s16/s32
 * widths, the (s8)(unk34*12) byte-truncated index and the shift-helper scale
 * formula all read from ROM asm; all three computed branches confirmed by
 * braf_resolve.py.
 */
extern s32   g_06096C74[];      /* g_slot_anim_id: per-slot anim id (idx unk34) */
extern s16   g_0608CE74[];      /* hit-anim id table (idx unk6D) */
extern s32   shc_shift_left(s32 value, s32 count);           /* ashl_var */
extern void  shc_copy_word_block(s32 count, void *dst, void *src);/* block copy (r0/r1/r2) */

#define G_RECORD_06096328 0x06096328   /* per-actor record, stride 108 */
#define G_POSE_SRC_0607D124 0x0607D124 /* pose source record, stride 28 */
#define G_POSE_BUF_0609661C 0x0609661C /* working pose buffer A, stride 108 */
#define G_POSE_BUF_06096910 0x06096910 /* working pose buffer B, stride 108 */
#define G_SLOT_060996EC     0x060996EC /* 12-byte per-actor slot */

void actor_load_pose_buffers(void *entity) {
    s32   anim_slot  = M8(entity, ACTOR_SLOT_ID);
    void *rec        = (void *)(uintptr_t)(G_RECORD_06096328
                               + (s16)(anim_slot * 108));
    void *pose;
    void *src;
    s16   anim;

    M16(entity, 0x58) = 1;
    M16(entity, 0x5A) = 0;

    anim = g_0608CE74[M8(entity, 0x6D)];
    g_06096C74[anim_slot] = anim;
    M16(entity, ACTOR_ANIM_REQUEST) = anim;

    pose = (void *)(uintptr_t)(G_POSE_SRC_0607D124
                   + (u16)M16(entity, 0x5A) * 28);
    src = MP(pose, 24);

    M32(rec, 0x60) = M32((u8 *)src + 96, 0);
    M32(rec, 0x64) = M32((u8 *)src + 96, 4);
    M32(rec, 0x68) = M32((u8 *)src + 96, 8);

    M8(entity, 0x40) = (u8)(shc_shift_left(1, M16(pose, 22)) + 0xFF);

    shc_copy_word_block(108, (void *)(uintptr_t)(G_POSE_BUF_0609661C + anim_slot * 108),
                  src);
    shc_copy_word_block(108, (void *)(uintptr_t)(G_POSE_BUF_06096910 + anim_slot * 108),
                  (u8 *)src + 108);

    {
        void *slot = (void *)(uintptr_t)(G_SLOT_060996EC
                             + (s8)(M8(entity, ACTOR_SLOT_ID) * 12));
        /* ROM 06031b34 / 06031be4: high word first. */
        M32(slot, 8) = 0;
        M32(slot, 4) = 0;
        M32(slot, 0) = 0;
    }
}
