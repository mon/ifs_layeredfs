#include <stdio.h>

#include "config.hpp"
#include "log.hpp"
#include "winxp_mutex.hpp"

#define SUPPRESS_PRINTF

static void log_to_file(char level, const char* fmt, va_list args) {
    static CriticalSectionLock log_mutex;
    static FILE* logfile = NULL;
    static bool tried_to_open = false;
#ifndef SUPPRESS_PRINTF
    printf("%c:", level);
    vprintf(fmt, args);
    printf("\n");
#endif
    // don't reopen every time: slow as shit
    if (!tried_to_open) {
        log_mutex.lock();

        if (!logfile) {
            // default to ifs_hook.log because we need *something* in the case
            // of a fatal error
            const char *path = config.logfile ? config.logfile : "ifs_hook.log";
            fopen_s(&logfile, path, "w");
        }
        tried_to_open = true;
        log_mutex.unlock();
    }
    if (logfile) {
        log_mutex.lock();

        fprintf(logfile, "%c:", level);
        vfprintf(logfile, fmt, args);
        fprintf(logfile, "\n");

        if(config.developer_mode)
            fflush(logfile);

        log_mutex.unlock();
    }
}

void default_log_body_fatal(const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_to_file('F', fmt, args);
    va_end(args);
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
