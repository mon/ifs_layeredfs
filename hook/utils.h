#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <string>

#include "MinHook.h"

#define lenof(x) (sizeof(x) / sizeof(*x))

extern FILE* logfile;
void logf(char* fmt, ...);
char* snprintf_auto(const char* fmt, ...);
int string_ends_with(const char * str, const char * suffix);
void string_replace(std::string &str, const char* to, const char* from);
wchar_t *str_widen(const char *src);
bool file_exists(const char* name);
bool folder_exists(const char* name);
time_t file_time(const char* path);
LONG time(void);