// Resumable readable 02223c74. Original shared-signal samples and taken-edge
// checkpoints are retained. Unsigned counters preserve SH-2 wraparound.
#include "round3_pulse.h"
void converge_pulse(void *context, struct PulseState *s) {
    if (!s->phase) {
        pulse_write(0x060e0bc8,4,1);
        uint32_t tick=pulse_read(0x060987bc,4)+1;
        pulse_write(0x060987bc,4,tick);
        pulse_boundary(context,s,0,tick);
    } else if (s->phase==1) goto low_increment;
    else if (s->phase==2) goto high_increment;
    // phase 3 resumes after the outer taken backedge's spent poll.
    for (;;) {
        for (;;) {
            s->sample=pulse_read(0x060986af,1);
            if (s->sample) break;
            pulse_boundary(context,s,1,0);
low_increment:
            s->low++;
        }
        pulse_boundary(context,s,5,0);
        s->run=0;
        for (;;) {
            s->sample=pulse_read(0x060986af,1);
            if (s->sample!=1) break;
            pulse_boundary(context,s,2,0);
high_increment:
            s->run++;
        }
        pulse_boundary(context,s,6,0);
        pulse_write(0x060986a4,4,s->run);
        s->gate=pulse_read(0x060986ae,1);
        s->total+=s->run;
        if (s->gate==1) break;
        pulse_boundary(context,s,3,0);
    }
    pulse_write(0x060986a0,4,s->total+s->low);
    pulse_write(0x060986ae,1,0);
    pulse_write(0x060e0bc8,4,0);
    pulse_write(0x0609869c,4,0);
    pulse_boundary(context,s,4,0);
}
