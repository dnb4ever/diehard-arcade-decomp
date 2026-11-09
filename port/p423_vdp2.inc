// Private VDP2 endpoints. reg/vb/vw select these only during composition;
// guest-visible arrays and state serialization always retain the real bytes.
struct P423Bg {
    std::array<uint8_t,0x200> regs{};
    std::vector<uint8_t> ram;
    std::array<uint8_t,0x1000> palette{};
};
static P423Bg p423_bg_old,p423_bg_new,p423_bg_mix;
static const uint8_t *p423_regs=nullptr,*p423_ram=nullptr;
static bool p423_bg_valid;
static unsigned p423_bg_reason;
static uint64_t p423_bg_changes;
static uint32_t p423_rd(const uint8_t* p,unsigned a,int bytes) {
    uint32_t v=0;for(int i=0;i<bytes;++i)v=v<<8|p[a+i];return v;
}
static void p423_wr(uint8_t* p,unsigned a,int bytes,uint32_t v) {
    for(int i=bytes-1;i>=0;--i) {p[a+i]=uint8_t(v);v>>=8;}
}
void vdp2_p423_reset() {
    p423_bg_old={};p423_bg_new={};p423_bg_mix={};p423_bg_valid=false;
    p423_regs=p423_ram=nullptr;
}
void vdp2_p423_field() {
    p423_bg_old=std::move(p423_bg_new);
    std::copy(std::begin(g_vdp2_regs),std::end(g_vdp2_regs),p423_bg_new.regs.begin());
    std::copy(std::begin(g_vdp2_cram),std::end(g_vdp2_cram),p423_bg_new.palette.begin());
    p423_bg_new.ram.assign(std::begin(g_vdp2_vram),std::end(g_vdp2_vram));
}
// Whitelist only understood smooth fields. Any other changed render state,
// palette, table topology, flag or content rejects the coordinated pair.
bool vdp2_p423_prepare(float alpha) {
    p423_bg_reason=0;p423_bg_changes=0;p423_bg_valid=false;
    p423_regs=p423_ram=nullptr;
    auto reject=[&](unsigned why){p423_bg_reason=why;return false;};
    if(p423_bg_old.ram.empty())return reject(1);
    const auto& a=p423_bg_old;const auto& b=p423_bg_new;
    bool palette_changed=a.palette!=b.palette;
    p423_bg_mix=b;
    std::array<uint8_t,0x200> rmask{};
    std::vector<uint8_t> vmask(0x80000);
    bool good=true;
    auto field=[&](bool memory,unsigned at,int bytes,int bits,int shift,bool sign,double limit,bool wrap=false){
        auto* src0=memory?a.ram.data():a.regs.data();auto* src1=memory?b.ram.data():b.regs.data();
        auto* dst=memory?p423_bg_mix.ram.data():p423_bg_mix.regs.data();
        uint32_t mask=bits==32?~0u:((1u<<bits)-1),raw0=p423_rd(src0,at,bytes),raw1=p423_rd(src1,at,bytes);
        int64_t v0=(raw0>>shift)&mask,v1=(raw1>>shift)&mask;
        if(sign) {int64_t s=1ll<<(bits-1);v0=(v0^s)-s;v1=(v1^s)-s;}
        int64_t delta=v1-v0;
        if(wrap) {int64_t period=1ll<<bits;if(delta>period/2)delta-=period;if(delta<-period/2)delta+=period;}
        if(std::abs(double(delta))>limit)good=false;
        uint32_t v=uint32_t(int64_t(std::llround(v0+delta*alpha)))&mask;
        p423_wr(dst,at,bytes,(raw1&~(mask<<shift))|(v<<shift));
        if(delta)++p423_bg_changes;
        auto* allowed=memory?vmask.data():rmask.data();
        for(int i=0;i<bytes;++i)allowed[at+i]|=uint8_t((mask<<shift)>>(8*(bytes-1-i)));
    };
    auto rr=[&](unsigned at){return p423_rd(b.regs.data(),at,2);};
    unsigned bgon=rr(0x20);
    for(int n=0;n<4;++n) if(bgon>>n&1) {
        if(n<2) {
            unsigned base=0x70+16*n;
            for(unsigned k:{0u,4u})field(false,base+k,4,19,8,false,48*256,true);
            for(unsigned k:{8u,12u})field(false,base+k,4,11,8,false,128);
            unsigned ctl=rr(0x9a)>>(n*8),flags=ctl&14;
            unsigned interval=(1u<<(ctl>>4&3))*((rr(0)>>6&3)==2?2:1);
            unsigned addr=((rr(0xa0+4*n)&7)<<16|rr(0xa2+4*n))*2;
            for(unsigned line=0;line<256;line+=interval)for(unsigned flag:{2u,4u,8u})if(flags&flag){
                if(addr+4>0x80000)return reject(3);
                field(true,addr,4,flag==8?11:19,8,true,flag==8?128:48*256);addr+=4;
            }
        } else {
            // NBG3 contains screen-fixed HUD. Its scroll must be unchanged.
            if(n==2)for(unsigned k:{0u,2u})field(false,0x90+k,2,11,0,false,48,true);
        }
    }
    unsigned rpt=(((rr(0xbc)&7)<<16|rr(0xbe))&~0x41u)*2;
    unsigned rpmd=rr(0xb0)&3;
    if(rpmd==3 && (bgon&0x30))return reject(3);
    for(int set=0;set<2;++set) {
        bool used=set==0?((bgon&16)&&rpmd!=1):((bgon&32)||((bgon&16)&&rpmd!=0));
        if(!used)continue;
        unsigned base=rpt+set*0x80;if(base+0x60>0x80000)return reject(3);
        for(unsigned off:{0u,4u,8u})field(true,base+off,4,23,6,true,64*1024);
        for(unsigned off:{12u,16u,20u,24u})field(true,base+off,4,12,6,true,512);
        for(unsigned off:{28u,32u,36u,40u,44u,48u})field(true,base+off,4,20,0,true,32768);
        for(unsigned off:{52u,54u,56u,60u,62u,64u})field(true,base+off,2,16,0,true,64);
        for(unsigned off:{68u,72u})field(true,base+off,4,24,6,true,64*1024);
        for(unsigned off:{76u,80u})field(true,base+off,4,24,0,true,32768);
        field(true,base+84,4,26,6,true,64*1024);
        for(unsigned off:{88u,92u})field(true,base+off,4,20,6,true,512);
        unsigned kt=rr(0xb4)>>(8*set);
        if(kt&1) {
            bool word=kt&2;unsigned bank=rr(0xb6)>>(8*set)&7;
            unsigned kbase=word?bank<<17:(bank&3)<<18;
            // Union of all coefficient entries sampled by both endpoint rectangles.
            // Contiguous range is intentionally conservative for subpixel/row rounding.
            double low=1e30,high=-1e30;
            for(const auto* src:{&a,&b}) {
                auto raw=[&](unsigned off){return p423_rd(src->ram.data(),base+off,4);};
                auto sx=[](uint32_t v,int bits){int32_t s=1u<<(bits-1);return int32_t((v&((1u<<bits)-1))^s)-s;};
                double start=int32_t(raw(84)&0xffffffc0)/65536.0;
                double dy=sx(raw(88)>>6,20)/1024.0,dx=sx(raw(92)>>6,20)/1024.0;
                for(double y:{0.0,255.0})for(double x:{0.0,351.5}) {
                    double v=start+y*dy+x*dx;low=std::min(low,v);high=std::max(high,v);
                }
            }
            if(high-low>32768 || low<0)return reject(3);
            for(int64_t idx=int64_t(std::floor(low));idx<=int64_t(std::ceil(high));++idx) {
                unsigned addr=(kbase+idx*(word?2:4))&(word?0x7fffe:0x7fffc);
                field(true,addr,word?2:4,word?15:24,0,true,word?256:16384);
            }
        }
    }
    if(palette_changed)return reject(2);
    if(!good)return reject(4);
    for(unsigned i=0;i<0x200;++i) {
        if(i>=4&&i<10)continue; // TVSTAT/HCNT/VCNT are not compositor inputs.
        if((a.regs[i]^b.regs[i])&~rmask[i])return reject(5);
    }
    for(unsigned i=0;i<0x80000;++i)if((a.ram[i]^b.ram[i])&~vmask[i])return reject(6);
    p423_bg_valid=true;return true;
}
void vdp2_p423_enable(bool on) {
    p423_regs=on&&p423_bg_valid?p423_bg_mix.regs.data():nullptr;
    p423_ram=on&&p423_bg_valid?p423_bg_mix.ram.data():nullptr;
}
unsigned vdp2_p423_reason(){return p423_bg_reason;}
uint64_t vdp2_p423_changes(){return p423_bg_changes;}

void vdp2_p423_dump(const std::string& path) {
    FILE* f=std::fopen(path.c_str(),"wb");if(!f)return;
    for(const auto* bg:{&p423_bg_old,&p423_bg_new})if(!bg->ram.empty()){
        std::fwrite(bg->regs.data(),1,bg->regs.size(),f);
        std::fwrite(bg->palette.data(),1,bg->palette.size(),f);
        std::fwrite(bg->ram.data(),1,bg->ram.size(),f);
    }
    std::fclose(f);
}
