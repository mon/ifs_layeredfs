#pragma once

#include <windows.h>

#include <string>
#include <vector>

#define lenof(x) (sizeof(x) / sizeof(*x))

char* snprintf_auto(const char* fmt, ...);
int string_ends_with(const char * str, const char * suffix);
void string_replace(std::string &str, const char* from, const char* to);
bool string_replace_first(std::string &str, const char* from, const char* to);
wchar_t *str_widen(const char *src);
void str_tolower_inline(char* str);
void str_tolower_inline(std::string &str);
void str_toupper_inline(std::string &str);
bool file_exists(const char* name);
bool folder_exists(const char* name);
std::vector<std::string> folders_in_folder(const char* root);
uint64_t file_time(const char* path);
LONG time(void);
std::string basename_without_extension(std::string const & path);
