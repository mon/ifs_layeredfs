#pragma once

#include <set>
#include <string>

#include "utils.hpp"

typedef struct {
    bool verbose_logs;
    bool developer_mode;
    bool disable;
    const char *logfile;
    std::set<std::string, CaseInsensitiveCompare> allowlist;
    std::set<std::string, CaseInsensitiveCompare> blocklist;
    std::string mod_folder;
} config_t;

#define DEFAULT_LOGFILE "ifs_hook.log"
#define DEFAULT_MOD_FOLDER "./data_mods"

#define CACHE_FOLDER (config.mod_folder + "/_cache")

extern config_t config;

void load_config(void);
void print_config(void);
