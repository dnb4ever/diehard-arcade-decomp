/* draw_actor_hier_shadowed [0x0222ca98]: Draws the articulated actor hierarchy with conditional projected shadows. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222ca98 (draw_actor_hier_shadowed) - hand-unrolled articulated-model draw for
 * one actor, with conditional ground-shadow projection. Sibling of
 * draw_actor_hier_model (0x0222d0e8); same three parallel per-bone arrays selected
 * off the actor:
 *   rot    = 0x06096328 + (s16)((s8)unk34 * 108)   rotation records, stride 6  (3 u16 angles @0/2/4)
 *   trans  = 0x0607976C + (s16)unk5E * 180         translation records, stride 0xC (3 s32 @0/4/8)
 *   partid = 0x06096248 + (s8)unk34 * 32           shadow part-id words, stride 2 (s16)
 *
 * Per bone the body does: matrix_stack_push(); mtx_apply_vec_at0(trans); trans+=0xC;
 * mtx_rotate_zyx(rot); rot+=6; then the draw call func_0222ec98(actor, part) and, on
 * the shadow path, a projected shadow sprite (actor->unk30 = *partid++;
 * emit_or_queue_sprite(actor)). The matrix stack pointer g_mtx_sp (0x06098878) is
 * pushed/popped by 0x30-byte frames to walk the bone hierarchy.
 *
 * Setup: push a frame, mtx_load_abc, draw the whole-body rotated sprite
 * (draw_rot_sprite(actor, rot[0])), apply the base translation (mtx_apply_vec_at8)
 * and rotation (mtx_rotate_zyx_at24). Then the active matrix (*g_mtx_sp) is tested:
 * if its unk2C bound is >=0 and |unkC| and |unk1C| are within it (model projects on
 * screen) the SHADOW path runs (bones + shadow sprites + the per-actor flash sprite
 * at g_0609974C[unk34]); otherwise the identical bone hierarchy is drawn WITHOUT
 * shadows. Both paths also apply the special per-actor offset record
 * 0x060996EC[unk34*12] before body-part 0xA. A final pop balances the setup frame.
 *
 * m2c corrections: the three PIC fn-ptrs (ptr_resolve: 0x02214a2e matrix_stack_push,
 * 0x022149e4 mtx_rotate_zyx, 0x02214a50 mtx_apply_vec_at0) were untyped; matrix_stack_push
 * takes NO args (its first-call (mtx,w) args in m2c are stale r4/r5). All 46 bsrf
 * resolve to real TARGETs; no register-count shifts (shll ×1/×2 only, no ashl/ashr
 * helper), no tst #imm8 traps. Bounded 0x22ca98..0x22d0b0 (single rts). fidelity: HIGH. */

#define M_SP (*(s32 *)(uintptr_t)DH_MATRIX_STACK_CURSOR_ADDR)
#define AI32(a) (*(s32 *)(uintptr_t)(a))

extern void matrix_stack_push(void);                /* 0x02214a2e */
extern void mtx_rotate_zyx(void *rec);              /* 0x022149e4 */
extern void mtx_apply_vec_at0(void *rec);           /* 0x02214a50 */
extern void mtx_load_abc(void);                     /* 0x022149aa */
extern void mtx_apply_vec_at8(void *actor);         /* 0x02214a40 */
extern void mtx_rotate_zyx_at24(void *actor);       /* 0x02214a06 */
extern void draw_rot_sprite(void *actor, s16 angle);/* 0x0223967a */
extern void vec3_array_matmul(s32 a, s32 b, s32 c); /* 0x02213ca4 */
extern void emit_or_queue_sprite(void *actor);      /* 0x02215964 */
extern void actor_store_part_transform(void *actor, s32 part);   /* draw body part */

void draw_actor_hier_shadowed(void *actor)
{
    u8 *rot    = (u8 *)(uintptr_t)(0x06096328 + (s16)((s8)ACTOR_WORK(actor)->slot_id * 108));
    u8 *trans  = (u8 *)(uintptr_t)(0x0607976C + (s16)ACTOR_M16(actor, model_index) * 0xB4);
    u8 *partid = (u8 *)(uintptr_t)(0x06096248 + (s8)ACTOR_WORK(actor)->slot_id * 32);
    void *mtx;
    s32 w, cx, cz, ax, az;

    /* per-bone idioms (rot/trans/partid advance exactly as the original) */
#define PUSH()   matrix_stack_push()
#define ROT()    (mtx_rotate_zyx(rot),  rot += 6)
#define VEC()    (mtx_apply_vec_at0(trans), trans += 0xC)
#define VECSP()  mtx_apply_vec_at0((void *)(uintptr_t)(0x060996EC + (s8)(ACTOR_WORK(actor)->slot_id * 0xC)))
#define SHADOW() (M16(actor, 0x30) = *(s16 *)partid, partid += 2, emit_or_queue_sprite(actor))
#define DRAW(n)  actor_store_part_transform(actor, (n))
#define POS(v)   vec3_array_matmul(0, (v), 0)
#define POP(k)   (M_SP -= 0x30 * (k))

    M_SP += 0x30;
    mtx_load_abc();
    draw_rot_sprite(actor, *(s16 *)rot);
    mtx_apply_vec_at8(actor);
    mtx_rotate_zyx_at24(actor);

    mtx = (void *)(uintptr_t)M_SP;
    w  = M32(mtx, 0x2C);
    cx = M32(mtx, 0xC);
    cz = M32(mtx, 0x1C);
    ax = cx < 0 ? -cx : cx;
    az = cz < 0 ? -cz : cz;

    if (w >= 0 && ax <= w && az <= w) {
        /* SHADOW path: bones + projected shadow sprites */
        PUSH(); POS(AI32(0x06096C58 + (s8)ACTOR_WORK(actor)->slot_id * 4)); ROT();
        PUSH(); VEC(); DRAW(0); ROT(); SHADOW();
        PUSH(); VEC(); ROT(); SHADOW();
        POS(0x2000); DRAW(1); POP(1);
        PUSH(); VEC(); ROT(); SHADOW();
        VEC(); DRAW(2); ROT(); SHADOW();
        VEC(); DRAW(3); ROT(); SHADOW();
        POP(1);
        PUSH(); VEC(); ROT(); SHADOW();
        VEC(); DRAW(4); ROT(); SHADOW();
        VEC(); DRAW(5); ROT(); SHADOW();
        {   /* per-actor flash sprite */
            s16 f = *(s16 *)(uintptr_t)(0x0609974C + (s8)ACTOR_WORK(actor)->slot_id * 2);
            M16(actor, 0x30) = f;
            if (f != 0) {
                *(s16 *)(uintptr_t)(0x0609974C + (s8)ACTOR_WORK(actor)->slot_id * 2) = 0;
                emit_or_queue_sprite(actor);
            }
        }
        VECSP(); DRAW(0xA); POP(2);
        PUSH(); VEC(); ROT(); SHADOW();
        PUSH(); VEC(); ROT(); SHADOW();
        PUSH(); VEC(); DRAW(6); ROT(); SHADOW();
        VEC(); DRAW(7); ROT(); SHADOW();
        POP(2);
        PUSH(); VEC(); ROT(); SHADOW();
        PUSH(); VEC(); DRAW(8); ROT(); SHADOW();
        VEC(); DRAW(9); ROT();
        M16(actor, 0x30) = *(s16 *)partid;         /* last shadow: current id, no advance */
        emit_or_queue_sprite(actor);
        POP(4);
    } else {
        /* no-shadow path: identical bone hierarchy, no sprite emits */
        PUSH(); POS(AI32(0x06096C58 + (s8)ACTOR_WORK(actor)->slot_id * 4)); ROT();
        PUSH(); VEC(); DRAW(0); ROT();
        PUSH(); VEC(); ROT();
        POS(0x2000); DRAW(1); POP(1);
        PUSH(); VEC(); ROT();
        PUSH(); VEC(); DRAW(2); ROT();
        PUSH(); VEC(); DRAW(3); ROT();
        POP(3);
        PUSH(); VEC(); ROT();
        PUSH(); VEC(); DRAW(4); ROT();
        PUSH(); VEC(); DRAW(5); ROT();
        VECSP(); DRAW(0xA); POP(4);
        PUSH(); VEC(); ROT();
        PUSH(); VEC(); ROT();
        PUSH(); VEC(); DRAW(6); ROT();
        PUSH(); VEC(); DRAW(7);
        POP(3);
        PUSH(); VEC(); ROT();
        PUSH(); VEC(); DRAW(8); ROT();
        PUSH(); VEC(); DRAW(9);
        POP(5);
    }
    POP(1);

#undef PUSH
#undef ROT
#undef VEC
#undef VECSP
#undef SHADOW
#undef DRAW
#undef POS
#undef POP
}
