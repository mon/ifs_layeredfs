#pragma once

#include <optional>
#include <set>
#include <string>

#include "utils.hpp"

struct config_t {
    bool verbose_logs = false;
    bool developer_mode = false;
    bool disable = false;
    std::optional<std::string> logfile;
    std::set<std::string, CaseInsensitiveCompare> allowlist;
    std::set<std::string, CaseInsensitiveCompare> blocklist;

    config_t();

    void set_mod_folder(std::string &&_mod_folder) {
        mod_folder = std::move(_mod_folder);
        mod_folder_native = mod_folder;
        string_replace_i(mod_folder_native, "/", "\\");

        cache_folder = mod_folder + "/_cache";
    }

    inline const std::string& get_mod_folder() const { return mod_folder; }
    inline const std::string& get_mod_folder_native() const { return mod_folder_native; }
    inline const std::string& get_cache_folder() const { return cache_folder; }

    private:
    std::string mod_folder;
    // for the GetLongPathNameA hook which uses backslashes
    std::string mod_folder_native;
    std::string cache_folder;
};

#define DEFAULT_LOGFILE "ifs_hook.log"

extern config_t config;

void load_config();
void print_config();
