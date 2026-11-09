// Linux-only verification helper: real SDL events enter and leave the pause feature panel.
#include <SDL3/SDL.h>
#include <dlfcn.h>
static int frame;
extern "C" bool SDL_RenderPresent(SDL_Renderer* renderer) {
    auto real=(decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT,"SDL_RenderPresent");
    auto key=[](SDL_Scancode code,bool down) {
        SDL_Event e{};e.type=down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        e.key.scancode=code;e.key.key=SDL_GetKeyFromScancode(code,SDL_KMOD_NONE,false);e.key.down=down;SDL_PushEvent(&e);
    };
    if(frame==100 || frame==135 || frame==140) key(SDL_SCANCODE_ESCAPE,true);
    if(frame==101 || frame==136 || frame==141) key(SDL_SCANCODE_ESCAPE,false);
    if(frame==106 || frame==109 || frame==112) key(SDL_SCANCODE_DOWN,true);
    if(frame==107 || frame==110 || frame==113) key(SDL_SCANCODE_DOWN,false);
    if(frame==115) key(SDL_SCANCODE_RETURN,true);
    if(frame==116) key(SDL_SCANCODE_RETURN,false);
    ++frame;return real(renderer);
}
