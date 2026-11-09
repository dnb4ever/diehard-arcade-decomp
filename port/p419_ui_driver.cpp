// Test-only SDL mouse/clipboard automation at the ordinary 960x720 launcher.
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
static int frame;
static void click(SDL_Renderer* renderer,int x,int y,bool down) {
    SDL_WarpMouseInWindow(SDL_GetRenderWindow(renderer),float(x),float(y));
    SDL_Event motion{}; motion.type=SDL_EVENT_MOUSE_MOTION; motion.motion.windowID=SDL_GetWindowID(SDL_GetRenderWindow(renderer)); motion.motion.x=float(x); motion.motion.y=float(y); SDL_PushEvent(&motion);
    SDL_Event e{}; e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP; e.button.windowID=SDL_GetWindowID(SDL_GetRenderWindow(renderer)); e.button.button=SDL_BUTTON_LEFT; e.button.down=down; e.button.x=float(x); e.button.y=float(y); SDL_PushEvent(&e);
}
extern "C" bool SDL_RenderPresent(SDL_Renderer* renderer) {
    auto real=(decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT,"SDL_RenderPresent");
    if(frame==8 || frame==9) click(renderer,120,160,frame==8);
    if(frame==30 || frame==31) click(renderer,320,160,frame==30);
    if(frame==33) {
        char* text=SDL_GetClipboardText(); std::fprintf(stderr,"P419 clipboard copy: %s\n",text?text:""); SDL_free(text);
        if(const char* remote=SDL_getenv("P419_REMOTE_CODE")) SDL_SetClipboardText(remote);
    }
    if(frame==36 || frame==37) click(renderer,800,212,frame==36);
    ++frame; return real(renderer);
}
