// Linux verification fixture only: a common RTC origin for separate exact-build
// executables. Never linked into the runtime or included in a distribution.
// cc -shared -fPIC port/p415_fixed_time.c -o work/p415/fixed-time.so
#include <time.h>
time_t time(time_t* out) {
    time_t value = 1794182400;
    if (out) *out = value;
    return value;
}
