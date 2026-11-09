#pragma once
#include "saturn/sh2.h"
#include <vector>
#include <initializer_list>
#include "round4_small_vector.h"
#include <algorithm>
#include <cstdint>

namespace converge {
struct Read { uint32_t address, value; int size; };
struct Write { uint32_t address, value; int size; };
struct Byte { uint32_t address; uint8_t value; };
// Each transaction sees the invocation's initial RAM plus its own writes.
struct Transaction {
    SmallVector<Write,24> writes;
    bool capture_reads=false;
    uint32_t output_mask=0;
    uint32_t register_outputs[16]{};
    mutable SmallVector<Read,24> reads;
    uint32_t read(uint32_t address, int size) const;
    uint32_t write(uint32_t address, int size, uint32_t value);
};
struct Span { uint32_t address, size; };
struct Tail { uint32_t target; };
using Body = void (*)(SH2Context&, Transaction&);
using CEntry = uint32_t (*)(uint32_t*);
using Recipe = void (*)(SH2Context&, const uint32_t*, uint32_t);
void invoke(SH2Context& context, Body residue, CEntry readable,
            unsigned arguments, unsigned index, bool mod, bool returns_word=true, std::initializer_list<Span> outputs={});
void invoke_direct(SH2Context&, Body, CEntry, unsigned, unsigned, Recipe, bool returns_word=true, std::initializer_list<int> registers={}, uint32_t output_mask=0, int result_register=0);
uint32_t abi_read(uint32_t, int);
uint32_t effect_read(uint32_t, int);
uint32_t effect_sample(unsigned);
Write effect_store(unsigned);
uint32_t register_output(unsigned);
void private_store(uint32_t,int,uint32_t);
void record(unsigned,int);
void poll_root(SH2Context&,unsigned);
void poll_slot(SH2Context&,unsigned);
void poll_sort(SH2Context&,unsigned,bool);
void poll_transform(SH2Context&,unsigned);
void poll_copy(SH2Context&,unsigned);
void poll_translate(SH2Context&,unsigned);
void poll_pulse(SH2Context&,unsigned);
}
