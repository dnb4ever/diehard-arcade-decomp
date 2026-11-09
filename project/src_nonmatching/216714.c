/* 0x2216714  [rank 7-12 unit]  f_216714: tilemap blit. STRUCTURALLY IDENTICAL to ROM; only register allocation differs (src=r11 vs r14, r11/r12 gap). 66% bytes.
   Context: project/src_nonmatching/dhstv_types.h */
#include "dhstv_types.h"

void func_216714(s32 arg0, s32 arg1, void *arg2, s32 arg3, s32 arg4) {
    T *t = (T *)arg2;
    int y, x;
    s16 *row;
    s16 *dst;
    s16 *src;
    src = t->unk4;
    dst = (s16 *)(arg1 + (arg4 << 7) + (arg3 * 2));
    y = 0;
    while (y < t->unk1) {
        row = dst;
        x = 0;
        while (x < t->unk0) {
            x++;
            *dst = *src + arg0;
            src++;
            dst++;
        }
        dst = (s16 *)((char *)row + 0x80);
        y++;
    }
}
