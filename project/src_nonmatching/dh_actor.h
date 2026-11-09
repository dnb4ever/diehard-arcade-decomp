/* dh_actor.h - shared types for Die Hard Arcade (ST-V) nonmatching decomp
 * (units ranks 19-24).  Actor is the main per-entity struct; field names are
 * byte offsets (fXX = +0xXX) inferred from SH-2 @(disp,Rn) access widths.
 * FB = heap free-block node, Pool = allocator owner, E = entity-table entry,
 * G = 8-byte slot-table record.
 */
#ifndef DH_ACTOR_H
#define DH_ACTOR_H
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

typedef struct Actor {
    char _p0[0x8];
    s32 f08;  /* 0x8 */
    s32 f0c;  /* 0xc */
    s32 f10;  /* 0x10 */
    char _p1[0x12];
    s16 f26;  /* 0x26 */
    char _p2[0xc];
    s8 f34;  /* 0x34 */
    char _p3[0x3];
    s32 f38;  /* 0x38 */
    char _p4[0x5];
    s8 f41;  /* 0x41 */
    char _p5[0x2];
    s8 f44;  /* 0x44 */
    s8 f45;  /* 0x45 */
    char _p6[0x2];
    s8 f48;  /* 0x48 */
    u8 f49;  /* 0x49 */
    s8 f4a;  /* 0x4a */
    char _p7[0x1];
    s8 f4c;  /* 0x4c */
    s8 f4d;  /* 0x4d */
    char _p8[0x4];
    s16 f52;  /* 0x52 */
    u16 f54;  /* 0x54 */
    char _p9[0x2];
    s16 f58;  /* 0x58 */
    char _p10[0x2];
    s16 f5c;  /* 0x5c */
    char _p11[0x2];
    s16 f60;  /* 0x60 */
    s16 f62;  /* 0x62 */
    s16 f64;  /* 0x64 */
    char _p12[0xe];
    s32 f74;  /* 0x74 */
    s32 f78;  /* 0x78 */
    s32 f7c;  /* 0x7c */
    s32 f80;  /* 0x80 */
    s32 f84;  /* 0x84 */
    s32 f88;  /* 0x88 */
    char _p13[0x2e];
    s16 fba;  /* 0xba */
    s32 fbc;  /* 0xbc */
    char _p14[0x4];
    s32 fc4;  /* 0xc4 */
    s32 fc8;  /* 0xc8 */
    char _p15[0x8];
    s32 fd4;  /* 0xd4 */
    char _p16[0x8];
    s32 fe0;  /* 0xe0 */
    char _p17[0x24];
    s32 f108;  /* 0x108 */
    s32 f10c;  /* 0x10c */
    s32 f110;  /* 0x110 */
    char _p18[0x4];
    void* f118;  /* 0x118 */
    s16 f11c;  /* 0x11c */
    s16 f11e;  /* 0x11e */
    char _p19[0x18];
    s32 f138;  /* 0x138 */
    char _p20[0x4];
    s32 f140;  /* 0x140 */
    char _p21[0x4];
    s16 f148;  /* 0x148 */
    char _p22[0xe];
    void* f158;  /* 0x158 */
    char _p23[0x24];
    void* f180;  /* 0x180 */
} Actor;

typedef struct FB { struct FB *f0; s32 f4; } FB;
typedef struct Pool { char _p[8]; void *f8; } Pool;
typedef struct E { char _p0[8]; struct E *f8; char _p1[4]; s32 f10; } E;
typedef struct G { s32 f0; s32 f4; } G;
#endif
