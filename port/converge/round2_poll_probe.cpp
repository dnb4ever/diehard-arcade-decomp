#include "p_dh_funcs.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace reference { using namespace p_dh;
void ref_06017164(SH2Context& c) {
    uint32_t x = c.cont_x, v = 0; bool k = c.cont_k;
    uint32_t entry = c.cont_entry; c.cont_entry = 0;
    switch (entry) {
    case 3227: goto R_3227;
    case 3228: goto R_3228;
    default: break;
    }
    ran[241] = 1;
R_3227:;
    c.r[0] = 0x0u;  // 06017164 mov #0,r0
    c.r[6] = 0x3u;  // 06017166 mov #3,r6
    c.r[3] = c.r[0];  // 06017168 mov r0,r3
    c.r[1] = 0x13u;  // 0601716A mov #19,r1
L_0601716C:
    c.t = c.r[4] >> 31; c.r[4] = c.r[4] << 1 | c.t;  // 0601716C rotl r4
    c.r[3] <<= 2;  // 0601716E shll2 r3
    c.t = c.r[4] >> 31; c.r[4] = c.r[4] << 1 | c.t;  // 06017170 rotl r4
    c.r[5] = c.r[4];  // 06017172 mov r4,r5
    c.r[5] &= c.r[6];  // 06017174 and r6,r5
    c.r[0] += c.r[0];  // 06017176 add r0,r0
    c.r[3] |= c.r[5];  // 06017178 or r5,r3
    c.t = c.r[0] >= c.r[3];  // 0601717A cmp/hs r3,r0
    { const bool k = c.t; c.r[1] -= 1; if (k) { goto L_06017186; } }  // 0601717C bt/s 0x06017186 ; add #-1,r1
    c.r[0] += 1;  // 06017180 add #1,r0
    c.r[3] -= c.r[0];  // 06017182 sub r0,r3
    c.r[0] += 1;  // 06017184 add #1,r0
L_06017186:
    c.t = c.r[1] == c.r[6];  // 06017186 cmp/eq r6,r1
    if (!c.t) { c.cont = 3228; c.cont_x = x; c.cont_k = k; SH2_POLL(c); R_3228:; goto L_0601716C; }  // 06017188 bf 0x0601716C
    c.t = c.r[0] & 1; c.r[0] >>= 1;  // 0601718A shlr r0
    { const uint32_t x = c.pr; c.r[0] <<= 8; c.pc = x; return; }  // 0601718C rts ; shll8 r0
}
void ref_0602C106(SH2Context& c) {
    uint32_t x = c.cont_x, v = 0; bool k = c.cont_k;
    uint32_t entry = c.cont_entry; c.cont_entry = 0;
    switch (entry) {
    case 5858: goto R_5858;
    case 5859: goto R_5859;
    default: break;
    }
    ran[576] = 1;
R_5858:;
    c.r[4] = 0xFFFFFFFFu;  // 0602C106 mov #-1,r4
    c.r[5] = 0x060996D4u;  // 0602C108 mov.l 0x0602C130,r5
L_0602C10A:
    c.r[4] += 1;  // 0602C10A add #1,r4
    c.r[0] = c.r[4];  // 0602C10C mov r4,r0
    c.r[2] = (uint32_t)(int8_t)ld8(c.r[0] + c.r[5]);  // 0602C10E mov.b @(r0,r5),r2
    c.t = (c.r[2] & c.r[2]) == 0;  // 0602C110 tst r2,r2
    if (!c.t) { c.cont = 5859; c.cont_x = x; c.cont_k = k; SH2_POLL(c); R_5859:; goto L_0602C10A; }  // 0602C112 bf 0x0602C10A
    c.r[3] = 0x18u;  // 0602C114 mov #24,r3
    c.t = (int32_t)c.r[4] >= (int32_t)c.r[3];  // 0602C116 cmp/ge r3,r4
    if (c.t) { goto L_0602C126; }  // 0602C118 bt 0x0602C126
    c.r[5] += c.r[4];  // 0602C11A add r4,r5
    c.r[1] = 0x1u;  // 0602C11C mov #1,r1
    c.r[0] = c.r[4];  // 0602C11E mov r4,r0
    st8(c.r[5], c.r[1]);  // 0602C120 mov.b r1,@r5
    { const uint32_t x = c.pr; c.r[0] += 8; c.pc = x; return; }  // 0602C122 rts ; add #8,r0
L_0602C126:
    c.r[0] = 0xFFFFFFFFu;  // 0602C126 mov #-1,r0
    { const uint32_t x = c.pr; c.pc = x; return; }  // 0602C128 rts ; nop
}
}
struct Yield {};
static std::vector<SH2Context> points;
static int yield_at=-1;
static bool mutate_occupancy=false;
extern "C" void __wrap__Z8sh2_pollR10SH2Context(SH2Context& c){
 points.push_back(c);
 if(mutate_occupancy && c.cont==5859)st8(0x060996d4+c.r[4]+1,0);
 if(int(points.size())==yield_at)throw Yield{};
 c.budget=0;
}
static uint32_t rng=0x4872a931;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool equal(const SH2Context&a,const SH2Context&b){return !std::memcmp(&a,&b,sizeof a);}
int main(){
 unsigned vectors=0,checkpoints=0,resumes=0;
 const uint32_t edges[]={0,1,2,3,0xffffffffu,0x80000000u,0x7fffffffu,0xffff,0x10000,0x40000000};
 for(unsigned kind=0;kind<2;++kind)for(unsigned i=0;i<2000;++i){
  SH2Context initial{};auto* b=(unsigned char*)&initial;
  for(size_t off=0;off<sizeof initial;off+=4){uint32_t v=next();std::memcpy(b+off,&v,4);}
  initial.cpu=i&1;initial.t&=1;initial.cont_entry=0;initial.budget=0;
  initial.r[4]=i<10?edges[i]:next();initial.r[15]=0x060ff000;
  unsigned length=i%27;
  for(unsigned j=0;j<32;++j)st8(0x060996d4+j,j<length?1+(next()%255):0);
  SH2Context expected=initial,actual=initial;
  auto ref=kind?reference::ref_0602C106:reference::ref_06017164;
  auto fn=kind?p_dh::f_0602C106:p_dh::f_06017164;
  unsigned char before[32];std::memcpy(before,g_wram_h+0x996d4,32);
  points.clear();yield_at=-1;ref(expected);auto expected_points=points;
  unsigned char after[32];std::memcpy(after,g_wram_h+0x996d4,32);
  std::memcpy(g_wram_h+0x996d4,before,32);points.clear();fn(actual);
  if(!equal(expected,actual)||std::memcmp(after,g_wram_h+0x996d4,32)||points.size()!=expected_points.size()){
   std::fprintf(stderr,"FAIL final kind=%u vector=%u\n",kind,i);return 1;}
  for(unsigned j=0;j<points.size();++j)if(!equal(points[j],expected_points[j])){
   std::fprintf(stderr,"FAIL checkpoint kind=%u vector=%u poll=%u\n",kind,i,j);return 1;}
  checkpoints+=points.size();++vectors;
  // Model quick-state unwind and dispatch at each continuation in turn.
  for(unsigned j=0;j<expected_points.size();++j){
   std::memcpy(g_wram_h+0x996d4,before,32);SH2Context resumed=expected_points[j];
   resumed.cont_entry=resumed.cont;resumed.budget=0;points.clear();fn(resumed);
   if(!equal(resumed,expected)||std::memcmp(after,g_wram_h+0x996d4,32)){
    std::fprintf(stderr,"FAIL resume kind=%u vector=%u poll=%u\n",kind,i,j);return 1;}
   ++resumes;
  }
  // Throw from a real checkpoint to verify no hidden C state is needed.
  if(!expected_points.empty()){
   std::memcpy(g_wram_h+0x996d4,before,32);actual=initial;points.clear();yield_at=1;
   try{fn(actual);return 3;}catch(const Yield&){}
   yield_at=-1;actual.cont_entry=actual.cont;actual.budget=0;fn(actual);
   if(!equal(actual,expected))return 4;
  }
 }
 unsigned budget_checks=0;
 const int budgets[]={-2,-1,0,1,2,14,15,16,30,1000};
 for(unsigned kind=0;kind<2;++kind)for(unsigned i=0;i<1500;++i){
  SH2Context initial{};initial.cpu=i&1;initial.r[15]=0x060ff000;
  initial.r[4]=next();initial.pr=next();initial.cont_x=next();initial.cont_k=next();initial.budget=budgets[i%10];
  unsigned char before[32];for(auto& b:before)b=1;before[25]=0;
  mutate_occupancy=kind && i%2;yield_at=-1;
  auto ref=kind?reference::ref_0602C106:reference::ref_06017164;
  auto fn=kind?p_dh::f_0602C106:p_dh::f_06017164;
  std::memcpy(g_wram_h+0x996d4,before,32);SH2Context expected=initial,actual=initial;
  points.clear();ref(expected);auto checkpoints=points;
  unsigned char after[32];std::memcpy(after,g_wram_h+0x996d4,32);
  std::memcpy(g_wram_h+0x996d4,before,32);points.clear();fn(actual);
  if(!equal(expected,actual)||std::memcmp(after,g_wram_h+0x996d4,32)||points.size()!=checkpoints.size())return 5;
  for(unsigned j=0;j<points.size();++j)if(!equal(points[j],checkpoints[j]))return 6;
  ++budget_checks;
 }
 std::printf("PASS: %u budget-boundary and live post-poll occupancy-change vectors\n",budget_checks);
 std::printf("PASS: %u full-context/memory vectors, %u exact checkpoint states, %u serialized continuation resumes; both CPU IDs and forced unwinds\n",vectors,checkpoints,resumes);
}
