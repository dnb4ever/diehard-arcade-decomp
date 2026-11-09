#include "bridge.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace converge {
static uint32_t physical(uint32_t a) {
    a &= 0xdfffffffu;
    if (a >= 0x06000000u && a < 0x08000000u)
        return 0x06000000u + (a & 0xfffffu);
    return a;
}
[[noreturn]] static void fail(const char* reason) {
    std::fprintf(stderr, "CONVERGE FAIL: %s\n", reason);
    std::abort();
}
static void ram(uint32_t a, int n) {
    auto p = physical(a);
    if (!((p >= 0x06000000u && uint64_t(p)+n <= 0x06100000u) ||
          (p >= 0x00200000u && uint64_t(p)+n <= 0x00300000u)))
        fail("adapter accessed hardware, ROM write, or unmapped memory");
}
uint32_t Transaction::read(uint32_t a, int n) const {
    uint32_t p = physical(a), v = 0;
    // Immutable cartridge reads are allowed; device reads are never speculative.
    if (!(p >= 0x02000000u && uint64_t(p)+n <= 0x03400000u)) ram(a,n);
    for (int i=0; i<n; ++i) {
        uint8_t b = 0; bool found = false;
        for (auto it = writes.rbegin(); it != writes.rend(); ++it) {
            uint32_t offset = p+i-physical(it->address);
            if (offset < unsigned(it->size)) {
                b = it->value >> (8*(it->size-1-offset)); found = true; break;
            }
        }
        if (!found) b = ld8(p+i);
        v = (v<<8)|b;
    }
    return v;
}
uint32_t Transaction::write(uint32_t a, int n, uint32_t v) {
    ram(a,n);
    if (writes.size() >= 1024) fail("unbounded transaction");
    writes.push_back({a,v,n}); return v;
}
static void store(const Write& w) {
    if (w.size==1) st8(w.address,w.value);
    else if (w.size==2) st16(w.address,w.value);
    else if (w.size==4) st32(w.address,w.value);
    else fail("invalid store width");
}
struct Session {
    SH2Context& live;
    SH2Context residue;
    Transaction machine, readable;
    uint32_t sp, target = 0;
    bool flushed = false, called = false, mod;
    bool private_stack(uint32_t a) const {
        return uint32_t(physical(a) - (physical(sp)-0x1000u)) < 0x1100u;
    }
    std::vector<Byte> public_bytes(const Transaction& tx, bool is_c) {
        std::vector<Byte> bytes;
        for (auto w: tx.writes) for (int i=0;i<w.size;++i) {
            if (private_stack(w.address+i)) {
                if (is_c) fail("C effects alias the ABI-private stack");
            } else bytes.push_back({physical(w.address+i),uint8_t(w.value >> (8*(w.size-i-1)))});
        }
        return bytes;
    }
    void flush() {
        if (flushed) fail("effects after tail call");
        auto a = public_bytes(machine,false), b = public_bytes(readable,true);
        if (a.size()!=b.size()) fail("C/ABI write trace lengths differ");
        for (size_t i=0;i<a.size();++i)
            if (a[i].address!=b[i].address || (!mod && a[i].value!=b[i].value))
                fail("C/ABI ordered write trace differs");
        // Retain the original bus widths/order and private spill bytes. Public
        // data comes exclusively from C, including in the deliberate mod build.
        size_t cursor=0;
        for (auto w: machine.writes) {
            uint32_t value=0;
            for (int i=0;i<w.size;++i) {
                uint8_t byte = private_stack(w.address+i) ?
                    uint8_t(w.value >> (8*(w.size-i-1))) : b.at(cursor++).value;
                value=(value<<8)|byte;
            }
            w.value=value; store(w);
        }
        live=residue; flushed=true;
    }
    uint32_t call(uint32_t t, unsigned n, const uint32_t* args) {
        if (called || !target || physical(t)!=physical(target)) fail("unsupported callee/ABI");
        for (unsigned i=0;i<n;++i) {
            uint32_t original=i<4 ? residue.r[4+i] : machine.read(residue.r[15]+4*(i-4),4);
            if (args[i]!=original) fail("C/ABI tail-call arguments differ");
        }
        flush(); called=true;
        // These are tail branches: the stock port does NOT poll at this edge.
        // All ordinary calls/backedges remain in the compiled callee.
        sh2_call(live,t);
        return live.r[0];
    }
};
static thread_local Session* active = nullptr;
struct ActiveSession {
    Session* previous;
    explicit ActiveSession(Session* current):previous(active) { active=current; }
    ~ActiveSession() { active=previous; }
};
static std::array<std::array<uint64_t,2>,256> counts{};
static void report() {
    if (!std::getenv("CONVERGE_COUNTS")) return;
    static bool reported=false;
    if (reported) return;
    reported=true;
    for (unsigned i=0;i<counts.size();++i) if (counts[i][0] || counts[i][1])
        std::fprintf(stderr,"CONVERGE count %u master=%llu slave=%llu\n",i,
            (unsigned long long)counts[i][0],(unsigned long long)counts[i][1]);
}
void invoke(SH2Context& c, Body body, CEntry entry, unsigned n, unsigned index, bool mod) {
    static const bool registered = []{std::atexit(report);return true;}(); (void)registered;
    if (index>=counts.size() || unsigned(c.cpu)>1 || n>16) fail("adapter bounds");
    ++counts[index][c.cpu];
    uint32_t args[16]{};
    for (unsigned i=0;i<n;++i) args[i]=i<4 ? c.r[4+i] : ld32(c.r[15]+4*(i-4));
    Session s{c,c,{}, {},c.r[15],0,false,false,mod};
    try { body(s.residue,s.machine); }
    catch (const Tail& tail) { s.target=tail.target; }
    if (s.residue.budget!=c.budget || s.residue.cpu!=c.cpu) fail("residue changed timing");
    ActiveSession scope(&s);
    uint32_t result=entry(args);
    if (s.target && !s.called) fail("C failed to forward tail call");
    if (!s.called) {
        if (!mod && result!=s.residue.r[0]) {
            std::fprintf(stderr,"CONVERGE index=%u C=%08x ABI=%08x\n",index,result,s.residue.r[0]);
            fail("C/ABI return differs");
        }
        s.flush();
    } else if (s.readable.writes.size() && !s.flushed) fail("unflushed tail effects");
    c.r[0]=result;
}
void invoke_direct(SH2Context& c, Body body, CEntry entry, unsigned n, unsigned index, Recipe recipe) {
    static const bool registered = []{std::atexit(report);return true;}(); (void)registered;
    if (index>=counts.size() || unsigned(c.cpu)>1 || n>4) fail("direct adapter bounds");
    ++counts[index][c.cpu];
    uint32_t args[4]{};
    for (unsigned i=0;i<n;++i) args[i]=c.r[4+i];
    Session s{c,c,{}, {},c.r[15],0,false,false,false};
    ActiveSession scope(&s);
    uint32_t result=entry(args);
    if (!s.readable.writes.empty()) fail("pure direct adapter wrote memory");
    recipe(s.residue,args,result);
    s.residue.r[0]=result;s.residue.pc=c.pr;s.residue.cont_entry=0;
    // Optional validation only. Normal execution never runs the old algorithm.
    if (std::getenv("CONVERGE_AUDIT")) {
        SH2Context expected=c;Transaction tx;body(expected,tx);
        if (!tx.writes.empty() || std::memcmp(&expected,&s.residue,sizeof expected))
            fail("direct ABI recipe differs from complete machine state");
    }
    c=s.residue;
}
}
extern "C" uint32_t dt_read(uint32_t a,int n) {
    if (!converge::active || converge::active->flushed) converge::fail("read after tail");
    return converge::active->readable.read(a,n);
}
extern "C" uint32_t dt_write(uint32_t a,int n,uint32_t v) {
    if (!converge::active || converge::active->flushed) converge::fail("write after tail");
    return converge::active->readable.write(a,n,v);
}
extern "C" uint32_t dt_callv(uint32_t t,unsigned n,const uint32_t* a) {
    if (!converge::active) converge::fail("call outside adapter");
    return converge::active->call(t,n,a);
}
// The ST-V harness deliberately uses _Exit to leave its parked slave thread.
// GNU link wrapping provides diagnostics without changing that existing file.
extern "C" [[noreturn]] void __real__Exit(int);
extern "C" [[noreturn]] void __wrap__Exit(int code) {
    converge::report();
    __real__Exit(code);
}
