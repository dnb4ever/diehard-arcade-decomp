// main_stv.cpp — ST-V cart-boot harness for the Die Hard Arcade RECOMP port.
//
// Sits beside the saturn-recomp runtime's own src/app/main.cpp. That one boots a
// Saturn CD (--cue -> cdrom_open -> bios_boot -> 1st-read file). An ST-V title has
// no disc and no IP.BIN: the game program lives in a cartridge and the ST-V BIOS
// copies it into HWRAM, runs a PIC security/HW-init stub, then falls into the game.
//
// This harness replaces that boot path and reuses the entire rest of the runtime
// (the two SH-2s, the SCU/SMPC/VDP1/VDP2/SCSP devices, the HLE BIOS services, the
// slave coroutine, the safe-point poll/interrupt machine) unchanged. It:
//   1. loads the proven linear program image (cart 0x200000..0x28de20) into HWRAM
//      at 0x06004000  (notes/rammap.py: delta +0x05E04000), which also makes the
//      recompiled module identify itself (sh2_identify CRCs the image in memory);
//   2. lays down the BIOS work area the way bios.cpp's bios_boot would: the master
//      and slave vector tables (each slot a fake ROM address the HLE maps back to a
//      service) so the game's reads of *0x06000300 (SYS_SETUINT) &c. dispatch;
//   3. seeds the master CPU as the ST-V reset leaves it (VBR 0x06000000, SR=1,
//      SP 0x06100000 — the ST-V BIOS master stack) and jumps straight to
//      main_init_entry (RAM 0x06005020). Entering there deliberately SKIPS the PIC
//      boot stub (cart 0x20013c..0x200258) and with it the security-CRC self-branch
//      hang (~cart 0x200748) — the task's "stub the security-CRC" requirement, met
//      by not running it rather than patching it.
//   4. runs headless, driven by the recompiled code's own safe-point polls, until
//      --vblanks is reached, a fatal error fires (which dumps where it died), or it
//      hangs (run under `timeout`). With --window it runs in real time instead: the
//      runtime's SDL3 host (host.cpp) shows each field, paces VBlank-IN to the host's
//      clock, plays the SCSP stream and reads the keyboard and gamepads as the ST-V
//      control panel (g_cfg.arcade -> host_stv_panel -> bios.cpp). P4.6,
//      notes/port/p46_realtime.md.
//
// Usage:
//   saturn_stv --cart diehard_main.bin [--base 06004000] [--entry 06005020]
//              [--sp 06100000] [--vblanks N] [--trace] [--out DIR] [--peek ADDR[:W],...]
//   saturn_stv --cart diehard_main.bin --cartrom cart.bin --window [--fullscreen] [--scale N]
//              [--settings FILE] [--unpaced] [--stats SECONDS] [--launcher | --no-launcher]
//
// NOTE on scope: this is a thin boot harness, not a full replacement of saturn_main.
// It does not implement program-start re-entry (sh2_program_start throws an internal
// type this file cannot catch), --write/--cheat/--state/--agent, or the stall watchdog.
// Die Hard's boot does not start a second program before frame_sync_loop, so none of
// that is needed to hit the milestone; a hang is bounded by running under `timeout`.
#include "settings.h"
#include "saturn.h"
#include "host.h"
#include "state.h"
#include "netplay.h"
#include "net_traversal.h"
#include "player.h"
#include <algorithm>
#include "p410_identity.h"
#include "p421_mods.h"
extern bool stv_continue(SH2Context&, uint32_t);
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// The runtime services this harness calls directly (declared in saturn.h / video.h):
void video_init();
void sound_init();
void cd_init();

static uint32_t hx(const char* s) { return (uint32_t)std::strtoul(s, nullptr, 16); }

// The runtime's work-RAM store watch (core.cpp): sh2_watch_store calls this for every store into
// [g_sh2_watch_lo, +g_sh2_watch_len). `host` is the host return address inside the recompiled
// function that stored; map it back to the guest function so a write is named. Used to find which
// master routine fills the slave's sound command ring (the documented step-1 diagnosis).
extern void (*g_sh2_watch_report)(uint32_t, uint32_t, int, const void*);

// The A-bus cartridge ROM window (mmio.cpp). An ST-V title reads code, assets and its
// sound/sequence tables straight out of the cartridge (0x02000000 / cache-through 0x22000000);
// without it mapped those reads float to 0xFFFFFFFF and become garbage/null pointers. We point
// this at the full cart.bin so the game reads real data. cart byte k -> A-bus 0x02000000 + k.
extern const uint8_t* g_cart_rom;
extern uint32_t g_cart_rom_size;
static std::vector<uint8_t> g_cart_rom_buf;

static uint32_t host_to_guest(const void* p) {
    uintptr_t h = (uintptr_t)p, best_h = 0;
    uint32_t best = 0;
    for (int i = 0; i < g_sh2_nmodules; ++i) {
        const SH2Module* m = g_sh2_modules[i];
        for (uint32_t k = 0; k < m->nfuncs; ++k) {
            uintptr_t fh = (uintptr_t)m->funcs[k].fn;
            if (fh <= h && fh > best_h) { best_h = fh; best = m->funcs[k].addr; }
        }
    }
    return best;
}

static void stv_watch_report(uint32_t a, uint32_t v, int size, const void* host) {
    static const char* cf = std::getenv("STV_WATCH_CPU");  // diag: "0"/"1" = only that cpu
    if (cf && (*cf - 48) != g_cpu->cpu) return;
    if (size < 4) v &= (1u << (size * 8)) - 1;
    sat_note("store%-2d %08X = %0*X  in fn %08X (cpu %d, pr %08X, VB %llu)",
             size * 8, a, size * 2, v, host_to_guest(host), g_cpu->cpu, g_cpu->pr,
             (unsigned long long)sat_vblanks());
}

// Lay down the BIOS work area exactly as bios.cpp's bios_boot does for a disc, minus
// the disc-specific parts (IP.BIN copy, 1st-read file, area/BUP-from-IP). The vector
// tables make every slot a "BIOS ROM" address (0x1000+v*4 master, 0x1400+v*4 slave);
// calling one lands in sh2_call_unknown -> bios_call, which maps it to the service.
// The game reads these pointers out of 0x06000000..0x060003FF (SBL's SYS_* macros).
static void install_bios_workarea() {
    for (uint32_t v = 0; v < 256; ++v) {
        st32(0x06000000u + v * 4, 0x00001000u + v * 4);   // master vector table slot
        st32(0x06000400u + v * 4, 0x00001400u + v * 4);   // slave  vector table slot
    }
    st32(0x06000324u, 1);                 // SYS_GETSYSCK: BIOS menu selected 352 dots / 28.6 MHz
    st32(0x06000348u, 0xFFFFFFFFu);       // SYS_GETSCUIM: everything masked at boot
    scu_set_mask(0xFFFFFFFFu);
    onchip_set_clock(true);
    // The US BIOS disables display but retains its 352-dot pixel clock at cart entry.
    st16(0x05F80000u, 0x0001);
    st32(0x06000354u, 0x06000A00u);       // BUP table pointer
    for (uint32_t k = 0; k < 16; ++k) st32(0x06000A00u + k * 4, 0x00003000u + k * 4);

    // ST-V BIOS service pointers. On real hardware the ST-V IPL / PIC boot stub (skipped by
    // this harness, along with the security-CRC hang) installs callable BIOS routines in the
    // 0x06000600 low-vector window; the game reads and calls them (e.g. *0x0600063C from
    // FUN_02201958, and the per-frame control-panel read *0x06000660). With no ST-V BIOS ROM
    // to replicate them we route the whole window at the runtime's ST-V service sentinel base
    // (bios.cpp, 0x4000+off): the control-panel read 0x06000660 gets a real body there (it
    // fills the panel buffer + injects scripted coin/start), every other service stays a benign
    // no-op. A Saturn disc boot never points here, so the ST-V service body is reached only on
    // this harness path. STV_STVBIOS (unset by default) selects bios.cpp's diagnostic overrides.
    for (uint32_t a = 0x06000600u; a < 0x06000700u; a += 4)
        st32(a, 0x00004000u + (a - 0x06000600u));
    // 0x06000650..0x0600065F are BIOS variables inside that window, not vectors. Values as
    // the US BIOS leaves them for a single cart in slot 0 (MAME 0.285, epr-17952a): slot
    // 0x650 = 0, boot reason 0x656 = 1 (power-on), slot-present nibbles 0x658 = 0xFFF0,
    // lamp/SMPC output shadows 0x65A/0x65B = 0 (the cart reads 0x65B, func_02214970).
    st8(0x06000650u, 0);
    st8(0x06000656u, 1);
    st16(0x06000658u, 0xFFF0u);
    st16(0x0600065Au, 0);
    // ROM 0x3080 clears the panel records before BIOS service 0x610 starts.
    for (uint32_t a = 0x06000700u; a < 0x06000780u; a += 4) st32(a, 0);
    // Factory per-cart settings, copied by BIOS 0x4DC0 (slot 0). These bytes
    // are in the cart header; use that source instead of synthesising a report.
    for (unsigned i = 0; i < 8; ++i) st8(0x0600079Eu+i, ld8(0x06004F48u+i));
}

#if defined(__linux__) && defined(SATURN_P417_RELEASE)
#include "p417_linux_frontend.h"
#endif

int main(int argc, char** argv) {
#if defined(__linux__) && defined(SATURN_P417_RELEASE)
    if (!p417::arguments(argc, argv)) return 0;
#endif
    std::string cart, cartrom, out = ".", peek, shots, dump, watch, input, stvinput, wav;
    bool no_sound = false;          // --no-sound: leave the 68000 held (the old render-only behaviour)
    bool spoof_sound_done = false;  // EXPERIMENT (--spoof-sound-done): NOP the 3 frame_sync_loop spins on 0x060E0DD8 bit7
    uint32_t base = 0x06004000u, entry = 0x06005020u, sp = 0x06100000u;
    uint64_t vblanks = 600;
    bool vblanks_given = false;
    std::string mods_dir,mods_selection,mods_zip;bool mods_inspect=false,mods_check=false;
    bool trace = false;
    bool window = false, fullscreen = false, unpaced = false;  // --window: real time, for a player
    bool launcher = false;
    int scale = 0, stats = 0, internal_resolution = 0;
    int net_host = 0, input_delay = 3, net_audio = -1, net_code_port = -1; std::string net_join, net_peer_code, net_code_out, net_stun_servers;
    bool net_no_map=false, net_no_stun=false;
    auto net_number = [](const char* text, int lo, int hi) {
        char* end = nullptr; long n = std::strtol(text, &end, 10);
        if (!*text || *end || n < lo || n > hi) { std::fprintf(stderr, "Invalid netplay number: %s\n", text); std::exit(2); }
        return int(n);
    };
    std::string settings, virtual_input, quick_path, movie_record, movie_play;
    int rewind_seconds = 0; uint64_t rewind_at = 0, rewind_frames = 0;
    bool quick_start = false; uint64_t save_at = 0, load_at = 0;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        bool more = i + 1 < argc;
        if (!std::strcmp(a, "--cart") && more) cart = argv[++i];
        else if (!std::strcmp(a, "--base") && more) base = hx(argv[++i]);
        else if (!std::strcmp(a, "--entry") && more) entry = hx(argv[++i]);
        else if (!std::strcmp(a, "--sp") && more) sp = hx(argv[++i]);
        else if (!std::strcmp(a,"--mods-dir") && more) mods_dir=argv[++i];
        else if (!std::strcmp(a,"--mods") && more) mods_selection=argv[++i];
        else if (!std::strcmp(a,"--mods-install") && more) mods_zip=argv[++i];
        else if (!std::strcmp(a,"--mods-check")) mods_check=true;
        else if (!std::strcmp(a,"--mods-list")) mods_inspect=true;
        else if (!std::strcmp(a, "--vblanks") && more) { vblanks = std::strtoull(argv[++i], nullptr, 10); vblanks_given = true; }
        else if (!std::strcmp(a, "--out") && more) out = argv[++i];
        else if (!std::strcmp(a, "--peek") && more) peek = argv[++i];
        else if (!std::strcmp(a, "--cartrom") && more) cartrom = argv[++i]; // full cart.bin for the A-bus window
        else if (!std::strcmp(a, "--shots") && more) shots = argv[++i];   // N,...: out/shot-N.png at VBlank-IN N
        else if (!std::strcmp(a, "--dump") && more) dump = argv[++i];     // N,...: out/dump-N.bin (VDP1/VDP2 memories)
        else if (!std::strcmp(a, "--watch") && more) watch = argv[++i];   // LO:HI: name every work-RAM store in range
        else if (!std::strcmp(a, "--input") && more) input = argv[++i];   // "VBLANK:BUTTON+BUTTON,..." scripted pad (smpc.cpp)
        else if (!std::strcmp(a, "--stvinput") && more) stvinput = argv[++i]; // "VB:TOKEN,...": ST-V panel timeline into the BIOS service (bios.cpp). TOKEN = START1/START2/COIN/COIN2/SERVICE, or a P1/P2 face/dir token (P1A..P1C, P1UP/DOWN/LEFT/RIGHT, P2*). START/COIN are faithful; face/dir feed raw peripheral words; optional :FRAMES holds the token (e.g. 1400:P1RIGHT:120)
        else if (!std::strcmp(a, "--wav") && more) wav = argv[++i];        // write the SCSP output to a 16-bit stereo WAV
        else if (!std::strcmp(a, "--no-sound")) no_sound = true;           // hold the 68000 (render-only, pre-sound behaviour)
        else if (!std::strcmp(a, "--spoof-sound-done")) spoof_sound_done = true;  // EXPERIMENT
        else if (!std::strcmp(a, "--trace")) trace = true;
        else if (!std::strcmp(a, "--rewind-seconds") && more) rewind_seconds = std::clamp(std::atoi(argv[++i]), 0, 20);
        else if (!std::strcmp(a, "--rewind-at") && more) rewind_at = std::strtoull(argv[++i], nullptr, 10);
        else if (!std::strcmp(a, "--rewind-frames") && more) rewind_frames = std::strtoull(argv[++i], nullptr, 10);
        else if (!std::strcmp(a, "--record-inputs") && more) movie_record = argv[++i];
        else if (!std::strcmp(a, "--replay-inputs") && more) movie_play = argv[++i];
        else if (!std::strcmp(a, "--quick-state") && more) quick_path = argv[++i];
        else if (!std::strcmp(a, "--load-state")) quick_start = true;
        else if (!std::strcmp(a, "--save-at") && more) save_at = std::strtoull(argv[++i], nullptr, 10);
        else if (!std::strcmp(a, "--load-at") && more) load_at = std::strtoull(argv[++i], nullptr, 10);
        else if (!std::strcmp(a, "--netplay-audio") && more) {
            std::string mode=argv[++i];
            if(mode!="low-latency" && mode!="glitch-free") { std::fprintf(stderr,"Invalid netplay audio mode\n"); return 2; }
            net_audio=mode=="glitch-free" ? 1 : 0;
        }
        else if (!std::strcmp(a, "--netplay-code") && more) net_code_port=net_number(argv[++i],0,65535);
        else if (!std::strcmp(a, "--netplay-peer-code") && more) net_peer_code=argv[++i];
        else if (!std::strcmp(a, "--netplay-code-out") && more) net_code_out=argv[++i];
        else if (!std::strcmp(a, "--netplay-stun-servers") && more) net_stun_servers=argv[++i];
        else if (!std::strcmp(a, "--netplay-no-map")) net_no_map=true;
        else if (!std::strcmp(a, "--netplay-no-stun")) net_no_stun=true;
        else if (!std::strcmp(a, "--netplay-host") && more) net_host = net_number(argv[++i], 1, 65535);
        else if (!std::strcmp(a, "--netplay-join") && more) net_join = argv[++i];
        else if (!std::strcmp(a, "--input-delay") && more) input_delay = net_number(argv[++i], 0, 3);
        else if (!std::strcmp(a, "--window")) window = true;               // real time in a window, sound out, keyboard/gamepads as the panel
        else if (!std::strcmp(a, "--launcher")) { launcher = true; window = true; }
        else if (!std::strcmp(a, "--no-launcher")) launcher = false;
        else if (!std::strcmp(a, "--fullscreen")) fullscreen = true;       // with --window (F11 / Alt+Enter switch)
        else if (!std::strcmp(a, "--internal-resolution") && more) internal_resolution = std::clamp(std::atoi(argv[++i]), 1, 3);
        else if (!std::strcmp(a, "--scale") && more) scale = std::atoi(argv[++i]);    // with --window: 320x240 times N
        else if (!std::strcmp(a, "--settings") && more) settings = argv[++i]; // with --window: the key map file ("-": defaults)
        else if (!std::strcmp(a, "--unpaced")) unpaced = true;             // with --window: as fast as the host can (measuring)
        else if (!std::strcmp(a, "--stats") && more) stats = std::atoi(argv[++i]);    // with --window: speed + audio queue every N s
        else if (!std::strcmp(a, "--virtual-input") && more) virtual_input = argv[++i]; // with --window, tests: VB:BUTTONS,... on virtual gamepads (host.cpp)
        else { std::fprintf(stderr, "usage: saturn_stv --cart FILE [--cartrom cart.bin] [--base H] [--entry H] [--sp H] [--vblanks N] [--trace] [--out DIR] [--peek A[:W],...] [--shots N,...] [--dump N,...] [--watch LO:HI] [--input VBLANK:BUTTONS,...] [--stvinput VB:START1,...] [--mods-dir DIR] [--mods none|ID,...] [--mods-list | --mods-check | --mods-install ZIP] [--internal-resolution 1|2|3] [--netplay-host PORT | --netplay-join IPv4:PORT] [--netplay-code PORT --netplay-peer-code CODE|@FILE] [--netplay-code-out FILE] [--netplay-no-map] [--netplay-no-stun] [--netplay-stun-servers HOST:PORT,HOST:PORT] [--netplay-audio low-latency|glitch-free] [--input-delay 0..3] [--window [--launcher | --no-launcher] [--fullscreen] [--scale N] [--settings FILE] [--unpaced] [--stats S]]\n"); return 2; }
    }
    if (cart.empty()) { std::fprintf(stderr, "saturn_stv: --cart is required\n"); return 2; }

    // Config the reused runtime reads: headless (host_open returns true), virtual time,
    // the VBlank stop, the log directory and tracing. --shots/--dump drive video.cpp's
    // VBlank-IN frame capture (PNG) and memory dump straight out of the reused runtime.
    if ((int(net_host!=0)+int(!net_join.empty())+int(net_code_port>=0))>1) { std::fprintf(stderr, "Choose netplay host or join\n"); return 2; }
    g_cfg = SaturnConfig{};
    g_cfg.netplay_base_build=STV_NETPLAY_CODE;
    g_cfg.netplay_build=STV_NETPLAY_CODE;
    g_cfg.netplay_code=net_code_port>=0; g_cfg.netplay_code_port=uint16_t(std::max(0,net_code_port));
    g_cfg.netplay_peer_code=net_peer_code; g_cfg.netplay_code_out=net_code_out;
    g_cfg.netplay_auto_map=!net_no_map; g_cfg.netplay_stun=!net_no_stun;
    if(net_no_map) g_cfg.netplay_map_override=0;
    if(net_no_stun) g_cfg.netplay_stun_override=0;
    if(!net_stun_servers.empty()) { g_cfg.netplay_stun_servers=net_stun_servers; g_cfg.netplay_stun_servers_override=true; }
    g_cfg.netplay_audio_override=net_audio; g_cfg.netplay_glitch_free=net_audio==1;
    g_cfg.netplay_host = net_host; g_cfg.netplay_join = net_join; g_cfg.input_delay = input_delay;
    g_cfg.headless = !window;
    g_cfg.out = out;
    g_cfg.trace = trace;
    g_cfg.stop_vblanks = window && !vblanks_given ? 0 : vblanks;   // a player's run ends when the window closes
    // --window: the runtime's SDL3 host, with the ST-V control panel read from the [arcade] keys and the
    // gamepads (host.cpp). Its own settings file, so a Saturn game's settings.ini is left alone.
    g_cfg.arcade = true;
    g_cfg.fullscreen = fullscreen;
    g_cfg.scale = scale;
    g_cfg.unpaced = unpaced;
    g_cfg.stats = stats;
    g_cfg.title = "Die Hard Arcade";
    g_cfg.launch_info = "Program: " + cart + "\nCartridge: " + cartrom;
    g_cfg.virtual_input = virtual_input;
    g_cfg.settings = !settings.empty() || !window || sat_data_dir().empty() ? settings : sat_data_dir() + "/saturn-recomp/stv.ini";
    g_cfg.quick_path = !quick_path.empty() ? quick_path :
        ((!g_cfg.settings.empty() && g_cfg.settings != "-") ? g_cfg.settings + ".diehard.quickstate" : out + "/diehard.quickstate");
    g_cfg.rewind_seconds = rewind_seconds; g_cfg.rewind_at = rewind_at; g_cfg.rewind_frames = rewind_frames;
    g_cfg.movie_record = movie_record; g_cfg.movie_play = movie_play;
    g_cfg.quick_start = quick_start; g_cfg.quick_save_at = save_at; g_cfg.quick_load_at = load_at;
    g_cfg.shots = shots;
    g_cfg.dump = dump;
    g_cfg.input = input;
    g_cfg.wav = wav;       // sound_init reads this; with --wav the whole run's SCSP output is written
    mods_configure(STV_MOD_TARGET,mods_dir,mods_selection);
    g_cfg.netplay_build=mods_preview_fingerprint(STV_NETPLAY_CODE);
    if(!mods_zip.empty()) { bool ok=mods_install(mods_zip);std::fprintf(stderr,"%s\n",mods_status().c_str());return ok?0:2; }
    if(mods_inspect) { std::printf("target=%s\nmods=%s\n",mods_target().c_str(),mods_directory().c_str());for(auto& m:mods_list())std::printf("%s enabled=%d %s %s\n",m.id.c_str(),m.enabled,m.hash.c_str(),m.error.c_str());return 0; }
    if (launcher && !host_launch(g_cfg)) { net_setup_close(); return 0; }
    if(!mods_open()) { net_setup_close();host_player_error(mods_status());return 2; }
    g_cfg.netplay_build=mods_fingerprint(STV_NETPLAY_CODE);
    if(mods_check) {std::printf("%s\n%s",mods_status().c_str(),mods_set().c_str());net_setup_close();return 0;}

    // --input "VBLANK:BUTTON+BUTTON,...": scripted pad presses, exactly as the runtime's own
    // saturn_main installs them (machine.cpp). smpc.cpp reports these through the SMPC peripheral
    // (INTBACK OREG) and PDR1 direct-mode read. NOTE: this is the Saturn digital-pad model (START,
    // A/B/C, directions, L/R/X/Y/Z); it has NO ST-V coin/service bit -- see notes/port/input_injection_status.md.
    if (!input.empty()) { smpc_input_script(input); sat_note("input script: %s", input.c_str()); }

    // --stvinput "VB:START1,VB:START2,VB:COIN,...": the ST-V equivalent of --input, fed into the
    // ST-V BIOS control-panel service (bios.cpp stv_panel_read) instead of the SMPC pad -- this is
    // how the game actually registers a coin/start (it never reads the SMPC pad). Default timeline:
    // insert a P1 COIN at VBlank 200 (the game's own logic accrues a credit, bios.cpp deliver_coin),
    // then press P1 START at VBlank 340 -- with a credit present this begins a PAID game (the
    // credit>0 branch of the coin-wait handler), not the no-credit demo/continue flow.
    if (!stvinput.empty() && stvinput[0] == '@') {
        FILE* f = std::fopen(stvinput.c_str()+1, "rb");
        if (!f) { std::fprintf(stderr, "Cannot open panel script\n"); return 2; }
        std::string text; char b[4096]; size_t n;
        while ((n=std::fread(b, 1, sizeof b, f))) text.append(b,n);
        std::fclose(f); stvinput=std::move(text);
    }
    if (stvinput.empty() && !window) stvinput = "200:COIN,340:START1";   // a player at the window inserts their own coin
    stv_input_script(stvinput);
    sat_note("ST-V input script: %s", stvinput.c_str());

    // --watch LO:HI: name every store into a work-RAM range (used to find who fills the
    // slave sound ring). Mirrors saturn_main's wiring of the core.cpp store watch.
    if (!watch.empty()) {
        size_t c = watch.find(':');
        uint32_t lo = hx(watch.c_str());
        uint32_t hi = c == std::string::npos ? lo : hx(watch.c_str() + c + 1);
        if (SH2_IS_WRAM_H(lo) || SH2_IS_WRAM_L(lo)) {
            g_sh2_watch_lo = lo & 0xDFFFFFFFu;
            g_sh2_watch_len = hi - lo + 1;
            g_sh2_watch_report = stv_watch_report;
            sat_note("watch: naming stores in %08X..%08X", lo, hi);
        } else {
            std::fprintf(stderr, "saturn_stv: --watch range %08X is not in a work RAM\n", lo);
        }
    }

    // Map the full cartridge ROM into the A-bus window so cartridge reads (code, assets, and
    // the sound/sequence tables the game reads at 0x02700000 etc.) return real data instead of
    // floating high. Without this the game's sound-sequence setup reads 0xFFFFFFFF and builds
    // null function pointers -- the master's "call to 0" blocker. cart byte k -> 0x02000000+k.
    if (!cartrom.empty()) {
        FILE* cf = std::fopen(cartrom.c_str(), "rb");
        if (!cf) { std::fprintf(stderr, "saturn_stv: cannot open cart ROM %s\n", cartrom.c_str()); return 2; }
        std::fseek(cf, 0, SEEK_END);
        long cn = std::ftell(cf);
        std::fseek(cf, 0, SEEK_SET);
        g_cart_rom_buf.resize((size_t)cn);
        if (std::fread(g_cart_rom_buf.data(), 1, (size_t)cn, cf) != (size_t)cn) { std::fprintf(stderr, "saturn_stv: short read on %s\n", cartrom.c_str()); return 2; }
        std::fclose(cf);
        g_cart_rom = g_cart_rom_buf.data();
        g_cart_rom_size = (uint32_t)cn;
        sat_note("cart ROM: %ld bytes mapped to A-bus 02000000..%08X", cn, 0x02000000u + (uint32_t)cn);
    }

    // Devices, in saturn_main's order, minus the disc (cdrom_open/bios_boot).
    g_master = SH2Context{};
    g_master.budget = 256;
    video_init();
    sound_init();
    cd_init();
    onchip_reset(0);

    // Sound bring-up. On real ST-V the BIOS boot stub (skipped by this harness) issues SMPC SNDON to
    // release the sound 68000; the game's sound_init then copies the Sega SCSP driver into sound RAM
    // and talks to it (ring push -> Timer-0 ISR sound_queue_service -> sound_send_command handshake at
    // sound RAM 0x3C20/0x3EFE). Because we bypass that BIOS stub, no SNDON ever fires and the 68000
    // stays held, so the driver never runs and the handshake times out. sound_stv_autostart makes the
    // runtime release the 68000 the moment the driver image is in sound RAM -- the BIOS's SNDON, issued
    // by the harness instead. FORCED (harness-issued), not faithful (the game never issues SNDON here).
    // --no-sound restores the old render-only behaviour (68000 held, as before this change).
    if (!no_sound) { sound_stv_autostart(true); sat_note("sound: 68000 auto-start armed (harness SNDON bridge)"); }
    else sat_note("sound: --no-sound, 68000 held (render-only)");

    // US ST-V IPL 0x268..0x28C, table 0x494: longword fills via ROM 0xD00.
    // The BIOS writes SEGA to all low WRAM and high WRAM 0x4000..0xFFFFF
    // before copying its program and the cartridge. Uninitialised cart globals
    // retain that RAM-test pattern; this is a bus-memory operation, not a seed.
    for (uint32_t a = 0x00200000u; a < 0x00300000u; a += 4) st32(a, 0x53454741u);
    for (uint32_t a = 0x06004000u; a < 0x06100000u; a += 4) st32(a, 0x53454741u);

    // 1. The proven cart->HWRAM program copy (rammap.py: single linear block).
    FILE* f = std::fopen(cart.c_str(), "rb");
    if (!f) { std::fprintf(stderr, "saturn_stv: cannot open cart image %s\n", cart.c_str()); return 2; }
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> img((size_t)n);
    if (std::fread(img.data(), 1, (size_t)n, f) != (size_t)n) { std::fprintf(stderr, "saturn_stv: short read on %s\n", cart.c_str()); return 2; }
    std::fclose(f);
    if (!sh2_mem_write(base, img.data(), img.size())) { std::fprintf(stderr, "saturn_stv: image does not fit at %08X\n", base); return 2; }
    // BIOS 0x471A copies the full 0x90000-byte cart block (header count 0x24000
    // longwords), including the ROM's 0xFF padding beyond our extracted module.
    if (base == 0x06004000u && img.size() <= 0x90000u && g_cart_rom_size >= 0x290000u)
        sh2_mem_write(base + (uint32_t)img.size(), g_cart_rom + 0x200000u + img.size(),
                      0x90000u - img.size());
    sat_note("cart image: %ld bytes copied to HWRAM %08X (delta +%08X vs cart 0x200000)", n, base, base - 0x00200000u + 0x02000000u);

    // 2. The BIOS work area (vector tables + service pointers + masks).
    install_bios_workarea();

    // EXPERIMENT (STV_BOOTMODE=N): the real ST-V boot entry stub (skipped by this harness) picks one
    // of three reset entries that each write a BOOT-MODE value {0,1,2} to 0x060987CC, then jump to
    // main_init_entry. The boot-controller task (handler 0x06005590, which frame_sync_loop installs
    // into the task list via the task-install-batch over the handler array @0x0604EBC0) reads
    // 0x060987CC in its state-0 and indexes the dispatch table @0x0604EBD8 to pick the START STATE of
    // the 10-state boot/attract/play machine func_022016a0 (0x060056AC), into which it then morphs:
    //   mode 0 -> state 3  (warm/soft reset: skips controller/sound/config re-init)
    //   mode 1 -> state 1  (COLD power-on full init: runs func_0201b92 = sound+panel+controller-present+config)
    //   mode 2 -> state 11 (handled by the boot controller itself)
    //   mode 3 -> state 1
    // The harness otherwise leaves 0x060987CC = 0 (the warm path), so the cold-boot init never runs.
    // Setting it reproduces that single missing boot-state write WITHOUT running the security-CRC stub.
    // Default (unset) = current behaviour. FUN_02201958 / frame_sync_loop never touch 0x060987CC, so
    // this value survives until the boot controller consumes it on frame 1.
    // FAITHFUL DEFAULT: a real ST-V power-on is a COLD boot (mode 1 -> boot-state 1 -> func_0201b92
    // full sound+panel+controller+config init). The harness used to default to the warm shortcut
    // (mode 0), which skips that init; that is less faithful than the real power-on. We now default
    // to the cold boot (mode 1) so the ST-V system actually initialises as hardware would. STV_NOSYSBIOS
    // restores the old warm default for A/B regression; STV_BOOTMODE still overrides explicitly.
    bool nosys = (std::getenv("STV_NOSYSBIOS") != nullptr);
    uint32_t boot_v = nosys ? 0u : 1u;
    bool bootmode;
    if (const char* bm = std::getenv("STV_BOOTMODE")) boot_v = (uint32_t)std::strtoul(bm, nullptr, 10);
    st32(0x060987CCu, boot_v);
    bootmode = (boot_v != 0);
    sat_note("ST-V boot-mode 0x060987CC = %u (%s; func_022016a0 start-state select)",
             boot_v, std::getenv("STV_BOOTMODE") ? "STV_BOOTMODE" : nosys ? "warm [STV_NOSYSBIOS]" : "cold power-on default");

    // ST-V BIOS settings word 0x06000798 and area code 0x06000248. install_bios_workarea lays this
    // area down as placeholder vectors, so without these writes the cart reads garbage (0x1798,
    // 0x1248). Earlier notes read bit5 of 0x06000798 as a test-mode bit; the BIOS disassembly and
    // MAME show it is the single-cart bit (clear = multi-cart rotation owned by the BIOS).
    const char* c7 = std::getenv("STV_CFG798");
    // P4.5 (notes/port/p45_bios_floors.md): the BIOS copies its settings word from backup RAM
    // to 0x06000798 and its area code to 0x06000248 before entering the cart. MAME 0.285 with
    // the US BIOS and factory settings shows 0x000808FD and 0x00004008 (area nibble 4 = USA).
    // Bit5 set is the single-cart mode: the cart runs its own attract and game-over loop.
    // STV_CFG798 / STV_CFG248 override (e.g. STV_CFG798=40 = the multi-cart configuration).
    if (c7 || bootmode) {
        uint32_t v = c7 ? hx(c7) : 0x000808FDu; st32(0x06000798u, v);
        sat_note("ST-V BIOS settings 0x06000798 = %08X (bit5 single-cart, bit6 sound)%s", v, c7 ? " [STV_CFG798]" : " [factory default]");
    }
    if (const char* c2 = std::getenv("STV_CFG248")) {
        uint32_t v = hx(c2); st32(0x06000248u, v);
        sat_note("STV_CFG248: region word 0x06000248 = %08X", v);
    } else if (!nosys) {
        st32(0x06000248u, 0x00004008u);
        sat_note("ST-V BIOS area code 0x06000248 = 00004008 (USA)");
    }

    // FAITHFUL: ST-V BIOS panel/credit status word 0x06000654. The real BIOS publishes the
    // control-panel configuration + coin-timer here; the game reads it in scene bootstrap
    // (func_022058a8) and the credit/join scene (func_0220600e). bit9(0x200)=2-player panel
    // present, bit8(0x100)=credit-required (coin-op) boot -> scene 1 credit-wait (clear -> scene 0
    // attract-demo first), bits12/13=force-coin dips (off), low 7 bits=insert-coin countdown
    // (0x7F=idle). Die Hard Arcade = 2-player cabinet, coins required: 0x200|0x100|0x7F = 0x37F.
    // STV_CFG654 overrides (e.g. 0x27F = attract-demo-first); STV_NOSYSBIOS leaves it at placeholder.
    // FAITHFUL CORRECTION (coin->director routing): both readers access this word as a 16-BIT value --
    // scene_bootstrap func_022058a8 does `(u16)R16A(0x06000654) & 0x100`, credit-wait func_0220600e
    // does `RU16A(0x06000654) & 0x200`. On the big-endian SH-2 a 16-bit read of 0x06000654 returns the
    // HIGH halfword, i.e. bytes [0x654][0x655]. The old `st32(0x06000654, 0x37F)` stored 00 00 03 7F, so
    // the 16-bit read was 0x0000 and BOTH the credit-required (0x100) and 2P-panel (0x200) bits read as
    // clear -- scene_bootstrap never took its scene-1 credit branch. Write the status as the 16-bit
    // field the game actually reads (st16 at 0x06000654) so 0x037F lands in [0x654][0x655].
    // This writes only the INITIAL value (low 7 bits = 0x7F). FAITHFUL SUB-FIX #1: bios.cpp's
    // stv_system_bios_tick() now DECREMENTS the low-7-bit insert-coin/attract countdown each frame to
    // 0 (the real BIOS coin/attract timer), so the credit-wait scene (func_0220600e) auto-commits at
    // timer==0 and the attract demo reel runs (scene 1 -> 2 -> 3...) instead of FREEZING at scene 1.
    // That decrement is the faithful default; STV_CFG654 still overrides the initial value, STV_NOSYSBIOS
    // disables the whole BIOS layer (static idle, A/B).
    if (!nosys) {
        const char* c654 = std::getenv("STV_CFG654");
        uint32_t v = c654 ? hx(c654) : 0x037Fu; st16(0x06000654u, (uint16_t)v);
        sat_note("ST-V BIOS panel/credit status 0x06000654 = %04X (2P panel|credit-req, low7=countdown start; bios.cpp decrements it)%s",
                 (uint16_t)v, c654 ? " [STV_CFG654]" : " [faithful default]");
    }

    // 3. Identify and activate the recompiled module now the image is in memory.
    const SH2Module* m = sh2_identify(base);
    if (!m) { std::fprintf(stderr, "saturn_stv: no recompiled module matches the image at %08X (crc mismatch)\n", base); return 2; }
    sh2_activate(m);
    sat_note("module %s active at %08X (%u bytes)", m->name, m->base, m->size);

    // Launcher fallback for the BIOS EEPROM editor. Only settings bytes observed
    // changing in MAME are written; the guest decodes these through service 0x634.
    {
        Settings game;
        if (!g_cfg.settings.empty() && g_cfg.settings != "-") {
            bool found = false;
            std::vector<std::string> problems;
            game.read(settings_load(g_cfg.settings, found), problems);
            for (auto& p : problems) sat_note("%s", p.c_str());
        }
        if (g_cfg.headless) {
            g_cfg.netplay_auto_map = net_no_map ? false : game.flag("netplay", "auto_map");
            g_cfg.netplay_stun = net_no_stun ? false : game.flag("netplay", "stun");
            if(net_stun_servers.empty()) g_cfg.netplay_stun_servers=game.get("netplay", "stun_servers");
            g_cfg.netplay_glitch_free = net_audio < 0 ? game.flag("netplay", "glitch_free") : net_audio != 0;
            player_settings(game);
            g_cfg.internal_resolution = game.number("display", "internal_resolution");
        }
        if (internal_resolution) g_cfg.internal_resolution = internal_resolution;
        unsigned bank = ld32(0x060511DCu + ((ld32(0x06000248u) >> 12) & 15) * 4) * 2;
        if (bank < 8) {
            uint8_t packed = (uint8_t)((game.number("stv", "difficulty") - 1) |
                (game.number("stv", "vitality") << 3) | ((game.number("stv", "lives") - 1) << 6));
            st8(0x0600079Eu + bank, packed);
            // MAME: selected region's second byte, EEPROM file offsets 20/5C
            // for USA, pause USE = bit 0, violence ON = bit 1. Preserve other bits.
            uint32_t flags = 0x0600079Fu + bank;
            st8(flags, (ld8(flags) & ~3u) | (game.flag("stv", "pause_button") ? 1 : 0) |
                (game.flag("stv", "violence") ? 2 : 0));
            sat_note("ST-V game settings: difficulty %d, vitality index %d, lives %d; soft DIP bank %u = %02X",
                game.number("stv", "difficulty"), game.number("stv", "vitality"), game.number("stv", "lives"), bank, packed);
        }
        // MAME Coin Assignments preset 25 (FREE PLAY): EEPROM logical bytes
        // 0x14..15 = 19 01, copied to the BIOS's 0x794..795. 0x798 is unchanged.
        if (game.flag("stv", "free_play")) st16(0x06000794u, 0x1901);
    }

    // EXPERIMENT (--spoof-sound-done): the master frame_sync_loop spins on bit7 of 0x060E0DD8 at
    // three sites (HWRAM 0x0600542A/0x06005488/0x060054C4, each a bf back-branch, bytes 8B F9).
    // This flag rewrites those three bf to nop (00 09) in the loaded image so the master NEVER
    // waits on the slave-idle flag -- a reversible probe of whether that wait is the freeze.
    if (spoof_sound_done) {
        const uint8_t nopbytes[2] = {0x00, 0x09};
        const uint32_t sites[3] = {0x0600542Au, 0x06005488u, 0x060054C4u};
        for (int i = 0; i < 3; ++i) sh2_mem_write(sites[i], nopbytes, 2);
        sat_note("spoof-sound-done: NOPd frame_sync_loop spins at 0600542A/06005488/060054C4");
    }

    // 4. The master at the single-cart power-on trampoline, then main_init_entry.
    g_master.vbr = 0x06000000u;
    // BIOS ROM 0x4500 cart entry and the cart's power-on trampoline
    // 0x06005004..0x0600501E leave these registers at main_init_entry.
    // SP is the BIOS master stack, not the Saturn disc-reset stack.
    g_master.r[0] = entry;
    g_master.r[1] = 0x060987CCu;
    g_master.r[2] = 0x0600064Cu;
    g_master.r[3] = 0xFFFFFFFDu;
    g_master.r[4] = 0x06094000u;
    g_master.r[5] = 0x22120000u;
    g_master.r[7] = 0x06005004u;
    g_master.gbr = 0xFFFFFE00u;
    g_master.pr = 0x00004516u;
    g_master.t = 1;
    g_master.imask = 0;
    g_master.r[15] = sp;
    g_master.budget = 256;
    SH2Func fn = sh2_lookup(entry);
    if (!fn) { std::fprintf(stderr, "saturn_stv: no recompiled function at entry %08X\n", entry); return 2; }
    sat_note("entering main_init_entry at %08X (SP %08X, VBR %08X)", entry, sp, g_master.vbr);

    // Run. A fatal error inside the runtime prints where it died and _Exit(1)s itself
    // (that is the crash report we want). A clean stop (the VBlank limit -> sat_stop)
    // throws the runtime's internal RunStop, which we catch here to end cleanly.
    uint64_t identity = STV_STATE_BUILD;
    for (uint8_t b : img) identity = (identity ^ b) * 1099511628211ull;
    for (uint8_t b : g_cart_rom_buf) identity = (identity ^ b) * 1099511628211ull;
    quick_init(g_cfg.quick_path, mods_fingerprint(identity), stv_continue);
    mods_start();
    if (netplay_active() && (g_cfg.quick_start || !g_cfg.movie_play.empty() || !g_cfg.movie_record.empty() ||
        no_sound || spoof_sound_done || base != 0x06004000u || entry != 0x06005020u || sp != 0x06100000u)) {
        host_player_error("Netplay requires normal power-on, sound and no input movies or quick loads."); return 2;
    }
    uint64_t rom_program = 14695981039346656037ull, rom_cart = rom_program;
    for (uint8_t b : img) rom_program = (rom_program ^ b) * 1099511628211ull;
    for (uint8_t b : g_cart_rom_buf) rom_cart = (rom_cart ^ b) * 1099511628211ull;
    try {
        if (!netplay_open(mods_fingerprint(STV_NETPLAY_CODE), rom_program, rom_cart)) { netplay_finish(); return 2; }
    } catch (...) { netplay_finish(); return 0; } // Window closed while connecting.
    bool resumed = g_cfg.quick_start && quick_restore();
    if (g_cfg.quick_start && !resumed) sat_note("Quick load rejected; starting normally");
    if ((!g_cfg.movie_record.empty() || !g_cfg.movie_play.empty()) && (no_sound || spoof_sound_done)) {
        sat_note("input files require normal sound/execution settings"); return 2;
    }
    if (g_cfg.quick_start && !resumed && !g_cfg.movie_record.empty()) {
        host_player_error("Recording from a save requires a valid quick state from this build. Create a new quick save or record from power-on.");
        return 2;
    }
    if (!vblanks_given && !g_cfg.movie_play.empty()) g_cfg.stop_vblanks = 0;
    if (!player_movie_open(resumed)) {
        host_player_error("Could not open this input file. Replay needs a complete recording from the same build and ROM set. For recording, choose a writable file path.");
        return 2;
    }
    if (!resumed) player_start();
    bool restoring = false;
    for (;;) {
        try {
            if (restoring) { resumed = quick_restore(); restoring = false; }
            if (resumed) quick_run_master();
            else fn(g_master);
            sat_note("main_init_entry returned (unexpected)");
            break;
        } catch (QuickLoad&) {
            restoring = true;
        } catch (...) {
            sat_note("run ended (RunStop: VBlank limit %llu reached, or internal control exception)", (unsigned long long)g_cfg.stop_vblanks);
            break;
        }
    }

    bool netplay_ok = netplay_finish();
    bool movie_ok = player_movie_finish() && netplay_ok;
    quick_rewind_report();
    sat_note("stopped after %llu VBlanks, %.3f s; master pr %08X pc %08X sr %03X",
             (unsigned long long)sat_vblanks(), sat_now() / 1e9, g_master.pr, g_master.pc, sh2_get_sr(g_master));
    // EXPERIMENT (STV_COV): report whether key boot/play functions ran this session (recomp ran[]).
    if (std::getenv("STV_COV")) {
        struct { uint32_t a; const char* n; } want[] = {
            {0x06005590u, "boot_controller(0x02201590)"},
            {0x060056ACu, "func_022016a0(10-state)"},
            {0x06005B92u, "func_0201b92(cold-init)"},
            {0x060098A8u, "scene_bootstrap(cart 0x2058a8)"},
            {0x06008868u, "attract_handler(cart 0x204868)"},
            {0x060085F8u, "attract_credit_join_service(cart 0x2045f8)"},
            {0x06008AFAu, "round_controller_join(cart 0x204afa)"},
            {0x06009232u, "round_controller_dual(cart 0x205232)"},
            {0x06009812u, "scene_run_handler(cart 0x205812)"},
            {0x0600A876u, "scene81_game_sm(cart 0x206876)"},
            {0x0602DB60u, "player_join_arm(cart 0x2229b60)"},
            {0x0602DC00u, "player_join_commit_fsm(cart 0x2229c00)"},
            {0x06013DE4u, "task_list_run"},
            {0x0602E5D0u, "process_spawn_list"},
        };
        for (auto& w : want) {
            int ran_it = -1;
            for (uint32_t k = 0; k < m->nfuncs; ++k)
                if (m->funcs[k].addr == w.a) { ran_it = m->ran ? m->ran[k] : -1; break; }
            sat_note("COV %-30s %08X ran=%d", w.n, w.a, ran_it);
        }
    }
    // EXPERIMENT (STV_COVDUMP): write every function that ran this session to out/cov.txt (one hex
    // addr per line, sorted). Used to diff which handlers run on the cold-boot path vs the warm path.
    if (std::getenv("STV_COVDUMP") && m->ran) {
        FILE* cv = std::fopen((out + "/cov.txt").c_str(), "w");
        if (cv) {
            for (uint32_t k = 0; k < m->nfuncs; ++k)
                if (m->ran[k]) std::fprintf(cv, "%08X\n", m->funcs[k].addr);
            std::fclose(cv);
            sat_note("STV_COVDUMP: wrote ran[] to %s/cov.txt", out.c_str());
        }
    }
    mmio_log_write(out + "/hw-log.txt");
    if (!peek.empty()) {
        size_t i = 0;
        while (i < peek.size()) {
            size_t j = peek.find(',', i);
            std::string item = peek.substr(i, j == std::string::npos ? std::string::npos : j - i);
            i = j == std::string::npos ? peek.size() : j + 1;
            uint32_t a = hx(item.c_str()), w = 1;
            size_t c = item.find(':');
            if (c != std::string::npos) w = (uint32_t)std::strtoul(item.c_str() + c + 1, nullptr, 10);
            std::fprintf(stderr, "  %08X:", a);
            for (uint32_t k = 0; k < w; ++k) std::fprintf(stderr, " %08X", ld32(a + 4 * k));
            std::fprintf(stderr, "\n");
        }
    }
    // Finish the sound side: flush/close the WAV and print the SCSP report (sample count, how many
    // were not silent, the 68000's final PC) -- the evidence that the driver ran and produced audio.
    sound_close();
    std::fflush(stderr);
    std::fflush(stdout);
    // The slave runs on a detached thread parked in its coroutine wait; a normal return
    // would block process teardown on it. Exit at once, as the runtime's own saturn_main
    // does ("the slave's thread is parked: do not wait for it").
    std::_Exit(movie_ok ? 0 : 2);
}
