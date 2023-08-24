// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include "hook.h"
#include "utils.hpp"

HMODULE my_module;
char dll_filename[MAX_PATH];
uint64_t dll_time;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    my_module = hModule;
    if (GetModuleFileNameA(my_module, dll_filename, MAX_PATH)) {
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

