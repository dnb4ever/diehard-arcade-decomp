// Offline P4.11 renderer probe. Uses captured device data; never runs guest code.
// Usage: p411_render_dump dump-N.bin shown-N.bin out.png [all|sprites|planes|back|no-lines|commands|rbg]
#include "saturn.h"
#include "video.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::vector<uint8_t> read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
static uint16_t word(const uint8_t* p) { return uint16_t(p[0] << 8 | p[1]); }
int main(int argc, char** argv) {
    if (argc < 4) return 2;
    auto d = read(argv[1]), shown = read(argv[2]);
    if (d.size() != 0x80000 + 0x40000 + 0x20 + 0x80000 + 0x1000 + 0x200 || shown.size() != 1024 * 512 * 2) {
        std::fprintf(stderr, "Invalid capture sizes: %zu / %zu\n", d.size(), shown.size()); return 2;
    }
    g_cfg.headless = true;
    const uint8_t* r1 = d.data() + 0xC0000;
    bool fb8 = word(r1) & 1;
    std::string layer = argc > 4 ? argv[4] : "all";
    std::memcpy(g_vdp1_vram, d.data(), 0x80000);
    // The displayed-dot capture has 1024 columns and double-density rows. Restore
    // the ordinary framebuffer used by this game's non-interlaced display.
    if (layer != "commands") for (int y = 0; y < 256; ++y) for (int x = 0; x < 512; ++x) {
        const uint8_t* row = shown.data() + y * 2 * 1024 * 2;
        uint16_t v = fb8 ? uint16_t((word(row + x * 4) & 255) << 8 | (word(row + x * 4 + 2) & 255)) : word(row + x * 2);
        vdp1_fb_write((y * 512 + x) * 2, v, 2);
    }
    vdp1_reg_write(0, word(r1) & 7, 2);
    if (layer == "commands") vdp1_reg_write(4, 1, 2);
    vdp1_vblank_out();
    std::memcpy(g_vdp2_vram, d.data() + 0xC0020, 0x80000);
    std::memcpy(g_vdp2_cram, d.data() + 0x140020, 0x1000);
    std::memcpy(g_vdp2_regs, d.data() + 0x141020, 0x200);
    if (layer == "sprites" || layer == "back") g_vdp2_regs[0x20] = g_vdp2_regs[0x21] = 0;
    if (layer == "planes" || layer == "back") std::memset(g_vdp2_regs + 0xF0, 0, 8);
    if (layer == "rbg") {
        g_vdp2_regs[0x21] &= 0x30;
        std::memset(g_vdp2_regs + 0xF0, 0, 8);
    }
    if (layer == "no-lines") {
        g_vdp2_regs[0x9A] &= ~0x0E;
        g_vdp2_regs[0x9B] &= ~0x0E;
    }
    Frame f;
    vdp2_compose(f);
    write_png(argv[3], f);
    std::printf("%dx%d TVMD=%04X BGON=%04X SPCTL=%04X BKTA=%04X%04X\n", f.w, f.h,
        word(g_vdp2_regs), word(g_vdp2_regs + 0x20), word(g_vdp2_regs + 0xE0), word(g_vdp2_regs + 0xAC), word(g_vdp2_regs + 0xAE));
}
