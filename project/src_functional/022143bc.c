/* 0x022143bc  lz_stream_step: resumable, queue-driven LZSS decompressor. Advances
 * one token (one literal, or the first byte of a match) OR one match-copy byte per
 * call, so decompression can be spread across frames. State struct S @g_06091e14:
 *   S[0]=enabled, S[1]=mode (0 idle, 1 token-decode, 2 match-copy),
 *   S[2]=queue read idx, S[3]=queue write idx (ring, &127),
 *   *(u32*)(S+4)=bit buffer, *(s32*)(S+8)=bits left, *(s32*)(S+12)=match len left,
 *   *(u8**)(S+16)=copy src, *(u8**)(S+20)=input ptr, *(u8**)(S+24)=output ptr,
 *   S+28 = ring of 8-byte {src,dst} jobs indexed by S[2].
 * Bit stream is LSB-first (shlr r6 / refill byte when count hits 0, primed with 9).
 * Token: control bit 1 => literal byte copied through; bit 0 => match. Match: next
 *   bit 0 => short (2 length bits +1, offset = inbyte|0xffffff00 in [-256,-1]); bit 1
 *   => long (16-bit word b0|b1<<8; word 0 ends the stream -> dequeue next job; offset
 *   = (word>>3)|0xffffe000; len = (b0&7)?(+1):(extra byte)). First match byte copied
 *   now, S[12]=len, mode=2; later calls copy one byte each until len hits 0 (mode->1).
 * Return code mirrors r0 by path: 0 idle/mode-1 literal, 1 dequeue/match-copy, 2 match.
 * fidelity: HIGH - faithful to the dt/shlr bit reader, rotcl length bits, sign-extend
 *   offsets, pre-dec/post-inc state writes, shared rts(=return 0) @0x2143b8. */
#include "dh_band21.h"

static int lz_getbit(const u8 **in, u32 *buf, s32 *cnt) {
    int bit;
    if (--(*cnt) == 0) {            /* dt r7 -> refill when exhausted */
        *buf = (u32)(s32)(s8)*(*in)++;
        *cnt = 8;
    }
    bit = (int)(*buf & 1);         /* shlr r6 -> T = old bit0 */
    *buf >>= 1;
    return bit;
}

int lz_stream_step(void) {
    u8 *S = (u8 *)(uintptr_t)0x06091e14;
    const u8 *in = 0;
    u8 *out = 0;
    const u8 *cpy = 0;
    u32 buf = 0;
    s32 cnt = 0, len = 0;
    int rc = 0, b0, b1;
    u32 word;
    s32 off;
    u8 rd, wr;

    if (S[0] == 0) return 0;                 /* disabled -> return 0 */

    if (S[1] == 0) goto dequeue;             /* mode 0 */
    rc = (int)(unsigned char)S[1] >> 1;      /* shlr r0 (return base) */
    if ((S[1] & 1) == 0) goto copy_more;     /* mode 2: continue match copy */

    /* mode 1: load stream state, decode one token */
    in  = *(u8 **)(S + 20);
    out = *(u8 **)(S + 24);
    buf = *(u32 *)(S + 4);
    cnt = *(s32 *)(S + 8);

token:
    if (lz_getbit(&in, &buf, &cnt)) {        /* control bit 1 -> literal */
        u8 litb = *in++;
        *(u8 **)(S + 20) = (u8 *)in;
        *out++ = litb;
        *(u8 **)(S + 24) = out;
        *(u32 *)(S + 4) = buf;
        *(s32 *)(S + 8) = cnt;
        return rc;
    }

    if (lz_getbit(&in, &buf, &cnt)) {        /* match, long form */
        b0 = *in++;
        b1 = *in++;
        word = ((u32)b1 << 8) | ((u32)b0 & 0xff);
        if (word == 0) goto dequeue;         /* word 0 -> stream finished */
        off = (s32)((word >> 3) | 0xffffe000u);
        cpy = out + off;
        len = (s32)(b0 & 7);
        if (len != 0) len += 1;
        else          len = (s32)(u8)*in++;
    } else {                                 /* match, short form */
        int r = 0;
        r = (r << 1) | lz_getbit(&in, &buf, &cnt);
        {
            u32 ob;
            r = (r << 1) | lz_getbit(&in, &buf, &cnt);
            ob = *in++; /* ROM 06018418..18424: bit refill precedes offset byte. */
            cpy = out + (s32)(ob | 0xffffff00u);
        }
        len = r + 1;
    }

    /* copy the first match byte, hand off to match-copy mode */
    {
        u8 mb = *cpy++;
        *(s32 *)(S + 12) = len;
        *out++ = mb;
        *(u8 **)(S + 16) = (u8 *)cpy;
        *(u8 **)(S + 20) = (u8 *)in;
        *(u8 **)(S + 24) = out;
        *(u32 *)(S + 4) = buf;
        *(s32 *)(S + 8) = cnt;
        S[1] = 2;
        return 2;
    }

copy_more:
    {
        const u8 *src = *(u8 **)(S + 16);
        u8 *o = *(u8 **)(S + 24);
        s32 l = *(s32 *)(S + 12);
        u8 cb = *src++;
        l -= 1;
        *o++ = cb;
        *(u8 **)(S + 16) = (u8 *)src;
        *(u8 **)(S + 24) = o;
        if (l == 0) { S[1] = 1; return 1; }  /* match done -> back to token mode */
        *(s32 *)(S + 12) = l;
        return l;                            /* ROM 06018498..184aa leaves decremented length in r0. */
    }

dequeue:
    rd = S[2];
    wr = S[3];
    if (rd == wr) { *(u32 *)(S + 0) = 0; return 0; }  /* queue empty -> disable */
    S[2] = (u8)((rd + 1) & 127);
    in  = *(u8 **)(S + 28 + rd * 8);
    out = *(u8 **)(S + 28 + rd * 8 + 4);
    buf = (u32)(s32)(s8)*in++;
    cnt = 9;                                 /* primed: 8 bits + the preloaded byte */
    rc = 1;
    S[1] = 1;
    goto token;
}
