#pragma once
#include <optional>
#include <string>

#include "avs.h"

// note: the demangler works exclusively on game-provided paths and so doesn't
// store/take istrings

void ramfs_demangler_on_fs_open(std::string path, AVS_FILE open_result);
void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest);
void ramfs_demangler_on_fs_mount(std::string_view mountpoint, std::string_view fsroot, std::string_view fstype, std::optional<std::string_view> flags);
void ramfs_demangler_demangle_if_possible(std::string &path);

// Registers an inner-ifs basename ("foo.ifs") -> demangled path
// ("<arc_norm_path with .arc->_arc>/.../foo.ifs"). When a later ramfs mount's
// mountpoint or fsroot contains that basename, the mount is demangled to the
// stored path. Used for arc-embedded ifs files: games extract them into a
// ramfs buffer through a path that bypasses our hooks, so the buffer pointer
// is unknown and we have to match on basename.
void ramfs_demangler_register_arc_inner_ifs(const std::string& basename, const std::string& demangled_path);
