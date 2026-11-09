// Test-only Linux SDL event driver; verifies netplay locks time/save/menu keys.
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <initializer_list>
static int frame;
extern "C" bool SDL_RenderPresent(SDL_Renderer* renderer) {
    auto real=(decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT,"SDL_RenderPresent");
    if(frame==100 || frame==101) for(auto code:{SDL_SCANCODE_F4,SDL_SCANCODE_F5,SDL_SCANCODE_F7,SDL_SCANCODE_BACKSPACE,SDL_SCANCODE_ESCAPE}) {
        SDL_Event e{}; e.type=frame==100?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
        e.key.scancode=code; e.key.key=SDL_GetKeyFromScancode(code,SDL_KMOD_NONE,false); e.key.down=frame==100; SDL_PushEvent(&e);
    }
    ++frame;return real(renderer);
}
