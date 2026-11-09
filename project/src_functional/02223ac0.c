#include "dh_work.h"
/* model_load_scene_records [0x02223ac0]: Copies listed model vertex/quad records into the two runtime arenas and queues their upload. */
#include "dh.h"
/* 0x02223ac0 (func_02223ac0) - scene/model table loader + VDP upload handshake.
 * Clears the id->record-index map g_0609EBD0 (s16[], 0..0x0C80 = -1), then walks a
 * caller-supplied s16 id list (terminated by -1). For each id it reads a 20-byte
 * source record from ROM table g_22D00000[id*20], copies src->unk0 sixteen-byte
 * blocks from src->unk4 into arena A (g_0609F850) and src->unk1 blocks from
 * src->unk8 into arena B (g_060AC050), and writes a 20-byte runtime record into
 * g_0609D7D0[index*20] holding the two counts (unk0/unk1), two flag bytes
 * (unk10/unk11), the arena-A/arena-B start pointers for this record (unk4/unk8)
 * and unkC=0. g_0609EBD0[id] is set to the record index. A zero terminator byte is
 * written after the last record. It then waits on the VDP/transfer-done flag (bit
 * 0x80 at the uncached mirror 0x260E0DD8), clears it, pushes command 1 to the queue
 * at g_060E0DD4, and waits once more (with a counter guard) before clearing again.
 *
 * ABI: r4 = id list. void return. The id list is consumed with a do/while, so the
 * body runs once even for an empty (leading -1) list, matching the asm.
 * fidelity: HIGH on structure/offsets. The 0x20000000 bit selects the cache-through
 * mirror of the flag RAM (g_260E0DD8 == g_060E0DD8); the second wait's guard counter
 * (sp0) deliberately inherits the last block pointer left by copy-loop B, exactly as
 * the stack slot does in the asm, so that guard expires almost immediately. */

extern u8  g_0609EBD0[];       /* id -> record-index map (s16 entries) */
extern u8  g_0609D7D0[];       /* runtime record table, 20-byte stride */
extern u8  g_0609F850[];       /* block arena A */
extern u8  g_060AC050[];       /* block arena B */
extern u8  g_22D00000[];       /* ROM source record table, 20-byte stride */
extern s32 *g_060E0DD4;        /* command-queue tail pointer */
extern u8  g_060E0DD8;         /* VDP handshake flag (cached write) */
extern volatile u8 g_260E0DD8; /* same flag, uncached mirror (polled) */

void model_load_scene_records(s16 *idlist)
{
    s32 off;
    s32 index = 0;
    s16 id;
    u8 *curA = g_0609F850;
    u8 *curB = g_060AC050;
    u8 *recA = g_0609F850;
    u8 *recB = g_060AC050;
    s32 sp0 = 0;

    for (off = 0; off <= 0x0c80; off += 2)
        M16(g_0609EBD0, off) = -1;

    id = *idlist++;
    do {
        void *rec = g_0609D7D0 + index * DH_MODEL_DEFINITION_STRIDE;
        void *src = g_22D00000 + id * DH_MODEL_DEFINITION_STRIDE;
        void *bp;
        s32 n;

        M16(g_0609EBD0, id * 2) = (s16)index;

        bp = MODEL_MP(src, vertices);
        for (n = MODEL_MU8(src, vertex_count); n != 0; n--) {
            M32(curA, 0)  = M32(bp, 0);
            M32(curA, 4)  = M32(bp, 4);
            M32(curA, 8)  = M32(bp, 8);
            M32(curA, 12) = M32(bp, 12);
            curA += 16;
            bp = (u8 *)bp + 16;
        }

        sp0 = (s32)(uintptr_t)MODEL_MP(src, quads);
        for (n = MODEL_MU8(src, quad_count); n != 0; n--) {
            void *s = (void *)(uintptr_t)(u32)sp0;
            M32(curB, 0)  = M32(s, 0);
            M32(curB, 4)  = M32(s, 4);
            M32(curB, 8)  = M32(s, 8);
            M32(curB, 12) = M32(s, 12);
            curB += 16;
            sp0 += 16;
        }

        MODEL_M8(rec, vertex_count) = (s8)MODEL_MU8(src, vertex_count);
        MODEL_M8(rec, quad_count) = (s8)MODEL_MU8(src, quad_count);
        MODEL_M8(rec, draw_handler_index) = (s8)MODEL_MU8(src, draw_handler_index);
        M8(rec, 0x11) = (s8)MU8(src, 0x11);
        MODEL_MP(rec, vertices) = recA;
        MODEL_MP(rec, quads) = recB;
        M32(rec, 0x0C) = 0;
        recA = curA;
        recB = curB;

        index++;
        id = *idlist++;
    } while (id != -1);

    M8(g_0609D7D0 + index * DH_MODEL_DEFINITION_STRIDE, 0) = 0;

    while ((g_260E0DD8 & 0x80) != 0x80)
        ;
    g_060E0DD8 = 0;

    {
        s32 *tail = g_060E0DD4;
        g_060E0DD4 = tail + 1;
        *tail = 1;
    }

    for (;;) {
        if ((g_260E0DD8 & 0x80) == 0x80)
            break;
        sp0++;
        if (sp0 > 0x0002bf20)
            break;
    }
    g_060E0DD8 = 0;
}
/* fidelity: HIGH - clear/build/copy/handshake verified vs --asm; guard counter inherits leftover block ptr as in the asm; uncached-mirror flag modelled as volatile g_260E0DD8. */
