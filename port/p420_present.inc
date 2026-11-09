// Snapshot-only host presentation experiment. Included by video.cpp.
// A complete background/palette/mode change rejects interpolation for this pair.
static bool p420_background_stable;
static Frame p420_background;
static std::vector<uint8_t> p420_palette;
static uint64_t p420_next_present;
static Frame p420_subframe;
static uint64_t p420_outputs, p420_accepted;
static void p420_video_field(uint64_t now,bool shot) {
    vdp1_p420_field();
    if(p420_hz() && vdp1_p420_age()==0) {
        Frame current;
        p420_background_only=true;vdp2_compose(current);p420_background_only=false;
        std::vector<uint8_t> palette(g_vdp2_cram,g_vdp2_cram+sizeof g_vdp2_cram);
        p420_background_stable=current.w==p420_background.w && current.h==p420_background.h &&
            current.px==p420_background.px && palette==p420_palette;
        p420_background=std::move(current);p420_palette=std::move(palette);
    }
    auto render=[&](double fraction,Frame& f) {
        uint64_t interval=vdp1_p420_interval(),age=vdp1_p420_age();
        float alpha=p420_hz() && p420_background_stable && interval>=1 && interval<=4 && age<interval ?
            float((age+fraction)/interval) : -1.f;
        bool matched=vdp1_p420_render(alpha);
        if(matched) ++p420_accepted;
        vdp2_compose(f);return matched;
    };
    render(0,g_frame);
    if(shot) write_png(g_cfg.out+"/shot-"+std::to_string(g_vblanks)+".png",g_frame);
    movie_frame(g_frame);
    int hz=p420_hz();
    if(hz<=60 || g_cfg.headless) {
        host_present(g_frame);++p420_outputs;
        // Still generate requested subfields headlessly for CPU evidence/captures.
        if(hz<=60) hz=0;
    }
    bool capture=false;unsigned long long lo=0,hi=0;
    if(const char* s=std::getenv("STV_P420_CAPTURE"))
        capture=std::sscanf(s,"%llu:%llu",&lo,&hi)==2 && lo<=g_vblanks && g_vblanks<=hi;
    int slot=0;
    if(hz) {
        uint64_t period=video_frame_ns();
        if(p420_next_present<now || p420_next_present>=now+period) p420_next_present=now;
        while(p420_next_present<now+period) {
            double fraction=double(p420_next_present-now)/period;
            bool matched=render(fraction,p420_subframe);
            if(!g_cfg.headless) {host_pace(p420_next_present);host_present(p420_subframe);}
            if(capture) write_png(g_cfg.out+"/sample-"+std::to_string(g_vblanks)+"-"+std::to_string(slot)+".png",p420_subframe);
            if(capture) sat_note("P420_SAMPLE VB=%llu slot=%d fraction=%.6f interval=%llu age=%llu background=%d matched=%d",
                (unsigned long long)g_vblanks,slot,fraction,(unsigned long long)vdp1_p420_interval(),(unsigned long long)vdp1_p420_age(),p420_background_stable,matched);
            ++p420_outputs;++slot;
            p420_next_present+=1000000000ull/hz;
        }
    } else if(capture) write_png(g_cfg.out+"/sample-"+std::to_string(g_vblanks)+"-0.png",g_frame);
    if(g_vblanks%600==0 || g_vblanks==g_cfg.stop_vblanks) {
        vdp1_p420_stats();sat_note("P420 outputs=%llu accepted_compositions=%llu",(unsigned long long)p420_outputs,(unsigned long long)p420_accepted);
    }
}
