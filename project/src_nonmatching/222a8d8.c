/* 0x222a8d8  reads two pad records @0x06098674/5, sets status bits in 0x06098564,
   returns count. Clean leaf (no calls). NONMATCHING: member of a misseg unit
   (0x222a872) whose functions share one trailing literal pool, so it cannot be
   isolated for a byte-exact unit grind; code is ~90% insn-identical, equivalent. */
typedef signed char s8; typedef unsigned char u8;
typedef signed short s16; typedef unsigned short u16;
typedef signed int s32; typedef unsigned int u32;
s32 f_22a8d8(void) {
    s32 var_r5;

    *(s32 *)0x06098564 = 0;
    var_r5 = 0;
    if (*(s32 *)0x06098670 == 0) {
        *(s32 *)0x06098564 |= 3;
        return 2;
    }
    if ((u8) *(u8 *)0x06098674 == 2) {
        var_r5 = 1;
        *(s32 *)0x06098564 |= 1;
    }
    if ((u8) *(u8 *)0x06098675 == 2) {
        var_r5 += 1;
        *(s32 *)0x06098564 |= 2;
    }
    return var_r5;
}
