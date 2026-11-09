// Test-only file-backed datagram transport for sandboxes denying socket(AF_INET).
// No production build links this. UDP bytes/address semantics are kept; there is
// no kernel UDP/network validation. Independent processes exchange atomic files.
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static int fake=-1, local_port, seq;
static uint32_t rng=416;
static uint64_t now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
static uint32_t random32(void) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
static int setting(const char* key) { const char* v=getenv(key); return v?atoi(v):0; }
int socket(int domain,int type,int protocol) {
 if(getenv("P416_DATAGRAM_DIR") && domain==AF_INET && type==SOCK_DGRAM) { fake=30000; rng^=(uint32_t)getpid(); return fake; }
 int (*real)(int,int,int)=dlsym(RTLD_NEXT,"socket");return real(domain,type,protocol);
}
int bind(int fd,const struct sockaddr* address,socklen_t length) {
 if(fd!=fake) { int (*real)(int,const struct sockaddr*,socklen_t)=dlsym(RTLD_NEXT,"bind");return real(fd,address,length); }
 local_port=ntohs(((const struct sockaddr_in*)address)->sin_port);
 if(!local_port)local_port=20000+getpid()%30000;
 char path[1024];snprintf(path,sizeof path,"%s/%d",getenv("P416_DATAGRAM_DIR"),local_port);
 if(mkdir(path,0700) && errno!=EEXIST)return -1;
 return 0;
}
struct header { uint64_t delivery; struct sockaddr_in from; uint32_t size; };
ssize_t sendto(int fd,const void* bytes,size_t size,int flags,const struct sockaddr* address,socklen_t length) {
 if(fd!=fake) { ssize_t (*real)(int,const void*,size_t,int,const struct sockaddr*,socklen_t)=dlsym(RTLD_NEXT,"sendto");return real(fd,bytes,size,flags,address,length); }
 if((int)(random32()%10000)<setting("P416_LOSS_BP"))return size;
 int dest=ntohs(((const struct sockaddr_in*)address)->sin_port);
 char dir[1024],tmp[1400],path[1300];snprintf(dir,sizeof dir,"%s/%d",getenv("P416_DATAGRAM_DIR"),dest);
 // Like UDP, sends before the receiving port is open may disappear.
 struct stat st;if(stat(dir,&st))return size;
 int jitter=setting("P416_JITTER_MS"),delay=setting("P416_LATENCY_MS");
 if(jitter)delay+=(int)(random32()%(2*jitter+1))-jitter;
 if(delay<0)delay=0;
 struct header h={0};h.delivery=now_ns()+(uint64_t)delay*1000000;h.from.sin_family=AF_INET;h.from.sin_addr.s_addr=htonl(INADDR_LOOPBACK);h.from.sin_port=htons(local_port);h.size=size;
 snprintf(path,sizeof path,"%s/%020llu-%d-%d.pkt",dir,(unsigned long long)h.delivery,getpid(),seq++);
 snprintf(tmp,sizeof tmp,"%s.tmp",path);
 FILE* f=fopen(tmp,"wb");if(!f)return size;
 uint8_t corrupt[2048];const void* payload=bytes;
 if(size<=sizeof corrupt && size && (int)(random32()%10000)<setting("P416_CORRUPT_BP")) {
  memcpy(corrupt,bytes,size);corrupt[size/2]^=0x40;payload=corrupt;
 }
 fwrite(&h,1,sizeof h,f);fwrite(payload,1,size,f);fclose(f);rename(tmp,path);return size;
}
ssize_t recvfrom(int fd,void* bytes,size_t size,int flags,struct sockaddr* address,socklen_t* length) {
 if(fd!=fake) { ssize_t (*real)(int,void*,size_t,int,struct sockaddr*,socklen_t*)=dlsym(RTLD_NEXT,"recvfrom");return real(fd,bytes,size,flags,address,length); }
 char dir[1024],chosen[1400]={0};snprintf(dir,sizeof dir,"%s/%d",getenv("P416_DATAGRAM_DIR"),local_port);
 DIR* d=opendir(dir);if(!d){errno=EAGAIN;return -1;}
 struct dirent* ent;uint64_t first=UINT64_MAX,now=now_ns();
 while((ent=readdir(d))) {
  if(!strstr(ent->d_name,".pkt") || strstr(ent->d_name,".tmp"))continue;
  uint64_t when=strtoull(ent->d_name,NULL,10);if(when>now || when>=first)continue;
  first=when;snprintf(chosen,sizeof chosen,"%s/%s",dir,ent->d_name);
 }
 closedir(d);if(!*chosen){errno=EAGAIN;return -1;}
 FILE* f=fopen(chosen,"rb");if(!f){errno=EAGAIN;return -1;}
 struct header h;if(fread(&h,1,sizeof h,f)!=sizeof h){fclose(f);unlink(chosen);errno=EAGAIN;return -1;}
 size_t n=fread(bytes,1,size<h.size?size:h.size,f);fclose(f);unlink(chosen);
 if(address && length){memcpy(address,&h.from,*length<sizeof h.from?*length:sizeof h.from);*length=sizeof h.from;}
 return n;
}
