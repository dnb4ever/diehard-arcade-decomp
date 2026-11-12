// Linux-only test fixture. Never linked or packaged with the game.
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <chrono>
#include <cstdio>
static unsigned frame, slow_returns;
static int interval;
static SDL_Window* window;
static bool fixture() { return SDL_getenv("P424_FAKE_PRESENT"); }
extern "C" bool SDL_SetRenderVSync(SDL_Renderer* r, int requested) {
    interval = requested;
    if (fixture()) { std::fprintf(stderr,"P424 interval=%d frame=%u\n",requested,frame); return true; }
    auto real=(decltype(&SDL_SetRenderVSync))dlsym(RTLD_NEXT,"SDL_SetRenderVSync");
    return real(r,requested);
}
static void click(SDL_Renderer* renderer,bool down) {
    window=SDL_GetRenderWindow(renderer);
    SDL_WarpMouseInWindow(window,20,248);
    SDL_Event e{};e.type=SDL_EVENT_MOUSE_MOTION;e.motion.windowID=SDL_GetWindowID(window);e.motion.x=20;e.motion.y=248;SDL_PushEvent(&e);
    e={};e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.windowID=SDL_GetWindowID(window);e.button.button=SDL_BUTTON_LEFT;e.button.down=down;e.button.x=20;e.button.y=248;SDL_PushEvent(&e);
}
extern "C" bool SDL_RenderPresent(SDL_Renderer* r) {
    auto real=(decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT,"SDL_RenderPresent");
    window=SDL_GetRenderWindow(r);
    if (fixture() && interval && slow_returns<3) { SDL_Delay(50); ++slow_returns; }
    if (SDL_getenv("P424_CLICK_VSYNC") && (frame==8 || frame==9)) click(r,frame==8);
    ++frame; return real(r);
}
extern "C" bool SDL_PollEvent(SDL_Event* e) {
    auto real=(decltype(&SDL_PollEvent))dlsym(RTLD_NEXT,"SDL_PollEvent");
    static auto start=std::chrono::steady_clock::now();
    static bool hidden=false, shown=false;
    double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    if (fixture() && window && !hidden && elapsed>.5) { SDL_HideWindow(window);hidden=true;std::fprintf(stderr,"P424 hide\n"); }
    if (fixture() && window && !shown && elapsed>.9) { SDL_ShowWindow(window);shown=true;std::fprintf(stderr,"P424 show\n"); }
    return real(e);
}
