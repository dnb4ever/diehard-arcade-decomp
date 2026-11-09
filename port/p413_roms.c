// P4.13 shared ROM preparation, factored from p411_app_mac.c's mapping and hashes.
// No external tools/libraries. ZIP stored/deflate members are read in memory;
// nothing from an archive is extracted as a pathname. Original inflater/SHA code.
#include "p413_roms.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define CART_SIZE 0x1400000u
#define MAIN_OFF 0x200000u
#define MAIN_SIZE 0x8DE20u
#define ZIP_LIMIT (128u*1024*1024)
static const char CART_HASH[]="309603533747f9e235c57576ec04b4c6274338b4a46efcf1a834fdd4f7d88ea9";
static const char MAIN_HASH[]="da6e3eda0ac78fe5e77cd29d185755292f342d38d5d53ee271f64d6ac1ec970c";
static FILE* open_file(const char* path,const char* mode) {
#ifdef _WIN32
    wchar_t wpath[32768],wmode[16];
    if (!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wpath,32768) ||
        !MultiByteToWideChar(CP_UTF8,0,mode,-1,wmode,16)) return NULL;
    return _wfopen(wpath,wmode);
#else
    return fopen(path,mode);
#endif
}
static int replace_file(const char* src,const char* dst) {
#ifdef _WIN32
    wchar_t a[32768],b[32768];
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,src,-1,a,32768) &&
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,dst,-1,b,32768) &&
        MoveFileExW(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
    return rename(src,dst)==0;
#endif
}
static int path_join(char* out,size_t cap,const char* dir,const char* name) {
    int n=snprintf(out,cap,"%s/%s",dir,name); return n>=0 && (size_t)n<cap;
}
static uint32_t rotate(uint32_t x,unsigned n) { return x>>n | x<<(32-n); }
static void sha256(const unsigned char* data,size_t size,char hex[65]) {
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    size_t blocks=(size+9+63)/64;
    for (size_t block=0;block<blocks;++block) {
        unsigned char bytes[64]={0}; uint32_t w[64];
        for (unsigned i=0;i<64;++i) {
            size_t at=block*64+i;
            if (at<size) bytes[i]=data[at]; else if (at==size) bytes[i]=0x80;
            if (at>=blocks*64-8) bytes[i]=(unsigned char)(((uint64_t)size*8)>>(8*(blocks*64-1-at)));
        }
        for (unsigned i=0;i<16;++i) w[i]=(uint32_t)bytes[4*i]<<24 | (uint32_t)bytes[4*i+1]<<16 | (uint32_t)bytes[4*i+2]<<8 | bytes[4*i+3];
        for (unsigned i=16;i<64;++i) {
            uint32_t a=w[i-15],b=w[i-2];
            w[i]=w[i-16]+(rotate(a,7)^rotate(a,18)^(a>>3))+w[i-7]+(rotate(b,17)^rotate(b,19)^(b>>10));
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (unsigned i=0;i<64;++i) {
            uint32_t t=hh+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i];
            uint32_t u=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
            hh=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
    }
    for (unsigned i=0;i<8;++i) snprintf(hex+8*i,9,"%08x",h[i]);
}
static unsigned char* read_all(const char* path,size_t limit,size_t* length) {
    FILE* f=open_file(path,"rb"); if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) { fclose(f);return NULL; }
    long n=ftell(f); rewind(f);
    if (n<=0 || (uint64_t)n>limit) { fclose(f);return NULL; }
    unsigned char* b=(unsigned char*)malloc((size_t)n+1);
    if (!b) { fclose(f);return NULL; }
    int ok=fread(b,1,(size_t)n,f)==(size_t)n; fclose(f);
    if (!ok) {free(b);return NULL;} *length=(size_t)n;return b;
}
static int read_exact(const char* path,unsigned char* out,size_t size) {
    FILE* f=open_file(path,"rb"); if (!f)return 0;
    int ok=fread(out,1,size,f)==size && fgetc(f)==EOF;fclose(f);return ok;
}
// A bounded RFC1951 bit reader and canonical Huffman decoder (max 15 bits).
typedef struct {const unsigned char* data;size_t size,at;uint32_t bits;unsigned count;int bad;} Bits;
static unsigned take(Bits* b,unsigned n) {
    while (b->count<n) {
        if (b->at==b->size) {b->bad=1;return 0;}
        b->bits|=(uint32_t)b->data[b->at++]<<b->count;b->count+=8;
    }
    unsigned v=b->bits&((1u<<n)-1);b->bits>>=n;b->count-=n;return v;
}
typedef struct {unsigned count[16],symbol[288];} Huffman;
static int tree(Huffman* h,const unsigned* lengths,unsigned n) {
    memset(h,0,sizeof *h);
    for (unsigned i=0;i<n;++i) {if(lengths[i]>15)return 0;h->count[lengths[i]]++;}
    if (h->count[0]==n)return 0;
    int left=1;for(unsigned i=1;i<=15;++i){left=2*left-(int)h->count[i];if(left<0)return 0;}
    unsigned offsets[16]={0};for(unsigned i=1;i<15;++i)offsets[i+1]=offsets[i]+h->count[i];
    for(unsigned i=0;i<n;++i)if(lengths[i])h->symbol[offsets[lengths[i]]++]=i;
    return 1;
}
static int decode(Bits* b,const Huffman* h) {
    unsigned code=0,first=0,index=0;
    for(unsigned len=1;len<=15;++len) {
        code|=take(b,1);if(b->bad)return -1;
        if(code>=first && code-first<h->count[len])return (int)h->symbol[index+code-first];
        index+=h->count[len];first=(first+h->count[len])<<1;code<<=1;
    }
    b->bad=1;return -1;
}
static int inflate_raw(const unsigned char* in,size_t size,unsigned char* out,size_t capacity) {
    static const unsigned lb[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const unsigned le[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const unsigned db[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const unsigned de[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    Bits b={in,size,0,0,0,0};size_t pos=0;unsigned last;
    do {
        last=take(&b,1);unsigned type=take(&b,2);if(b.bad)return 0;
        if(type==0) {
            b.bits=0;b.count=0;unsigned len=take(&b,16),inverse=take(&b,16);
            if(b.bad || (len^inverse)!=65535 || len>capacity-pos)return 0;
            for(unsigned i=0;i<len;++i)out[pos++]=(unsigned char)take(&b,8);
            if(b.bad)return 0;
            continue;
        }
        Huffman lit,dist;unsigned lengths[320]={0};
        if(type==1) {
            for(unsigned i=0;i<288;++i)lengths[i]=i<144?8:i<256?9:i<280?7:8;
            if(!tree(&lit,lengths,288))return 0;
            for(unsigned i=0;i<32;++i)lengths[i]=5;
            if(!tree(&dist,lengths,32))return 0;
        } else if(type==2) {
            // RFC1951 code-length order, explicitly independent of alphabet order.
            static const unsigned code_order[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
            unsigned nl=take(&b,5)+257,nd=take(&b,5)+1,nc=take(&b,4)+4;
            if(nl>286 || nd>32)return 0;
            unsigned cl[19]={0};for(unsigned i=0;i<nc;++i)cl[code_order[i]]=take(&b,3);
            Huffman codes;if(b.bad || !tree(&codes,cl,19))return 0;
            unsigned i=0;
            while(i<nl+nd) {
                int sym=decode(&b,&codes);if(sym<0)return 0;
                if(sym<16)lengths[i++]=(unsigned)sym;
                else {
                    unsigned n,value=0;
                    if(sym==16) {if(!i)return 0;n=take(&b,2)+3;value=lengths[i-1];}
                    else if(sym==17)n=take(&b,3)+3;
                    else if(sym==18)n=take(&b,7)+11;
                    else return 0;
                    if(b.bad || n>nl+nd-i)return 0;
                    while(n--)lengths[i++]=value;
                }
            }
            if(!lengths[256] || !tree(&lit,lengths,nl))return 0;
            // A literal-only block can encode an unused single distance code.
            if(!tree(&dist,lengths+nl,nd)) {memset(&dist,0,sizeof dist);}
        } else return 0;
        for(;;) {
            int sym=decode(&b,&lit);if(sym<0)return 0;
            if(sym<256) {if(pos==capacity)return 0;out[pos++]=(unsigned char)sym;continue;}
            if(sym==256)break;
            if(sym>285)return 0;
            unsigned len=lb[sym-257]+take(&b,le[sym-257]);
            int distance=decode(&b,&dist);if(distance<0 || distance>29)return 0;
            unsigned back=db[distance]+take(&b,de[distance]);
            if(b.bad || back>pos || len>capacity-pos)return 0;
            for(unsigned i=0;i<len;++i){out[pos]=out[pos-back];++pos;}
        }
    }while(!last);
    return !b.bad && pos==capacity;
}
static unsigned u16(const unsigned char* p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t u32(const unsigned char* p){return u16(p)|(uint32_t)u16(p+2)<<16;}
static int zip_member(const unsigned char* z,size_t size,const char* name,unsigned char* out,size_t capacity) {
    if(size<22)return 0;
    size_t low=size>65557?size-65557:0,eocd=size-22;
    for(;;){if(u32(z+eocd)==0x06054b50 && eocd+22+u16(z+eocd+20)==size)break;if(eocd==low)return 0;--eocd;}
    if(u16(z+eocd+4)||u16(z+eocd+6)||u16(z+eocd+8)!=u16(z+eocd+10))return 0;
    size_t at=u32(z+eocd+16),central_size=u32(z+eocd+12);
    if(at>eocd || central_size>eocd-at)return 0;
    size_t end=at+central_size;
    for(unsigned i=0;i<u16(z+eocd+10);++i) {
        if(at>end || end-at<46 || u32(z+at)!=0x02014b50)return 0;
        unsigned n=u16(z+at+28),extra=u16(z+at+30),comment=u16(z+at+32);
        if((size_t)n+extra+comment>end-at-46)return 0;
        if(n==strlen(name) && !memcmp(z+at+46,name,n)) {
            size_t offset=u32(z+at+42),compressed=u32(z+at+20);
            if(u16(z+at+8)&1 || u32(z+at+24)!=capacity || offset>size || size-offset<30 || u32(z+offset)!=0x04034b50)return 0;
            size_t header=30u+u16(z+offset+26)+u16(z+offset+28);
            if(header>size-offset || compressed>size-offset-header)return 0;
            unsigned method=u16(z+at+10);const unsigned char* bytes=z+offset+header;
            if(method==0 && compressed==capacity){memcpy(out,bytes,capacity);return 1;}
            return method==8 && inflate_raw(bytes,compressed,out,capacity);
        }
        at+=46u+n+extra+comment;
    }
    return 0;
}
static int member(const unsigned char* zip,size_t zip_size,const char* dir,const char* name,unsigned char* out,size_t size) {
    if(zip)return zip_member(zip,zip_size,name,out,size);
    char path[32768];return path_join(path,sizeof path,dir,name) && read_exact(path,out,size);
}
static int build_set(const unsigned char* zip,size_t zip_size,const char* dir,unsigned char* cart) {
    static const char* names[4]={"mpr19115.2","mpr19116.3","mpr19117.4","mpr19118.5"};
    unsigned char* fpr=(unsigned char*)malloc(0x100000);if(!fpr)return 0;
    memset(cart,255,CART_SIZE);int ok=member(zip,zip_size,dir,"fpr19119.13",fpr,0x100000);
    if(ok) {
        for(size_t i=0;i<0x100000;++i)cart[1+2*i]=fpr[i];
        memcpy(cart+0x200000,fpr,0x100000);memcpy(cart+0x300000,fpr,0x100000);
    }
    for(unsigned k=0;ok && k<4;++k) {
        unsigned char* b=cart+0x400000+0x400000*k;
        ok=member(zip,zip_size,dir,names[k],b,0x400000);
        if(ok)for(size_t i=0;i<0x400000;i+=2){unsigned char t=b[i];b[i]=b[i+1];b[i+1]=t;}
    }
    free(fpr);return ok;
}
static int write_atomic(const char* dir,const char* name,const unsigned char* data,size_t size) {
    char path[32768],tmp[32768];
    if(!path_join(path,sizeof path,dir,name))return 0;
    int n=snprintf(tmp,sizeof tmp,"%s.part",path);if(n<0 || (size_t)n>=sizeof tmp)return 0;
    FILE* f=open_file(tmp,"wb");if(!f)return 0;
    int ok=fwrite(data,1,size,f)==size;ok=(fclose(f)==0)&&ok;
    return ok && replace_file(tmp,path);
}
int dha_roms_ready(const char* dir) {
    char path[32768],hex[65];size_t n;
    if(!path_join(path,sizeof path,dir,"cart.bin"))return 0;
    unsigned char* b=read_all(path,CART_SIZE,&n);if(!b)return 0;
    sha256(b,n,hex);int ok=n==CART_SIZE && !strcmp(hex,CART_HASH);free(b);if(!ok)return 0;
    if(!path_join(path,sizeof path,dir,"diehard_main.bin"))return 0;
    b=read_all(path,MAIN_SIZE,&n);if(!b)return 0;
    sha256(b,n,hex);ok=n==MAIN_SIZE && !strcmp(hex,MAIN_HASH);free(b);return ok;
}
int dha_prepare_roms(const char* src,const char* destination,char* error,size_t cap) {
    char path[32768],hex[65];unsigned char* zip=NULL;size_t zip_size=0;
    unsigned char* cart=(unsigned char*)malloc(CART_SIZE);int result=0,ok=0;
    const char* message="No usable diehard.zip or loose ROM files were found in that folder.";
    if(!cart){message="Not enough memory to prepare ROMs.";result=-1;goto done;}
    // Prefer prepared cart data; derive the program slice again after verification.
    if(path_join(path,sizeof path,src,"cart.bin"))ok=read_exact(path,cart,CART_SIZE);
    if(!ok) {
        zip=read_all(src,ZIP_LIMIT,&zip_size);
        if(!zip && path_join(path,sizeof path,src,"diehard.zip"))zip=read_all(path,ZIP_LIMIT,&zip_size);
        ok=build_set(zip,zip_size,src,cart);
    }
    if(!ok)goto done;
    sha256(cart,CART_SIZE,hex);
    if(strcmp(hex,CART_HASH)) {result=-1;message="ROMs do not match the supported MAME diehard (USA) set. Use a verified diehard.zip.";goto done;}
    sha256(cart+MAIN_OFF,MAIN_SIZE,hex);
    if(strcmp(hex,MAIN_HASH)) {result=-1;message="The cartridge program does not match the supported revision.";goto done;}
    if(!write_atomic(destination,"cart.bin",cart,CART_SIZE) || !write_atomic(destination,"diehard_main.bin",cart+MAIN_OFF,MAIN_SIZE)) {
        result=-1;message="Could not write prepared ROMs into the app data folder.";goto done;
    }
    result=1;message="ROM preparation complete.";
done:
    if(error && cap)snprintf(error,cap,"%s",message);
    free(zip);free(cart);return result;
}
