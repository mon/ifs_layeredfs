#include <windows.h>
#include <algorithm>
#include <set>

#include "ramfs_demangler.h"

#include "modpath_handler.h"
#include "config.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "avs.h"
#include "winxp_mutex.hpp"

using std::nullopt;

typedef struct {
    std::string name;
    std::set<string, CaseInsensitiveCompare> contents;
} mod_contents_t;

std::vector<mod_contents_t> cached_mods;

std::set<string, CaseInsensitiveCompare> walk_dir(const string &path, const string &root) {
    std::set<string, CaseInsensitiveCompare> result;

    WIN32_FIND_DATAA ffd;
    auto contents = FindFirstFileA((path + "/*").c_str(), &ffd);

    if (contents != INVALID_HANDLE_VALUE) {
        do {
            if (!strcmp(ffd.cFileName, ".") ||
                !strcmp(ffd.cFileName, "..")) {
                continue;
            }

            string result_path;
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // sanity check a common mistake
                if (root == "" && !strcasecmp(ffd.cFileName, "data")) {
                    log_warning("\"data\" folder detected in mod root. Move all files inside to the mod root, or it will not work");
                }
                result_path = root + ffd.cFileName + "/";
                log_verbose("  %s", result_path.c_str());
                auto subdir_walk = walk_dir(path + "/" + ffd.cFileName, result_path);
                result.insert(subdir_walk.begin(), subdir_walk.end());
            }
            else {
                result_path = root + ffd.cFileName;
                log_verbose("  %s", result_path.c_str());
            }
            result.insert(result_path);
        } while (FindNextFileA(contents, &ffd) != 0);

        FindClose(contents);
    }

    return result;
}

void cache_mods(void) {
    // this is a bit hacky
    bool devmode = config.developer_mode;
    config.developer_mode = true;
    auto avail_mods = available_mods();
    config.developer_mode = devmode;

    for (auto &dir : avail_mods) {
        log_verbose("Walking %s", dir.c_str());
        mod_contents_t mod;
        mod.name = dir;
        // even in developer mode we want to walk the mods directory for effective logging
        mod.contents = walk_dir(dir, "");
        if (!config.developer_mode) {
            cached_mods.push_back(mod);
        }
    }
}

// data, data2, data_op2 etc
// data is "flat", all others must have their own special subfolders
static vector<string> game_folders;

void init_modpath_handler(void) {
    for (auto folder : folders_in_folder(".")) {
        // data is the normal case we transparently handle
        if (!strcasecmp(folder.c_str(), "data")) {
            continue;
        }

        game_folders.push_back(folder + "/");
    }
}

optional<string> normalise_path(const string &_path) {
    auto path = _path;
    ramfs_demangler_demangle_if_possible(path);

    auto data_pos = string_find_icase(path, "data/");
    auto other_pos = string::npos;

    if (data_pos == string::npos) {
        // search all our other folders for anything that matches
        for (auto folder : game_folders) {
            other_pos = string_find_icase(path, folder);
            if (other_pos != string::npos) {
                break;
            }
        }

        if (other_pos == string::npos) {
            return nullopt;
        }
    }
    auto actual_pos = (data_pos != string::npos) ? data_pos : other_pos;
    // if data2 was found, for example, use root mod/data2/.../... instead of just mod/.../...
    auto offset = (other_pos != string::npos) ? 0 : strlen("data/");
    auto data_str = path.substr(actual_pos + offset);
    // nuke backslash
    string_replace(data_str, "\\", "/");
    // nuke double slash
    string_replace(data_str, "//", "/");

    return data_str;
}

vector<string> available_mods() {
    vector<string> ret;
    string mod_root = config.mod_folder + "/";

    // just pretend we have no mods at all
    if (config.disable) {
        return ret;
    }

    if (config.developer_mode) {
        static bool first_search = true;
        for (auto folder : folders_in_folder(config.mod_folder.c_str())) {
            if (!strcasecmp(folder.c_str(), "_cache")) {
                continue;
            }

            // if there is an allowlist, is this mod on it?
            if (!config.allowlist.empty() && config.allowlist.find(folder) == config.allowlist.end()) {
                if (first_search)
                    log_info("Ignoring non-allowlisted mod %s", folder.c_str());

                continue;
            }

            // is this mod in the blocklist?
            if (config.blocklist.find(folder) != config.blocklist.end()) {
                if (first_search)
                    log_info("Ignoring blocklisted mod %s", folder.c_str());

                continue;
            }

            ret.push_back(mod_root + folder);
        }

        first_search = false;
    }
    else {
        for (auto &dir : cached_mods) {
            ret.push_back(dir.name);
        }
    }
    // case insensitive, so apple comes before English
    std::sort(ret.begin(), ret.end(), [](const string& a, const string& b){
            return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
    return ret;
}

bool mkdir_p(const string &path) {
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    char *p;
    errno = 0;
    auto _path = path;

    /* Iterate the string */
    for (p = _path.data() + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (!CreateDirectoryA(_path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }

            *p = '/';
        }
    }

    if (!CreateDirectoryA(_path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }

    return true;
}

// same for files and folders when cached
optional<string> find_first_cached_item(const string &norm_path) {
    for (auto &dir : cached_mods) {
        auto file_search = dir.contents.find(norm_path);
        if (file_search == dir.contents.end()) {
            continue;
        }
        return dir.name + "/" + *file_search;
    }

    return nullopt;
}

optional<string> find_first_modfile(const string &norm_path) {
    //log_verbose("%s(%s)", __FUNCTION__, norm_path.c_str());
    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = dir + "/" + norm_path;
            if (file_exists(mod_path.c_str())) {
                return path_to_actual_case(mod_path);
            }
        }
    }
    else {
        return find_first_cached_item(norm_path);
    }
    return nullopt;
}

optional<string> find_first_modfolder(const string &norm_path) {
    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = dir + "/" + norm_path;
            if (folder_exists(mod_path.c_str())) {
                return path_to_actual_case(mod_path) + "/";
            }
        }
    }
    else {
        return find_first_cached_item(norm_path + "/");
    }
    return nullopt;
}

vector<string> find_all_modfile(const string &norm_path) {
    vector<string> ret;

    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = dir + "/" + norm_path;
            if (file_exists(mod_path.c_str())) {
                ret.push_back(mod_path);
            }
        }
    }
    else {
        for (auto &dir : cached_mods) {
            auto file_search = dir.contents.find(norm_path);
            if (file_search == dir.contents.end()) {
                continue;
            }
            ret.push_back(dir.name + "/" + *file_search);
        }
    }
    // needed for consistency when hashing names
    std::sort(ret.begin(), ret.end());
    return ret;
}
