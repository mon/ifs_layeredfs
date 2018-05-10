#include "stdafx.h"
#include "utils.h"
#include "avs.h"

FILE* logfile;

#define SUPPRESS_PRINTF

void logf(char* fmt, ...) {
	va_list args;
#ifndef SUPPRESS_PRINTF
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
#endif
	// reopened every time since logging is sparse and I want to see live updates
	fopen_s(&logfile, "ifs_hook.log", "a");
	if (logfile) {
		va_start(args, fmt);
		vfprintf(logfile, fmt, args);
		va_end(args);
		fprintf(logfile, "\n");
		fclose(logfile);
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
	int str_len = strlen(str);
	int suffix_len = strlen(suffix);

	return
		(str_len >= suffix_len) &&
		(0 == strcmp(str + (str_len - suffix_len), suffix));
}

void string_replace(std::string &str, const char* from, const char* to) {
	auto to_len = strlen(to);
	for (auto pos = str.find(from); pos != std::string::npos; pos = str.find(from)) {
		str.replace(pos, to_len, to);
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
	auto res = avs_fs_open(name, 1, 420);
	// yes, that extra comparison really is needed
	if (res > 0 && res != (AVS_FILE)name)
		avs_fs_close(res);
	return res > 0 && res != (AVS_FILE)name;
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