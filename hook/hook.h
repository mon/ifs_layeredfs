#pragma once

#include "avs.h"

int hook_avs_fs_fstat(int flags, struct avs_stat *st);
AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags);

extern "C" {
	__declspec(dllexport) int init(void);
	extern HMODULE my_module;
}