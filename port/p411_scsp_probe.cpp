#include "saturn.h"
#include "sound.h"
#include <cstdio>
int main() {
 int cases[][5]={{15,0,-3,0,4410},{12,2,1,0,4410},{15,15,7,0,4410},{15,0,0,512,4410},{20,14,7,0,4410}};
 for (auto& c:cases) {
  scsp_reset();scsp_write(0x06,0xffff,2);scsp_write(0x08,31,2);
  scsp_write(0x0A,c[1]<<10|c[0],2);scsp_write(0x10,(c[2]&15)<<11|c[3],2);scsp_write(0,0x1820,2);
  int16_t out[2];for (int i=0;i<64;++i) scsp_sample(0,0,out);scsp_write(0,0x1020,2);
  for (int i=0;i<c[4];++i) scsp_sample(0,0,out);
  scsp_write(0x408,0,2);std::printf("rr=%d krs=%d oct=%d fns=%d samples=%d monitor=%u\n",c[0],c[1],c[2],c[3],c[4],scsp_read(0x408,2)&31);
 }
}
