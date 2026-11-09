/* 0x02214678  DMAC ch0 copy with a leading 16-byte-alignment long-copy (Duff ladder on
   (src & 0xF)>>2), then the same DMAC setup (CHCR tag 0x5E11). Returns 1 if aligned else 0.
   NONMATCHING: functionally correct, but shares the unit whose member-0 (0x22145ba) carries
   a foreign-pool prefix, so the unit's shared pool never aligns; also fp-call + DMAC codegen
   exhibit the SHC r32 register-alloc version trait. */
typedef int s32;
typedef int (*dmac_err_fp)(s32, s32, s32, s32, s32, s32);
s32 f_214678(s32 dst, s32 *src, s32 count) {
    s32 rem = dst & 0xF;
    s32 timeout = 0;
    s32 tcr = count * 4;
    if (rem != 0) {
        s32 *d = src;
        s32 *s = (s32 *)dst;
        switch ((unsigned)rem >> 2) {
        case 0: *d++ = *s++; tcr--;   /* fallthrough */
        case 1: *d++ = *s++; tcr--;   /* fallthrough */
        case 2: *d++ = *s++; tcr--;   /* fallthrough */
        case 3: tcr--; *d = *s; break;
        }
        dst = (dst - rem) + 0x10;
        src = (s32 *)(((s32)src - rem) + 0x10);
    }
    if ((dst & 3) || ((s32)src & 3)) {
        *(s32 *)0x060997A4 = 10;
        *(s32 *)0x060997A8 = 12;
        (*(dmac_err_fp *)0x060902A0)(3, rem, (s32)src, 0x06053D98, dst, (s32)src);
    }
    while ((*(s32 *)0xFFFFFFB0 & 4) == 0 && (*(s32 *)0xFFFFFF8C & 2) == 0) {
        if (timeout >= 0x10000) break;
        timeout++;
    }
    *(s32 *)0xFFFFFF8C = 0;
    *(s32 *)0xFFFFFFB0 = 1;
    *(s32 *)0xFFFFFF80 = dst & ~3;
    *(s32 *)0xFFFFFF84 = (s32)src & ~3;
    *(s32 *)0xFFFFFF88 = tcr;
    *(s32 *)0xFFFFFF8C = 0;
    *(s32 *)0xFFFFFF8C = 0x5E11;
    if ((dst & 3) || ((s32)src & 3)) return 0;
    return 1;
}
