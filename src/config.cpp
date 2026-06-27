#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <windows.h>
#include <shellapi.h>

#include "config.hpp"
#include "log.hpp"
#include "utils.hpp"

static constexpr std::string_view VERBOSE_FLAG    = "--layered-verbose";
static constexpr std::string_view DEVMODE_FLAG    = "--layered-devmode";
static constexpr std::string_view DISABLE_FLAG    = "--layered-disable";
static constexpr std::string_view ALLOWLIST_FLAG  = "--layered-allowlist=";
static constexpr std::string_view BLOCKLIST_FLAG  = "--layered-blocklist=";
static constexpr std::string_view LOGFILE_FLAG    = "--layered-logfile=";
static constexpr std::string_view MOD_FOLDER_FLAG = "--layered-data-mods-folder=";

config_t config;

static void parse_list(std::string_view arg, std::set<std::string, CaseInsensitiveCompare> &dest) {
    for (const auto el : std::views::split(arg, std::string_view(",")))
        dest.emplace(std::string_view(el));
}

void load_config() {
    config.disable = false;
    config.allowlist.clear();
    config.blocklist.clear();
    config.set_mod_folder(DEFAULT_MOD_FOLDER);

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
    config.logfile = std::nullopt;
#endif

    int argc;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        log_warning("Couldn't fetch commandline args!");
        return;
    }

    std::span args(argv, argv + argc);
    // so close to just pulling in a third party argparsing lib...
    for (auto warg : args) {
        // narrowing
        std::string _arg(warg, warg + wcslen(warg));
        std::string_view arg(_arg);

        if (arg == VERBOSE_FLAG) {
            config.verbose_logs = true;
        }
        else if (arg == DEVMODE_FLAG) {
            config.developer_mode = true;
        }
        else if (arg == DISABLE_FLAG) {
            config.disable = true;
        }
        else if (arg.starts_with(ALLOWLIST_FLAG)) {
            arg.remove_prefix(ALLOWLIST_FLAG.size());
            parse_list(arg, config.allowlist);
        }
        else if (arg.starts_with(BLOCKLIST_FLAG)) {
            arg.remove_prefix(BLOCKLIST_FLAG.size());
            parse_list(arg, config.blocklist);
        }
        else if (arg.starts_with(LOGFILE_FLAG)) {
            arg.remove_prefix(LOGFILE_FLAG.size());
            // correct format: --layered-logfile=whatever.log
            if(!arg.empty()) {
                config.logfile = arg;
            }
        }
        else if (arg.starts_with(MOD_FOLDER_FLAG)) {
            arg.remove_prefix(MOD_FOLDER_FLAG.size());
            // correct format: --layered-data-mods-folder=./my_mods
            if(arg.starts_with("./")) {
                config.set_mod_folder(std::string(arg));
            }
        }
    }
}

void print_config() {
    log_info("Options: {}={} {}={} {}={} {}{} {}{} {}{} {}{}",
        VERBOSE_FLAG, config.verbose_logs,
        DEVMODE_FLAG, config.developer_mode,
        DISABLE_FLAG, config.disable,
        LOGFILE_FLAG, config.logfile,
        ALLOWLIST_FLAG, config.allowlist,
        BLOCKLIST_FLAG, config.blocklist,
        MOD_FOLDER_FLAG, config.get_mod_folder().c_str()
    );
}
