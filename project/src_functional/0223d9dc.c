/* projectile_tick_grenade_fuse [0x0223d9dc]: Integrates XYZ and detonates on target latch or sixty-frame fuse. */
#include "dh.h"
/* 0x0223d9dc - per-frame update for a projectile/grenade entity (child object
 * obj = entity->unk8). Dispatches on entity->unk10.
 * case 0: entity->unk10++.
 * case 1: save prev position (unk4C/50/54 = unk8/C/10), integrate velocity
 *   (unk8/C/10 += unk14/18/1C). Detonate when unk5D != 0 or the fuse timer
 *   ++unk58 reaches 60: if unk5D < 7 spawn a debris object via
 *   func_0220ff54(entity, (void*)0x0603d9bc) and seed its unk8/C/10 from
 *   g_06097d58[(unk5D<<8) + (u8)unk5E*16] plus unk26/30/34/38 copies; then
 *   entity->unk0 = 0 and return. Otherwise keep flying: if unkC >= 0x1000 call
 *   func_0222eea2(obj); else optionally sound_queue_push(0xA01233FF) when g_060e0ddc
 *   is set, and advance entity->unk10.
 * case 2: if ++unk5A > 30 -> entity->unk0 = 0, return.
 * Common tail (non-return paths): push draw offset (g_06098878 += 48), run
 *   mtx_load_abc/0214a40/0214a06/0215a2c, pop.
 * branches resolved: bsrf 0x20ff54,0x215f6c,0x22eea2,0x2149aa,0x214a40,
 *   0x214a06,0x215a2c. fidelity: HIGH (mtx_load_abc arg-less from this caller). */
extern u8   g_06097d58[];
extern s32  g_matrix_stack_cursor;
extern s32  g_sound_effects_enabled;
extern void *task_spawn_current_work40(void *a, void *b);
extern void sound_queue_push(u32 a);
extern void collision_queue_actor(void *obj);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *obj);
extern void mtx_rotate_zyx_at24(void *obj);
extern void model_submit_culled(void *obj, s32 b);

void projectile_tick_grenade_fuse(void *entity) {
    void *obj = MP(entity, 8);
    void *ret;
    void *p;
    void *tbl;

    switch (M8(entity, 0x10)) {
    case 0:
        M8(entity, 0x10)++;
        break;
    case 1:
        M32(obj, 0x4C) = M32(obj, 8);
        M32(obj, 0x50) = M32(obj, 0xC);
        M32(obj, 0x54) = M32(obj, 0x10);
        M32(obj, 8)    += M32(obj, 0x14);
        M32(obj, 0xC)  += M32(obj, 0x18);
        M32(obj, 0x10) += M32(obj, 0x1C);

        if (M8(obj, 0x5D) != 0 || ++M16(obj, 0x58) >= 60) {
            if (M8(obj, 0x5D) < 7) {
                ret = task_spawn_current_work40(entity, (void *)0x0603d9bc);
                p = MP(ret, 8);
                tbl = &g_06097d58[(M8(obj, 0x5D) << 8) + MU8(obj, 0x5E) * 16];
                M32(p, 8)  = M32(tbl, 0);
                M32(p, 0xC) = M32(tbl, 4);
                M32(p, 0x10) = M32(tbl, 8);
                M16(p, 0x26) = M16(obj, 0x26);
                M32(p, 0x38) = MU8(obj, 0x5E);
                M8(p, 0x34) = M8(obj, 0x5D);
                M16(p, 0x30) = M16(obj, 0x30);
            }
            M32(entity, 0) = 0;
            return;
        }

        if (M32(obj, 0xC) >= 0x1000) {
            collision_queue_actor(obj);
        } else {
            if (g_sound_effects_enabled != 0)
                sound_queue_push(0xA01233FFu);
            M8(entity, 0x10)++;
        }
        break;
    case 2:
        if (++M16(obj, 0x5A) > 30) {
            M32(entity, 0) = 0;
            return;
        }
        break;
    default:
        break;
    }

    g_matrix_stack_cursor += 48;
    mtx_load_abc();
    mtx_apply_vec_at8(obj);
    mtx_rotate_zyx_at24(obj);
    model_submit_culled(obj, 0);
    g_matrix_stack_cursor -= 48;
}
