// Windows GUI frontend; included only by the isolated portability patch.
#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include "p413_roms.h"

static std::string win_utf8(const wchar_t* value) {
    int n=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);
    if(n<=0)return {};
    std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,0,value,-1,out.data(),n,nullptr,nullptr);
    out.pop_back();return out;
}
static std::filesystem::path win_path(const std::string& s) {return std::filesystem::path(std::u8string(s.begin(),s.end()));}
static std::string win_data_dir() {
    wchar_t path[MAX_PATH];
    if(FAILED(SHGetFolderPathW(nullptr,CSIDL_APPDATA|CSIDL_FLAG_CREATE,nullptr,0,path)))return {};
    return win_utf8(path)+"/Die Hard Arcade";
}
static std::string win_exe_dir() {
    wchar_t exe[32768];DWORD n=GetModuleFileNameW(nullptr,exe,32768);if(!n || n==32768)return {};
    auto s=std::filesystem::path(exe).parent_path().u8string();return std::string(s.begin(),s.end());
}
static void win_console(int argc,char** argv) {
    bool stats=false;for(int i=1;i<argc;++i)if(!std::strcmp(argv[i],"--stats"))stats=true;
    if(!stats || GetFileType(GetStdHandle(STD_OUTPUT_HANDLE))!=FILE_TYPE_UNKNOWN)return;
    if(!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole())return;
    std::freopen("CONOUT$","w",stdout);std::freopen("CONOUT$","w",stderr);
}
static void win_log(const std::string& data_dir) {
    if(GetFileType(GetStdHandle(STD_ERROR_HANDLE))!=FILE_TYPE_UNKNOWN)return;
    auto path=win_path(data_dir+"/app.log");
    _wfreopen(path.c_str(),L"a",stderr);_wfreopen(path.c_str(),L"a",stdout);
}
static std::string win_pick_roms() {
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    BROWSEINFOW info{};info.lpszTitle=L"Choose the folder containing diehard.zip or the loose MAME diehard ROMs";
    info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE item=SHBrowseForFolderW(&info);wchar_t path[MAX_PATH];std::string result;
    if(item){if(SHGetPathFromIDListW(item,path))result=win_utf8(path);CoTaskMemFree(item);}
    if(SUCCEEDED(initialized))CoUninitialize();return result;
}
static bool win_prepare_from(const std::string& src,const std::string& data_dir,bool tell=false) {
    if(src.empty())return false;
    char error[512];int result=dha_prepare_roms(src.c_str(),data_dir.c_str(),error,sizeof error);
    std::fprintf(stderr,"ROM preparation from %s: %s\n",src.c_str(),error);
    if(result!=1 && tell)MessageBoxA(nullptr,error,"Die Hard Arcade: ROM preparation",MB_OK|MB_ICONERROR);
    return result==1;
}
static bool win_select_roms(std::string& requested,bool choose,const std::string& data_dir,std::string& cart,std::string& cartrom) {
    std::error_code error;std::filesystem::create_directories(win_path(data_dir),error);
    if(error){MessageBoxW(nullptr,L"Cannot create the Die Hard Arcade AppData folder.",L"Die Hard Arcade",MB_OK|MB_ICONERROR);return false;}
    win_log(data_dir);
    if(!requested.empty()) {
        auto path=std::filesystem::absolute(win_path(requested),error).u8string();
        if(!error)requested.assign(path.begin(),path.end());
    }
    const auto remembered=win_path(data_dir+"/rom-folder.txt");std::string prior;
    {std::ifstream f(remembered,std::ios::binary);std::getline(f,prior);}
    bool ready=false;
    if(!choose) {
        if(!requested.empty())ready=win_prepare_from(requested,data_dir,true);
        else ready=dha_roms_ready(data_dir.c_str()) || win_prepare_from(data_dir,data_dir) ||
            win_prepare_from(win_exe_dir(),data_dir) || win_prepare_from(prior,data_dir);
    }
    if(!ready && !choose) {
        std::fprintf(stderr,"ROMs missing: supply MAME diehard.zip in %s or choose its folder\n",data_dir.c_str());
        if(MessageBoxW(nullptr,
            L"Die Hard Arcade needs the MAME 'diehard' (USA) ROM set. No ROMs are included.\n\n"
            L"Place diehard.zip in %APPDATA%\\Die Hard Arcade, or choose a folder containing diehard.zip "
            L"(or fpr19119.13 and mpr19115.2 through mpr19118.5). Verified prepared cart.bin files are also accepted.\n\n"
            L"Click OK to choose a ROM folder, or Cancel to quit.",
            L"Die Hard Arcade: ROMs needed",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return false;
    }
    while(!ready) {
        requested=win_pick_roms();if(requested.empty())return false;
        ready=win_prepare_from(requested,data_dir,true);
    }
    if(!requested.empty()) {std::ofstream f(remembered,std::ios::binary|std::ios::trunc);f<<requested<<"\n";}
    cart=data_dir+"/diehard_main.bin";cartrom=data_dir+"/cart.bin";return true;
}
#endif
