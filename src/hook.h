#pragma once

#include <windows.h>
#include "avs.h"

extern uint64_t dll_time;

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags);
int hook_avs_fs_lstat(const char* name, struct avs_stat *st);
int hook_avs_fs_convert_path(char dest_name[256], const char* name);
int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes);

extern "C" {
    __declspec(dllexport) int init(void);
    extern HMODULE my_module;
}
