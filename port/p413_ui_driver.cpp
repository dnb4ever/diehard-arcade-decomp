// CPU-only SDL event driver, built as an LD_PRELOAD helper for P4.13 UI verification.
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cstdio>
static int frame;
extern "C" bool SDL_RenderPresent(SDL_Renderer* renderer) {
    auto real = (decltype(&SDL_RenderPresent))dlsym(RTLD_NEXT, "SDL_RenderPresent");
    auto key = [&](SDL_Scancode code, bool down) {
        SDL_Event e{}; e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        e.key.scancode = code; e.key.key = SDL_GetKeyFromScancode(code, SDL_KMOD_NONE, false);
        e.key.down = down; SDL_PushEvent(&e);
    };
    auto mouse = [&](float x, float y, bool down) {
        SDL_Event e{}; auto id = SDL_GetWindowID(SDL_GetRenderWindow(renderer));
        e.type = SDL_EVENT_MOUSE_MOTION; e.motion.windowID = id; e.motion.x=x; e.motion.y=y; SDL_PushEvent(&e);
        e = {}; e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        e.button.windowID=id; e.button.button=SDL_BUTTON_LEFT; e.button.x=x; e.button.y=y; e.button.down=down;
        SDL_PushEvent(&e);
    };
    if (const char* action = SDL_getenv("P413_MOVIE_START")) {
        float y = action[0]=='p' ? 212 : action[0]=='s' ? 186 : 160;
        if (frame==5) mouse(90,y,true);
        if (frame==6) mouse(90,y,false);
    }
    if (SDL_getenv("P413_TRAINING_DURING_FILE")) {
        if (frame==8) { SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(),"SATURN_MOVIE_SHOT"); SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SATURN_TRAINING_SHOT","1",true); }
        if (frame==124) mouse(20,170,true);
        if (frame==125) mouse(20,170,false);
        if (frame==160) std::fprintf(stderr,"P413 after attempted recording edit: %s\n",SDL_GetWindowTitle(SDL_GetRenderWindow(renderer)));
    }
    if (SDL_getenv("P413_TRAINING_EDIT")) {
        if (frame==5 || frame==9 || frame==13) mouse(20, frame==5 ? 144 : frame==9 ? 170 : 196, true);
        if (frame==6 || frame==10 || frame==14) mouse(20, frame==6 ? 144 : frame==10 ? 170 : 196, false);
    }
    if (SDL_getenv("P413_TITLE") && frame==60) std::fprintf(stderr, "P413 title: %s\n", SDL_GetWindowTitle(SDL_GetRenderWindow(renderer)));
    if (SDL_getenv("P413_FORWARD_MENU")) {
        if (frame==20) key(SDL_SCANCODE_TAB,true);
        if (frame==120) key(SDL_SCANCODE_TAB,false);
    }
    if (SDL_getenv("P413_FORWARD_UI")) {
        if (frame==20) key(SDL_SCANCODE_TAB,true);
        if (frame==40) key(SDL_SCANCODE_TAB,false);
        if (frame==60 || frame==90) key(SDL_SCANCODE_F4,true);
        if (frame==61 || frame==91) key(SDL_SCANCODE_F4,false);
    }
    if (SDL_getenv("P413_REWIND_UI")) {
        if (frame==60) key(SDL_SCANCODE_BACKSPACE,true);
        if (frame==90) key(SDL_SCANCODE_BACKSPACE,false);
    }
    if (SDL_getenv("P413_PLAYER_UI")) {
        if (frame==100) key(SDL_SCANCODE_ESCAPE,true);
        if (frame==101) key(SDL_SCANCODE_ESCAPE,false);
        if (frame==104) { auto* s=SDL_RenderReadPixels(renderer,nullptr); SDL_SavePNG(s,"work/p413/pause.png"); SDL_DestroySurface(s); }
        if (frame==106 || frame==109 || frame==112) key(SDL_SCANCODE_DOWN,true);
        if (frame==107 || frame==110 || frame==113) key(SDL_SCANCODE_DOWN,false);
        if (frame==115) key(SDL_SCANCODE_RETURN,true);
        if (frame==116) key(SDL_SCANCODE_RETURN,false);
        if (frame==135 || frame==140) key(SDL_SCANCODE_ESCAPE,true);
        if (frame==136 || frame==141) key(SDL_SCANCODE_ESCAPE,false);
    }
    ++frame;
    return real(renderer);
}
