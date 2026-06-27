// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include "hook.h"
#include "utils.hpp"

extern "C" __declspec(dllexport) const char __layeredfs_version[] = VER_STRING;

std::filesystem::file_time_type dll_time;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    wchar_t dll_filename[MAX_PATH];

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    if (GetModuleFileNameW(hModule, dll_filename, MAX_PATH)) {
      dll_time = file_time(dll_filename);
    }
    return init() == 0;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

