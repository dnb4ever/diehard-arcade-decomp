/* dh_band21.h - shared context for Die Hard Arcade (ST-V / SH-2 BE)
 * FUNCTIONAL (equivalence-verified) decomp of cart band 0x210000-0x220000
 * (SH-2 0x02210000-0x02220000). These TUs are control-flow / operation
 * faithful to the SH-2 disassembly and compile on host gcc; they are NOT
 * byte-exact (the 1996 SHC build is unobtainable - PDS-style functional decomp).
 */
#ifndef DH_BAND21_H
#define DH_BAND21_H
#include <stdint.h>
#include "dh_names.h"

typedef signed char   s8;
typedef unsigned char u8;
typedef signed short   s16;
typedef unsigned short u16;
typedef signed int   s32;
typedef unsigned int u32;
typedef float f32;

/* Memory-mapped / absolute-address accessors (SH-2 flat address space).
 * On host gcc the literal is widened through uintptr_t so the cast is clean. */
#define MEM8(a)   (*(volatile u8  *)(uintptr_t)(a))
#define MEM16(a)  (*(volatile s16 *)(uintptr_t)(a))
#define MEMU16(a) (*(volatile u16 *)(uintptr_t)(a))
#define MEM32(a)  (*(volatile s32 *)(uintptr_t)(a))
#define MEMU32(a) (*(volatile u32 *)(uintptr_t)(a))
#define PTR(t,a)  ((t *)(uintptr_t)(a))

/* SH-2 on-chip DIVU (64/32 and 32/32 hardware divider) @0xFFFFFF00 */
#define DIVU_DVSR    0xFFFFFF00u  /* divisor                              */
#define DIVU_DVDNT   0xFFFFFF04u  /* 32-bit dividend (write starts 32/32) */
#define DIVU_DVCR    0xFFFFFF08u
#define DIVU_VCRDIV  0xFFFFFF0Cu
#define DIVU_DVDNTH  0xFFFFFF10u  /* 64-bit dividend high                 */
#define DIVU_DVDNTL  0xFFFFFF14u  /* 64-bit dividend low (write starts 64/32) */
#define DIVU_DVDNTUH 0xFFFFFF18u  /* quotient/remainder readback          */
#define DIVU_DVDNTUL 0xFFFFFF1Cu  /* quotient readback                    */

/* Trig table used by sin_lookup / cos_lookup (HWRAM) */
#define TRIG_TABLE   0x06075718u

#endif /* DH_BAND21_H */
