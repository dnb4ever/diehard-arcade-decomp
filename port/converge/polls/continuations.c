// Resumable forms of frozen readable 02213164 / 02228106. Checkpoints are
// original taken backedges, after their delay slots; never extra C-loop polls.
#include "continuations.h"
uint32_t converge_root(void *context, struct ConvergeRoot *s) {
    do {
        s->input=(s->input<<1)|(s->input>>31);
        s->input=(s->input<<1)|(s->input>>31);
        s->top=s->input&3;
        s->remainder=(s->remainder<<2)|s->top;
        s->root<<=1;
        s->iterations--;
        if(s->root<s->remainder){s->root++;s->remainder-=s->root;s->root++;}
        if(s->iterations!=3) converge_root_checkpoint(context,s);
    } while(s->iterations!=3);
    return (s->root>>1)<<8;
}
int32_t converge_slot(void *context,uint32_t index) {
    do {
        index++;
        if(!converge_slot_read(context,index)) break;
        converge_slot_checkpoint(context,index);
    } while(1);
    return converge_slot_finish(context,index);
}
