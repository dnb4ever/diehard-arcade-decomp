// Exercise the actual asynchronous lease lifecycle, independently of the game.
#include "saturn.h"
#include "net_traversal.h"
#include <chrono>
#include <cstdio>
#include <thread>
SaturnConfig g_cfg;
int main() {
    g_cfg.netplay_build=123; g_cfg.netplay_host=7845; g_cfg.netplay_stun=false;
    net_setup_begin(7845,false);
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    NetSetupView view;
    do { view=net_setup_view(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); } while(view.busy && std::chrono::steady_clock::now()<until);
    std::printf("%s external=%s\n",view.status.c_str(),view.external.c_str());
    if(!view.ready || view.external.empty()) { net_setup_close(); return 1; }
    std::this_thread::sleep_for(std::chrono::milliseconds(1250));
    net::close(net_setup_take()); net_setup_close(); return 0;
}
