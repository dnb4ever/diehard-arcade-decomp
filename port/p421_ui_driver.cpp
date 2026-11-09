// Test-only SDL event injection; never linked into the application.
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <cstdio>
static int frame;
extern "C" bool SDL_OpenURL(const char* url){std::fprintf(stderr,"P421 OPEN %s\n",url);return true;}
extern "C" bool SDL_RenderPresent(SDL_Renderer* renderer) {
    auto real=(decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT,"SDL_RenderPresent");
    auto mouse=[&](float x,float y,bool down){
        auto window=SDL_GetWindowID(SDL_GetRenderWindow(renderer));SDL_Event e{};
        e.type=SDL_EVENT_MOUSE_MOTION;e.motion.windowID=window;e.motion.x=x;e.motion.y=y;SDL_PushEvent(&e);
        e={};e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;e.button.windowID=window;
        e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;e.button.down=down;SDL_PushEvent(&e);
    };
    if(frame==5)mouse(18,200,true);if(frame==6)mouse(18,200,false);
    if(frame==9)mouse(80,154,true);if(frame==10)mouse(80,154,false);
    if(frame==12)if(const char* path=SDL_getenv("P421_DROP_ZIP")){
        SDL_Event e{};e.type=SDL_EVENT_DROP_FILE;e.drop.windowID=SDL_GetWindowID(SDL_GetRenderWindow(renderer));e.drop.data=path;SDL_PushEvent(&e);
    }
    ++frame;return real(renderer);
}
