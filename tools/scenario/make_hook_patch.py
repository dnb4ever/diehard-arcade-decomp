#!/usr/bin/env python3
"""Generate (never apply) the optional read-only runtime hook against current sources."""
import difflib
from pathlib import Path

HOOK = r'''
// Scenario checker v1.01, opt-in read-only capture. Off unless explicitly requested.
static FILE* scenario_stream() {
    static FILE* f = []() -> FILE* {
        const char* path = std::getenv("STV_SCENARIO_PIPE");
        if (!g_cfg.headless || !path || !*path) return nullptr;
        FILE* result = std::fopen(path, "wb");
        if (!result) { std::perror("STV_SCENARIO_PIPE"); std::exit(2); }
        return result;
    }();
    return f;
}
static bool scenario_video() {
    const char* value = std::getenv("STV_SCENARIO_VIDEO");
    return scenario_stream() && (!value || std::strcmp(value, "0"));
}
static bool scenario_memory() {
    const char* value = std::getenv("STV_SCENARIO_MEMORY");
    return !value || std::strcmp(value, "0");
}
static bool scenario_pointer(uint32_t p) {
    return p >= 0x06000000u && p <= 0x060FFE00u && !(p & 3);
}
static void scenario_capture() {
    FILE* f = scenario_stream();
    if (!f) return;
    std::string actors;
    char text[4096];
    for (unsigned slot = 1; slot <= 6; ++slot) {
        uint32_t task = ld32(0x060995B0u + slot * 4);
        uint32_t ent = scenario_pointer(task) ? ld32(task + 8) : 0;
        bool valid = scenario_pointer(ent);
        std::snprintf(text, sizeof text,
            "%s{\"slot\":%u,\"active\":%s,\"id\":%u,\"hp\":%u,\"x\":%u,\"y\":%u,\"z\":%u}",
            slot == 1 ? "" : ",", slot, valid ? "true" : "false",
            valid ? ld8(ent + 0x34) : 0, valid ? ld16(ent + 0x56) : 0,
            valid ? ld32(ent + 8) : 0, valid ? ld32(ent + 12) : 0,
            valid ? ld32(ent + 16) : 0);
        actors += text;
    }
    bool memory = scenario_memory(), video = scenario_video();
    unsigned w = video ? (unsigned)g_frame.w : 0;
    unsigned h = video ? (unsigned)g_frame.h : 0;
    std::snprintf(text, sizeof text,
        "{\"version\":\"1.01\",\"frame\":%llu,\"scene\":%u,\"mode\":%u,\"arm\":%u,"
        "\"progress\":%u,\"join1\":%u,\"join2\":%u,\"cw\":%u,\"rng\":%u,\"counter\":%u,"
        "\"actors\":[%s],\"ram_bytes\":%u,\"pixel_bytes\":%u,\"width\":%u,\"height\":%u}",
        (unsigned long long)g_vblanks, ld32(0x06098668u), ld16(0x0609979Au),
        ld32(0x06098670u), ld32(0x06098830u), ld8(0x06098674u), ld8(0x06098675u),
        ld16(0x06098798u), ld32(0x0609957Cu), ld32(0x060987BCu), actors.c_str(),
        memory ? 0x200000u : 0, w * h * 4, w, h);
    uint32_t n = (uint32_t)std::strlen(text);
    uint8_t length[4] = {(uint8_t)n, (uint8_t)(n >> 8), (uint8_t)(n >> 16), (uint8_t)(n >> 24)};
    auto write = [&](const void* data, size_t size) {
        if (std::fwrite(data, 1, size, f) != size) { std::perror("scenario capture"); std::exit(2); }
    };
    write(length, 4);
    write(text, n);
    if (memory) { write(g_wram_l, 0x100000); write(g_wram_h, 0x100000); }
    if (video) {
        // Explicit little-endian BGRX transport; independent of the host's byte order.
        std::vector<uint8_t> pixels((size_t)w * h * 4);
        for (size_t i = 0; i < (size_t)w * h; ++i) {
            uint32_t p = g_frame.px[i];
            pixels[i*4] = (uint8_t)p;
            pixels[i*4+1] = (uint8_t)(p >> 8);
            pixels[i*4+2] = (uint8_t)(p >> 16);
            pixels[i*4+3] = 0;
        }
        write(pixels.data(), pixels.size());
    }
    if (std::fflush(f)) { std::perror("scenario capture flush"); std::exit(2); }
}
'''


def main():
    Path('work/scenario_v101').mkdir(parents=True, exist_ok=True)
    path = Path('tools/saturn-recomp/runtime/src/video/video.cpp')
    original = path.read_text()
    if 'static void scenario_capture()' in original:
        # P4.7 installs the hook. Preserve the canonical, reviewed patch and emit
        # the installed compile copy; never add a duplicate hook.
        Path('work/scenario_port_hook.patch').write_text(
            Path('port/runtime-patches/p47-scenario-capture.patch').read_text())
        Path('work/scenario_v101/hook_video.cpp').write_text(original)
        return
    updated = original.replace('static void vblank_in(uint64_t now) {', HOOK+'\nstatic void vblank_in(uint64_t now) {', 1)
    updated = updated.replace('bool compose = shot || host_wants_frame() || movie_on();',
                              'bool compose = shot || host_wants_frame() || movie_on() || scenario_video();', 1)
    updated = updated.replace('    scu_raise(IRQ_VBLANK_IN);', '    scenario_capture(); // Sample before raising the game\'s VBlank-IN interrupt.\n    scu_raise(IRQ_VBLANK_IN);', 1)
    assert updated != original and updated.count('scenario_capture();') == 1
    patch = ''.join(difflib.unified_diff(original.splitlines(True), updated.splitlines(True),
                                       fromfile='a/'+str(path), tofile='b/'+str(path)))
    path = Path('tools/saturn-recomp/runtime/src/machine/bios.cpp')
    original = path.read_text()
    updated = original.replace('tok == "SERVICE" || tok == "SVC"  ? 5 : 0;',
                               'tok == "SERVICE" || tok == "SVC"  ? 5 : tok == "TEST" ? 8 : 0;', 1)
    updated = updated.replace('if (event.kind >= 1 && event.kind <= 5) sys |= kSysBit[event.kind];',
                               'if (event.kind >= 1 && event.kind <= 5) sys |= kSysBit[event.kind];\n        if (event.kind == 8) sys |= 0x04; // Physical TEST switch (still exits to unimplemented BIOS menus).', 1)
    assert updated != original
    patch += ''.join(difflib.unified_diff(original.splitlines(True), updated.splitlines(True),
                                        fromfile='a/'+str(path), tofile='b/'+str(path)))
    Path('work/scenario_port_hook.patch').write_text(patch)
    # A review/compile copy only. Sources in port/ and tools/saturn-recomp/ are untouched.
    Path('work/scenario_v101/hook_video.cpp').write_text(
        Path('tools/saturn-recomp/runtime/src/video/video.cpp').read_text()
        .replace('static void vblank_in(uint64_t now) {', HOOK+'\nstatic void vblank_in(uint64_t now) {', 1)
        .replace('bool compose = shot || host_wants_frame() || movie_on();',
                 'bool compose = shot || host_wants_frame() || movie_on() || scenario_video();', 1)
        .replace('    scu_raise(IRQ_VBLANK_IN);', '    scenario_capture();\n    scu_raise(IRQ_VBLANK_IN);', 1))

if __name__ == '__main__':
    main()
