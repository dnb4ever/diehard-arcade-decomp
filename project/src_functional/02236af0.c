/* debug_cursor_editor [0x02236af0]: Existing map corroborated by source: 2-state cursor/text editor. out-of-cart tables (src symbols_band234.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02236af0 - debug object/cursor text editor task (dispatch on task->unk10).
 *
 * ent = task->unk8.  padA = *0x06098734, padB = *0x0609874c (->unk0 held bits, ->unk4
 * edge bits, ->unkC auto-repeat counter).  Two out-of-cart description tables drive it:
 * g_22700000[] (stride 0x14, field +0xC = per-page row count) and g_22700aa0[] (stride
 * 8, field +4 = entry type code).  Scratch {value,-1} pairs are published to the VDP2
 * attribute helpers func_02215228 / func_022150d4.
 *
 *   state 0 (init): unk30=1, unk32=0, unk60=0, unk64=1, unk68=0, unk6C=0x10000 (zoom),
 *       unk8=unkC=0, unk10=0x10000; advance to state 1; fall through.
 *   state 1 (run): translate pad input (when padA&7==0) into index edits - the 1..0x18AC
 *       primary index ent->unk30 (down 0x200 / up 0x100), the 0..0x58 sub-index ent->unk32
 *       (down 0x40 / up 0x80), each gated by edge-or-repeat; padB.unk4 adjusts the zoom
 *       ent->unk6C by +/-0x4000; padA&1 moves ent->unk10 by +/-0x1000; padA&4 moves
 *       ent->unkC / ent->unk8 by +/-0x1000.  On a change, republish unk32 (func_02215228)
 *       and the computed cursor page (walk g_22700000 row widths until the running total
 *       reaches unk30, func_022150d4).  Then render func_022153f6(ent, unk6C, 0), pick a
 *       label by the g_22700aa0 type code (switch, 1 glyph arg/site), and write the six
 *       field read-outs (2 glyph args/site).
 *   other:  no-op.
 *   (all paths) ent->unk60 = unk30; ent->unk64 = unk32  (remember last frame).
 *
 * fidelity: HIGH - all 3 computed branches resolved (braf_resolve); the glyph thunk
 * *0x060902a0 jsr sites are hand-counted: the switch cases push ONE stack arg
 * (add #4,r15), the final six push TWO each (add #48,r15).  m2c reported 5 args at those
 * sites by counting dead leftover registers (r4/r5/r6/r7 from func_022153f6) - those are
 * NOT thunk arguments. */

#define GLYPH1 (*(void (*volatile *)(void *))0x060902a0)       /* 1 stack arg / site */
#define GLYPH2 (*(void (*volatile *)(void *, s32))0x060902a0)  /* 2 stack args / site */
#define PADA ((void *)DH_PLAYER_INPUT_RECORDS_ADDR)
#define PADB ((void *)0x0609874c)

extern s32 g_glyph_param0;                  /* 0x060997a4 */
extern s32 g_glyph_param1;                  /* 0x060997a8 */

extern void vdp2_upload_pattern_groups(void *);
extern void vdp1_upload_texture_groups(void *);
extern void sprite_draw_child(void *obj, s32 zoom, s32 flag);   /* sprite renderer */

void debug_cursor_editor(void *task) {
    void *ent = TASK_PTR(task, work);
    s16   scratch4[2];                      /* @(4,r15) - func_02215228 input {unk32,-1} */
    s16   scratch8[2];                      /* @(8,r15) - func_022150d4 input {page,-1} */

    switch (TASK_U8(task, state)) {
    case 0:
        M16(ent, 0x30) = 1;
        M32(ent, 0x60) = 0;
        M16(ent, 0x32) = 0;
        M32(ent, 0x64) = 1;
        M32(ent, 0x68) = 0;
        M32(ent, 0x6C) = 0x10000;
        M32(ent, 8) = 0; M32(ent, 0xC) = 0; M32(ent, 0x10) = 0x10000;
        TASK_U8(task, state)++;
        /* fallthrough */
    case 1:
        if (!(MU16(PADA, 0) & 7)) {
            if ((MU16(PADA, 0) & 0x200) && ((MU16(PADA, 4) & 0x200) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x30)--;
                if (M16(ent, 0x30) == 0) M16(ent, 0x30) = 0x18AC;
            }
            if ((MU16(PADA, 0) & 0x100) && ((MU16(PADA, 4) & 0x100) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x30)++;
                if (M16(ent, 0x30) > 0x18AC) M16(ent, 0x30) = 1;
            }
            if ((MU16(PADA, 0) & 0x40) && ((MU16(PADA, 4) & 0x40) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x32)--;
                if (M16(ent, 0x32) < 0) M16(ent, 0x32) = 0x58;
            }
            if ((MU16(PADA, 0) & 0x80) && ((MU16(PADA, 4) & 0x80) || (M16(PADA, 0xC) > 10))) {
                M16(ent, 0x32)++;
                if (M16(ent, 0x32) > 0x58) M16(ent, 0x32) = 0;
            }
        }
        if (MU16(PADB, 4) & 1)      M32(ent, 0x6C) += 0x4000;
        else if (MU16(PADB, 4) & 2) M32(ent, 0x6C) -= 0x4000;
        if (MU16(PADA, 0) & 1) {
            if (MU16(PADA, 0) & 0x80) M32(ent, 0x10) -= 0x1000;
            if (MU16(PADA, 0) & 0x40) M32(ent, 0x10) += 0x1000;
        }
        if (MU16(PADA, 0) & 4) {
            if (MU16(PADA, 0) & 0x80)  M32(ent, 0xC) += 0x1000;
            if (MU16(PADA, 0) & 0x40)  M32(ent, 0xC) -= 0x1000;
            if (MU16(PADA, 0) & 0x200) M32(ent, 8)   -= 0x1000;
            if (MU16(PADA, 0) & 0x100) M32(ent, 8)   += 0x1000;
        }
        if (M16(ent, 0x32) != M32(ent, 0x64)) {
            scratch4[1] = -1;
            scratch4[0] = M16(ent, 0x32);
            vdp2_upload_pattern_groups(&scratch4[0]);
        }
        if (M16(ent, 0x30) != M32(ent, 0x60)) {
            s32 *rows = (s32 *)0x22700000;      /* stride 0x14 = 5 words, field +0xC */
            s32 idx = 1;
            s32 total = 0;
            scratch8[1] = -1;
            while (M16(ent, 0x30) > total) {
                total += rows[idx * 5 + 3];
                idx++;
            }
            scratch8[0] = (s16)(idx - 1);
            if (M32(ent, 0x68) != scratch8[0]) {
                vdp1_upload_texture_groups(&scratch8[0]);
                M32(ent, 0x68) = scratch8[0];
            }
        }
        sprite_draw_child(ent, M32(ent, 0x6C), 0);
        switch (((s16 *)0x22700aa0)[M16(ent, 0x32) * 4 + 2]) {   /* stride 8, field +4 */
        case 0:  g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c200); break;
        case 8:  g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c208); break;
        case 16: g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c210); break;
        case 24: g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c218); break;
        case 32: g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c220); break;
        case 40: g_glyph_param0 = 77; g_glyph_param1 = 12; GLYPH1((void *)0x0608c228); break;
        default: break;
        }
        g_glyph_param0 = 77; g_glyph_param1 = 10; GLYPH2((void *)0x0608c230, M16(ent, 0x30));
        g_glyph_param0 = 77; g_glyph_param1 = 11; GLYPH2((void *)0x0608c230, M16(ent, 0x32));
        g_glyph_param0 = 78; g_glyph_param1 = 15; GLYPH2((void *)0x0608c238, M32(ent, 8));
        g_glyph_param0 = 78; g_glyph_param1 = 16; GLYPH2((void *)0x0608c238, M32(ent, 0xC));
        g_glyph_param0 = 78; g_glyph_param1 = 17; GLYPH2((void *)0x0608c238, M32(ent, 0x10));
        g_glyph_param0 = 76; g_glyph_param1 = 18; GLYPH2((void *)0x0608c238, M32(ent, 0x6C));
        break;
    default:
        break;
    }
    M32(ent, 0x60) = M16(ent, 0x30);
    M32(ent, 0x64) = M16(ent, 0x32);
}
