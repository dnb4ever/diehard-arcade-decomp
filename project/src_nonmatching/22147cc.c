/* 0x022147cc  set two pointers (0x0609855C, 0x06098558) to 0x060F8000.
   NONMATCHING: instruction-for-instruction identical to ROM, but this NO-ARG leaf hits
   the first-scratch-register version trait -- SHC r32 allocates r4,r3,r2 where the 1996
   ROM build uses r3,r2,r1; no -optimize/-speed/-size flag variant changes it. Also shares
   the unit blocked by member-0's foreign-pool prefix. */
typedef int s32;
void f_2147cc(void) {
    *(s32 *)0x0609855C = 0x060F8000;
    *(s32 *)0x06098558 = 0x060F8000;
}
