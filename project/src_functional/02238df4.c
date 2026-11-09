/* display_pulse_decay_task [0x02238df4]: Seeds a three-channel display pulse and decays it over eight frames. */
#include "dh.h"
/* 0x02238df4 (leafC) - state machine; state=*(s8)(arg0+16), obj=*(arg0+8).
 * state 0: (obj+52)=(obj+54)=100, (obj+56)=40 (words); state++.
 * state 1: (obj+52)=(obj+54)=(obj+56)=80 (words); (obj+64)=8 (long); state++.
 * state 2: (obj+52/54/56)-=10 (words); if (--(obj+64))==0 clear *arg0.
 * All: tail-call func_022386fc(obj+52).  bra cart 0x2386fc -> RAM 0x0603c6fc.
 * fidelity: HIGH - fields 52/54/56 16-bit (mov.w); 64 32-bit (mov.l); state 8-bit. */
extern void display_accumulate_channel(void *);
void display_pulse_decay_task(void *arg0) {
    void *o = MP(arg0, 0x8);
    s8 state = M8(arg0, 16);
    if (state == 0) {
        M16(o, 52) = 100;
        M16(o, 54) = 100;
        M16(o, 56) = 40;
        M8(arg0, 16) = (s8)(state + 1);
    } else if (state == 1) {
        M16(o, 52) = 80;
        M16(o, 54) = 80;
        M16(o, 56) = 80;
        M32(o, 64) = 8;
        M8(arg0, 16) = (s8)(state + 1);
    } else if (state == 2) {
        M16(o, 52) = (s16)(M16(o, 52) - 10);
        M16(o, 54) = (s16)(M16(o, 54) - 10);
        M16(o, 56) = (s16)(M16(o, 56) - 10);
        M32(o, 64) = M32(o, 64) - 1;
        if (M32(o, 64) == 0)
            M32(arg0, 0) = 0;
    }
    display_accumulate_channel((char *)o + 52);
}
