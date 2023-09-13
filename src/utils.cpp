#include <stdarg.h>
#include <windows.h>

#include <algorithm>

#include "utils.hpp"
#include "log.hpp"
#include "avs.h"

char* snprintf_auto(const char* fmt, ...) {
    va_list argList;

    va_start(argList, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, argList);
    auto s = (char*)malloc(len + 1);
    vsnprintf(s, len + 1, fmt, argList);
    va_end(argList);
    return s;
}

bool string_ends_with(const char * str, const char * suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    return
        (str_len >= suffix_len) &&
        (0 == _stricmp(str + (str_len - suffix_len), suffix));
}

bool string_ends_with(const std::string &str, const char * suffix) {
    return string_ends_with(str.c_str(), suffix);
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
    if(string_find_icase(to, from) == std::string::npos) {
        increment = 0;
    }

    size_t offset = 0;
    for (auto pos = string_find_icase(str, from); pos != std::string::npos; pos = string_find_icase(str, from, offset)) {
        str.replace(pos, from_len, to);
        // avoid recursion if to contains from
        offset = pos + increment;
    }
}

bool string_replace_first(std::string& str, const char* from, const char* to) {
    auto pos = string_find_icase(str, from);
    if (pos == std::string::npos) {
        return false;
    }

    str.replace(pos, strlen(from), to);

    return true;
}

std::size_t string_find_icase(const std::string & strHaystack, const std::string & strNeedle, std::size_t off) {
    auto it = std::search(
        strHaystack.begin() + off, strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),
        [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    if(it == strHaystack.end()) {
        return string::npos;
    } else {
        return it - strHaystack.begin();
    }
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
    // file_exists is only used by the modfile machinery, so use the easy
    // method, not avs_fs_open or avs_fs_lstat
    DWORD dwAttrib = GetFileAttributesA(name);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
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

void str_toupper_inline(std::string& str) {
    for (size_t i = 0; i < str.length(); i++) {
        str[i] = toupper(str[i]);
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

        results.push_back(ffd.cFileName);
    } while (FindNextFileA(contents, &ffd) != 0);

    FindClose(contents);

    return results;
}

uint64_t file_time(const char* path) {
    auto wide = str_widen(path);
    auto hFile = CreateFileW(wide,  // file to open
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

    ULARGE_INTEGER result;
    result.LowPart = mtime.dwLowDateTime;
    result.HighPart = mtime.dwHighDateTime;
    // log_verbose("file time %lu for %s", result.QuadPart, path);
    return result.QuadPart;

    // NOTE: can't use this method because the DLL time is taken before AVS is
    // hooked

    // struct avs_stat st;
    // auto res = avs_fs_lstat(path, &st);
    // if (res) {
    //     log_verbose("file time %ld for %s", st.st_mtime, path);
    //     return st.st_mtime;
    // } else {
    //     return 0;
    // }
}

LONG time(void) {
    SYSTEMTIME time;
    GetSystemTime(&time);
    return (time.wSecond * 1000) + time.wMilliseconds;
}

string basename_without_extension(string const & path) {
    auto basename = path.substr(path.find_last_of("/\\") + 1);
    string::size_type const p(basename.find_last_of('.'));
    return p > 0 && p != string::npos ? basename.substr(0, p) : basename;
}

void hash_filenames(std::vector<std::string> &filenames, uint8_t hash[MD5_LEN]) {
    auto digest = mdigest_create(MD5);

    for (auto &path : filenames) {
        mdigest_update(digest, path.c_str(), (int)path.length());
    }

    mdigest_finish(digest, hash, MD5_LEN);
    mdigest_destroy(digest);
}
