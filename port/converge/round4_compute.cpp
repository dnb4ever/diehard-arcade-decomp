#include "p_dh_funcs.h"
#include "bridge.h"
#include "round4_compute.h"
#include <cstdio>
#include <cstdlib>
static uint32_t physical(uint32_t a) {
    a&=0xdfffffffu;
    if(a>=0x06000000 && a<0x08000000)return 0x06000000+(a&0xfffff);
    return a;
}
static void require_ram(uint32_t a) {
    uint32_t p=physical(a);
    if(!((p>=0x06000000 && p<=0x060ffffc) || (p>=0x00200000 && p<=0x002ffffc))) {
        std::fprintf(stderr,"CONVERGE compute rejected bus %08x\n",a);std::abort();
    }
}
extern "C" uint32_t compute_read(uint32_t a) {
    uint32_t p=physical(a);
    // Same immutable cartridge contract as Transaction::read; writes still RAM only.
    if(!(p>=0x02000000u && p<=0x033ffffcu))require_ram(a);
    return ld32(a);
}
extern "C" void compute_write(uint32_t a,uint32_t v) {require_ram(a);st32(a,v);}
static void checkpoint(SH2Context& c,unsigned cont,unsigned branch) {
    c.cont=cont;c.cont_k=bool(branch);SH2_POLL(c);
}
extern "C" void sort_boundary(void *context,const SortState *state,unsigned phase) {
    auto& c=*(SH2Context*)context;
    auto& s=*const_cast<SortState*>(state);
    unsigned base=s.base;
    switch(phase) {
    case 0:
        c.r[7]=0x060bacc0;c.r[4]=0x060bacb8;c.r[3]=0x060cf0c4;
        c.r[2]=0x060c00c0;c.r[10]=0x060bf4c0;c.r[13]=0x060bacbc;
        c.r[11]=0x060c3cc4;c.r[14]=255;c.r[6]=s.cursor;break;
    case 1:
        c.r[6]=s.cursor;c.t=s.cursor>=0x060c3cc4;
        if(c.t)checkpoint(c,base,s.branch);break;
    case 2:
        c.r[12]=s.destination;c.r[5]=s.node;c.r[3]=s.key;c.r[2]=s.head;c.t=0;break;
    case 3:
        c.r[6]=0x20000000;c.r[5]=s.cursor;c.r[11]=0x260cf0c4;c.t=s.cursor>=0x260cf0c4;break;
    case 4:
        c.r[12]=s.destination;c.r[6]=s.node;c.r[3]=s.key;c.r[2]=s.head;c.r[5]=s.cursor;
        c.t=s.cursor>=0x260cf0c4;s.branch=!c.t;
        if(s.branch)checkpoint(c,base+1,s.branch);break;
    case 5:c.r[9]=256;c.r[11]=0;c.r[8]=0xff00;c.r[14]=s.bucket;break;
    case 6:case 10:c.r[0]=s.source;c.t=!s.source;break;
    case 7:
        c.r[12]=s.destination;c.r[6]=s.node;c.r[3]=s.entity;c.r[2]=s.key;c.r[5]=s.source;c.t=!s.source;
        if(!c.t)checkpoint(c,base+2,s.branch);break;
    case 8:
        c.r[9]=s.remaining;c.r[14]=s.bucket;c.t=!s.remaining;s.branch=!c.t;
        if(s.branch)checkpoint(c,base+3,s.branch);break;
    case 9:c.r[14]=s.bucket;c.r[12]=256;c.r[3]=0x060bb8c0;c.r[10]=0x3f0000;break;
    case 11:
        c.r[13]=s.destination;c.r[6]=s.node;c.r[2]=s.head;c.r[3]=s.entity;c.r[5]=s.source;c.t=!s.source;
        if(!c.t)checkpoint(c,base+4,s.branch);break;
    case 12:
        c.r[12]=s.remaining;c.r[14]=s.bucket;c.t=!s.remaining;s.branch=!c.t;
        if(s.branch)checkpoint(c,base+5,s.branch);break;
    case 13:
        for(unsigned i=8;i<=14;++i){c.r[i]=compute_read(c.r[15]);c.r[15]+=4;}
        c.pc=c.pr;break;
    default:std::abort();
    }
}
extern "C" void matrix_boundary(void *context,const MatrixState *s,unsigned phase) {
    auto& c=*(SH2Context*)context;
    c.mach=s->accumulator>>32;c.macl=s->accumulator;
    c.r[0]=s->matrix;c.r[1]=s->vector;c.r[2]=s->output;
    c.r[3]=s->result;c.r[5]=s->offset;c.t=!s->remaining;
    if(s->mode){c.r[4]=c.mach;c.r[7]=s->remaining;}
    else c.r[4]=s->remaining;
    if(phase==1)checkpoint(c,s->mode ? 3288 : 3302,1);
    else {c.r[15]+=12;c.pc=c.pr;}
}
extern "C" void copy_boundary(void *context,const CopyState *s) {
    auto& c=*(SH2Context*)context;
    c.r[0]=s->value;c.r[4]=s->source;c.r[5]=s->destination;c.r[6]=s->remaining;c.t=!s->remaining;
    if(s->remaining)checkpoint(c,3237,1);else c.pc=c.pr;
}
static void save_frame(const SH2Context& c,unsigned kind) {
    const char* folder=std::getenv("CONVERGE_SAVE_FRAMES");
    static bool saved[5]{};
    if(!folder || saved[kind])return;
    char path[4096];std::snprintf(path,sizeof path,"%s/compute-%u.bin",folder,kind);
    FILE* f=std::fopen(path,"wb");if(!f)std::abort();
    bool good=std::fwrite(&kind,4,1,f)==1 && std::fwrite(&c,sizeof c,1,f)==1 &&
        std::fwrite(g_wram_h,1,0x100000,f)==0x100000 && std::fwrite(g_wram_l,1,0x100000,f)==0x100000;
    if(std::fclose(f) || !good)std::abort();saved[kind]=true;
}
namespace converge {
void poll_sort(SH2Context& c,unsigned index,bool short_entry) {
    unsigned base=short_entry ? 3455 : 3448;
    unsigned entry=c.cont_entry;c.cont_entry=0;
    unsigned phase=entry>=base && entry<=base+5 ? entry-base+1 : 0;
    if(!phase) {
        save_frame(c,short_entry ? 1 : 0);
        record(index,c.cpu);
        for(int i=short_entry ? 13 : 14;i>=8;--i) {c.r[15]-=4;compute_write(c.r[15],c.r[i]);}
    }
    SortState s{};s.phase=phase;s.branch=bool(c.cont_k);
    s.cursor=phase==1 ? c.r[6] : c.r[5];s.source=c.r[5];s.bucket=c.r[14];
    s.remaining=phase<=4 ? c.r[9] : c.r[12];
    s.base=base;
    converge_sort(&c,&s);
}
void poll_translate(SH2Context& c,unsigned index) {
    if(c.s) {std::fputs("CONVERGE matrix requires SR.S=0\n",stderr);std::abort();}
    unsigned entry=c.cont_entry;c.cont_entry=0;
    MatrixState s{};
    if(entry!=3302) {
        save_frame(c,2);
        record(index,c.cpu);
        for(int i=6;i>=4;--i) {c.r[15]-=4;compute_write(c.r[15],c.r[i]);}
        compute_write(0x06098ba4,c.r[6]);compute_write(0x06098ba0,c.r[5]);compute_write(0x06098b9c,c.r[4]);
        c.r[3]=0x06098b9c;c.r[0]=compute_read(0x06098878);c.r[1]=c.r[15];c.r[2]=c.r[0]+12;c.r[4]=3;
    }
    s.matrix=c.r[0];s.output=c.r[2];s.vector=c.r[15];s.remaining=c.r[4];
    converge_translate(&c,&s);
}
void poll_transform(SH2Context& c,unsigned index) {
    if(c.s){std::fputs("CONVERGE matrix requires SR.S=0\n",stderr);std::abort();}
    unsigned entry=c.cont_entry;c.cont_entry=0;
    if(entry!=3288) {
        save_frame(c,3);record(index,c.cpu);
        c.r[0]=compute_read(0x06098878);c.r[2]=0x06098b1c;c.r[7]=3;
        c.r[4]=compute_read(c.r[0]+44);c.r[5]=compute_read(c.r[0]+28);
        c.r[15]-=4;compute_write(c.r[15],c.r[4]);
        c.r[6]=compute_read(c.r[0]+12);c.r[15]-=4;compute_write(c.r[15],c.r[5]);
        c.r[0]=0x06098aec;c.r[15]-=4;compute_write(c.r[15],c.r[6]);c.r[1]=c.r[15];
    }
    MatrixState s{};s.matrix=c.r[0];s.output=c.r[2];s.vector=c.r[15];s.remaining=c.r[7];s.mode=1;
    converge_translate(&c,&s);
}
void poll_copy(SH2Context& c,unsigned index) {
    unsigned entry=c.cont_entry;c.cont_entry=0;
    if(entry!=3237){save_frame(c,4);record(index,c.cpu);}
    CopyState s{c.r[4],c.r[5],c.r[6],0};converge_copy(&c,&s);
}

}
