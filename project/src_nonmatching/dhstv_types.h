/* Shared context for Die Hard Arcade (ST-V/SH-2) non-matching decomps
   (ranks 7-12). These TUs are functionally-correct but not byte-exact
   under SHC v5.0r32 (the exact 1996 SHC build is unobtainable). */
#ifndef DHSTV_TYPES_H
#define DHSTV_TYPES_H
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;

/* unit 2216714: tilemap blit source descriptor (arg2) */
typedef struct T {
    s8 unk0;      /* +0  width  */
    s8 unk1;      /* +1  height */
    s16 unk2;     /* +2  value bias */
    s16 *unk4;    /* +4  source tile data */
} T;

/* unit 2245eb8 */
typedef struct E {
    s8 pad0[0x4A];
    s8 unk4A;
    s8 pad1[0x84 - 0x4B];
    s32 unk84;
} E;
typedef struct P { s8 pad0[8]; E *unk8; } P;
typedef struct A {
    s8 pad0[0x34];
    s8 unk34;
    s8 pad1[0x8C - 0x35];
    s32 unk8C;
    s8 pad2[0x114 - 0x90];
    s32 unk114;
} A;
#endif
