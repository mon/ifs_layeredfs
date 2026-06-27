#pragma once

#include <windows.h>
#include <stdint.h>

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "3rd_party/md5.h"

#define strcasecmp _stricmp

bool string_ends_with_i(std::string_view str, std::string_view suffix);
// case insensitive
void string_replace_i(std::string &str, std::string_view from, std::string_view to);
// // case insensitive
bool string_replace_first_i(std::string &str, std::string_view from, std::string_view to);
// Like string.find(), but case insensitive
std::size_t string_find_i(std::string_view strHaystack, std::string_view strNeedle, std::size_t off = 0);
wchar_t *str_widen(const char *src);
void str_toupper_inline(std::string &str);
bool file_exists(const char* name);
bool folder_exists(const char* name);
// the given path:
// -  must be known to exist
// - must start with "/" or "./"
// - must not end with "/"
std::string path_to_actual_case(std::string path);
std::vector<std::string> folders_in_folder(const char* root);
std::filesystem::file_time_type file_time(const std::filesystem::path &path);
LONG time(void);
std::string basename_without_extension(std::string const & path);

// Hashes the names and timestamps of input files into a rebuilt output.
// Invalidates on DLL timestamp change, input timestamp change, or input change
class CacheHasher {
    public:
    CacheHasher(std::string hash_file);
    // add a path and its timestamp to the hash. Should not be called after `finish`
    void add(const std::string &path);
    // complete the hashing op
    void finish();
    // check if the hashfile matches
    bool matches();
    // write out an updated hashfile. Should be called after `finish`
    void commit();

    private:
    std::string hash_file;
    MD5 digest;
    uint8_t existing_hash[MD5::HashBytes] = {0};
    uint8_t new_hash[MD5::HashBytes] = {0};
};

struct CaseInsensitiveCompare {
    bool operator() (const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};

typedef std::unordered_set<std::string> string_set;
