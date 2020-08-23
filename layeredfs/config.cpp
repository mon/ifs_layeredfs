#include <string.h>
#include <windows.h>
#include <shellapi.h>

#include "config.hpp"
#include "utils.h"

#define VERBOSE_FLAG   "--layered-verbose"
#define DEVMODE_FLAG   "--layered-devmode"
#define DISABLE_FLAG   "--layered-disable"
#define ALLOWLIST_FLAG "--layered-allowlist"
#define BLOCKLIST_FLAG "--layered-blocklist"

config_t config;

void comma_separated_to_set(std::unordered_set<std::string> &dest, const char* arg) {
    char *str = _strdup(arg);
    str_tolower_inline(str); // ignore case for filenames
    char *state = NULL;
    char* token = strtok_s(str, ",", &state);

    do {
        dest.insert(token);
        token = strtok_s(NULL, ",", &state);
    } while (token != NULL);

    free(str);
}

const char* parse_list(const char* prefix, const char* arg, std::unordered_set<std::string> &dest) {
    size_t prefix_len = strlen(prefix) + strlen("=");
    if (strlen(arg) <= prefix_len) {
        return NULL;
    }

    const char* list_start = &arg[prefix_len];

    comma_separated_to_set(dest, list_start);

    return list_start;
}

void load_config(void) {
    config.allowlist.clear();
    config.blocklist.clear();
    config.verbose_logs = false;
    config.developer_mode = false;
    config.disable = false;

    int i;

    // so close to just pulling in a third party argparsing lib...
    const char *allowlist = NULL;
    const char *blocklist = NULL;

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
    }

    logf("Options: %s=%d %s=%d %s=%d %s=%s %s=%s",
        VERBOSE_FLAG, config.verbose_logs,
        DEVMODE_FLAG, config.developer_mode,
        DISABLE_FLAG, config.disable,
        ALLOWLIST_FLAG, allowlist,
        BLOCKLIST_FLAG, blocklist
    );
}