#pragma once

#include <windows.h>
#include <optional>
#include <string>
#include "avs.h"
#include "log.hpp"
#include "utils.hpp"

extern uint64_t dll_time;

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags);
int hook_avs_fs_lstat(const char* name, struct avs_stat *st);
int hook_avs_fs_convert_path(char dest_name[256], const char* name);
int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes);

string_set list_pngs(string const&folder);

extern "C" {
    __declspec(dllexport) int init(void);
    extern HMODULE my_module;
}

// Used to simplify file opens - pkfs/avs_fs have different signatures, but
// otherwise have almost the exact same semantics and mod behaviour. It lets you
// replace a file's path, or read the original file (using the correct API) into
// a vector.
// NOTE: the APIs to read XML files (specifically prop_from_file_path and
// rapidxml_from_avs_filepath) still use avs_fs_open, because I can't find any
// evidence the games are using XMLs in a way that anybody would want to mod.
// May this decision not bite me later...
class HookFile {
    public:
    // The original path requested by the game
    const std::string path;
    // Regardless of how many prefixes, extraneous slashes, back/forward
    // slashes, the normalised path is the canonical game-folder-relative path
    // used to search for mods eg:
    //   graphics/ver03/cmn_sys.ifs
    //   data2/graphics/whatever.ifs
    const std::string norm_path;
    // If a mod has been found, this is its path. This can be used to overwrite
    // an entire ifs, but also have a subsequent mod overwrite an individual
    // file inside that ifs
    std::optional<std::string> mod_path;

    // Using the mod_path (if available) or path, call the original open func.
    // The return type is conveniently the same width for both pkfs and avs_fs.
    // Will need rework if another opening method is found...
    virtual uint32_t call_real() = 0;

    // Load the mod_path (if available) or path into a vector
    virtual std::optional<std::vector<uint8_t>> load_to_vec() = 0;

    const std::string& get_path_to_open() {
        return mod_path ? *mod_path : path;
    }

    void log_if_modfile() {
        if (mod_path) {
            log_info("Using %s", mod_path->c_str());
        }
    }

    // Whether this should be included in the ramfs demangler machinery (yes for
    // avs/pkfs_open, no for lstat/convert_path)
    virtual bool ramfs_demangle() {return false;};

    HookFile(const std::string path, const std::string norm_path)
        : path(path)
        , norm_path(norm_path)
        , mod_path(std::nullopt)
    {}
    virtual ~HookFile() {}
};
