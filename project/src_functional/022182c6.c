/* prop_tick_triggered_shutter [0x022182c6]: Seven phases use scene trigger masks for opening/closing slide and hold timers. */
#include "dh.h"
/* 0x022182c6  shutter_update: 7-state opening/closing panel driven by the global
 * trigger-flag word g_06098580 and a per-face parameter table at 0x06054748.
 *
 * w = obj->unk8 (work record). States (obj->unk10):
 *   0  init: pick motion deltas by obj->unk12 (0/1/2 = vertical/horizontal/other),
 *      latch the trigger mask (w->unk60) and sub-mask (w->unk64) from table
 *      0x06054748[obj->unkF], snapshot position (unk8->unk14) and angles
 *      (unk24->unk2A, biasing unk2C by 0x4000), advance.
 *   1  wait for the trigger mask in g_06098580; on hit clear it, reset and open.
 *   2/3  opening slide (two speeds); advance when the frame counter (unk58) elapses.
 *   4  fully open: hold for obj->unk12==1, then wait for the sub-mask to re-trigger.
 *   5/6  closing slide; a mid-close re-trigger reverses back into opening with a
 *      phase correction (state 6 also snaps unk58 onto a 4-frame grid).
 * Every state then draws the panel: 2 (or 3, when w->unk5C set) layers, each a
 * matrix push + load + translate + rotate + VDP1 submit around g_06098878.
 *
 * fidelity: HIGH control flow / MED semantics. m2c decompiled cleanly (no
 * M2C_ERROR); all 4 mova+add call pointers resolved and their ABIs confirmed:
 * mtx_load_abc (no args), mtx_apply_vec_at0(vec3*), mtx_rotate_zyx(u16*),
 * func_02215a2c (undecompiled sprite submitter, args work,0). The sprite-id table
 * reads 0x060546F8/0x06054704 and the 0x06054748 param table are indexed but their
 * element meaning is inferred. */
extern void mtx_load_abc(void);                 /* 0x022149aa */
extern void mtx_apply_vec_at0(u8 *vec);         /* 0x02214a50 vec3 matmul in place  */
extern void mtx_rotate_zyx(u16 *angles);        /* 0x022149e4 rotate active matrix   */
extern void model_submit_culled(void *obj, s32 flags);/* 0x02215a2c sprite submit (undecompiled) */

void prop_tick_triggered_shutter(void *obj) {
    void *w     = MP(obj, 8);
    s8    state = M8(obj, 0x10);

    switch (state) {
    case 1:
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(w, 0x60)) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(w, 0x60);
            M32(w, 0x58) = 0;
            M32(w, 0x5C) = 1;
            M8(obj, 0x10) += 1;
        }
        break;
    case 2:
        M32(w, 0x08) -= M32(w, 0x70);
        M32(w, 0x14) += M32(w, 0x70);
        M32(w, 0x10) += M32(w, 0x74);
        M32(w, 0x1C) -= M32(w, 0x74);
        M32(w, 0x58) += 1;
        if (M32(w, 0x58) >= 0x1C) {
            M8(obj, 0x10) += 1;
        }
        break;
    case 3:
        M32(w, 0x08) -= M32(w, 0x78);
        M32(w, 0x14) += M32(w, 0x78);
        M32(w, 0x10) += M32(w, 0x7C);
        M32(w, 0x1C) -= M32(w, 0x7C);
        M32(w, 0x58) += 1;
        if (M32(w, 0x58) >= 0x2C) {
            M32(w, 0x6C) = 0;
            M8(obj, 0x10) = 4;
        }
        break;
    case 4:
        if (M8(obj, 0x12) == 1) {
            M32(w, 0x6C) += 1;
            if (M32(w, 0x6C) > 0x3C) {
                M32(w, 0x58) = 0;
                M8(obj, 0x10) += 1;
            }
        }
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(w, 0x64)) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(w, 0x64);
            M32(w, 0x58) = 0;
            M8(obj, 0x10) += 1;
        }
        break;
    case 5:
        M32(w, 0x08) += M32(w, 0x70);
        M32(w, 0x14) -= M32(w, 0x70);
        M32(w, 0x10) -= M32(w, 0x74);
        M32(w, 0x1C) += M32(w, 0x74);
        M32(w, 0x58) += 1;
        if (M32(w, 0x58) >= 0x1C) {
            M8(obj, 0x10) += 1;
        }
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(w, 0x60)) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(w, 0x60);
            if (M32(w, 0x58) <= 4) {
                M32(w, 0x58) = -(M32(w, 0x58) * 4) + 0x24;
                M8(obj, 0x10) = 3;
            } else {
                M32(w, 0x58) = -M32(w, 0x58) + 0x20;
                M8(obj, 0x10) = 2;
            }
        }
        break;
    case 6:
        M32(w, 0x08) += M32(w, 0x78);
        M32(w, 0x14) -= M32(w, 0x78);
        M32(w, 0x10) -= M32(w, 0x7C);
        M32(w, 0x1C) += M32(w, 0x7C);
        M32(w, 0x58) += 1;
        if (M32(w, 0x58) >= 0x2C) {
            M32(w, 0x5C) = 0;
            M8(obj, 0x10) = 1;
        }
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(w, 0x60)) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(w, 0x60);
            if (M32(w, 0x58) & 3) {
                s32 i = 0;
                while (i < (M32(w, 0x58) & 3)) {
                    i++;
                    M32(w, 0x08) -= M32(w, 0x78);
                    M32(w, 0x14) += M32(w, 0x78);
                    M32(w, 0x10) += M32(w, 0x7C);
                    M32(w, 0x1C) -= M32(w, 0x7C);
                }
                M32(w, 0x58) -= (M32(w, 0x58) & 3);
            }
            M32(w, 0x58) = -((M32(w, 0x58) - 0x1C) >> 2) + 4;
            M32(w, 0x5C) = 1;
            M8(obj, 0x10) = 2;
        }
        break;
    case 0: {
        u8 *tbl;
        switch (M8(obj, 0x12)) {
        case 0:
            M32(w, 0x68) = 0;
            M32(w, 0x74) = 0x800;
            M32(w, 0x7C) = 0x200;
            break;
        case 1:
            M32(w, 0x68) = 2;
            M32(w, 0x70) = 0x800;
            M32(w, 0x78) = 0x200;
            break;
        case 2:
            M32(w, 0x68) = 4;
            M32(w, 0x74) = -0x800;
            M32(w, 0x7C) = -0x200;
            break;
        }
        tbl = (u8 *)0x06054748 + M8(obj, 0x0F) * 4;
        M32(w, 0x60) = M32(tbl, 0);
        M32(w, 0x64) = M16(tbl, 2);
        M32(w, 0x14) = M32(w, 0x08);
        M32(w, 0x18) = M32(w, 0x0C);
        M32(w, 0x1C) = M32(w, 0x10);
        M16(w, 0x2A) = M16(w, 0x24);
        M16(w, 0x2C) = M16(w, 0x26);
        M16(w, 0x2E) = M16(w, 0x28);
        M16(w, 0x2C) += 0x4000;
        M8(obj, 0x10) += 1;
        break;
    }
    default:
        break;
    }

    /* draw: push matrix stack, render 2-3 translated/rotated panel layers */
    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) += 0x30;

    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)w + 0x14);
    mtx_rotate_zyx((u16 *)((u8 *)w + 0x24));
    M16(w, 0x30) = M16(0x060546F8 + M32(w, 0x68) * 2, 0);
    model_submit_culled(w, 0);

    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)w + 0x08);
    mtx_rotate_zyx((u16 *)((u8 *)w + 0x24));
    M16(w, 0x30) = M16(0x060546F8 + (M32(w, 0x68) + 1) * 2, 0);
    model_submit_culled(w, 0);

    if (M32(w, 0x5C) != 0) {
        mtx_load_abc();
        mtx_apply_vec_at0((u8 *)0x0605470C + (s8)((M8(obj, 0x0F) + M8(w, 0x6B)) * 0xC));
        mtx_rotate_zyx((u16 *)((u8 *)w + 0x2A));
        M16(w, 0x30) = M16(0x06054704 + (M32(w, 0x68) >> 1) * 2, 0);
        model_submit_culled(w, 0);
    }

    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) -= 0x30;
}
