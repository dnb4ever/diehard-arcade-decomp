// P4.17 opt-in Linux release frontend. Shared P4.13 converter owns ROM validation.
#pragma once
#include "p413_roms.h"
#include "host_sdl.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_scale.h"
#include <filesystem>
#include <fstream>

namespace p417 {
namespace fs = std::filesystem;
static std::string env(const char* key) { const char* v = std::getenv(key); return v ? v : ""; }

// A folder browser inside the same SDL/ImGui window: no desktop picker or keyboard required.
static bool choose(fs::path& folder, const fs::path& data, const fs::path& beside, std::string& error) {
    SaturnConfig cfg; cfg.arcade = true; cfg.title = "Die Hard Arcade - ROMs needed";
    cfg.settings = (data / "stv.ini").string();
    g_cfg = cfg; // the preview uses the same arcade/per-player bindings as the launcher
    if (!host_window(cfg)) return false;
    auto* win = host_sdl_window(); auto* ren = host_sdl_renderer();
    ImGui::CreateContext(); auto& io = ImGui::GetIO(); io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark(); ImGui::GetStyle().ScaleAllSizes(1.3f); ImGui::GetStyle().FontScaleDpi = 1.5f;
    ImGui_ImplSDL3_InitForSDLRenderer(win, ren); ImGui_ImplSDLRenderer3_Init(ren);
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_AutoAll);
    bool open = true, accepted = false, hidden = false;
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) folder = env("HOME").empty() ? fs::path("/") : fs::path(env("HOME"));
    for (int frame = 0; open; ++frame) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) open = false;
            if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) folder = folder.parent_path();
            host_pad_event(e); ImGui_ImplSDL3_ProcessEvent(&e);
        }
        ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
        auto* vp = ImGui::GetMainViewport(); ImGui::SetNextWindowPos(vp->WorkPos); ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("ROMs needed", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::SeparatorText("Die Hard Arcade - ROMs needed");
        ImGui::TextWrapped("Supply your own MAME diehard (USA) set: diehard.zip, the five loose files, or prepared cart.bin. No ROMs are included.");
        ImGui::TextWrapped("Loose files: fpr19119.13, mpr19115.2, mpr19116.3, mpr19117.4, mpr19118.5.");
        ImGui::TextWrapped("Put ROMs beside the AppImage, or choose their folder below. Verified images are cached in %s.", (data / "roms").c_str());
        ImGui::TextUnformatted("D-pad / left stick: move   A: select   B: parent folder");
        if (!error.empty()) ImGui::TextWrapped("%s", error.c_str());
        if (ImGui::Button("Use this ROM folder")) {
            char why[1024] = {};
            int result = dha_prepare_roms(folder.c_str(), (data / "roms").c_str(), why, sizeof why);
            if (result == 1) { accepted = true; open = false; }
            else error = result < 0 ? why : "ROMs missing here. Choose the folder containing your set.";
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetItemDefaultFocus();
        ImGui::SameLine(); if (ImGui::Button("Quit")) open = false;
        if (ImGui::Button("Home")) folder = env("HOME").empty() ? "/" : env("HOME");
        ImGui::SameLine(); if (ImGui::Button("Package folder")) folder = beside;
        ImGui::SameLine(); if (ImGui::Button("Storage / SD card")) folder = "/run/media";
        ImGui::SameLine(); if (ImGui::Button("Parent folder")) folder = folder.parent_path();
        ImGui::SameLine(); ImGui::Checkbox("Show hidden", &hidden);
        ImGui::TextWrapped("Choose ROM folder: %s", folder.c_str());
        ImGui::BeginChild("folders", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
        std::vector<fs::path> dirs;
        for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; it != end && !ec; it.increment(ec)) {
            if (it->is_directory(ec) && (hidden || it->path().filename().string()[0] != '.')) dirs.push_back(it->path());
        }
        if (ec) { ImGui::TextWrapped("Cannot browse: %s", ec.message().c_str()); ec.clear(); }
        std::sort(dirs.begin(), dirs.end());
        for (auto& dir : dirs) if (ImGui::Selectable((dir.filename().string() + "/").c_str())) folder = dir;
        ImGui::EndChild(); ImGui::End(); ImGui::Render();
        SDL_SetRenderDrawColor(ren, 15, 18, 24, 255); SDL_RenderClear(ren);
        render_imgui_scaled(ImGui::GetDrawData(), ren);
        if (const char* shot = SDL_getenv("SATURN_LAUNCHER_SHOT"); shot && frame == 3) { host_save_picture(shot); open = false; }
        SDL_RenderPresent(ren); SDL_Delay(16);
    }
    ImGui_ImplSDLRenderer3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
    return accepted;
}

static bool arguments(int& argc, char**& argv) {
    // Explicit cartridge commands retain the original harness/headless defaults.
    for (int i = 1; i < argc; ++i) if (!std::strcmp(argv[i], "--cart")) return true;
    fs::path data = sat_data_dir();
    if (data.empty()) { std::fprintf(stderr, "Set HOME or XDG_DATA_HOME for settings and ROM cache.\n"); return false; }
    data /= "saturn-recomp";
    std::error_code ec; fs::create_directories(data / "roms", ec); fs::create_directories(data / "shots", ec);
    if (ec) { std::fprintf(stderr, "Cannot create data folder: %s\n", ec.message().c_str()); return false; }
    fs::path beside = env("APPIMAGE").empty() ? fs::path(env("DHA_PACKAGE_DIR")) : fs::path(env("APPIMAGE")).parent_path();
    if (beside.empty()) beside = fs::canonical("/proc/self/exe", ec).parent_path();
    fs::path chosen; bool force = false;
    std::vector<std::string> extra;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--choose-roms")) force = true;
        else if (!std::strcmp(argv[i], "--rom-dir") && i + 1 < argc) chosen = argv[++i];
        else extra.push_back(argv[i]);
    }
    std::string remembered; std::getline(std::ifstream(data / "rom-folder.txt"), remembered);
    bool ready = false; std::string error;
    std::vector<fs::path> candidates = chosen.empty()
        ? std::vector<fs::path>{data / "roms", beside, remembered, data}
        : std::vector<fs::path>{chosen};
    for (auto& source : candidates) {
        if (force || source.empty()) continue;
        char why[1024] = {}; int result = dha_prepare_roms(source.c_str(), (data / "roms").c_str(), why, sizeof why);
        if (result == 1) { ready = true; chosen = source; break; }
        if (result < 0) error = why;
    }
    if (!ready) {
        if (chosen.empty()) chosen = remembered.empty() ? beside : fs::path(remembered);
        if (!choose(chosen, data, beside, error)) return false;
    }
    // Keep the original external source when reopening an already prepared cache.
    if (chosen != data / "roms") std::ofstream(data / "rom-folder.txt") << chosen.string() << '\n';
    static std::vector<std::string> args;
    static std::vector<char*> pointers;
    args = {argv[0], "--window", "--launcher", "--cart", (data / "roms/diehard_main.bin").string(),
            "--cartrom", (data / "roms/cart.bin").string(), "--out", (data / "shots").string()};
    args.insert(args.end(), extra.begin(), extra.end());
    for (auto& arg : args) pointers.push_back(arg.data());
    pointers.push_back(nullptr); argc = int(args.size()); argv = pointers.data();
    return true;
}
} // namespace p417
