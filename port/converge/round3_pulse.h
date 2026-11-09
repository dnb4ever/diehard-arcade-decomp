#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct PulseState { uint32_t low, total, run, sample, gate, phase; };
void converge_pulse(void*, struct PulseState*);
uint32_t pulse_read(uint32_t,int);
void pulse_write(uint32_t,int,uint32_t);
void pulse_boundary(void*,const struct PulseState*,unsigned,uint32_t);
#ifdef __cplusplus
}
#endif
