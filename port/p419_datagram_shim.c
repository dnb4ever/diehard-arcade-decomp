// Test only: P4.16 file datagrams extended to multiple descriptors and bounded
// HTTP request/response files for local IGD fixtures. Never linked/packaged.
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
struct fake { int type,port,eof; struct sockaddr_in remote; size_t sent,read; char* request; char path[1200]; };
static struct fake descriptors[FD_SETSIZE];
static int sequence;
static __thread uint32_t rng=419;
static struct fake* lookup(int fd) { return fd>=0 && fd<FD_SETSIZE && descriptors[fd].type?&descriptors[fd]:NULL; }
static uint64_t now_ns(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
static uint32_t random32(void) { rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng; }
static int setting(const char* key) { const char* v=getenv(key);return v?atoi(v):0; }
int socket(int family,int type,int protocol) {
 int base_type=type&15;
 if(getenv("P416_DATAGRAM_DIR") && family==AF_INET && (base_type==SOCK_DGRAM || base_type==SOCK_STREAM)) {
  int fd=open("/dev/null",O_RDWR|O_CLOEXEC);if(fd<0)return fd;if(fd>=FD_SETSIZE){close(fd);errno=EMFILE;return -1;}
  descriptors[fd]=(struct fake){.type=base_type};rng^=(uint32_t)getpid();return fd;
 }
 int(*real)(int,int,int)=dlsym(RTLD_NEXT,"socket");return real(family,type,protocol);
}
int bind(int fd,const struct sockaddr* address,socklen_t length) {
 struct fake* f=lookup(fd);if(!f){int(*real)(int,const struct sockaddr*,socklen_t)=dlsym(RTLD_NEXT,"bind");return real(fd,address,length);}
 f->port=ntohs(((const struct sockaddr_in*)address)->sin_port);if(!f->port)f->port=20000+(getpid()*17+fd)%30000;
 char path[1200];snprintf(path,sizeof path,"%s/%d",getenv("P416_DATAGRAM_DIR"),f->port);return mkdir(path,0700)&&errno!=EEXIST?-1:0;
}
int getsockname(int fd,struct sockaddr* address,socklen_t* length) {
 struct fake* f=lookup(fd);if(!f){int(*real)(int,struct sockaddr*,socklen_t*)=dlsym(RTLD_NEXT,"getsockname");return real(fd,address,length);}
 struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(f->port),.sin_addr={.s_addr=htonl(INADDR_ANY)}};
 memcpy(address,&a,*length<sizeof a?*length:sizeof a);*length=sizeof a;return 0;
}
struct header { uint64_t delivery;struct sockaddr_in from;uint32_t size; };
ssize_t sendto(int fd,const void* bytes,size_t size,int flags,const struct sockaddr* address,socklen_t length) {
 struct fake* f=lookup(fd);if(!f){ssize_t(*real)(int,const void*,size_t,int,const struct sockaddr*,socklen_t)=dlsym(RTLD_NEXT,"sendto");return real(fd,bytes,size,flags,address,length);}
 if((int)(random32()%10000)<setting("P416_LOSS_BP"))return size;
 int dest=ntohs(((const struct sockaddr_in*)address)->sin_port),jitter=setting("P416_JITTER_MS"),delay=setting("P416_LATENCY_MS");
 if(jitter)delay+=(int)(random32()%(2*jitter+1))-jitter;if(delay<0)delay=0;
 char dir[1200],tmp[1500],path[1400];snprintf(dir,sizeof dir,"%s/%d",getenv("P416_DATAGRAM_DIR"),dest);struct stat st;if(stat(dir,&st))return size;
 struct header h={0};h.delivery=now_ns()+(uint64_t)delay*1000000;h.from.sin_family=AF_INET;h.from.sin_addr.s_addr=htonl(INADDR_LOOPBACK);h.from.sin_port=htons(f->port);h.size=size;
 snprintf(path,sizeof path,"%s/%020llu-%d-%d.pkt",dir,(unsigned long long)h.delivery,getpid(),__sync_fetch_and_add(&sequence,1));snprintf(tmp,sizeof tmp,"%s.tmp",path);
 FILE* file=fopen(tmp,"wb");if(!file)return size;
 uint8_t corrupt[2048];const void* payload=bytes;if(size<=sizeof corrupt&&size&&(int)(random32()%10000)<setting("P416_CORRUPT_BP")){memcpy(corrupt,bytes,size);corrupt[size/2]^=0x40;payload=corrupt;}
 fwrite(&h,1,sizeof h,file);fwrite(payload,1,size,file);fclose(file);rename(tmp,path);return size;
}
ssize_t recvfrom(int fd,void* bytes,size_t size,int flags,struct sockaddr* address,socklen_t* length) {
 struct fake* f=lookup(fd);if(!f){ssize_t(*real)(int,void*,size_t,int,struct sockaddr*,socklen_t*)=dlsym(RTLD_NEXT,"recvfrom");return real(fd,bytes,size,flags,address,length);}
 char dir[1200],chosen[1500]={0};snprintf(dir,sizeof dir,"%s/%d",getenv("P416_DATAGRAM_DIR"),f->port);DIR* d=opendir(dir);if(!d){errno=EAGAIN;return -1;}
 struct dirent* entry;uint64_t first=UINT64_MAX,now=now_ns();
 while((entry=readdir(d))){if(!strstr(entry->d_name,".pkt")||strstr(entry->d_name,".tmp"))continue;uint64_t when=strtoull(entry->d_name,NULL,10);if(when>now||when>=first)continue;first=when;snprintf(chosen,sizeof chosen,"%s/%s",dir,entry->d_name);}
 closedir(d);if(!*chosen){errno=EAGAIN;return -1;}FILE* file=fopen(chosen,"rb");if(!file){errno=EAGAIN;return -1;}
 struct header h;if(fread(&h,1,sizeof h,file)!=sizeof h){fclose(file);unlink(chosen);errno=EAGAIN;return -1;}
 size_t n=fread(bytes,1,size<h.size?size:h.size,file);fclose(file);unlink(chosen);
 if(address&&length){memcpy(address,&h.from,*length<sizeof h.from?*length:sizeof h.from);*length=sizeof h.from;}return n;
}
int connect(int fd,const struct sockaddr* address,socklen_t length) {
 struct fake* f=lookup(fd);if(!f){int(*real)(int,const struct sockaddr*,socklen_t)=dlsym(RTLD_NEXT,"connect");return real(fd,address,length);}
 f->remote=*(const struct sockaddr_in*)address;snprintf(f->path,sizeof f->path,"%s/tcp-%d-%d-%d",getenv("P416_DATAGRAM_DIR"),getpid(),fd,__sync_fetch_and_add(&sequence,1));return 0;
}
int getsockopt(int fd,int level,int option,void* value,socklen_t* size) {
 if(lookup(fd)&&level==SOL_SOCKET&&option==SO_ERROR){*(int*)value=0;*size=sizeof(int);return 0;}
 int(*real)(int,int,int,void*,socklen_t*)=dlsym(RTLD_NEXT,"getsockopt");return real(fd,level,option,value,size);
}
ssize_t send(int fd,const void* bytes,size_t size,int flags) {
 struct fake* f=lookup(fd);if(!f){ssize_t(*real)(int,const void*,size_t,int)=dlsym(RTLD_NEXT,"send");return real(fd,bytes,size,flags);}
 if(f->sent+size>278528){errno=EMSGSIZE;return -1;}char* pending=realloc(f->request,f->sent+size);if(!pending){errno=ENOMEM;return -1;}f->request=pending;memcpy(pending+f->sent,bytes,size);f->sent+=size;
 char tmp[1400],path[1400];snprintf(tmp,sizeof tmp,"%s.tmp",f->path);snprintf(path,sizeof path,"%s.request",f->path);
 FILE* file=fopen(tmp,"wb");if(!file)return -1;fprintf(file,"%d\n",ntohs(f->remote.sin_port));fwrite(f->request,1,f->sent,file);fclose(file);rename(tmp,path);return size;
}
ssize_t recv(int fd,void* bytes,size_t size,int flags) {
 struct fake* f=lookup(fd);if(!f){ssize_t(*real)(int,void*,size_t,int)=dlsym(RTLD_NEXT,"recv");return real(fd,bytes,size,flags);}
 char path[1400];snprintf(path,sizeof path,"%s.response",f->path);FILE* file=fopen(path,"rb");if(!file){errno=EAGAIN;return -1;}
 fseek(file,(long)f->read,SEEK_SET);size_t n=fread(bytes,1,size,file);f->read+=n;if(!n)f->eof=1;fclose(file);return n;
}
static int readable(struct fake* f) {
 if(f->type==SOCK_STREAM){char path[1400];snprintf(path,sizeof path,"%s.response",f->path);return access(path,F_OK)==0;}
 char dir[1200];snprintf(dir,sizeof dir,"%s/%d",getenv("P416_DATAGRAM_DIR"),f->port);DIR* d=opendir(dir);if(!d)return 0;
 int found=0;struct dirent* e;uint64_t now=now_ns();
 while((e=readdir(d)))if(strstr(e->d_name,".pkt")&&!strstr(e->d_name,".tmp")&&strtoull(e->d_name,NULL,10)<=now){found=1;break;}
 closedir(d);return found;
}
int select(int nfds,fd_set* reads,fd_set* writes,fd_set* errors,struct timeval* timeout) {
 int fake=0;for(int fd=0;fd<nfds&&fd<FD_SETSIZE;++fd)if(lookup(fd)&&((reads&&FD_ISSET(fd,reads))||(writes&&FD_ISSET(fd,writes)))){fake=1;break;}
 if(!fake){int(*real)(int,fd_set*,fd_set*,fd_set*,struct timeval*)=dlsym(RTLD_NEXT,"select");return real(nfds,reads,writes,errors,timeout);}
 fd_set requested;if(reads)requested=*reads;uint64_t until=now_ns()+(timeout?(uint64_t)timeout->tv_sec*1000000000+(uint64_t)timeout->tv_usec*1000:20000000);
 for(;;){int count=0;for(int fd=0;fd<nfds&&fd<FD_SETSIZE;++fd){struct fake* f=lookup(fd);if(!f)continue;if(writes&&FD_ISSET(fd,writes))++count;if(reads&&FD_ISSET(fd,&requested)){if(readable(f)){FD_SET(fd,reads);++count;}else FD_CLR(fd,reads);}}if(errors)FD_ZERO(errors);if(count||now_ns()>=until)return count;usleep(1000);}
}
int ioctl(int fd,unsigned long request,...) {
 va_list args;va_start(args,request);void* value=va_arg(args,void*);va_end(args);
 if(lookup(fd)&&request==FIONBIO)return 0;
 int(*real)(int,unsigned long,...)=dlsym(RTLD_NEXT,"ioctl");return real(fd,request,value);
}
int close(int fd) {
 struct fake* f=lookup(fd);if(f){free(f->request);memset(f,0,sizeof *f);}int(*real)(int)=dlsym(RTLD_NEXT,"close");return real(fd);
}
