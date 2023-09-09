#pragma once

#include <windows.h>

#include <string>
#include <vector>

#define lenof(x) (sizeof(x) / sizeof(*x))

char* snprintf_auto(const char* fmt, ...);
int string_ends_with(const char * str, const char * suffix);
// case insensitive
void string_replace(std::string &str, const char* from, const char* to);
// // case insensitive
bool string_replace_first(std::string &str, const char* from, const char* to);
// Like string.find(), but case insensitive
std::size_t string_find_icase(const std::string & strHaystack, const std::string & strNeedle, std::size_t off = 0);
wchar_t *str_widen(const char *src);
void str_toupper_inline(std::string &str);
bool file_exists(const char* name);
bool folder_exists(const char* name);
std::vector<std::string> folders_in_folder(const char* root);
uint64_t file_time(const char* path);
LONG time(void);
std::string basename_without_extension(std::string const & path);

struct CaseInsensitiveCompare {
    bool operator() (const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
