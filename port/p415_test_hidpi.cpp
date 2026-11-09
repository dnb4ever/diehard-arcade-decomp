// Exercise the production wrapper on real SDL software render targets, without
// depending on a Retina display or changing SDL's window-size/DPI reporting.
#include "imgui_scale.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr,"FAIL: %s (%s)\n",why,SDL_GetError()); std::exit(1); } }
int main() {
    require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");
    SDL_Window* win=SDL_CreateWindow("HiDPI test",960,720,0);
    SDL_Renderer* ren=SDL_CreateRenderer(win,"software");require(ren,"renderer");
    ImGui::CreateContext(); ImGui_ImplSDLRenderer3_Init(ren);
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(960,720);io.DeltaTime=1.0f/60;
    for (float scale : {1.0f,1.5f,2.0f,1.0f}) {
        int w=int(960*scale),h=int(720*scale);
        auto* target=SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,w,h);require(target,"target");
        require(SDL_SetRenderTarget(ren,target),"set target");
        io.DisplayFramebufferScale=ImVec2(scale,scale);
        ImGui_ImplSDLRenderer3_NewFrame();ImGui::NewFrame();
        auto* list=ImGui::GetBackgroundDrawList();
        list->PushClipRect(ImVec2(700,500),ImVec2(950,700));
        list->AddRectFilled(ImVec2(600,400),ImVec2(960,720),IM_COL32(255,0,0,255));list->PopClipRect();
        ImGui::Render();SDL_SetRenderDrawColor(ren,0,0,0,255);SDL_RenderClear(ren);
        SDL_SetRenderScale(ren,0.75f,0.75f); // verify exact restoration too
        render_imgui_scaled(ImGui::GetDrawData(),ren);
        float x,y;SDL_GetRenderScale(ren,&x,&y);require(x==0.75f && y==0.75f,"scale restoration");
        auto* surface=SDL_RenderReadPixels(ren,nullptr);require(surface,"read pixels");
        auto pixel=[&](int px,int py) {Uint8 r,g,b,a;require(SDL_ReadSurfacePixel(surface,int(px*scale),int(py*scale),&r,&g,&b,&a),"pixel");return r==255 && g==0 && b==0;};
        require(pixel(900,650),"right/bottom geometry present");
        require(!pixel(650,550) && !pixel(900,450),"logical clip rect respected");
        SDL_DestroySurface(surface);SDL_SetRenderScale(ren,1,1);SDL_SetRenderTarget(ren,nullptr);SDL_DestroyTexture(target);
        std::printf("PASS scale %.1f: right column, lower geometry, clipping, exact renderer restoration\n",scale);
    }
    ImGui_ImplSDLRenderer3_Shutdown();ImGui::DestroyContext();SDL_DestroyRenderer(ren);SDL_DestroyWindow(win);SDL_Quit();
}
