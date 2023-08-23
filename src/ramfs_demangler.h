#pragma once
#include <string>

#include "avs.h"

void ramfs_demangler_on_fs_open(const std::string& norm_path, AVS_FILE open_result);
void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest);
void ramfs_demangler_on_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
void ramfs_demangler_demangle_if_possible(std::string& norm_path);