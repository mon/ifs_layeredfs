#pragma once

#include <windows.h>
#include "avs.h"

extern time_t dll_time;

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags);
int hook_avs_fs_lstat(const char* name, struct avs_stat *st);
int hook_avs_fs_convert_path(char dest_name[256], const char* name);

extern "C" {
    __declspec(dllexport) int init(void);
    extern HMODULE my_module;
}