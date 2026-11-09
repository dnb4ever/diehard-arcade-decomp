/* actor_draw_skeletal_mesh [0x0222d450]: Walks the unrolled skeletal hierarchy and submits each transformed mesh part. */
#include "dh.h"
/* 0x0222d450 - skeletal-mesh draw for one actor (unrolled, ~25 bones).
 *
 * Builds the actor's pose on the shared matrix stack (*0x06098878) and submits
 * each bone's mesh. NOT a jump-table dispatcher: every "computed branch" in the
 * raw asm is a bsrf CALL (the 0xFFFExxxx pool words are negative PC-relative call
 * deltas, not table offsets). The body is straight-line, mirroring the unrolled
 * asm bone-by-bone; the per-bone cadence varies, so it is transcribed 1:1 rather
 * than looped.
 *
 * Three cursors walk in lock-step with the bones:
 *   mcur (u8*)  matrix-vector array   base 0x0607976C + M16(actor,0x5E)*0xB4,
 *               advanced +0xC (one 3-long vec) per mtx_apply_vec_at0.
 *   acur (u16*) bone rotation record  base 0x06096328 + (s16)((s8)unk34*0x6C),
 *               advanced +6 (three u16 angles) per mtx_rotate_zyx.
 *   pcur (s16*) pose-word stream      base 0x06096248 + ((s8)unk34<<5),
 *               read into actor->unk30 (reduced path only), advanced +2 per word.
 * The matrix-stack top *0x06098878 is pushed (+0x30) / popped (-0x30, in groups)
 * along the way; mtx_stack_push_dup also bumps it by +0x30 internally.
 *
 * A cull guard on the pushed matrix (bound=unk2C, |unkC|, |unk1C|) selects which
 * of two unrolled sequences runs: when in-bound the reduced path (writes pose
 * words into unk30 and submits via func_02215964), otherwise the full-detail path
 * (static vec sources, no pose-word writes).
 *
 * fidelity: HIGH. Equivalence-faithful; call targets from decomp_bounded.py
 *   (--end 0x22da90) + ptr_resolve.py (r8=mtx_rotate_zyx, r10=mtx_stack_push_dup,
 *   r12=mtx_apply_vec_at0). Chained stack adjusts collapsed to a single subtract
 *   of the same total (pure integer arithmetic, no intervening read). All stack
 *   deltas are +/-0x30 (48 decimal); there is no 0x48 delta. mtx_load_abc takes
 *   no argument (the leftover r4 is unused by it).
 */

extern void matrix_translate_current(s32 x, s32 y, s32 z);     /* vec3_array_matmul */
extern void actor_store_part_transform(void *actor, s32 part);   /* submit bone mesh (indexed) */
extern void sprite_emit_or_queue(void *actor);             /* submit bone mesh (reduced) */
extern void draw_rot_sprite_source_alias(void *obj, s32 angle);    /* draw_rot_sprite */

extern void mtx_load_abc(void);                     /* 0x022149aa */
extern void mtx_apply_vec_at8(u8 *obj);             /* 0x02214a40 */
extern void mtx_rotate_zyx_at24(u8 *obj);           /* 0x02214a06 */
extern void mtx_stack_push_dup(void);               /* 0x02214a2e */
extern void mtx_rotate_zyx(u16 *angles);            /* 0x022149e4 */
extern void mtx_apply_vec_at0(u8 *obj);             /* 0x02214a50 */

#define MTX_SP MU32(DH_MATRIX_STACK_CURSOR_ADDR, 0)                  /* matrix-stack top (u32 addr) */

void actor_draw_skeletal_mesh(void *actor) {
    s32 slot = M8(actor, ACTOR_SLOT_ID);
    u8 *mcur = (u8 *)(uintptr_t)0x0607976CU + M16(actor, 0x5E) * 0xB4;
    u16 *acur = (u16 *)(uintptr_t)(0x06096328U + (u32)(s16)(slot * 0x6C));
    s16 *pcur = (s16 *)(uintptr_t)(0x06096248U + ((u32)slot << 5));
    u8 *mtx;
    s32 bound;
    s32 ax;
    s32 az;
    u32 sp_tail = 0;

    /* setup: push the stack, load the base matrix, orient the actor */
    MTX_SP = MTX_SP + 0x30;
    mtx_load_abc();
    draw_rot_sprite_source_alias(actor, *(s16 *)acur);
    mtx_apply_vec_at8((u8 *)actor);
    mtx_rotate_zyx_at24((u8 *)actor);

    /* cull guard on the freshly pushed matrix */
    mtx = (u8 *)(uintptr_t)MTX_SP;
    bound = M32(mtx, 0x2C);
    ax = M32(mtx, 0xC);
    if (ax < 0) {
        ax = -ax;
    }
    az = M32(mtx, 0x1C);
    if (az < 0) {
        az = -az;
    }

    if (bound >= 0 && ax <= bound && az <= bound) {
        /* reduced path (.L22d738): streams pose words into unk30 */
        mtx_stack_push_dup();
        matrix_translate_current(0, M32(0x06096C58, slot * 4), 0);
        actor_store_part_transform(actor, 0);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 0xA);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        MTX_SP = MTX_SP - 0x30;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 0xB);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        pcur++;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        pcur++;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 1);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        MTX_SP = MTX_SP - 0xF0;                 /* ((((-0x30)-0x30)-0x30)-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 2);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 3);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 4);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 5);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 6);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 7);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur++;
        sprite_emit_or_queue(actor);
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 8);
        mtx_rotate_zyx(acur); acur += 3;
        M16(actor, 0x30) = *pcur;       /* read-only (no advance) */
        sprite_emit_or_queue(actor);
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 9);
        sp_tail = ((MTX_SP - 0x30) - 0x30) - 0x30;
    } else {
        /* full-detail path (.L22d526): static vec sources, no pose-word writes */
        mtx_stack_push_dup();
        matrix_translate_current(0, M32(0x06096C58, slot * 4), 0);
        actor_store_part_transform(actor, 0);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        MTX_SP = MTX_SP - 0x30;
        mtx_stack_push_dup();
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F18U);
        actor_store_part_transform(actor, 0xA);
        MTX_SP = MTX_SP - 0x30;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 0xB);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 1);
        mtx_rotate_zyx(acur); acur += 3;
        MTX_SP = MTX_SP - 0xF0;                 /* ((((-0x30)-0x30)-0x30)-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 2);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 3);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 4);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 5);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 6);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 7);
        MTX_SP = MTX_SP - 0x60;                 /* (-0x30)-0x30 */
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        mtx_rotate_zyx(acur); acur += 3;
        mtx_stack_push_dup();
        mtx_apply_vec_at0(mcur); mcur += 12;
        actor_store_part_transform(actor, 8);
        mtx_rotate_zyx(acur); acur += 3;
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06087F0CU);
        actor_store_part_transform(actor, 9);
        sp_tail = ((MTX_SP - 0x30) - 0x30) - 0x30;
    }

    MTX_SP = (sp_tail - 0x30) - 0x30;
}
/* fidelity: HIGH - straight-line transcription of the unrolled asm (both cull
   paths), call targets resolved via decomp_bounded.py/ptr_resolve.py; chained
   +/-0x30 stack adjusts collapsed to equal-value single subtracts. */
