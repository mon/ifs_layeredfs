#pragma once

#include <chrono>
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
void str_toupper_inline(std::string &str);
// the given path:
// -  must be known to exist
// - must start with "/" or "./"
// - must not end with "/"
std::string path_to_actual_case(std::string path);
std::vector<std::string> folders_in_folder(const std::filesystem::path &root);
bool mkdir_p(const std::filesystem::path &path);
std::filesystem::file_time_type file_time(const std::filesystem::path &path);

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

class Timer {
    public:
    Timer();
    std::chrono::milliseconds elapsed();

    private:
    std::chrono::steady_clock::time_point start;
};

struct CaseInsensitiveCompare {
    bool operator() (const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};

typedef std::unordered_set<std::string> string_set;
