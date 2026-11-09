/* Test input only: reproduce the original Sunday SMPC RTC weekday.
 * Used through LD_PRELOAD by round2_verify.py; never linked into the port. */
#include <time.h>
time_t time(time_t *out) { time_t t=1794196800; if(out)*out=t; return t; }
