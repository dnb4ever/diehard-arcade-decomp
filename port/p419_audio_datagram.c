// Audio regression harness: P4.16 datagrams plus a fixed power-on RTC.
// Never linked into a production executable.
#include "p419_datagram_shim.c"
time_t time(time_t* out) {
 const char* epoch=getenv("P419_EPOCH");
 if(!epoch) { time_t (*real)(time_t*)=dlsym(RTLD_NEXT,"time"); return real(out); }
 time_t value=(time_t)strtoll(epoch,NULL,10); if(out)*out=value; return value;
}
