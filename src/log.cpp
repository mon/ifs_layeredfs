#include <functional>
#include <mutex>
#include <stdio.h>

#include "config.hpp"
#include "log.hpp"

#define SUPPRESS_PRINTF

void stdout_log(char level, const char *fmt, va_list args) {
    printf("%c:", level);
    vprintf(fmt, args);
    printf("\n");
}

static void log_to_file(char level, const char* fmt, va_list args) {
    static std::mutex log_mutex;

#ifndef SUPPRESS_PRINTF
    stdout_log(level, fmt, args);
#endif

    // only try and open the logfile once; if the FS is readonly then retrying
    // every log call is slow as shit
    static FILE* logfile = std::invoke([]() {
        // default to ifs_hook.log because we need *something* in the case
        // of a fatal error
        return fopen(config.logfile.value_or(DEFAULT_LOGFILE).c_str(), "w");
    });

    if (logfile) {
        std::lock_guard lock(log_mutex);

        fprintf(logfile, "%c:", level);
        vfprintf(logfile, fmt, args);
        fprintf(logfile, "\n");

        if(config.developer_mode || level == 'F')
            fflush(logfile);
    }
}

void default_log_body_fatal(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_to_file('F', fmt, args);
    va_end(args);

    abort();
}
void default_log_body_warning(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_to_file('W', fmt, args);
    va_end(args);
}
void default_log_body_info(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_to_file('I', fmt, args);
    va_end(args);
}
void default_log_body_misc(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_to_file('M', fmt, args);
    va_end(args);
}

log_formatter_t imp_log_body_fatal = default_log_body_fatal;
log_formatter_t imp_log_body_warning = default_log_body_warning;
log_formatter_t imp_log_body_info = default_log_body_info;
log_formatter_t imp_log_body_misc = default_log_body_misc;

void stdout_log_body_fatal(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    stdout_log('F', fmt, args);
    va_end(args);

    abort();
}
void stdout_log_body_warning(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    stdout_log('W', fmt, args);
    va_end(args);
}
void stdout_log_body_info(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    stdout_log('I', fmt, args);
    va_end(args);
}
void stdout_log_body_misc(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    stdout_log('M', fmt, args);
    va_end(args);
}

void log_to_stdout(void) {
    imp_log_body_fatal = stdout_log_body_fatal;
    imp_log_body_warning = stdout_log_body_warning;
    imp_log_body_info = stdout_log_body_info;
    imp_log_body_misc = stdout_log_body_misc;
}
