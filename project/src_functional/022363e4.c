/* debug_obj_inspector [0x022363e4]: Existing map corroborated by source: 2-state. s0 zeroes editor fields, tail func_02223ac0; (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x022363e4 - debug object-inspector / cursor editor task (dispatch on task->unk10).
 *
 * ent = task->unk8.  padA = *0x06098734, padB = *0x0609874c are the two input-state
 * blocks (->unk0 held bits, ->unk4 edge bits, ->unkC auto-repeat counter).
 * g_060e1324[] and g_060e1328[] are {value,-1} shadow pairs fed to the VDP2 attribute
 * helpers func_02223ac0 / func_02215228.
 *
 *   state 0 (init): zero the editor fields, advance task->unk10 to 1, seed resource
 *       slot func_022150d4(g_060915f4[0]), publish the initial index to g_060e1324 /
 *       g_060e1328 and TAIL-CALL func_02223ac0(&g_060e1324).
 *   state 1 (run): translate pad input into edits -
 *       padA&7==0: nudge ent->unk24 (+/-0x400 on 0x80/0x40), ent->unk26 (-/+ on
 *           0x100/0x200);  padB&7==0: nudge ent->unk28 (-/+ on 0x200/0x100).
 *       padA&1: step the 1..0x618 index ent->unk30 up (0x100 held + edge/repeat) or
 *           down (0x200), republishing it through func_02223ac0.
 *       padB.unk4&1: toggle ent->unk36;  &2: flip ent->unk20 bit 0x100.
 *       padB&4: edit mode - step ent->unk37 (0..14 wrap) on 0x80/0x40 and reseed its
 *           resource; step ent->unk58 (0..88 wrap) on 0x100/0x200, republishing unk5A.
 *       Then the draw sequence (matrix push/setup + func_02215a2c) and SIX debug-text
 *       glyph writes through the variadic thunk *0x060902a0, each passing exactly TWO
 *       stack arguments (format descriptor, value) with the per-site latches
 *       g_060997a4 / g_060997a8; the 12 pushes are reclaimed together (add #48,r15).
 *   other: no-op.
 *
 * fidelity: HIGH - all 14 computed branches resolved (braf_resolve).  m2c corrections:
 * the state-0 braf is a tail-call to func_02223ac0 (m2c mis-read the trailing pool as
 * code); every glyph site takes 2 stack args (m2c reported 5/2 by counting dead
 * leftover registers r4/r5/r6 from the preceding draw calls - those are NOT thunk
 * arguments). */

#define PADA ((void *)DH_PLAYER_INPUT_RECORDS_ADDR)
#define PADB ((void *)0x0609874c)
#define GLYPH2 (*(void (*volatile *)(void *, s32))0x060902a0)   /* variadic debug-glyph thunk, 2 args/site here */

extern s16   g_060e1324[];                  /* 0x060e1324 - {index,-1} shadow pair A */
extern s16   g_060e1328[];                  /* 0x060e1328 - {value,-1} shadow pair B */
extern void *g_060915f4[];                  /* 0x060915f4 - per-unk37 resource table */
extern s32   g_matrix_stack_cursor;                    /* 0x06098878 - VDP1 command-list write ptr */
extern s32   g_glyph_param0;                /* 0x060997a4 - glyph thunk latch A */
extern s32   g_glyph_param1;                /* 0x060997a8 - glyph thunk latch B */

extern void  vdp1_upload_texture_groups(void *);
extern void  vdp2_upload_pattern_groups(void *);
extern void  model_load_scene_records(void *);
extern void  mtx_load_abc(void);
extern void  mtx_stack_push_dup(void);
extern void  mtx_apply_vec_at8(void *);
extern void  mtx_rotate_zyx_at24(void *);
extern void  model_submit_culled(void *, s32);

void debug_obj_inspector(void *task) {
    void *ent = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0:
        M16(ent, 0x30) = 1;
        M32(ent, 8) = 0; M32(ent, 0xC) = 0; M32(ent, 0x10) = 0;
        M8(ent, 0x36) = 0;
        M16(ent, 0x24) = 0; M16(ent, 0x26) = 0; M16(ent, 0x28) = 0;
        M32(ent, 8) = 0; M32(ent, 0xC) = 0; M32(ent, 0x10) = 0;
        TASK_U8(task, state)++;
        M8(ent, 0x37) = 0;
        vdp1_upload_texture_groups(g_060915f4[(u8)M8(ent, 0x37)]);
        M32(ent, 0x58) = 0;
        g_060e1328[0] = 0;
        g_060e1328[1] = -1;
        vdp2_upload_pattern_groups(&g_060e1328[0]);
        g_060e1324[0] = M16(ent, 0x30);
        g_060e1324[1] = -1;
        model_load_scene_records(&g_060e1324[0]);          /* tail call */
        break;
    case 1:
        if (!(MU16(PADA, 0) & 7)) {
            if (MU16(PADA, 0) & 0x80)  M16(ent, 0x24) += 0x400;
            if (MU16(PADA, 0) & 0x40)  M16(ent, 0x24) -= 0x400;
            if (MU16(PADA, 0) & 0x100) M16(ent, 0x26) -= 0x400;
            if (MU16(PADA, 0) & 0x200) M16(ent, 0x26) += 0x400;
        }
        if (!(MU16(PADB, 0) & 7)) {
            if (MU16(PADB, 0) & 0x200) M16(ent, 0x28) -= 0x400;
            if (MU16(PADB, 0) & 0x100) M16(ent, 0x28) += 0x400;
        }
        if (MU16(PADA, 0) & 1) {
            if ((MU16(PADA, 0) & 0x100) &&
                ((MU16(PADA, 4) & 0x100) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x30)++;
                if (M16(ent, 0x30) > 0x618) M16(ent, 0x30) = 1;
                g_060e1324[0] = M16(ent, 0x30);
                model_load_scene_records(&g_060e1324[0]);
            }
            if ((MU16(PADA, 0) & 0x200) &&
                ((MU16(PADA, 4) & 0x200) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x30)--;
                if (M16(ent, 0x30) == 0) M16(ent, 0x30) = 0x618;
                g_060e1324[0] = M16(ent, 0x30);
                model_load_scene_records(&g_060e1324[0]);
            }
        }
        if (MU16(PADB, 4) & 1) M8(ent, 0x36) = (s8)((M8(ent, 0x36) + 1) & 1);
        if (MU16(PADB, 4) & 2) M16(ent, 0x20) ^= 0x100;
        if (MU16(PADB, 0) & 4) {
            if (MU16(PADB, 4) & 0x80) {
                M8(ent, 0x37) = (s8)((M8(ent, 0x37) >= 14) ? 0 : M8(ent, 0x37) + 1);
                vdp1_upload_texture_groups(g_060915f4[(u8)M8(ent, 0x37)]);
            } else if (MU16(PADB, 4) & 0x40) {
                M8(ent, 0x37) = (s8)((M8(ent, 0x37) > 0) ? M8(ent, 0x37) - 1 : 14);
                vdp1_upload_texture_groups(g_060915f4[(u8)M8(ent, 0x37)]);
            }
            if (MU16(PADB, 4) & 0x100) {
                M32(ent, 0x58) = (M32(ent, 0x58) >= 88) ? 0 : M32(ent, 0x58) + 1;
                g_060e1328[0] = M16(ent, 0x5A);
                vdp2_upload_pattern_groups(&g_060e1328[0]);
            } else if (MU16(PADB, 4) & 0x200) {
                M32(ent, 0x58) = (M32(ent, 0x58) > 0) ? M32(ent, 0x58) - 1 : 88;
                g_060e1328[0] = M16(ent, 0x5A);
                vdp2_upload_pattern_groups(&g_060e1328[0]);
            }
        }
        mtx_load_abc();
        mtx_stack_push_dup();
        mtx_apply_vec_at8(ent);
        mtx_rotate_zyx_at24(ent);
        model_submit_culled(ent, (s8)M8(ent, 0x36) << 2);
        g_matrix_stack_cursor -= 0x30;
        g_glyph_param0 = 0x4E; g_glyph_param1 = 0x0A; GLYPH2((void *)0x0608c16c, M16(ent, 0x30));
        g_glyph_param0 = 0x4C; g_glyph_param1 = 0x0D; GLYPH2((void *)0x0608c174, (u16)M16(ent, 0x24));
        g_glyph_param0 = 0x4C; g_glyph_param1 = 0x0E; GLYPH2((void *)0x0608c174, (u16)M16(ent, 0x26));
        g_glyph_param0 = 0x4C; g_glyph_param1 = 0x0F; GLYPH2((void *)0x0608c174, (u16)M16(ent, 0x28));
        g_glyph_param0 = 0x4C; g_glyph_param1 = 0x16; GLYPH2((void *)0x0608c174, M8(ent, 0x37));
        g_glyph_param0 = 0x4C; g_glyph_param1 = 0x17; GLYPH2((void *)0x0608c174, M32(ent, 0x58));
        break;
    default:
        break;
    }
}
