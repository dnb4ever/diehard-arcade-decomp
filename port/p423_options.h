#pragma once
// Experimental snapshot controls. No saved settings and no guest-visible storage.
#include <cstdint>
#include <cstdlib>
#include <cstring>
inline int p423_width() {
    return 704;
}
inline int p423_hz() {
    static int hz=[] { const char* s=std::getenv("STV_P423_HZ"); int n=s?std::atoi(s):0;
        return n==60 ? n : 0; }(); return hz;
}
inline bool p423_capture() { static bool on=std::getenv("STV_P423_CAPTURE")!=nullptr; return on; }
inline bool p423_active() { return p423_width()!=704 || p423_hz() || p423_capture(); }
inline bool p423_edges() { return false; }
inline bool p423_culling() { return false; }

inline bool p423_background_only=false; // host compositor probe, outside guest state
