#include "dh_work.h"
/* model_build_pending_quad_list [0x02215bf4]: Builds the current buffered model's quad list and dispatches its draw handler. */
/* 0x02215bf4 - build the quad draw list for the current per-frame display buffer
 * and tail-dispatch to its type handler. Twin of func_02215cb4 (build_quad_drawlist),
 * but driven by the buffer *g_060DA0C0 instead of an (obj, mask) argument pair;
 * called as frame step 6 (func_02201320). void(void).
 * buf = *(u32 *)0x060DA0C0; its fields are read through the cache-through mirror
 * (buf + 0x20000000):
 *   mask = buf->unk6C;  id = *(s16 *)(0x0609EBD0 + buf->unk70*2);  tag = buf->unk78
 *   rec  = 0x0609D7D0 + (s16)(id*0x14)           (exts.w after the *20)
 *   - func_022137a0((u8)rec[0], rec->unk4, buf)   per-type preamble (r6 = buf).
 *   - live quad counter: g_06098818 = *(0x26098818) + (u8)rec[1]  (mirror read).
 *   - walk the (u8)rec[1] sub-quad descriptors at rec->unk8 (stride 0x10): for each
 *     func_02213d24(desc, cursor, 0x060DFBC8); if func_02213e0c((s8)desc[7] | mask,
 *     (s16)desc->unk4, cursor) != 0, append: cursor+0x2C (s16) = tag, cursor+0x28 =
 *     desc, cursor += 0x30. Cursor starts at 0x060DC648.
 *   - terminate with -1 at cursor+0x20.
 *   - tail jmp g_06090778[(s8)rec[16] | mask]; r4-r6 are dead at that point
 *     (clobbered by the callees), so it is modelled as a no-arg call.
 * fidelity: HIGH - hand-traced vs capstone 0x215bf4..0x215cb2 (jmp tail @0x215cb0);
 *   bsrf targets resolved: 0x2137a0, 0x213d24, 0x213e0c. The word stored at @r15
 *   before the first call is a dead spill. */
#include "dh_band21.h"

extern void vertex_project_buffered_source_entry(s32 type, s32 param, u32 buf);   /* 0x022137a0 (no C yet) */
extern void quad_build_projected_corners(u32 desc, u32 cursor, u32 ctx);
extern s32  quad_test_visible(s32 flag, s32 sel, u32 quad);

typedef void (*draw_handler)(void);

void model_build_pending_quad_list(void)
{
    u32 buf  = MEMU32(0x060da0c0);
    s32 mask = MEM32(buf + 0x6c + 0x20000000u);
    s32 sel  = MEM32(buf + 0x70 + 0x20000000u);
    s32 tag  = MEM32(buf + 0x78 + 0x20000000u);
    u32 rec  = 0x0609d7d0 + (s16)(MEM16(0x0609ebd0 + sel * 2) * DH_MODEL_DEFINITION_STRIDE);
    u32 count;
    u32 desc;
    u32 cur;
    u32 i;
    draw_handler *tbl;

    vertex_project_buffered_source_entry((u8)MODEL_MEM8(rec, vertex_count), MODEL_MEM32(rec, vertices), MEMU32(0x060da0c0));

    count = (u8)MODEL_MEM8(rec, quad_count);
    MEM32(0x06098818) = MEM32(0x26098818) + (s32)count;

    cur  = 0x060dc648;
    desc = MODEL_MEMU32(rec, quads);
    for (i = 0; i < count; i++) {
        quad_build_projected_corners(desc, cur, 0x060dfbc8);
        if (quad_test_visible((s8)MEM8(desc + 7) | mask, MEM16(desc + 4), cur) != 0) {
            MEM16(cur + 0x2c) = (s16)tag;
            MEM32(cur + 0x28) = (s32)desc;
            cur += 0x30;
        }
        desc += 0x10;
    }

    MEM32(cur + 0x20) = -1;

    tbl = (draw_handler *)(uintptr_t)0x06090778;
    tbl[(s8)MODEL_MEM8(rec, draw_handler_index) | mask]();
}
