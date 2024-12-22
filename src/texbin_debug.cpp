#include <windows.h>

#include "log.hpp"
#include "texbin.hpp"

LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo);

int main(int argc, char** argv) {
    log_to_stdout();
    AddVectoredExceptionHandler(1, exc_handler);
    config.verbose_logs = true;

    log_info("texbin_debug, from IFS layeredFS v" VER_STRING);

    if(argc != 2) {
        log_fatal("Must provide a single .bin file to examine");
    }

    Texbin::from_path(argv[1]);

    return 0;
}

LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo) {
    fprintf(stderr, "Unhandled exception %lX\n", ExceptionInfo->ExceptionRecord->ExceptionCode);

    return EXCEPTION_CONTINUE_SEARCH;
}
