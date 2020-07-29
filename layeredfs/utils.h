#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <string>
#include <vector>

#include "3rd_party/MinHook.h"

#include "config.hpp"

#define lenof(x) (sizeof(x) / sizeof(*x))

void logf(char* fmt, ...);
#define logf_verbose(...) if(config.verbose_logs) {logf(__VA_ARGS__);}

char* snprintf_auto(const char* fmt, ...);
int string_ends_with(const char * str, const char * suffix);
void string_replace(std::string &str, const char* from, const char* to);
wchar_t *str_widen(const char *src);
bool file_exists(const char* name);
bool folder_exists(const char* name);
std::vector<std::string> folders_in_folder(const char* root);
time_t file_time(const char* path);
LONG time(void);