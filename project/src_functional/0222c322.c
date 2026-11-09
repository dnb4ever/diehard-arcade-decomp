/* actor_fall_setup [0x0222c322]: Existing map corroborated by source: init fall physics; dispatch via fp table g_060911BC[g_0609979A](actor,0xE) */
#include "dh.h"
/* 0x0222c322 actor_fall_setup - initialise an actor's fall/landing physics and kick
 * off its state handler via the global state-dispatch table.
 * arg0 = actor.
 *   arg0->unkC4 = g_06096C58[arg0->unk34] + arg0->unkC - 0xC000;   (landing height)
 *   arg0->unkC8 = (0x30000 - arg0->unkC4) >> 4;                    (fall step)
 *   arg0->unk78 = 0; arg0->unk80 = 0; arg0->unk84 = 0x01000000;
 *   arg0->unk5C = g_06087EB0[arg0->unk45];                         (anim id)
 *   arg0->unk41 = 1; arg0->unk43 = 0x2A; arg0->unkB0 = 0;
 *   g_060911BC[g_0609979A](arg0, 0xE);        (dispatch through runtime fp table)
 *   arg0->unkCC = 1;
 * fidelity: HIGH - final call is an INDIRECT jump through fp table g_060911BC indexed
 * by g_0609979A (jsr @r3, r3 = *(0x060911BC + g_0609979A*4)). unkC8 uses arithmetic
 * >>4 of (0x30000 - unkC4). g_06096C58 s32[], g_06087EB0 s16[] lookup tables; field
 * widths per asm. Matches m2c --asm. */
extern void (*g_actor_pose_handlers[])(void *actor, s32 arg);   /* state-dispatch fp table */
extern s32  g_06096C58[];
extern s16  g_06087EB0[];
extern s16  g_actor_pose_set;

void actor_fall_setup(void *arg0) {
    M32(arg0, 0xC4) = g_06096C58[M8(arg0, 0x34)] + M32(arg0, 0xC) + (s32)0xFFFF4000;
    M32(arg0, 0xC8) = (0x30000 - M32(arg0, 0xC4)) >> 4;
    M32(arg0, 0x78) = 0;
    M32(arg0, 0x80) = 0;
    M32(arg0, 0x84) = 0x01000000;
    M16(arg0, 0x5C) = g_06087EB0[M8(arg0, 0x45)];
    M8(arg0, 0x41) = 1;
    M8(arg0, 0x43) = 0x2A;
    M32(arg0, 0xB0) = 0;
    g_actor_pose_handlers[g_actor_pose_set](arg0, 0xE);
    M16(arg0, 0xCC) = 1;
}
