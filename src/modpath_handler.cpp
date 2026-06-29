#include <filesystem>
#include <algorithm>
#include <set>

#include "ramfs_demangler.hpp"

#include "modpath_handler.hpp"
#include "config.hpp"
#include "log.hpp"
#include "utils.hpp"

typedef struct {
    istring name;
    std::set<NormPath> contents;
} mod_contents_t;

std::vector<mod_contents_t> cached_mods;

std::set<NormPath> walk_dir(const std::filesystem::path &root) {
    std::set<NormPath> result;

    for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
        auto& entry_path = entry.path();

        istring relative = entry_path.lexically_relative(root).string();
        relative.replace_all("\\", "/");
        if (entry.is_directory())
            relative += "/";

        log_verbose("  {}", relative);
        result.insert(NormPath(std::move(relative)));

        // sanity check a common mistake
        if (entry_path.parent_path() == root && entry_path.filename() == "data") {
            log_warning("\"data\" folder detected in mod root. Move all files inside to the mod root, or it will not work");
        }
    }

    return result;
}

void cache_mods() {
    // this is a bit hacky
    bool devmode = config.developer_mode;
    config.developer_mode = true;
    auto avail_mods = available_mods();
    config.developer_mode = devmode;

    for (auto &dir : avail_mods) {
        log_verbose("Walking {}", dir);
        mod_contents_t mod;
        mod.name = dir;
        // even in developer mode we want to walk the mods directory for effective logging
        mod.contents = walk_dir(dir);
        if (!config.developer_mode) {
            cached_mods.push_back(mod);
        }
    }
}

// data, data2, data_op2 etc
// data is "flat", all others must have their own special subfolders
static std::vector<istring> game_folders;

void init_modpath_handler() {
    log_verbose("Top level folders:");
    for (auto folder : folders_in_folder(".")) {
        log_verbose("  {}", folder);

        // data is the normal case we transparently handle
        if (folder == "data") {
            continue;
        }

        game_folders.push_back(folder + "/");
    }
}

void modpath_debug_add_folder(std::string_view folder) {
    game_folders.push_back(fmt::format("{}/", folder));
}

std::optional<NormPath> normalise_path(std::string _path, bool demangle) {
    if (demangle)
        ramfs_demangler_demangle_if_possible(_path);

    istring path(std::move(_path));
    auto data_pos = path.find("data/");
    auto other_pos = std::string::npos;

    if (data_pos == std::string::npos) {
        // search all our other folders for anything that matches
        for (auto folder : game_folders) {
            other_pos = path.find(folder);
            if (other_pos != std::string::npos) {
                break;
            }
        }

        if (other_pos == std::string::npos) {
            return std::nullopt;
        }
    }
    auto actual_pos = (data_pos != std::string::npos) ? data_pos : other_pos;
    // if data2 was found, for example, use root mod/data2/.../... instead of just mod/.../...
    auto offset = (other_pos != std::string::npos) ? 0 : strlen("data/");
    istring data_str = path.substr(actual_pos + offset);
    // nuke backslash
    data_str.replace_all("\\", "/");
    // nuke double slash
    data_str.replace_all("//", "/");

    return NormPath(std::move(data_str));
}

std::vector<istring> available_mods() {
    std::vector<istring> ret;
    istring mod_root = config.get_mod_folder() + "/";

    // just pretend we have no mods at all
    if (config.disable) {
        return ret;
    }

    if (config.developer_mode) {
        static bool first_search = true;
        for (auto folder : folders_in_folder(config.get_mod_folder())) {
            if (folder == "_cache") {
                continue;
            }

            // if there is an allowlist, is this mod on it?
            if (!config.allowlist.empty() && config.allowlist.contains(folder)) {
                if (first_search)
                    log_info("Ignoring non-allowlisted mod {}", folder);

                continue;
            }

            // is this mod in the blocklist?
            if (config.blocklist.contains(folder)) {
                if (first_search)
                    log_info("Ignoring blocklisted mod {}", folder);

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
    std::sort(ret.begin(), ret.end());
    return ret;
}

// same for files and folders when cached
std::optional<istring> find_first_cached_item(const NormPath &norm_path) {
    for (auto &dir : cached_mods) {
        auto file_search = dir.contents.find(norm_path);
        if (file_search == dir.contents.end()) {
            continue;
        }
        return fmt::format("{}/{}", dir.name, *file_search);
    }

    return std::nullopt;
}

std::optional<istring> find_first_modfile(const NormPath &norm_path) {
    //log_verbose("{}({})", __FUNCTION__, norm_path);
    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = fmt::format("{}/{}", dir, norm_path);
            if (std::filesystem::is_regular_file(mod_path)) {
                return path_to_actual_case(mod_path);
            }
        }
    }
    else {
        return find_first_cached_item(norm_path);
    }
    return std::nullopt;
}

std::optional<istring> find_first_modfolder(const NormPath &norm_path) {
    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = fmt::format("{}/{}", dir, norm_path);
            if (std::filesystem::is_directory(mod_path)) {
                return path_to_actual_case(mod_path) + "/";
            }
        }
    }
    else {
        return find_first_cached_item(norm_path / "");
    }
    return std::nullopt;
}

std::vector<istring> find_all_modfile(const NormPath &norm_path) {
    std::vector<istring> ret;

    if (config.developer_mode) {
        for (auto &dir : available_mods()) {
            auto mod_path = fmt::format("{}/{}", dir, norm_path);
            if (std::filesystem::is_regular_file(mod_path)) {
                ret.emplace_back(std::move(mod_path));
            }
        }
    }
    else {
        for (auto &dir : cached_mods) {
            auto file_search = dir.contents.find(norm_path);
            if (file_search == dir.contents.end()) {
                continue;
            }
            ret.emplace_back(fmt::format("{}/{}", dir.name, *file_search));
        }
    }
    // needed for consistency when hashing names
    std::sort(ret.begin(), ret.end());
    return ret;
}
