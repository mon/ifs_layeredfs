#include "utils.h"
#include "avs.h"
#include "winxp_mutex.hpp"

#define SUPPRESS_PRINTF

void logf(char* fmt, ...) {
    static CriticalSectionLock log_mutex;
    static FILE* logfile = NULL;
    static bool tried_to_open = false;
    va_list args;
#ifndef SUPPRESS_PRINTF
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
#endif
    // don't reopen every time: slow as shit
    if (!tried_to_open) {
        log_mutex.lock();

        if (!logfile) {
            fopen_s(&logfile, "ifs_hook.log", "w");
        }
        tried_to_open = true;
        log_mutex.unlock();
    }
    if (logfile) {
        log_mutex.lock();

        va_start(args, fmt);
        vfprintf(logfile, fmt, args);
        va_end(args);
        fprintf(logfile, "\n");

        if(config.developer_mode)
            fflush(logfile);

        log_mutex.unlock();
    }
}

char* snprintf_auto(const char* fmt, ...) {
    va_list argList;

    va_start(argList, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, argList);
    auto s = (char*)malloc(len + 1);
    vsnprintf(s, len + 1, fmt, argList);
    va_end(argList);
    return s;
}

int string_ends_with(const char * str, const char * suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    return
        (str_len >= suffix_len) &&
        (0 == _stricmp(str + (str_len - suffix_len), suffix));
}

void string_replace(std::string &str, const char* from, const char* to) {
    auto to_len = strlen(to);
    auto from_len = strlen(from);

    // two cases:
    // recursively nuke all double slashes until we are left with one
    //   '//' does not exist in '/', so we just move through the string
    // replace '.xml' with '.merged.xml'
    //   .xml exists in .merged.xml, if we start at the previous match we will
    //   replace forever. In this case, we skip the replaced string
    size_t increment = to_len;
    if(strstr(to, from) == NULL) {
        increment = 0;
    }

    size_t offset = 0;
    for (auto pos = str.find(from); pos != std::string::npos; pos = str.find(from, offset)) {
        str.replace(pos, from_len, to);
        // avoid recursion if to contains from
        offset = pos + increment;
    }
}

bool string_replace_first(std::string& str, const char* from, const char* to) {
    auto pos = str.find(from);
    if (pos == std::string::npos) {
        return false;
    }

    str.replace(pos, strlen(from), to);

    return true;
}

wchar_t *str_widen(const char *src)
{
    int nchars;
    wchar_t *result;

    nchars = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);

    if (!nchars) {
        abort();
    }

    result = (wchar_t*)malloc(nchars * sizeof(wchar_t));

    if (!MultiByteToWideChar(CP_ACP, 0, src, -1, result, nchars)) {
        abort();
    }

    return result;
}

bool file_exists(const char* name) {
    auto res = avs_fs_open(name, 1, 420);
    if (res > 0)
        avs_fs_close(res);
    return res > 0;
}

bool folder_exists(const char* name) {
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(name, &ffd);

    if (hFind == INVALID_HANDLE_VALUE || !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }
    FindClose(hFind);
    return true;
}

void str_tolower_inline(char* str) {
    while (*str) {
        *str++ = tolower(*str);
    }
}

void str_tolower_inline(std::string& str) {
    for (size_t i = 0; i < str.length(); i++) {
        str[i] = tolower(str[i]);
    }
}

std::vector<std::string> folders_in_folder(const char* root) {
    std::vector<std::string> results;
    WIN32_FIND_DATAA ffd;
    std::string wildcard = root;
    wildcard += "/*";

    auto contents = FindFirstFileA(wildcard.c_str(), &ffd);
    if (contents == INVALID_HANDLE_VALUE) {
        return results;
    }

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            !strcmp(ffd.cFileName, ".") ||
            !strcmp(ffd.cFileName, "..")) {
            continue;
        }

        // all lowercase
        str_tolower_inline(ffd.cFileName);

        results.push_back(ffd.cFileName);
    } while (FindNextFileA(contents, &ffd) != 0);

    FindClose(contents);

    return results;
}

time_t file_time(const char* path) {
    auto wide = str_widen(path);
    auto hFile = CreateFile(wide,  // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
        NULL);                 // no attr. template
    free(wide);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    FILETIME mtime;
    GetFileTime(hFile, NULL, NULL, &mtime);
    CloseHandle(hFile);

    /*struct avs_stat st;
    auto f = avs_fs_open(path, 1, 420);
    if (f < 0) {
        return 0;
    }
    auto res = avs_fs_fstat(f, &st);
    avs_fs_close(f);
    if (!res) {
        return 0;
    }*/
    ULARGE_INTEGER result;
    result.LowPart = mtime.dwLowDateTime;
    result.HighPart = mtime.dwHighDateTime;
    return result.QuadPart;
}

LONG time(void) {
    SYSTEMTIME time;
    GetSystemTime(&time);
    return (time.wSecond * 1000) + time.wMilliseconds;
}
