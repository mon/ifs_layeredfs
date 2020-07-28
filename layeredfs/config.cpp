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
    char *state = NULL;
    char* token = strtok_s(str, ",", &state);

    do {
        dest.insert(token);
        token = strtok_s(NULL, ",", &state);
    } while (token != NULL);

    free(str);
}

void load_config(void) {
    config.allowlist.clear();
    config.blocklist.clear();
    config.verbose_logs = false;
    config.developer_mode = false;
    config.disable = false;

    int i;

    // so close to just pulling in a third party argparsing lib...
    bool next_is_allowlist = false;
    bool next_is_blocklist = false;
    const char *allowlist = NULL;
    const char *blocklist = NULL;

    for (i = 0; i < __argc; i++) {
        if (next_is_allowlist) {
            next_is_allowlist = false;
            allowlist = __argv[i];
            comma_separated_to_set(config.allowlist, allowlist);
        }
        else if (next_is_blocklist) {
            next_is_blocklist = false;
            blocklist = __argv[i];
            comma_separated_to_set(config.blocklist, blocklist);
        }

        if (strcmp(__argv[i], VERBOSE_FLAG) == 0) {
            config.verbose_logs = true;
        }
        else if (strcmp(__argv[i], DEVMODE_FLAG) == 0) {
            config.developer_mode = true;
        }
        else if (strcmp(__argv[i], DISABLE_FLAG) == 0) {
            config.disable = true;
        }
        else if (strcmp(__argv[i], ALLOWLIST_FLAG) == 0) {
            next_is_allowlist = true;
        }
        else if (strcmp(__argv[i], BLOCKLIST_FLAG) == 0) {
            next_is_blocklist = true;
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