/* dh.h - shared decls for Die Hard Arcade (ST-V / SH-2) functional decomp.
 *
 * FUNCTIONAL (not byte-exact) decomp support header. Provides the SH-2 integer
 * typedefs and typed byte-offset memory accessors used to model the game's
 * struct fields faithfully without reconstructing full struct layouts.
 *
 * M<width>(p,off)  = *(signed   T *)((u8*)p + off)   read/write at byte offset
 * MU<width>(p,off) = *(unsigned T *)((u8*)p + off)
 * MP(p,off)        = *(void **)((u8*)p + off)        pointer-sized slot
 *
 * Globals (HWRAM 0x06xxxxxx) and hardware registers (0x25xxxxxx / 0xffffffxx)
 * are declared in each translation unit as extern / volatile symbols so the
 * code stays portable for the eventual decomp-based port.
 */
#ifndef DH_H
#define DH_H
#include <stdint.h>
#include "dh_names.h"

typedef signed char        s8;
typedef unsigned char      u8;
typedef short              s16;
typedef unsigned short     u16;
typedef int                s32;
typedef unsigned int       u32;
typedef long long          s64;
typedef unsigned long long u64;

#define M8(p, o)   (*(s8  *)((u8 *)(uintptr_t)(p) + (o)))
#define MU8(p, o)  (*(u8  *)((u8 *)(uintptr_t)(p) + (o)))
#define M16(p, o)  (*(s16 *)((u8 *)(uintptr_t)(p) + (o)))
#define MU16(p, o) (*(u16 *)((u8 *)(uintptr_t)(p) + (o)))
#define M32(p, o)  (*(s32 *)((u8 *)(uintptr_t)(p) + (o)))
#define MU32(p, o) (*(u32 *)((u8 *)(uintptr_t)(p) + (o)))
#define MP(p, o)   (*(void **)((u8 *)(uintptr_t)(p) + (o)))

#endif /* DH_H */
