#pragma once

#include "config.hpp" // since log_verbose uses it
#include "fmt/format.h"
#include "fmt/std.h"
#include "fmt/xchar.h"

#ifndef LOG_MODULE
#define LOG_MODULE "layeredfs"
#endif

inline constexpr char _log_module[] = LOG_MODULE;

using log_formatter_t = void (*)(const char *module, const char *fmt, ...);

template<log_formatter_t* Logger, const char *Module, class... Args>
void log_base(fmt::format_string<Args...> fmt, Args&&... args) {
    (*Logger)(Module, "%s", fmt::format(fmt, std::forward<Args>(args)...).c_str());
}


// functions that default to file output, but will be overriden to point to AVS
// logging functions if the user doesn't specify their own log file
extern log_formatter_t imp_log_body_fatal;
extern log_formatter_t imp_log_body_warning;
extern log_formatter_t imp_log_body_info;
extern log_formatter_t imp_log_body_misc;

#define log_fatal(...) log_base<&imp_log_body_fatal, _log_module>(__VA_ARGS__);
#define log_warning(...) log_base<&imp_log_body_warning, _log_module>(__VA_ARGS__);
#define log_info(...) log_base<&imp_log_body_info, _log_module>(__VA_ARGS__);
#define log_misc(...) log_base<&imp_log_body_misc, _log_module>(__VA_ARGS__);
// layeredfs super-verbose (since most people have loglevel misc already)
#define log_verbose(...) if(config.verbose_logs) {log_misc(__VA_ARGS__);}

// for the playpen
void log_to_stdout();
