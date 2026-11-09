// Standalone production-I/O probe for local fake STUN and router responders.
#include "net_traversal.h"
#include <cstdio>
int main(int argc,char** argv) {
    if(argc!=3) return 2;
    auto socket=net::udp(); if(socket==net::invalid) { std::puts("SKIP: kernel sockets unavailable"); return 77; }
    net::SocketIO io(socket); net::Transaction nonce{1,2,3,4,5,6,7,8,9,10,11,12};
    auto server=net::resolve(argv[2]);
    int rc=0;
    if(std::string(argv[1])=="stun") {
        auto result=net::stun_discover(io,{server,{server.ip,uint16_t(server.port+1)}},{0x7f000001,7845},nonce);
        std::printf("STUN replies=%u symmetric=%d\n",result.replies,int(result.symmetric)); if(result.replies!=2) rc=1;
    } else {
        auto m=net::map_port(io,server,{0x7f000001,7845},nonce);
        std::printf("mapping kind=%d endpoint=%s lease=%u\n",int(m.kind),net::text(m.external).c_str(),m.lease);
        if(m.kind==net::Mapping::None || !net::renew_mapping(io,m) || !net::renew_mapping(io,m,true)) rc=1;
    }
    net::close(socket); return rc;
}
