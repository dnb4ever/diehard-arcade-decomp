// 30->60 only: age 0 is the previous endpoint, age 1 the coordinated midpoint.
// One game interval of latency; no guest ticks inserted.
static uint64_t p423_outputs,p423_accepted,p423_intermediate,p423_genuine;
static void p423_video_field(uint64_t now,bool shot) {
    (void)now;
    vdp1_p423_field();
    if(vdp1_p423_age()==0)vdp2_p423_field();
    auto interval=vdp1_p423_interval(),age=vdp1_p423_age();
    bool eligible=p423_hz() && interval==2 && age<2;
    float alpha=eligible?float(age)/2:-1.f;
    bool bg=eligible&&vdp2_p423_prepare(alpha);
    auto moved0=vdp1_p423_moved();
    // Run both matchers for independent rejection diagnostics.
    bool actor=vdp1_p423_render(eligible?alpha:-1.f);
    bool matched=bg&&actor;
    if(!matched)vdp1_p423_hide();
    vdp2_p423_enable(matched);
    vdp2_compose(g_frame);
    vdp2_p423_enable(false);
    auto moved=vdp1_p423_moved()-moved0;
    bool genuine=matched&&age==1&&(moved||vdp2_p423_changes());
    ++p423_outputs;p423_accepted+=matched;
    if(eligible&&age==1){++p423_intermediate;p423_genuine+=genuine;}
    if(shot)write_png(g_cfg.out+"/shot-"+std::to_string(g_vblanks)+".png",g_frame);
    movie_frame(g_frame);host_present(g_frame);
    unsigned long long lo=0,hi=0;bool capture=false;
    if(const char* s=std::getenv("STV_P423_CAPTURE"))capture=std::sscanf(s,"%llu:%llu",&lo,&hi)==2&&lo<=g_vblanks&&g_vblanks<=hi;
    if(capture) {
        if(age==0){vdp2_p423_dump(g_cfg.out+"/bg-"+std::to_string(g_vblanks)+".bin");vdp1_p423_dump(g_cfg.out+"/commands-"+std::to_string(g_vblanks)+".bin");}
        write_png(g_cfg.out+"/sample-"+std::to_string(g_vblanks)+".png",g_frame);
        vdp1_p423_hide();Frame real;vdp2_compose(real);
        write_png(g_cfg.out+"/real-"+std::to_string(g_vblanks)+".png",real);
    }
    if(eligible || capture) sat_note("P423_SAMPLE VB=%llu scene=%u interval=%llu age=%llu alpha=%.1f bg=%d bg_reason=%u actor=%d actor_reason=%u matched=%d genuine=%d vertices=%llu bg_fields=%llu",
        (unsigned long long)g_vblanks,ld32(0x06098668),(unsigned long long)interval,(unsigned long long)age,alpha,bg,vdp2_p423_reason(),actor,vdp1_p423_reason(),matched,genuine,(unsigned long long)moved,(unsigned long long)vdp2_p423_changes());
    if(g_vblanks%600==0||g_vblanks==g_cfg.stop_vblanks){vdp1_p423_stats();sat_note("P423 outputs=%llu matched=%llu intermediate=%llu genuine=%llu",(unsigned long long)p423_outputs,(unsigned long long)p423_accepted,(unsigned long long)p423_intermediate,(unsigned long long)p423_genuine);}
}
