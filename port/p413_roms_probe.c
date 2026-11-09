// Test-only adapter. The application links p413_roms.c directly.
#include <stdio.h>
#include "p413_roms.h"
#ifdef P413_PROBE_INTERNALS
#include "p413_roms.c"
int p413_inflate_probe(const unsigned char* in,size_t n,unsigned char* out,size_t cap) {return inflate_raw(in,n,out,cap);}
void p413_sha_probe(const unsigned char* in,size_t n,char* hex) {sha256(in,n,hex);}
#endif
int main(int argc,char** argv) {
    if(argc!=3) {fprintf(stderr,"usage: roms_probe SOURCE DESTINATION\n");return 2;}
    char error[512];int r=dha_prepare_roms(argv[1],argv[2],error,sizeof error);
    printf("%s\n",error);
    return r==1 && dha_roms_ready(argv[2]) ? 0 : 1;
}
