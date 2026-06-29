#pragma once

#include <optional>
#include <string>

struct BuildConfig {
    bool verbose_logs     = false;
    bool external_logfile = false;
    bool developer_mode   = false;
    bool pkfs_unpack      = false;
    std::optional<std::string> special_ver;
};

extern const BuildConfig g_build_config;
