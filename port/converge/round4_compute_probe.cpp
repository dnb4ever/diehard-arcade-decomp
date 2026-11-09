#include "round4_compute_references.h"
#include <vector>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
static std::vector<uint8_t> cartridge;
extern "C" uint32_t __real__Z11sh2_io_readji(uint32_t,int);
extern "C" uint32_t __wrap__Z11sh2_io_readji(uint32_t a,int n) {
    uint32_t p=a&0xdfffffffu;
    if(p>=0x02000000u && uint64_t(p)+n<=0x03400000u) {
        uint32_t offset=p-0x02000000u,value=0;
        if(uint64_t(offset)+n>cartridge.size())std::abort();
        for(int j=0;j<n;++j)value=(value<<8)|cartridge[offset+j];
        return value;
    }
    return __real__Z11sh2_io_readji(a,n);
}
struct Yield {};
struct Point {SH2Context c;std::vector<uint8_t> h,l;};
static std::vector<Point> points;
static unsigned polls,throw_at;
static bool capture=true,mutate=false;
extern "C" void __wrap__Z8sh2_pollR10SH2Context(SH2Context& c) {
    ++polls;
    if(mutate && (c.cont==3302 || c.cont==3288))st32(c.r[15],0x80000000u+polls);
    if(capture)points.push_back({c,{g_wram_h,g_wram_h+0x100000},{g_wram_l,g_wram_l+0x100000}});
    c.budget=0;
    if(polls==throw_at)throw Yield{};
}
static uint32_t rng=0x4156abcd;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool equal(const SH2Context&a,const SH2Context&b){return !std::memcmp(&a,&b,sizeof a);}
static bool memory(const Point&p){return !std::memcmp(p.h.data(),g_wram_h,0x100000)&&!std::memcmp(p.l.data(),g_wram_l,0x100000);}
static void restore(const Point&p){std::memcpy(g_wram_h,p.h.data(),0x100000);std::memcpy(g_wram_l,p.l.data(),0x100000);}
static unsigned vectors,checkpoints,resumes,unwinds;
static int check(SH2Func fn,SH2Func ref,SH2Context initial,unsigned stride) {
    Point before{initial,{g_wram_h,g_wram_h+0x100000},{g_wram_l,g_wram_l+0x100000}};
    points.clear();polls=throw_at=0;capture=true;SH2Context expected=initial;ref(expected);
    auto refs=std::move(points);Point after{expected,{g_wram_h,g_wram_h+0x100000},{g_wram_l,g_wram_l+0x100000}};
    restore(before);polls=0;SH2Context actual=initial;fn(actual);
    if(!equal(expected,actual)||!memory(after)||points.size()!=refs.size()) {
        std::fprintf(stderr,"FAIL final vector=%u points=%zu/%zu cont=%u/%u\n",vectors,points.size(),refs.size(),actual.cont,expected.cont);
        for(unsigned i=0;i<16;++i)if(expected.r[i]!=actual.r[i])std::fprintf(stderr,"r%u %08x %08x\n",i,expected.r[i],actual.r[i]);return 1;
    }
    for(unsigned i=0;i<refs.size();++i) {
        if(!equal(refs[i].c,points[i].c)||refs[i].h!=points[i].h||refs[i].l!=points[i].l) {
            std::fprintf(stderr,"FAIL checkpoint vector=%u poll=%u cont=%u RAM=%d\n",vectors,i,refs[i].c.cont,int(refs[i].h!=points[i].h));
            for(unsigned j=0;j<16;++j)if(refs[i].c.r[j]!=points[i].c.r[j])std::fprintf(stderr,"r%u %08x %08x\n",j,refs[i].c.r[j],points[i].c.r[j]);return 2;
        }
    }
    checkpoints+=refs.size();points.clear();capture=false;
    for(unsigned i=0;i<refs.size();i+=stride) {
        restore(refs[i]);SH2Context resumed=refs[i].c;resumed.cont_entry=resumed.cont;resumed.budget=0;polls=i+1;fn(resumed);
        if(!equal(resumed,expected)||!memory(after)){std::fprintf(stderr,"FAIL resume %u cont=%u\n",i,refs[i].c.cont);return 3;}++resumes;
    }
    if(!refs.empty()) {
        restore(before);actual=initial;polls=0;throw_at=1;
        try{fn(actual);return 4;}catch(const Yield&){}
        throw_at=0;actual.cont_entry=actual.cont;actual.budget=0;fn(actual);
        if(!equal(actual,expected)||!memory(after))return 5;++unwinds;
    }
    ++vectors;return 0;
}
int main(int argc,char**argv) {
    const SH2Func fns[]={p_dh::f_06018E2C,p_dh::f_06018E2E,p_dh::f_06017CA4,p_dh::f_06017B5C,p_dh::f_0601726C};
    const SH2Func refs[]={compute_reference::ref_06018E2C,compute_reference::ref_06018E2E,compute_reference::ref_06017CA4,compute_reference::ref_06017B5C,compute_reference::ref_0601726C};
    FILE* rom=std::fopen("work/converge/round4/snapshot/cart.bin","rb");if(!rom)return 20;
    std::fseek(rom,0,SEEK_END);cartridge.resize(std::ftell(rom));std::rewind(rom);
    if(std::fread(cartridge.data(),1,cartridge.size(),rom)!=cartridge.size())return 21;std::fclose(rom);
    uint32_t word=compute_read(0x22db30a4);if(!word)return 22;
    cartridge[0xdb30a4]^=0x80;
    if(compute_read(0x22db30a4)!=(word^0x80000000u))return 23;
    cartridge[0xdb30a4]^=0x80;
    if(argc>1) {
        FILE*f=std::fopen(argv[1],"rb");if(!f)return 10;SH2Context c;unsigned kind;
        if(std::fread(&kind,4,1,f)!=1 || std::fread(&c,sizeof c,1,f)!=1 ||std::fread(g_wram_h,1,0x100000,f)!=0x100000||std::fread(g_wram_l,1,0x100000,f)!=0x100000)return 11;
        if(kind>=5 || check(fns[kind],refs[kind],c,47))return 12;
    } else {
        for(unsigned kind=0;kind<5;++kind)for(unsigned i=0;i<(kind>=2?100:8);++i) {
            std::memset(g_wram_h,0,0x100000);std::memset(g_wram_l,0,0x100000);
            SH2Context c{};auto*b=(uint8_t*)&c;for(unsigned j=0;j<sizeof c;j+=4){uint32_t v=next();std::memcpy(b+j,&v,4);}
            c.cpu=i&1;c.t&=1;c.s=0;c.cont_entry=0;c.r[15]=0x060ff000;c.budget=-1;
            for(unsigned off=0;off<128;off+=4)st32(c.r[15]+off,next());
            if(kind<2) {
                unsigned count=i*2,secondary=i%4;
                st32(0x060cf0c4,0x060c3cc4+count*36);st32(0x060cf0c8,0x060cf0c4-(secondary+1)*36);
                for(unsigned j=0;j<count;++j)st32(0x060c3cc4+j*36+32,i%2?next():0x123456);
                for(unsigned j=0;j<secondary;++j)st32(0x060cf0c4-(j+1)*36+32,next());
                mutate=false;
                if(int r=check(kind?p_dh::f_06018E2E:p_dh::f_06018E2C,kind?compute_reference::ref_06018E2E:compute_reference::ref_06018E2C,c,31))return r;
            } else if(kind<4) {
                for(unsigned j=0;j<48;j+=4)st32(0x06098aec+j,next());
                st32(0x06098878,i%3==0?c.r[15]-12:0x06080000);
                for(unsigned j=0;j<48;j+=4)st32(0x06080000+j,next());
                mutate=i%2;
                if(int r=check(fns[kind],refs[kind],c,1))return r;
            } else {
                c.r[4]=0x06080000;c.r[5]=c.r[4]+(i%5)*4;c.r[6]=1+i%24;
                if(i%2)c.r[5]|=0x20000000;
                for(unsigned j=0;j<128;j+=4)st32(0x06080000+j,next());
                if(i%3==0)c.r[4]=0x22db30a4;
                mutate=false;if(int r=check(fns[kind],refs[kind],c,1))return r;
            }
        }
        for(unsigned kind=0;kind<3;++kind) {
            auto pid=fork();if(!pid){rlimit limit{0,0};setrlimit(RLIMIT_CORE,&limit);if(!std::freopen("/dev/null","w",stderr))_exit(7);
                if(kind==0){SH2Context c{};c.s=1;converge::poll_translate(c,255);}else if(kind==1)compute_read(0x25c00000);else compute_write(0x02000000,0);_exit(0);}
            int status;waitpid(pid,&status,0);if(!WIFSIGNALED(status))return 6;
        }
    }
    std::printf("PASS compute: %u vectors, %u exact complete-context/both-RAM checkpoints, %u serialized resumes, %u forced unwinds; both entry prologues, duplicate keys, empty lists, aliases, modulo MAC overflow, post-poll input mutation, nonzero ROM byte mutation, S/device/ROM-write rejection\n",vectors,checkpoints,resumes,unwinds);
}
