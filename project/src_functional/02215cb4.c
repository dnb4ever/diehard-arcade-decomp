#include "dh_work.h"
/* model_build_quad_list [0x02215cb4]: Transforms model vertices, culls quads and appends visible draw records before dispatch. */
/* 0x02215cb4 (build_quad_drawlist) - build a draw command list for display object
 * arg0 and tail-dispatch to its type handler. arg1 is an or-mask/selector.
 * The object type record is rec = 0x0609D7D0 + id*0x14, where
 * id = *(s16*)(0x0609EBD0 + arg0->unk30*2) (the id->record map). Steps:
 *   - func_022135a0((u8)rec[0], rec->unk4): per-type preamble.
 *   - add rec->unk1 (the sub-quad count) to the live quad counter: the SH-2 reads
 *     the count accumulator through the cache mirror 0x26098818 and writes it back to
 *     0x06098818.
 *   - walk the count sub-quad descriptors at rec->unk8 (stride 0x10): for each run
 *     func_02213d24(desc, cursor, 0x060DEBC8), then test visibility with
 *     func_02213e0c(desc->unk7 | arg1, desc->unk4, cursor); when it returns non-zero
 *     append to the command cursor (starts at 0x060DA0C8, stride 0x30): write
 *     arg0->unk20 at cursor+0x2C and the desc pointer at cursor+0x28.
 *   - terminate the list with -1 at cursor+0x20.
 *   - tail-call the handler g_06090794[rec->unk10 | arg1].
 * ABI: r4 = arg0, r5 = arg1; tail jmp (no value consumed by callers).
 * fidelity: HIGH - reconstructed from fn.py; the 3 bsrf resolved (braf_resolve) to
 *   func_022135a0 (0x2135a0), func_02213d24 (0x213d24), func_02213e0c (0x213e0c).
 *   The final jmp @r2 is a tail dispatch: r4/r5/r6 at that point are dead (clobbered
 *   by func_02213e0c), so the handler keys off the command list just built, not
 *   registers - modelled as a no-arg call through the table. 0x26098818 (read) vs
 *   0x06098818 (write) kept as the exact mirror/base pair the SH-2 uses. */
#include "dh_band21.h"

extern void vertex_transform_project_raw_entry(s32 type, s32 param);
extern void quad_build_projected_corners(u32 desc, u32 cursor, u32 ctx);
extern s32  quad_test_visible(s32 flag, s32 sel, u32 quad);

typedef void (*draw_handler)(void);

void model_build_quad_list(u32 arg0, s32 arg1)
{
    u32 rec = 0x0609d7d0 + (s16)MEM16(0x0609ebd0 + MEM16(arg0 + 0x30) * 2) * DH_MODEL_DEFINITION_STRIDE;
    s32 count;
    u32 desc;
    u32 cur;
    u32 i;
    draw_handler *tbl;

    vertex_transform_project_raw_entry((u8)MODEL_MEM8(rec, vertex_count), MODEL_MEM32(rec, vertices));

    count = (u8)MODEL_MEM8(rec, quad_count);
    desc  = MODEL_MEMU32(rec, quads);
    cur   = 0x060da0c8;

    MEM32(0x06098818) = MEM32(0x26098818) + count;

    for (i = 0; i < (u32)count; i++) {
        quad_build_projected_corners(desc, cur, 0x060debc8);
        if (quad_test_visible((s8)MEM8(desc + 7) | arg1, MEM16(desc + 4), cur) != 0) {
            MEM16(cur + 0x2c) = MEM16(arg0 + 0x20);
            MEM32(cur + 0x28) = (s32)desc;
            cur += 0x30;
        }
        desc += 0x10;
    }

    MEM32(cur + 0x20) = -1;

    tbl = (draw_handler *)(uintptr_t)0x06090794;
    tbl[(s8)MODEL_MEM8(rec, draw_handler_index) | arg1]();
}
