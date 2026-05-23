#pragma once

#include <set>
#include <string>

#include "src/utils.hpp"
#include "utils.hpp"

struct config_t {
    bool verbose_logs;
    bool developer_mode;
    bool disable;
    const char *logfile;
    std::set<std::string, CaseInsensitiveCompare> allowlist;
    std::set<std::string, CaseInsensitiveCompare> blocklist;

    void set_mod_folder(std::string _mod_folder) {
        mod_folder = _mod_folder;
        mod_folder_native = mod_folder;
        string_replace(mod_folder_native, "/", "\\");
    }

    const std::string& get_mod_folder() {
        return mod_folder;
    }

    const std::string& get_mod_folder_native() {
        return mod_folder_native;
    }

    private:
    std::string mod_folder;
    // for the GetLongPathNameA hook which uses backslashes
    std::string mod_folder_native;
};

#define DEFAULT_LOGFILE "ifs_hook.log"
#define DEFAULT_MOD_FOLDER "./data_mods"

#define CACHE_FOLDER (config.get_mod_folder() + "/_cache")

extern config_t config;

void load_config(void);
void print_config(void);
