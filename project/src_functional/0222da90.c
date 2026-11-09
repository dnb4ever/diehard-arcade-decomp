/* actor_load_current_pose_buffers [0x0222da90]: Initializes pose interpolation with the selected animation as the current source. */
#include "dh.h"
/* 0x0222da90 actor_load_pose_buffers (variant) - sibling of func_0222db44.
 * Initialise an actor pose state from a variant record. arg0 = entity.
 * Latches hit anim id g_0608CE74[unk6D] into g_06096C74[unk34] and entity->unk5C,
 * sets running flag unk58 = 1, and - unlike the 22db44 sibling which seeds unk5A = 0
 * - seeds frame index unk5A = unk5C (the anim id), so the pose source record is
 * selected by the anim id: pose = &g_0607D124[(u16)entity->unk5A] (stride 28).
 * Copies three position longs (pose->unk18 + 0x60) into g_06096328[unk34] + 0x60,
 * derives unk40 = (1 << pose->shift) + 0xFF, block-copies the two 108-byte pose
 * halves into g_0609661C / g_06096910 (indexed unk34*108), zeroes g_060996EC slot.
 * braf_resolve: 22dafa bsrf->0x24a360 (ashl_var val<<cnt, r0/r1 ABI);
 *   22db10/22db1e bsrf->0x24a280 (block copy, r0=cnt/r1=dst/r2=src).
 * fidelity: HIGH - offsets, strides (108/28/12), s16/s32 widths, (s16)(unk34*108)
 * and (s8)(unk34*12) byte-truncated indices, and the shift scale formula all read
 * from ROM asm; all three computed branches confirmed by braf_resolve.py. */
extern s32   g_06096C74[];      /* per-slot anim id (idx unk34) */
extern s16   g_0608CE74[];      /* hit-anim id table (idx unk6D) */
extern s32   shc_shift_left(s32 value, s32 count);           /* ashl_var */
extern void  shc_copy_word_block(s32 count, void *dst, void *src);/* block copy (r0/r1/r2) */

#define G_RECORD_06096328   0x06096328 /* per-actor record, stride 108 */
#define G_POSE_SRC_0607D124 0x0607D124 /* pose source record, stride 28 */
#define G_POSE_BUF_0609661C 0x0609661C /* working pose buffer A, stride 108 */
#define G_POSE_BUF_06096910 0x06096910 /* working pose buffer B, stride 108 */
#define G_SLOT_060996EC     0x060996EC /* 12-byte per-actor slot */

void actor_load_current_pose_buffers(void *entity) {
    s32   anim_slot = M8(entity, ACTOR_SLOT_ID);
    s32   off108    = (s16)(anim_slot * 108);
    void *rec       = (void *)(uintptr_t)(G_RECORD_06096328 + off108);
    void *pose;
    void *src;
    s16   anim;

    anim = g_0608CE74[M8(entity, 0x6D)];
    g_06096C74[anim_slot] = anim;
    M16(entity, ACTOR_ANIM_REQUEST) = anim;
    M16(entity, 0x58) = 1;
    M16(entity, 0x5A) = M16(entity, ACTOR_ANIM_REQUEST);

    pose = (void *)(uintptr_t)(G_POSE_SRC_0607D124 + (u16)M16(entity, 0x5A) * 28);
    src  = MP(pose, 24);

    M32(rec, 0x60) = M32((u8 *)src + 96, 0);
    M32(rec, 0x64) = M32((u8 *)src + 96, 4);
    M32(rec, 0x68) = M32((u8 *)src + 96, 8);

    M8(entity, 0x40) = (u8)(shc_shift_left(1, M16(pose, 22)) + 0xFF);

    shc_copy_word_block(108, (void *)(uintptr_t)(G_POSE_BUF_0609661C + off108), src);
    shc_copy_word_block(108, (void *)(uintptr_t)(G_POSE_BUF_06096910 + off108),
                  (u8 *)src + 108);

    {
        void *slot = (void *)(uintptr_t)(G_SLOT_060996EC + (s8)(M8(entity, ACTOR_SLOT_ID) * 12));
        /* ROM 06031b34 / 06031be4: high word first. */
        M32(slot, 8) = 0;
        M32(slot, 4) = 0;
        M32(slot, 0) = 0;
    }
}
