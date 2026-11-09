// Reviewed resumable forms of display_radix_sort_{records,entities} and
// matrix_translate_current. Memory operations keep their original order;
// only original taken backedges spend polls. Phase state lives in guest
// registers at each boundary; no native local survives a scheduler unwind.
#include "round4_compute.h"
#define R(a) compute_read(a)
#define W(a,v) compute_write(a,v)
static void insert(struct SortState *s,uint32_t table,uint32_t arena,int key_store) {
    s->node=R(arena);
    s->destination=table+s->digit*12;
    W(arena,s->node+12);
    s->head=R(s->destination);
    W(s->node,s->head);
    s->entity=R(s->source+4);
    W(s->node+4,s->entity);
    if(key_store) {s->key=R(s->source+8);W(s->node+8,s->key);}
    W(s->destination,s->node);
    s->next=R(s->source);
}
void converge_sort(void *context,struct SortState *s) {
    switch(s->phase) {
    case 1:goto primary_body;
    case 2:goto secondary_body;
    case 3:goto middle_body;
    case 4:goto middle_bucket;
    case 5:goto final_body;
    case 6:goto final_bucket;
    }
    W(0x060bacb8,0x060bb8c0);W(0x060bacbc,0x060c00c0);
    s->cursor=R(0x060cf0c4);sort_boundary(context,s,0);
    for(;;) {
        s->cursor-=36;
        sort_boundary(context,s,1);
        if(s->cursor<0x060c3cc4u)break;
primary_body:
        s->digit=R(s->cursor+32)&255;
        s->node=R(0x060bacb8);
        s->destination=0x060bacc0+s->digit*12;
        W(0x060bacb8,s->node+12);
        s->head=R(s->destination);W(s->node,s->head);
        W(s->node+4,s->cursor);
        s->key=R(s->cursor+32);W(s->node+8,s->key);
        W(s->destination,s->node);sort_boundary(context,s,2);
    }
    s->cursor=R(0x260cf0c8)+0x20000024u;
    sort_boundary(context,s,3);
    while(s->cursor<0x260cf0c4u) {
secondary_body:
        s->digit=R(s->cursor+32)&255;
        s->node=R(0x060bacb8);
        s->destination=0x060bacc0+s->digit*12;
        W(0x060bacb8,s->node+12);
        s->head=R(s->destination);W(s->node,s->head);
        W(s->node+4,s->cursor);
        s->key=R(s->cursor+32);s->cursor+=36;
        W(s->node+8,s->key);W(s->destination,s->node);
        sort_boundary(context,s,4);
    }
    s->bucket=0x060bacc0;s->remaining=256;sort_boundary(context,s,5);
    for(;;) {
middle_bucket:
        s->source=R(s->bucket);sort_boundary(context,s,6);
        if(s->source) {
            // The ROM rereads the bucket before following its list.
            s->source=R(s->bucket);
            do {
middle_body:
                s->digit=(R(s->source+8)&0xff00)>>8;
                insert(s,0x060bf4c0,0x060bacbc,1);
                s->source=s->next;sort_boundary(context,s,7);
            } while(s->source);
            W(s->bucket,0);
        }
        --s->remaining;s->bucket+=12;sort_boundary(context,s,8);
        if(!s->remaining)break;
    }
    s->bucket=0x060bf4c0;s->remaining=256;
    W(0x060bacb8,0x060bb8c0);sort_boundary(context,s,9);
    for(;;) {
final_bucket:
        s->source=R(s->bucket);sort_boundary(context,s,10);
        if(s->source) {
            s->source=R(s->bucket);
            do {
final_body:
                s->digit=(R(s->source+8)&0x3f0000)>>16;
                insert(s,0x060bacc0,0x060bacb8,0);
                s->source=s->next;sort_boundary(context,s,11);
            } while(s->source);
            W(s->bucket,0);
        }
        --s->remaining;s->bucket+=12;sort_boundary(context,s,12);
        if(!s->remaining)break;
    }
    sort_boundary(context,s,13);
}
void converge_translate(void *context,struct MatrixState *s) {
    while(s->remaining) {
        // SR.S=0 contract: sum modulo 2^64, then extract bits 16..47.
        // Input words are reread from the original guest spills each row.
        s->accumulator=0;
        for(unsigned j=0;j<3;++j) {
            int64_t a=(int32_t)R(s->vector+4*j);
            int64_t b=(int32_t)R(s->matrix+4*j);
            s->accumulator+=(uint64_t)(a*b);
        }
        s->offset=R(s->matrix+12);
        s->result=(uint32_t)(s->accumulator>>16)+s->offset;
        W(s->output,s->result);
        s->matrix+=16;s->output+=s->mode ? 4 : 16;--s->remaining;
        matrix_boundary(context,s,s->remaining ? 1 : 2);
    }
}

void converge_copy(void *context,struct CopyState *s) {
    do {
        s->value=R(s->source);s->source+=4;
        W(s->destination,s->value);s->destination+=4;
        --s->remaining;copy_boundary(context,s);
    } while(s->remaining);
}
