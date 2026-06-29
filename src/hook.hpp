#pragma once

#include <windows.h>
#include <optional>
#include <unordered_set>
#include "avs.hpp"
#include "log.hpp"
#include "modpath_handler.hpp"
#include "utils.hpp"

extern std::filesystem::file_time_type dll_time;

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags);
int hook_avs_fs_lstat(const char* name, struct avs_stat *st);
int hook_avs_fs_convert_path(char dest_name[256], const char* name);
int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags);
size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes);

std::unordered_set<istring> list_pngs(NormPath const&folder);

extern "C" {
    __declspec(dllexport) int init();
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
    const istring path;
    const NormPath norm_path;
    // If a mod has been found, this is its path. This can be used to overwrite
    // an entire ifs, but also have a subsequent mod overwrite an individual
    // file inside that ifs
    std::optional<istring> mod_path;

    // Using the mod_path (if available) or path, call the original open func.
    // The return type is conveniently the same width for both pkfs and avs_fs.
    // Will need rework if another opening method is found...
    virtual uint32_t call_real() = 0;

    // Load the mod_path (if available) or path into a vector
    virtual std::optional<std::vector<uint8_t>> load_to_vec() = 0;

    const istring& get_path_to_open() {
        return mod_path ? *mod_path : path;
    }

    void log_if_modfile() {
        if (mod_path) {
            log_verbose("Using {}", *mod_path);
        }
    }

    // Whether this should be included in the ramfs demangler machinery (yes for
    // avs/pkfs_open, no for lstat/convert_path)
    virtual bool ramfs_demangle() {return false;};

    HookFile(istring &&path, NormPath &&norm_path)
        : path(std::move(path))
        , norm_path(std::move(norm_path))
        , mod_path(std::nullopt)
    {}
    virtual ~HookFile() {}
};

class AvsHookFile : public HookFile {
    using HookFile::HookFile;

    public:
    std::optional<std::vector<uint8_t>> load_to_vec() override {
        AVS_FILE f = avs_fs_open(get_path_to_open().c_str(), avs_open_mode_read(), 420);
        if (f >= 0) {
            auto ret = avs_file_to_vec(f);
            avs_fs_close(f);
            return ret;
        } else {
            return std::nullopt;
        }
    }
};
class AvsOpenHookFile final : public AvsHookFile {
    private:
    uint16_t mode;
    int flags;

    public:
    AvsOpenHookFile(istring &&path, NormPath &&norm_path, uint16_t mode, int flags)
        : AvsHookFile(std::move(path), std::move(norm_path))
        , mode(mode)
        , flags(flags)
    {}

    bool ramfs_demangle() override {return true;};

    uint32_t call_real() override {
        log_if_modfile();
        return (uint32_t)avs_fs_open(get_path_to_open().c_str(), mode, flags);
    }
};

class AvsLstatHookFile final : public AvsHookFile {
    private:
    struct avs_stat *st;

    public:
    AvsLstatHookFile(istring &&path, NormPath &&norm_path, struct avs_stat *st)
        : AvsHookFile(std::move(path), std::move(norm_path))
        , st(st)
    {}

    uint32_t call_real() override {
        log_if_modfile();
        return (uint32_t)avs_fs_lstat(get_path_to_open().c_str(), st);
    }
};

class AvsConvertPathHookFile final : public AvsHookFile {
    private:
    char *dest_name;

    public:
    AvsConvertPathHookFile(istring &&path, NormPath &&norm_path, char *dest_name)
        : AvsHookFile(std::move(path), std::move(norm_path))
        , dest_name(dest_name)
    {}

    uint32_t call_real() override {
        log_if_modfile();
        return (uint32_t)avs_fs_convert_path(dest_name, get_path_to_open().c_str());
    }
};

// for the tests
void handle_arc(HookFile &file);
