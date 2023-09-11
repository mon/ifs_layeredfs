#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <inttypes.h>

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

void handle_texbin(string const& path, string const&norm_path, optional<string> &mod_path) {
    auto start = time();

    auto bin_orig_path = mod_path ? *mod_path : path; // may have layered pre-built mod .bin with extra PNGs
    auto bin_mod_path = norm_path;
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

    string out = CACHE_FOLDER "/" + norm_path;
    auto out_hashed = out + ".hashed";

    uint8_t hash[MD5_LEN];
    hash_filenames(pngs_list, hash);

    uint8_t cache_hash[MD5_LEN] = {0};
    FILE* cache_hashfile;
    cache_hashfile = fopen(out_hashed.c_str(), "rb");
    if (cache_hashfile) {
        fread(cache_hash, 1, sizeof(cache_hash), cache_hashfile);
        fclose(cache_hashfile);
    }

    auto time_out = file_time(out.c_str());
    auto newest = file_time(bin_orig_path.c_str());
    for (auto &path : pngs_list)
        newest = std::max(newest, file_time(path.c_str()));
    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(time_out >= newest && time_out >= dll_time && memcmp(hash, cache_hash, sizeof(hash)) == 0) {
        mod_path = out;
        log_misc("texbin cache up to date, skip");
        return;
    }
    // log_verbose("Regenerating cache");
    // log_verbose("  time_out >= newest == %d", time_out >= newest);
    // log_verbose("  time_out >= dll_time == %d", time_out >= dll_time);
    // log_verbose("  memcmp(hash, cache_hash, sizeof(hash)) == 0 == %d", memcmp(hash, cache_hash, sizeof(hash)) == 0);

    Texbin texbin;
    AVS_FILE f = avs_fs_open(bin_orig_path.c_str(), avs_open_mode_read(), 420);
    if (f >= 0) {
        auto orig_data = avs_file_to_vec(f);
        avs_fs_close(f);

        // one extra copy which *sucks* but whatever
        std::istringstream stream(string((char*)&orig_data[0], orig_data.size()));
        auto _texbin = Texbin::from_stream(stream);
        if(!_texbin) {
            log_warning("Texbin load failed, aborting modding");
            return;
        }
        texbin = *_texbin;
    } else {
        log_info("Found texbin mods but no original file, creating from scratch: \"%s\"", norm_path.c_str());
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

    cache_hashfile = fopen(out_hashed.c_str(), "wb");
    if (cache_hashfile) {
        fwrite(hash, 1, sizeof(hash), cache_hashfile);
        fclose(cache_hashfile);
    }
    mod_path = out;

    log_misc("Texbin generation took %d ms", time() - start);
}

int hook_avs_fs_lstat(const char* name, struct avs_stat *st) {
    if (name == NULL)
        return avs_fs_lstat(name, st);

    log_verbose("statting %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if(!norm_path)
        return avs_fs_lstat(name, st);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        log_verbose("Overwriting lstat");
        return avs_fs_lstat(mod_path->c_str(), st);
    }
    // called prior to avs_fs_open
    if(string_ends_with(path.c_str(), ".bin")) {
        handle_texbin(name, *norm_path, mod_path);
        if(mod_path) {
            log_verbose("Overwriting texbin lstat");
            return avs_fs_lstat(mod_path->c_str(), st);
        }
    }
    return avs_fs_lstat(name, st);
}

int hook_avs_fs_convert_path(char dest_name[256], const char *name) {
    if (name == NULL)
        return avs_fs_convert_path(dest_name, name);

    log_verbose("convert_path %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_convert_path(dest_name, name);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        log_verbose("Overwriting convert_path");
        return avs_fs_convert_path(dest_name, mod_path->c_str());
    }
    return avs_fs_convert_path(dest_name, name);
}

int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* args) {
    log_verbose("mounting %s to %s with type %s and args %s", fsroot, mountpoint, fstype, args);
    ramfs_demangler_on_fs_mount(mountpoint, fsroot, fstype, args);

    return avs_fs_mount(mountpoint, fsroot, fstype, args);
}

size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes) {
    ramfs_demangler_on_fs_read(context, bytes);
    return avs_fs_read(context, bytes, nbytes);
}

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
    if(name == NULL)
        return avs_fs_open(name, mode, flags);
    log_verbose("opening %s mode %d flags %d", name, mode, flags);
    // only touch reads - new AVS has bitflags (R=1,W=2), old AVS has enum (R=0,W=1,RW=2)
    if ((avs_loaded_version >= 1400 && mode != 1) || (avs_loaded_version < 1400 && mode != 0)) {
        return avs_fs_open(name, mode, flags);
    }
    string path = name;
    string orig_path = name;

    // can it be modded ie is it under /data ?
    auto _norm_path = normalise_path(path);
    if (!_norm_path)
        return avs_fs_open(name, mode, flags);
    // unpack success
    auto norm_path = *_norm_path;

    auto mod_path = find_first_modfile(norm_path);
    // mod ifs paths use _ifs, go one at a time for ifs-inside-ifs
    while (!mod_path && string_replace_first(norm_path, ".ifs", "_ifs")) {
        mod_path = find_first_modfile(norm_path);
    }

    if(string_ends_with(path.c_str(), ".xml")) {
        merge_xmls(orig_path, norm_path, mod_path);
    }

    if(string_ends_with(path.c_str(), ".bin")) {
        handle_texbin(orig_path, norm_path, mod_path);
    }

    if (string_ends_with(path.c_str(), "texturelist.xml")) {
        parse_texturelist(orig_path, norm_path, mod_path);
    }
    else {
        handle_texture(norm_path, mod_path);
    }

    if (mod_path) {
        log_info("Using %s", mod_path->c_str());
    }

    auto to_open = mod_path ? *mod_path : orig_path;
    auto ret = avs_fs_open(to_open.c_str(), mode, flags);
    ramfs_demangler_on_fs_open(to_open, ret);
    // log_verbose("(returned %d)", ret);
    return ret;
}

extern "C" {
    __declspec(dllexport) int init(void) {
        // all logs up until init_avs succeeds will go to a file for debugging purposes

        // find out where we're logging to
        load_config();

        if (MH_Initialize() != MH_OK) {
            log_fatal("Couldn't initialize MinHook");
            return 1;
        }

        if (!init_avs()) {
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
        log_info("AVS DLL detected: %s", avs_loaded_dll_name);
        print_config();

        cache_mods();

        //jb_texhook_init();

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
