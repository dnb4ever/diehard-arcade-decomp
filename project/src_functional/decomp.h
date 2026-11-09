/* decomp.h - shared decls for Die Hard Arcade (Sega ST-V / SH-2) functional decomp.
 * Functional (NOT byte-exact) C. Types mirror m2c output; HW/global access via
 * volatile pointers at absolute addresses (compiles on host gcc).
 */
#ifndef DIEHARD_DECOMP_H
#define DIEHARD_DECOMP_H
#include <stdint.h>
#include "dh_names.h"

typedef signed char        s8;
typedef unsigned char      u8;
typedef signed short       s16;
typedef unsigned short     u16;
typedef signed int         s32;
typedef unsigned int       u32;
typedef signed long long   s64;
typedef unsigned long long u64;
typedef float              f32;
typedef double             f64;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Absolute-address accessors. SH-2 is big-endian; on host this is a functional
 * model only, so endianness of the host is irrelevant to control-flow fidelity. */
#define MEM8(a)   (*(volatile u8  *)(uintptr_t)(a))
#define MEM16(a)  (*(volatile u16 *)(uintptr_t)(a))
#define MEM32(a)  (*(volatile u32 *)(uintptr_t)(a))
#define MEMS8(a)  (*(volatile s8  *)(uintptr_t)(a))
#define MEMS16(a) (*(volatile s16 *)(uintptr_t)(a))
#define MEMS32(a) (*(volatile s32 *)(uintptr_t)(a))

/* typed pointer at an absolute address */
#define PTR8(a)   ((volatile u8  *)(uintptr_t)(a))
#define PTR16(a)  ((volatile u16 *)(uintptr_t)(a))
#define PTR32(a)  ((volatile u32 *)(uintptr_t)(a))

/* Struct-field access by byte offset on an opaque base pointer. The SH-2 loads
 * carry the width (mov.b/mov.w/mov.l), so these preserve the exact size+offset
 * of the original access while compiling as plain C. */
#define FS8(base,off)  (*(s8  *)((char *)(base) + (off)))
#define FU8(base,off)  (*(u8  *)((char *)(base) + (off)))
#define FS16(base,off) (*(s16 *)((char *)(base) + (off)))
#define FU16(base,off) (*(u16 *)((char *)(base) + (off)))
#define FS32(base,off) (*(s32 *)((char *)(base) + (off)))
#define FU32(base,off) (*(u32 *)((char *)(base) + (off)))
#define FPTR(base,off) (*(void **)((char *)(base) + (off)))

#endif /* DIEHARD_DECOMP_H */
