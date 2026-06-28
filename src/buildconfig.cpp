#include "buildconfig.hpp"

const BuildConfig g_build_config {
#ifdef CFG_VERBOSE
    .verbose_logs = true,
#endif
#ifdef CFG_LOGFILE
    .external_logfile = true,
#endif
#ifdef CFG_DEVMODE
    .developer_mode = true,
#endif
#ifdef UNPAK
    .pkfs_unpack = true,
#endif
#ifdef SPECIAL_VER
    .special_ver = SPECIAL_VER,
#endif
};
