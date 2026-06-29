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
    std::set<istring> allowlist;
    std::set<istring> blocklist;

    config_t();

    void set_mod_folder(istring &&_mod_folder) {
        mod_folder = std::move(_mod_folder);
        mod_folder_native = mod_folder;
        mod_folder_native.replace_all("/", "\\");

        cache_folder = mod_folder / "_cache";
    }

    inline const istring& get_mod_folder() const { return mod_folder; }
    inline const istring& get_mod_folder_native() const { return mod_folder_native; }
    inline const istring& get_cache_folder() const { return cache_folder; }

    private:
    istring mod_folder;
    // for the GetLongPathNameA hook which uses backslashes
    istring mod_folder_native;
    istring cache_folder;
};

#define DEFAULT_LOGFILE "ifs_hook.log"

extern config_t config;

void load_config();
void print_config();
