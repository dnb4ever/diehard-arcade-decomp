#pragma once
// Experimental snapshot controls. No saved settings and no guest-visible storage.
#include <cstdint>
#include <cstdlib>
#include <cstring>
inline int p420_width() {
    static int w=[] { const char* s=std::getenv("STV_P420_ASPECT");
        return s && !std::strcmp(s,"16:9") ? 940 : s && !std::strcmp(s,"21:9") ? 1232 : 704; }();
    return w;
}
inline int p420_hz() {
    static int hz=[] { const char* s=std::getenv("STV_P420_HZ"); int n=s?std::atoi(s):0;
        return n==60 || n==120 || n==144 ? n : 0; }(); return hz;
}
inline bool p420_capture() { static bool on=std::getenv("STV_P420_CAPTURE")!=nullptr; return on; }
inline bool p420_active() { return p420_width()!=704 || p420_hz() || p420_capture(); }
inline bool p420_edges() { const char* s=std::getenv("STV_P420_HUD"); return s && !std::strcmp(s,"edges"); }
inline bool p420_culling() { return p420_width()!=704 && std::getenv("STV_P420_CULL"); }

inline bool p420_background_only=false; // host compositor probe, outside guest state
