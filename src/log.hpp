#include "config.hpp" // since log_verbose uses it

#ifndef LOG_MODULE
#define LOG_MODULE "layeredfs"
#endif

// functions that default to file output, but will be overriden to point to AVS
// logging functions if the user doesn't specify their own log file
#define log_fatal(...) imp_log_body_fatal(LOG_MODULE, __VA_ARGS__)
#define log_warning(...) imp_log_body_warning(LOG_MODULE, __VA_ARGS__)
#define log_info(...) imp_log_body_info(LOG_MODULE, __VA_ARGS__)
#define log_misc(...) imp_log_body_misc(LOG_MODULE, __VA_ARGS__)
// layeredfs super-verbose (since most people have loglevel misc already)
#define log_verbose(...) if(config.verbose_logs) {log_misc(__VA_ARGS__);}

// for the playpen
void log_to_stdout(void);

typedef void (*log_formatter_t)(const char *module, const char *fmt, ...);

extern log_formatter_t imp_log_body_fatal;
extern log_formatter_t imp_log_body_warning;
extern log_formatter_t imp_log_body_info;
extern log_formatter_t imp_log_body_misc;
