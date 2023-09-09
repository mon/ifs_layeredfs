#include <string.h>
#include <windows.h>
#include <shellapi.h>

#include "config.hpp"
#include "log.hpp"
#include "utils.hpp"

#define VERBOSE_FLAG   "--layered-verbose"
#define DEVMODE_FLAG   "--layered-devmode"
#define DISABLE_FLAG   "--layered-disable"
#define ALLOWLIST_FLAG "--layered-allowlist"
#define BLOCKLIST_FLAG "--layered-blocklist"
#define LOGFILE_FLAG   "--layered-logfile"

config_t config;

// so we can just print the exact arg
static const char *allowlist = NULL;
static const char *blocklist = NULL;

void comma_separated_to_set(std::set<std::string, CaseInsensitiveCompare> &dest, const std::string &arg) {
    size_t last = 0;
    size_t next = 0;
    while ((next = arg.find(",", last)) != std::string::npos) {
        dest.insert(arg.substr(last, next-last));
        last = next + 1;
    }
    dest.insert(arg.substr(last));
}

const char* parse_list(const char* prefix, const char* arg, std::set<std::string, CaseInsensitiveCompare> &dest) {
    size_t prefix_len = strlen(prefix) + strlen("=");
    if (strlen(arg) <= prefix_len) {
        return NULL;
    }

    const char* list_start = &arg[prefix_len];

    comma_separated_to_set(dest, std::string(list_start));

    return list_start;
}

void load_config(void) {
    config.disable = false;
    config.allowlist.clear();
    config.blocklist.clear();

#ifdef CFG_VERBOSE
    config.verbose_logs = true;
#else
    config.verbose_logs = false;
#endif

#ifdef CFG_DEVMODE
    config.developer_mode = true;
#else
    config.developer_mode = false;
#endif

#ifdef CFG_LOGFILE
    config.logfile = DEFAULT_LOGFILE;
#else
    config.logfile = NULL;
#endif

    int i;

    // so close to just pulling in a third party argparsing lib...
    for (i = 0; i < __argc; i++) {
        if (strcmp(__argv[i], VERBOSE_FLAG) == 0) {
            config.verbose_logs = true;
        }
        else if (strcmp(__argv[i], DEVMODE_FLAG) == 0) {
            config.developer_mode = true;
        }
        else if (strcmp(__argv[i], DISABLE_FLAG) == 0) {
            config.disable = true;
        }
        else if (strncmp(__argv[i], ALLOWLIST_FLAG, strlen(ALLOWLIST_FLAG)) == 0) {
            allowlist = parse_list(ALLOWLIST_FLAG, __argv[i], config.allowlist);
        }
        else if (strncmp(__argv[i], BLOCKLIST_FLAG, strlen(BLOCKLIST_FLAG)) == 0) {
            blocklist = parse_list(BLOCKLIST_FLAG, __argv[i], config.blocklist);
        }
        else if (strncmp(__argv[i], LOGFILE_FLAG, strlen(LOGFILE_FLAG)) == 0) {
            const char *path = &__argv[i][strlen(LOGFILE_FLAG)];
            // correct format: --layered-logfile=whatever.log
            if(path[0] == '=' && path[1]) {
                config.logfile = &path[1];
            }
        }
    }
}

void print_config(void) {
    log_info("Options: %s=%d %s=%d %s=%d %s=%s %s=%s %s=%s",
        VERBOSE_FLAG, config.verbose_logs,
        DEVMODE_FLAG, config.developer_mode,
        DISABLE_FLAG, config.disable,
        LOGFILE_FLAG, config.logfile,
        ALLOWLIST_FLAG, allowlist,
        BLOCKLIST_FLAG, blocklist
    );
}
