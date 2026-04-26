#pragma once
#include <string>

#include "avs.h"

void ramfs_demangler_on_fs_open(const std::string& norm_path, AVS_FILE open_result);
void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest, size_t nbytes);
void ramfs_demangler_on_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
void ramfs_demangler_demangle_if_possible(std::string& norm_path);

// Tells the demangler that the .arc at original_path will be served from a
// repacked, uncompressed copy. When the read for it lands, the demangler
// walks the directory and registers each inner .ifs as
// (buffer + entry.file_offset) -> "<original_path>/<inner.ifs>", so a later
// ramfs/imagefs mount on that pointer is demangled correctly.
void ramfs_demangler_register_arc_repack(const std::string& original_path);