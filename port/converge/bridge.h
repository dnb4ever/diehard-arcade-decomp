#pragma once
#include "saturn/sh2.h"
#include <vector>
#include <cstdint>

namespace converge {
struct Write { uint32_t address, value; int size; };
struct Byte { uint32_t address; uint8_t value; };
// Each transaction sees the invocation's initial RAM plus its own writes.
struct Transaction {
    std::vector<Write> writes;
    uint32_t read(uint32_t address, int size) const;
    uint32_t write(uint32_t address, int size, uint32_t value);
};
struct Tail { uint32_t target; };
using Body = void (*)(SH2Context&, Transaction&);
using CEntry = uint32_t (*)(uint32_t*);
using Recipe = void (*)(SH2Context&, const uint32_t*, uint32_t);
void invoke(SH2Context& context, Body residue, CEntry readable,
            unsigned arguments, unsigned index, bool mod);
void invoke_direct(SH2Context&, Body, CEntry, unsigned, unsigned, Recipe);
}
