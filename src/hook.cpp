#include <stdint.h>
#include <winternl.h>

#include "hook.h"

// all good code mixes C and C++, right?
using std::string;
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "3rd_party/MinHook.h"

#include "ramfs_demangler.h"
#include "config.hpp"
#include "log.hpp"
#include "imagefs.hpp"
#include "texbin.hpp"
#include "utils.hpp"
#include "avs.h"
#include "modpath_handler.h"

// let me use the std:: version, damnit
#undef max
#undef min

#ifdef _DEBUG
#define DBG_VER_STRING "_DEBUG"
#else
#define DBG_VER_STRING
#endif

#define VERSION VER_STRING DBG_VER_STRING

// debugging
//#define ALWAYS_CACHE

// used by pkfs hooks - we don't want to hook avs_fs_open if we just hooked pkfs
thread_local static bool inside_pkfs_hook;

unsigned int (*pkfs_fs_open)(const char *path);
unsigned int (*pkfs_fs_fstat)(unsigned int f, struct avs_stat *stat);
unsigned int (*pkfs_fs_read)(unsigned int f, void *buf, int sz);
unsigned int (*pkfs_fs_close)(unsigned int f);
void (*pkfs_clear_hdd_error)();

class AvsHookFile : public HookFile {
    using HookFile::HookFile;

    std::optional<std::vector<uint8_t>> load_to_vec() override {
        AVS_FILE f = avs_fs_open(get_path_to_open().c_str(), avs_open_mode_read(), 420);
        if (f >= 0) {
            auto ret = avs_file_to_vec(f);
            avs_fs_close(f);
            return ret;
        } else {
            return nullopt;
        }
    }
};
class AvsOpenHookFile final : public AvsHookFile {
    private:
    uint16_t mode;
    int flags;

    public:
    AvsOpenHookFile(const std::string path, const std::string norm_path, uint16_t mode, int flags)
        : AvsHookFile(path, norm_path)
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
    AvsLstatHookFile(const std::string path, const std::string norm_path, struct avs_stat *st)
        : AvsHookFile(path, norm_path)
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
    AvsConvertPathHookFile(const std::string path, const std::string norm_path, char *dest_name)
        : AvsHookFile(path, norm_path)
        , dest_name(dest_name)
    {}

    uint32_t call_real() override {
        log_if_modfile();
        return (uint32_t)avs_fs_convert_path(dest_name, get_path_to_open().c_str());
    }
};

class PkfsHookFile final : public HookFile {
    public:
    PkfsHookFile(const std::string path, const std::string norm_path)
        : HookFile(path, norm_path)
    {}

    bool ramfs_demangle() {return false;};

    uint32_t call_real() override {
        log_if_modfile();
        auto ret = pkfs_fs_open(get_path_to_open().c_str());
        if(ret == 0) {
            log_verbose("pkfs_fs_open(%s) failed in call_real", get_path_to_open().c_str());
        }
        return ret;
    }

    std::optional<std::vector<uint8_t>> load_to_vec() override {
        AVS_FILE f = pkfs_fs_open(get_path_to_open().c_str());
        if (f != 0) {
            avs_stat stat = {0}; // stat type is shared!
            pkfs_fs_fstat(f, &stat);
            std::vector<uint8_t> ret;
            ret.resize(stat.filesize);
            pkfs_fs_read(f, &ret[0], stat.filesize);
            pkfs_fs_close(f);
            return ret;
        } else {
            // failed pkfs_fs_open will set an HDD read error, which is actually
            // checked during boot. Usually a non-issue, since every read is
            // preceded by an avs_fs_fstat, which creates the cached .bin.
            //
            // This of course is racey, so if there is a *real* error in another
            // thread, this resets it. But it's a tight race, and I'll take that
            // chance.
            log_verbose("pkfs_open(%s) failed in load_to_vec, clearing HDD error", get_path_to_open().c_str());
            pkfs_clear_hdd_error();
            return nullopt;
        }
    }
};

// this should probably be part of the modpath h/cpp
void list_pngs_onefolder(string_set &names, string const& folder) {
    auto search_path = folder + "/*.png";
    const auto extension_len = strlen(".png");
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                fd.cFileName[strlen(fd.cFileName) - extension_len] = '\0';
                names.insert(fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
}

string_set list_pngs(string const&folder) {
    string_set ret;

    for (auto &mod : available_mods()) {
        auto path = mod + "/" + folder;
        list_pngs_onefolder(ret, path);
        list_pngs_onefolder(ret, path + "/tex");
    }

    return ret;
}

void handle_texbin(HookFile &file) {
    auto start = time();

    auto bin_mod_path = file.norm_path;
    // mod texbins strip the .bin off the end. This isn't consistent with the _ifs
    // used for ifs files, but it's consistent with gitadora-texbintool, the *only*
    // tool to extract .bin files currently.
    string_replace(bin_mod_path, ".bin", "");

    if (!find_first_modfolder(bin_mod_path)) {
        // log_verbose("texbin: mod folder doesn't exist, skipping");
        return;
    }

    auto pngs = list_pngs(bin_mod_path);

    //// This whole hashing section is just a tiny bit different from the XML
    //// hashing :(

    // convert the string_set to a vec for repeatable hashes
    vector<string> pngs_list;
    pngs_list.reserve(pngs.size());
    for (auto it = pngs.begin(); it != pngs.end(); ) {
        auto png = std::move(pngs.extract(it++).value());
        // todo: hacky as hell, list_pngs should do better
        png = png + ".png";
        auto png_res = find_first_modfile(bin_mod_path + "/" + png);
        if(png_res) {
            pngs_list.push_back(*png_res);
        }
    }
    // sort
    std::sort(pngs_list.begin(), pngs_list.end());

    // nothing to do...
    if (pngs_list.size() == 0) {
        log_verbose("texbin: mod folder has no PNGs, skipping");
        return;
    }

    auto starting = file.get_path_to_open();
    string out = CACHE_FOLDER + "/" + file.norm_path;
    auto out_hashed = out + ".hashed";
    auto cache_hasher = CacheHasher(out_hashed);

    cache_hasher.add(starting);
    for (auto &path : pngs_list) {
        cache_hasher.add(path);
    }
    cache_hasher.finish();

    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(cache_hasher.matches()) {
        file.mod_path = out;
        log_verbose("texbin cache up to date, skip");
        return;
    }
    log_verbose("Regenerating cache");

    Texbin texbin;
    auto _orig_data = file.load_to_vec();
    if (_orig_data) {
        auto orig_data = *_orig_data;
        // one extra copy which *sucks* but whatever
        std::istringstream stream(string((char*)&orig_data[0], orig_data.size()));
        auto _texbin = Texbin::from_stream(stream);
        if(!_texbin) {
            log_warning("Texbin load failed, aborting modding");
            return;
        }
        texbin = *_texbin;
    } else {
        log_info("Found texbin mods but no original file, creating from scratch: \"%s\"", file.norm_path.c_str());
    }

    auto folder_terminator = out.rfind("/");
    auto out_folder = out.substr(0, folder_terminator);
    if (!mkdir_p(out_folder)) {
        log_warning("Texbin: Couldn't create output cache folder");
        return;
    }

    for (auto &path : pngs_list) {
        // I have yet to see a texbin without allcaps names for textures
        auto tex_name = basename_without_extension(path);
        str_toupper_inline(tex_name);
        texbin.add_or_replace_image(tex_name.c_str(), path.c_str());
    }

    if(!texbin.save(out.c_str())) {
        log_warning("Texbin: Couldn't create output");
        return;
    }

    cache_hasher.commit();
    file.mod_path = out;

    log_misc("Texbin generation took %d ms", time() - start);
}

uint32_t handle_file_open(HookFile &file) {
    auto norm_copy = file.norm_path;
    file.mod_path = find_first_modfile(norm_copy);
    // mod ifs paths use _ifs, go one at a time for ifs-inside-ifs
    while (!file.mod_path && string_replace_first(norm_copy, ".ifs", "_ifs")) {
        file.mod_path = find_first_modfile(norm_copy);
    }

    if(string_ends_with(file.path, ".xml")) {
        merge_xmls(file);
    }

    if(string_ends_with(file.path, ".bin")) {
        handle_texbin(file);
    }

    if (string_ends_with(file.path, "texturelist.xml")) {
        parse_texturelist(file);
    } else if(string_ends_with(file.path, "afplist.xml")) {
        parse_afplist(file);
    } else {
        handle_texture(file);
        handle_afp(file);
    }

    auto ret = file.call_real();
    if(file.ramfs_demangle()) {
        ramfs_demangler_on_fs_open(file.path, ret);
    }
    // log_verbose("(returned %d)", ret);
    return ret;
}

int hook_avs_fs_lstat(const char* name, struct avs_stat *st) {
    if (name == NULL)
        return avs_fs_lstat(name, st);

    log_verbose("statting %s", name);
    string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_lstat(name, st);
    // unpack success
    AvsLstatHookFile file(path, *norm_path, st);

    return handle_file_open(file);
}

int hook_avs_fs_convert_path(char dest_name[256], const char *name) {
    if (name == NULL)
        return avs_fs_convert_path(dest_name, name);

    log_verbose("convert_path %s", name);
    string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_convert_path(dest_name, name);
    // unpack success
    AvsConvertPathHookFile file(path, *norm_path, dest_name);

    return handle_file_open(file);
}

int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* args) {
    log_verbose("mounting %s to %s with type %s and args %s", fsroot, mountpoint, fstype, args);
    ramfs_demangler_on_fs_mount(mountpoint, fsroot, fstype, args);

    // In new jubeat, a modded IFS file will be loaded as such:
    // pkfs_open data/music/xxxx/bsc.eve
    // mounting /data/ifs_pack/xxxx/yyyy_msc.ifs to /data/imagefs/msc/xxxx with type imagefs and args (NULL)
    // avs_fs_open /data/ifs_pack/xxxx/yyyy_msc.ifs
    // ... Because of this, we have to go back to hooking avs_fs_open when a mount is seen.
    // It shouldn't affect old-beat (the reason pkfs hooks exist at all) because
    // they use .bin files instead of .ifs files.
    inside_pkfs_hook = false;

    return avs_fs_mount(mountpoint, fsroot, fstype, args);
}

size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes) {
    ramfs_demangler_on_fs_read(context, bytes);
    return avs_fs_read(context, bytes, nbytes);
}

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
    if(name == NULL || inside_pkfs_hook)
        return avs_fs_open(name, mode, flags);
    log_verbose("opening %s mode %d flags %d", name, mode, flags);
    // only touch reads
    if (mode != avs_open_mode_read()) {
        return avs_fs_open(name, mode, flags);
    }
    string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_open(name, mode, flags);
    // unpack success
    AvsOpenHookFile file(path, *norm_path, mode, flags);

    return handle_file_open(file);
}

unsigned int hook_pkfs_open(const char *name) {
    log_verbose("pkfs_open %s", name);

    string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path) {
        log_verbose("pkfs_open falling back to real (no norm)");
        return pkfs_fs_open(name);
    }
    // unpack success
    PkfsHookFile file(path, *norm_path);

    // note that this also hides the avs_fs_open of the pakfile holding a
    // particular file - acceptable compromise IMO
    inside_pkfs_hook = true;

#ifdef UNPAK
    string pakdump_loc = "./data_unpak/" + file.norm_path;
    if(!file_exists(pakdump_loc.c_str())) {
        auto folder_terminator = pakdump_loc.rfind("/");
        auto out_folder = pakdump_loc.substr(0, folder_terminator);
        auto data = file.load_to_vec();
        if(data) {
            if(mkdir_p(out_folder)) {
                auto dump = fopen(pakdump_loc.c_str(), "wb");
                if(dump) {
                    fwrite(&(*data)[0], 1, data->size(), dump);
                    fclose(dump);

                    log_info("Dumped new pkfs file!");
                } else {
                    log_warning("Pakdump: Couldn't open output file");
                }
            } else {
                log_warning("Pakdump: Couldn't create output folder");
            }
        }
    }
#endif

    auto ret = handle_file_open(file);
    inside_pkfs_hook = false;
    return ret;
}

static void dump_loaded_dll_info() {
    log_verbose("DLLs loaded:");
    auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;
    auto head = peb->Ldr->InMemoryOrderModuleList.Flink;
    bool first = true;
    for(auto mod = head; mod && (first || mod != head); mod = mod->Flink) {
        auto ldr = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(mod);
        log_verbose("  %.*ls", ldr->FullDllName.Length, ldr->FullDllName.Buffer);
        first = false;
    }
}

extern "C" {
    __declspec(dllexport) int init(void) {
        // all logs up until init_avs succeeds will go to a file for debugging purposes

        // find out where we're logging to
        load_config();

        if (MH_Initialize() != MH_OK) {
            dump_loaded_dll_info();
            log_fatal("Couldn't initialize MinHook");
            return 1;
        }

        if (!init_avs()) {
            dump_loaded_dll_info();
            log_fatal("Couldn't find ifs operations in dll. Send avs dll (libavs-winxx.dll or avs2-core.dll) to mon.");
            return 2;
        }

        // re-route to AVS logs if no external file specified
        if(!config.logfile) {
            imp_log_body_fatal = log_body_fatal;
            imp_log_body_warning = log_body_warning;
            imp_log_body_info = log_body_info;
            imp_log_body_misc = log_body_misc;
        }

        // now we can say hello!
        log_info("IFS layeredFS v" VERSION " init");
#ifdef SPECIAL_VER
        log_info("Build config: " SPECIAL_VER);
#endif
        log_info("AVS DLL detected: %s", avs_loaded_dll_name);
        print_config();
#ifdef UNPAK
        log_info(".pak dumper mode enabled");
#endif

        dump_loaded_dll_info();

        init_modpath_handler();
        cache_mods();

        // hook pkfs, not big enough to be its own file
        if(MH_CreateHookApi(L"libpackfs.dll", "?pkfs_fs_open@@YAIPBD@Z", (LPVOID)&hook_pkfs_open, (LPVOID*)&pkfs_fs_open) == MH_OK) {
            auto mod = GetModuleHandleA("libpackfs.dll");
            pkfs_fs_fstat = (decltype(pkfs_fs_fstat))GetProcAddress(mod, "?pkfs_fs_fstat@@YAEIPAUT_AVS_FS_STAT@@@Z");
            pkfs_fs_read = (decltype(pkfs_fs_read))GetProcAddress(mod, "?pkfs_fs_read@@YAHIPAXH@Z");
            pkfs_fs_close = (decltype(pkfs_fs_close))GetProcAddress(mod, "?pkfs_fs_close@@YAHI@Z");
            pkfs_clear_hdd_error = (decltype(pkfs_clear_hdd_error))GetProcAddress(mod, "?pkfs_clear_hdd_error@@YAXXZ");
        } else if(MH_CreateHookApi(L"pkfs.dll", "pkfs_fs_open", (LPVOID)&hook_pkfs_open, (LPVOID*)&pkfs_fs_open) == MH_OK) {
            // jubeat DLL has no mangling - only one of these will succeed (if at all)
            auto mod = GetModuleHandleA("pkfs.dll");
            pkfs_fs_fstat = (decltype(pkfs_fs_fstat))GetProcAddress(mod, "pkfs_fs_fstat");
            pkfs_fs_read = (decltype(pkfs_fs_read))GetProcAddress(mod, "pkfs_fs_read");
            pkfs_fs_close = (decltype(pkfs_fs_close))GetProcAddress(mod, "pkfs_fs_close");
            pkfs_clear_hdd_error = (decltype(pkfs_clear_hdd_error))GetProcAddress(mod, "pkfs_clear_hdd_error");
        }

        if(pkfs_fs_open) {
            if(pkfs_fs_fstat && pkfs_fs_read && pkfs_fs_close && pkfs_clear_hdd_error) {
                log_info("pkfs hooks activated");
            } else {
                log_fatal("Couldn't fully init pkfs hook - open an issue!");
            }
        }

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            log_warning("Couldn't enable hooks");
            return 2;
        }
        log_info("Hook DLL init success");

        log_info("Detected mod folders:");
        for (auto &p : available_mods()) {
            log_info("%s", p.c_str());
        }

        return 0;
    }
}
