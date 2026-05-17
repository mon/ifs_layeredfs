#pragma once
#include <string>

#include "avs.h"

void ramfs_demangler_on_fs_open(const std::string& norm_path, AVS_FILE open_result);
void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest, size_t nbytes);
void ramfs_demangler_on_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
void ramfs_demangler_demangle_if_possible(std::string& norm_path);

// Registers an inner-ifs basename ("foo.ifs") -> demangled path
// ("<arc_norm_path with .arc->_arc>/.../foo.ifs"). When a later ramfs mount's
// mountpoint or fsroot contains that basename, the mount is demangled to the
// stored path. Used for arc-embedded ifs files: games extract them into a
// ramfs buffer through a path that bypasses our hooks, so the buffer pointer
// is unknown and we have to match on basename.
void ramfs_demangler_register_arc_inner_ifs(const std::string& basename, const std::string& demangled_path);
