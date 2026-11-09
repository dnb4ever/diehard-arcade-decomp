#include "dh_work.h"
/* model_emit_mesh_part [0x02235dc0]: Transforms one mesh part, optionally emits its selected vertex marker and appends visible quads. */
#include "dh.h"
/* 0x02235dc0 - per-block mesh-part emitter hook, called after each
 * matrix_copy_reverse by the block builders 0x02234b58 / 0x02235198 / 0x0223579c
 * (vdp_colour_build_*). obj = the actor/part object.
 *   part = g_0609EBD0[(s16)obj->unk30]  (s16; < 0 -> nothing to draw)
 *   rec  = 0x0609D7D0 + (s16)(part * 20)   mesh record:
 *          +0 u8 nverts, +1 u8 nquads, +4 vertex src, +8 quad array (16 B each),
 *          +17 s8 (loaded into r4 before the tail call; unused by the callee)
 *   nverts == 0 -> return.
 *   func_022135a0(nverts, rec->vsrc)  transforms the vertices into 0x060DEBC8.
 *   If obj->unk98 > 0 and the id halfword g_06096248[(s8)unk34*32 + (unk98-1)*2]
 *   equals obj->unk30: gfx_submit_box8(v.x, -v.y, v.z) for vertex v =
 *   0x060DEBC8 + obj->unk9C*16. (The ROM first spins an empty counting loop
 *   to unk9C; it has no effect and is not modelled.)
 *   g_06098818 += nquads (always stored).
 *   For each quad q: build its corner record at the list cursor (0x060DA0C8,
 *   48-byte entries) with func_02213d24; if the clip test func_02213e0c(4 | q[7],
 *   q.s16[4/2], cursor) passes, fill +44 = (s16)obj->unk20, +46 = 0,
 *   +40 = q and advance the cursor.
 *   Terminate the list (cursor+32 = -1) and tail-call build_displist_b (0x022158dc).
 * fidelity: HIGH - hand-checked vs asm 0x235dc0..0x235edc (bsrf -> 0x2135a0,
 *   0x234aae, 0x213d24, 0x213e0c; braf tail -> 0x2158dc). The +46 halfword is a
 *   stack local that is only ever 0. */
extern s32  g_06098818;                                     /* emitted-quad counter */
extern void vertex_transform_project_raw_entry(s32 count, u32 verts);            /* 0x022135a0 vertex transform */
extern void gfx_submit_box8(s32 x, s32 y, s32 attr);          /* 0x02234aae gfx_submit_box8 */
extern void quad_build_projected_corners(u32 idx, u32 out, u32 base);      /* 0x02213d24 quad corner record */
extern s32  quad_test_visible(s32 flag, s32 sel, void *quad);   /* 0x02213e0c quad clip test */
extern void build_displist_b(void);                         /* 0x022158dc */

#define VTX_BUF  0x060DEBC8u   /* transformed vertices, 16-byte stride */
#define QUAD_BUF 0x060DA0C8u   /* emitted quad list, 48-byte entries */

void model_emit_mesh_part(u8 *obj)
{
    s16 part = M16(0x0609EBD0u, M16(obj, 0x30) * 2);
    u32 rec, q, out, v;
    s32 nquads, i;

    if (part < 0)
        return;
    rec = 0x0609D7D0u + (s16)(part * DH_MODEL_DEFINITION_STRIDE);
    if (MODEL_MU8(rec, vertex_count) == 0)
        return;
    vertex_transform_project_raw_entry(MODEL_MU8(rec, vertex_count), MODEL_MU32(rec, vertices));

    if (M32(obj, 0x98) > 0 &&
        M16(0x06096248u + (M8(obj, 0x34) << 5), (M32(obj, 0x98) - 1) * 2) == M16(obj, 0x30)) {
        v = VTX_BUF + M32(obj, 0x9C) * 16;
        gfx_submit_box8(M32(v, 0), -M32(v, 4), M32(v, 8));
    }

    nquads = MODEL_MU8(rec, quad_count);
    q = MODEL_MU32(rec, quads);
    g_06098818 = g_06098818 + nquads;
    out = QUAD_BUF;
    for (i = 0; i < nquads; i++, q += 16) {
        quad_build_projected_corners(q, out, VTX_BUF);
        if (quad_test_visible(4 | M8(q, 7), M16(q, 4), (void *)(uintptr_t)out) != 0) {
            M16(out, 44) = M16(obj, 0x20);
            M16(out, 46) = 0;
            M32(out, 40) = (s32)q;
            out += 48;
        }
    }
    M32(out, 32) = -1;
    build_displist_b();
}
