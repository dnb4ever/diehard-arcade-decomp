#include "round3_references.h"
#include <vector>
#include <cstdio>
#include <cstring>
struct Yield {};
struct Point {SH2Context c;std::vector<uint8_t> ram;};
static std::vector<Point> points;
static unsigned polls=0,stop=16,throw_at=0;
extern "C" void __wrap__Z8sh2_pollR10SH2Context(SH2Context& c) {
 ++polls;
 // Model an external signal after the original spent poll. No body instruction
 // or extra safe point drives this fixture. Include repeated low/high runs.
 unsigned signal=polls<3 ? 0 : polls<7 ? 1 : polls<10 ? 0 : polls<stop ? 1 : 2;
 st8(0x060986af,signal);st8(0x060986ae,polls>=stop ? 1 : 0);
 points.push_back({c,{g_wram_h,g_wram_h+0x100000}});
 c.budget=0;
 if(polls==throw_at)throw Yield{};
}
static uint32_t rng=0x53784581;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool equal(const SH2Context&a,const SH2Context&b){return !std::memcmp(&a,&b,sizeof a);}
int main() {
 unsigned vectors=0,checkpoints=0,resumes=0;
 for(unsigned i=0;i<1000;++i) {
  SH2Context initial{};auto *b=(uint8_t*)&initial;
  for(unsigned o=0;o<sizeof initial;o+=4){uint32_t v=next();std::memcpy(b+o,&v,4);}
  initial.cpu=i&1;initial.r[15]=0x060ff000;initial.cont_entry=0;initial.t&=1;initial.budget=int(i%7)-3;
  st32(0x060987bc,next());st8(0x060986af,i%3);st8(0x060986ae,0);
  std::vector<uint8_t> before(g_wram_h,g_wram_h+0x100000);
  stop=12+i%13;polls=0;throw_at=0;points.clear();SH2Context expected=initial;
  reference::ref_06027C74(expected);auto refpoints=points;
  std::vector<uint8_t> after(g_wram_h,g_wram_h+0x100000);
  std::memcpy(g_wram_h,before.data(),before.size());points.clear();polls=0;SH2Context actual=initial;
  p_dh::f_06027C74(actual);
  if(!equal(expected,actual)||std::memcmp(g_wram_h,after.data(),after.size())||points.size()!=refpoints.size()){
   std::fprintf(stderr,"FAIL pulse final vector=%u points=%zu/%zu\n",i,points.size(),refpoints.size());
   for(unsigned j=0;j<16;++j)if(expected.r[j]!=actual.r[j])std::fprintf(stderr,"r%u %08x %08x\n",j,expected.r[j],actual.r[j]);return 1;}
  for(unsigned j=0;j<points.size();++j) {
   if(!equal(points[j].c,refpoints[j].c)||points[j].ram!=refpoints[j].ram){std::fprintf(stderr,"FAIL checkpoint vector=%u poll=%u\n",i,j);return 2;}
  }
  for(unsigned j=0;j<refpoints.size();++j) {

   std::memcpy(g_wram_h,refpoints[j].ram.data(),0x100000);
   SH2Context resume=refpoints[j].c;resume.cont_entry=resume.cont;resume.budget=0;
   polls=j+1;points.clear();p_dh::f_06027C74(resume);
   if(!equal(resume,expected)||std::memcmp(g_wram_h,after.data(),after.size())){std::fprintf(stderr,"FAIL resume vector=%u poll=%u\n",i,j);return 3;}
   ++resumes;
  }
  // Real unwinding drops the native C frame; serialized context + RAM suffice.
  std::memcpy(g_wram_h,before.data(),before.size());actual=initial;points.clear();polls=0;throw_at=1;
  try{p_dh::f_06027C74(actual);return 4;}catch(const Yield&){}
  throw_at=0;actual.cont_entry=actual.cont;actual.budget=0;points.clear();p_dh::f_06027C74(actual);
  if(!equal(actual,expected)||std::memcmp(g_wram_h,after.data(),after.size()))return 5;
  checkpoints+=refpoints.size();++vectors;
 }
 std::printf("PASS pulse: %u complete-context/RAM vectors, %u checkpoints, %u serialized resumes, 1000 forced unwinds; both CPUs, budget edges and post-poll signal changes\n",vectors,checkpoints,resumes);
}
