/* effect_tick_reference_debris [0x02239194]: Owner slot indexes transform table; cancel byte and task states drive rotation/scale. */
#include "dh.h"
/* 0x02239194 - per-frame update for a thrown/spawned debris sprite (the child
 * object arg0->unk8). Follows a reference transform table indexed by
 * arg0->unk14->unk8->unk34. If arg0->unk12 != 0 the sprite is cancelled
 * (arg0->unk0 = 0) and the function returns.
 * State machine on arg0->unk10 drives rotation/scale fields of obj:
 *   0: init spin dir from arg0->unkF, set unk40=8/unk44=0xC000, advance; falls
 *      into case 1.
 *   1: obj->unk34 += 10, unk36/unk38 += 2; when --unk40 == 0 -> unk40=6, next.
 *   2: when --unk40 == 0 -> unk40=8, next.
 *   3: obj->unk34 -= 10, unk36/unk38 -= 2; when --unk40 == 0 -> arg0->unk0=0.
 * Then func_022386fc(&obj->unk34, 8, n); integrate unk44 += 0x1000, unk48 += 0x1400;
 * set obj position unk8/C/10 from the table plus offsets; cycle unk30 0x4B..0x4E.
 * Tail: push draw offset (g_06098878 += 48), run transform/draw helpers, pop.
 * branches resolved: bsrf 0x2149aa,0x214a50,0x2149e4,0x213190,0x2131a8,
 *   0x213ce4,0x215a2c; bsr func_022386fc.
 * fidelity: HIGH for the state machine/fields; MED for terminal draw-helper arg
 * shapes (mtx_load_abc gets no register set by this caller). */
extern u8   g_06097d58[];
extern s32  g_matrix_stack_cursor;
extern void display_accumulate_channel(void *p, s32 a, s32 b);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at0(void *v);
extern void mtx_rotate_zyx(void *v);
extern s32  sin_lookup(s32 angle);
extern s32  cos_lookup(s32 angle);
extern void matrix_scale_columns(s32 a, s32 b, s32 c);
extern void model_submit_culled(void *obj, s32 b);

void effect_tick_reference_debris(void *arg0) {
    void *obj = MP(arg0, 8);
    void *tbl = &g_06097d58[(M8(MP(MP(arg0, 0x14), 8), 0x34) << 8) + 0x50];
    s32 n = 0;
    s32 a4;
    s32 a5;

    if (M8(arg0, 0x12) != 0) {
        M32(arg0, 0) = 0;
        return;
    }

    switch (M8(arg0, 0x10)) {
    case 0:
        n = 0xC000;
        if (M8(arg0, 0xF) != 0) {
            M16(obj, 0x26) = 0x4000;
            M32(obj, 0x60) = -0x3C00;
        } else {
            M16(obj, 0x26) = (s16)0xC000;
            M32(obj, 0x60) = 0x3C00;
        }
        M32(obj, 0x40) = 8;
        M32(obj, 0x44) = 0xC000;
        M8(arg0, 0x10)++;
        /* fallthrough */
    case 1:
        M16(obj, 0x34) += 10;
        M16(obj, 0x36) += 2;
        M16(obj, 0x38) += 2;
        if (--M32(obj, 0x40) == 0) {
            M32(obj, 0x40) = 6;
            M8(arg0, 0x10)++;
        }
        break;
    case 2:
        if (--M32(obj, 0x40) == 0) {
            M32(obj, 0x40) = 8;
            M8(arg0, 0x10)++;
        }
        break;
    case 3:
        M16(obj, 0x34) -= 10;
        M16(obj, 0x36) -= 2;
        M16(obj, 0x38) -= 2;
        if (--M32(obj, 0x40) == 0)
            M32(arg0, 0) = 0;
        break;
    default:
        break;
    }

    display_accumulate_channel((u8 *)obj + 0x34, 8, n);
    M32(obj, 0x44) += 0x1000;
    M32(obj, 0x48) += 0x1400;
    M32(obj, 8)    = M32(tbl, 0) + M32(obj, 0x60);
    M32(obj, 0xC)  = M32(tbl, 4) + 0x2000;
    M32(obj, 0x10) = M32(tbl, 8);
    M16(obj, 0x30) = (s16)(((M32(obj, 0x54)++) & 3) + 0x4B);

    g_matrix_stack_cursor += 48;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)obj + 8);
    mtx_rotate_zyx((u8 *)obj + 36);
    a4 = sin_lookup(M32(obj, 0x44));
    a5 = cos_lookup(M32(obj, 0x48));
    matrix_scale_columns(0x00010000, 0x00020000 + (-a5 >> 1), 0x00028000 + a4);
    model_submit_culled(obj, 0);
    g_matrix_stack_cursor -= 48;
}
