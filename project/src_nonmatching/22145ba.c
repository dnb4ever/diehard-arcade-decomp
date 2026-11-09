/* 0x022145ba  SH-2 on-chip DMAC ch0 copy (CHCR tag 0x5A11). If src/dst not 4-aligned,
   report error (codes 10,11 to 0x060997A4/A8) via fp *0x060902A0; spin-wait DMAOR/CHCR,
   then program SAR0/DAR0/TCR0/CHCR0. Returns 1 if both 4-aligned else 0.
   NONMATCHING BLOCKER: fn.py gives this unit a 22-byte foreign-pool DATA prefix at the
   unit start (0x22145ba..0x22145d0, belongs to the previous function); real code starts
   at the register-save prologue. That prefix cannot be emitted from C, so this member-0
   (and thus the whole unit's shared-pool alignment) cannot byte-match. Logic reconstructed. */
typedef int s32;
typedef int (*dmac_err_fp)(s32, s32, s32, s32, s32, s32, s32);
s32 f_2145ba(s32 src, s32 dst, s32 count) {
    s32 timeout = 0;
    if ((src & 3) || (dst & 3)) {
        *(s32 *)0x060997A4 = 10;
        *(s32 *)0x060997A8 = 11;
        (*(dmac_err_fp *)0x060902A0)(3, dst, count, -0x2CCE, 0x06053D84, src, dst);
    }
    while ((*(s32 *)0xFFFFFFB0 & 4) == 0 && (*(s32 *)0xFFFFFF8C & 2) == 0) {
        if (timeout >= 0x10000) break;
        timeout++;
    }
    *(s32 *)0xFFFFFF8C = 0;          /* CHCR0 */
    *(s32 *)0xFFFFFFB0 = 1;          /* DMAOR */
    *(s32 *)0xFFFFFF80 = src & ~3;   /* SAR0 */
    *(s32 *)0xFFFFFF84 = dst & ~3;   /* DAR0 */
    *(s32 *)0xFFFFFF88 = count;      /* TCR0 */
    *(s32 *)0xFFFFFF8C = 0;          /* CHCR0 clear */
    *(s32 *)0xFFFFFF8C = 0x5A11;     /* CHCR0 enable */
    if ((src & 3) || (dst & 3)) return 0;
    return 1;
}
