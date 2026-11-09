// Included after RecCmd/RasterState and command() in vdp1.cpp. Snapshot-only probe.
// All raster work has private state, targets, textures, counters and clip/local state.
struct P423Draw { RasterState initial; std::vector<RecCmd> commands; std::vector<uint8_t> texture; };
struct P423Frame { std::vector<P423Draw> draws; uint64_t serial=0, field=0; unsigned scene=0; };
static P423Frame p423_buffers[2], p423_before, p423_latest;
static std::vector<uint16_t> p423_dots;
static bool p423_show;
static uint64_t p423_last_serial=~0ull;
static uint64_t p423_candidates, p423_matches, p423_moving;
static void p423_reset() {
    for(auto& f:p423_buffers) f={};
    p423_before={};p423_latest={};p423_show=false;p423_last_serial=~0ull;
}
void vdp1_p423_field() {
    p423_show=false;
    const auto& shown=p423_buffers[g_draw_fb^1];
    if(shown.serial && shown.serial!=p423_last_serial) {
        p423_before=std::move(p423_latest);p423_latest=shown;
        p423_latest.field=sat_vblanks();p423_latest.scene=ld32(0x06098668);
        p423_last_serial=shown.serial;
    }
}
static int p423_vertex_mask(const uint8_t* b) {
    int comm=cw(b,0)&15;
    return comm==0 ? 1 : comm==1 ? ((cw(b,0)>>8&15) ? 1 : 5) : comm==6 ? 3 : 15;
}
static bool p423_attributes(const RecCmd& a,const RecCmd& b) {
    return (cw(a.b,0)&0x0fff)==(cw(b.b,0)&0x0fff) &&
        !std::memcmp(a.b+4,b.b+4,8) && !std::memcmp(a.b+28,b.b+28,4);
}
static bool p423_texture(const P423Draw& a,const P423Draw& b,const RecCmd& c) {
    auto equal=[&](size_t pos,size_t bytes) {
        if(pos+bytes>0x80000) return false;
        return !std::memcmp(a.texture.data()+pos,b.texture.data()+pos,bytes);
    };
    int comm=cw(c.b,0)&15, mode=cw(c.b,4)>>3&7;
    if(comm<=3) {
        size_t texels=(cw(c.b,10)>>8&63)*8*(cw(c.b,10)&255);
        size_t bytes=mode<=1 ? (texels+1)/2 : mode<=4 ? texels : texels*2;
        if(!equal(size_t(cw(c.b,8))*8,bytes)) return false;
        if(mode==1 && !equal(size_t(cw(c.b,6))*8,32)) return false;
    }
    return !(cw(c.b,4)&4) || equal(size_t(cw(c.b,28))*8,8);
}
// Order is required. A moving command must also be the unique closest command
// with those attributes. Ties, count changes and large moves reject the whole frame.
static uint64_t p423_reasons[9]{};
static unsigned p423_vdp1_reason;
static bool p423_fail(int why) {p423_vdp1_reason=why+1;++p423_reasons[why];return false;}
static bool p423_pair() {
    if(p423_before.draws.empty() || p423_before.scene!=p423_latest.scene ||
       p423_before.draws.size()!=p423_latest.draws.size()) return p423_fail(0);
    for(size_t d=0;d<p423_latest.draws.size();++d) {
        const auto& a=p423_before.draws[d];const auto& b=p423_latest.draws[d];
        if(a.commands.size()!=b.commands.size()) return p423_fail(1);
        if(a.initial.fb8!=b.initial.fb8 ||
           a.initial.local_x!=b.initial.local_x || a.initial.local_y!=b.initial.local_y) return p423_fail(2);
        for(size_t k=0;k<b.commands.size();++k) {
            const auto& p=a.commands[k]; const auto& q=b.commands[k];
            int comm=cw(q.b,0)&15;
            if(!p423_attributes(p,q)) return p423_fail(3);
            if(comm>7) { if(std::memcmp(p.b+12,q.b+12,16)) return p423_fail(4);continue; }
            if(!p423_texture(a,b,q)) return p423_fail(5);
            int mask=p423_vertex_mask(q.b),ax=0,ay=0,bx=0,by=0;
            bool moving=false;
            for(int i=0;i<4;++i) if(mask>>i&1) {
                int x0=sx13(cw(p.b,12+4*i)),y0=sx13(cw(p.b,14+4*i));
                int x1=sx13(cw(q.b,12+4*i)),y1=sx13(cw(q.b,14+4*i));
                if(std::abs(x1-x0)>48 || std::abs(y1-y0)>32) return p423_fail(6);
                moving|=x0!=x1 || y0!=y1;ax+=x0;ay+=y0;bx+=x1;by+=y1;
            }
            if(!moving) continue;
            int64_t dist=int64_t(ax-bx)*(ax-bx)+int64_t(ay-by)*(ay-by);
            for(size_t j=0;j<a.commands.size();++j) if(j!=k && p423_attributes(a.commands[j],q)) {
                int x=0,y=0;
                for(int i=0;i<4;++i) if(mask>>i&1) {x+=sx13(cw(a.commands[j].b,12+4*i));y+=sx13(cw(a.commands[j].b,14+4*i));}
                if(int64_t(x-bx)*(x-bx)+int64_t(y-by)*(y-by)<=dist) return p423_fail(7);
            }
        }
    }
    return true;
}
// alpha<0 means real latest geometry. Intermediate geometry never becomes a
// hardware framebuffer, recorded draw, device revision or serialized state.
bool vdp1_p423_render(float alpha) {
    p423_vdp1_reason=0;
    ++p423_candidates;
    bool matched=alpha>=0 && p423_pair();
    if(matched) ++p423_matches;
    int width=p423_width(),pad=(width-704)/2;
    if(!matched && width==704) {p423_show=false;return false;}
    p423_dots.assign(size_t(width)*256,0);
    RasterState* saved=g_raster;
    for(size_t d=0;d<p423_latest.draws.size();++d) {
        const auto& draw=p423_latest.draws[d];
        RasterState state=draw.initial;
        state.target=nullptr;state.hi_target=nullptr;state.tex=draw.texture.data();state.steps=0;
        state.local_x+=pad;state.sys_x+=2*pad;state.ux0+=pad;state.ux1+=pad;
        g_raster=&state;p423_target=p423_dots.data();p423_stride=width;
        for(size_t k=0;k<draw.commands.size();++k) {
            const auto& rc=draw.commands[k];uint8_t cmd[32];std::copy(rc.b,rc.b+32,cmd);
            int comm=cw(cmd,0)&15;
            if(matched && comm<=7) {
                int mask=p423_vertex_mask(cmd);
                const auto& old=p423_before.draws[d].commands[k];
                for(int i=0;i<4;++i) if(mask>>i&1) for(int off:{12+4*i,14+4*i}) {
                    int a=sx13(cw(old.b,off)),b=sx13(cw(cmd,off));
                    int v=int(std::lround(a+(b-a)*alpha));
                    cmd[off]=uint8_t(v>>8);cmd[off+1]=uint8_t(v);
                    if(a!=b) ++p423_moving;
                }
            }
            command(cmd,rc.addr);
            if(comm==9) state.sys_x+=2*pad;
            if(comm==8 || comm==11) {state.ux0+=pad;state.ux1+=pad;}
            if(comm==10) state.local_x+=pad;
        }
        p423_target=nullptr;g_raster=saved;
    }
    p423_show=!p423_latest.draws.empty();return matched;
}
bool vdp1_p423_dot(int x,int y,uint16_t& value) {
    if(!p423_show) return false;
    value=x>=0 && x<p423_width() && y>=0 && y<256 ? p423_dots[size_t(y)*p423_width()+x] : 0;
    return true;
}
uint64_t vdp1_p423_age() { return sat_vblanks()-p423_latest.field; }
uint64_t vdp1_p423_interval() { return p423_latest.field-p423_before.field; }
void vdp1_p423_stats() { sat_note("P423 rejects draw_scene=%llu count=%llu initial=%llu attributes=%llu controls=%llu texture=%llu motion=%llu ambiguous=%llu",(unsigned long long)p423_reasons[0],(unsigned long long)p423_reasons[1],(unsigned long long)p423_reasons[2],(unsigned long long)p423_reasons[3],(unsigned long long)p423_reasons[4],(unsigned long long)p423_reasons[5],(unsigned long long)p423_reasons[6],(unsigned long long)p423_reasons[7]); sat_note("P423 candidates=%llu matched=%llu moved_coordinates=%llu",(unsigned long long)p423_candidates,(unsigned long long)p423_matches,(unsigned long long)p423_moving); }

void vdp1_p423_hide(){p423_show=false;}
uint64_t vdp1_p423_moved(){return p423_moving;}
unsigned vdp1_p423_reason(){return p423_vdp1_reason;}

void vdp1_p423_dump(const std::string& path) {
    FILE* f=std::fopen(path.c_str(),"wb");if(!f)return;
    for(const auto* frame:{&p423_before,&p423_latest}){
        uint32_t count=frame->draws.size();std::fwrite(&count,4,1,f);
        for(const auto& draw:frame->draws){
            count=draw.commands.size();std::fwrite(&count,4,1,f);
            for(const auto& cmd:draw.commands){std::fwrite(cmd.b,1,32,f);std::fwrite(&cmd.addr,4,1,f);}
        }
    }
    std::fclose(f);
}
