#include <chrono>
#include <filesystem>
#include <stdarg.h>
#include <windows.h>

#include <algorithm>

#include "utils.hpp"
#include "hook.h"

static bool char_eq_i(unsigned char ch1, unsigned char ch2) {
    return std::toupper(ch1) == std::toupper(ch2);
}

bool string_ends_with_i(std::string_view str, std::string_view suffix) {
    return
        (str.size() >= suffix.size()) &&
        std::ranges::equal(str.substr(str.size() - suffix.size()), suffix, char_eq_i);
}

void string_replace_i(std::string &str, std::string_view from, std::string_view to) {
    // two cases:
    // recursively nuke all double slashes until we are left with one
    //   '//' does not exist in '/', so we just move through the string
    // replace '.xml' with '.merged.xml'
    //   .xml exists in .merged.xml, if we start at the previous match we will
    //   replace forever. In this case, we skip the replaced string
    size_t increment = to.size();
    if(string_find_i(to, from) == std::string::npos) {
        increment = 0;
    }

    size_t offset = 0;
    for (auto pos = string_find_i(str, from); pos != std::string::npos; pos = string_find_i(str, from, offset)) {
        str.replace(pos, from.size(), to);
        // avoid recursion if to contains from
        offset = pos + increment;
    }
}

bool string_replace_first_i(std::string& str, std::string_view from, std::string_view to) {
    auto pos = string_find_i(str, from);
    if (pos == std::string::npos) {
        return false;
    }

    str.replace(pos, from.size(), to);

    return true;
}

std::size_t string_find_i(std::string_view strHaystack, std::string_view strNeedle, std::size_t off) {
    auto it = std::search(
        strHaystack.begin() + off, strHaystack.end(),
        strNeedle.begin(),   strNeedle.end(),
        char_eq_i
    );
    if(it == strHaystack.end()) {
        return std::string::npos;
    } else {
        return it - strHaystack.begin();
    }
}

std::string path_to_actual_case(std::string path) {
    WIN32_FIND_DATAA ffd;
    HANDLE hFind;
    size_t start = std::string::npos;

    while((start = path.rfind('/', start)) != std::string::npos) {
        hFind = FindFirstFileA(path.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            continue;
        }
        FindClose(hFind);

        auto segment = &path[start+1];
        size_t segment_len = strlen(segment);
        if(strcasecmp(segment, ffd.cFileName) == 0)
            memcpy(segment, ffd.cFileName, segment_len);

        path[start] = '\0';
    }

    // restore slashes
    for(auto& c: path) {
        if(c == '\0')
            c = '/';
    }

    return path;
}

void str_toupper_inline(std::string& str) {
    for (size_t i = 0; i < str.length(); i++) {
        str[i] = toupper(str[i]);
    }
}

std::vector<std::string> folders_in_folder(const std::filesystem::path &root) {
    std::vector<std::string> results;

    for (const auto& child : std::filesystem::directory_iterator(root)) {
        if (child.is_directory()) {
            results.emplace_back(child.path().filename().string());
        }
    }

    return results;
}

bool mkdir_p(const std::filesystem::path &path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::filesystem::file_time_type file_time(const std::filesystem::path &path) {
    try {
        return std::filesystem::last_write_time(path);
    } catch(const std::filesystem::filesystem_error&) {
        return std::filesystem::file_time_type::min();
    }
}

CacheHasher::CacheHasher(std::string hash_file): hash_file(hash_file) {
    // always hash the DLL time
    digest.add(&dll_time, sizeof(dll_time));

    auto cache_hashfile = fopen(hash_file.c_str(), "rb");
    if (cache_hashfile) {
        fread(existing_hash, 1, MD5::HashBytes, cache_hashfile);
        fclose(cache_hashfile);
    }
}

void CacheHasher::add(const std::string &path) {
    digest.add(path.c_str(), path.length());

    auto ts = file_time(path.c_str());
    digest.add(&ts, sizeof(ts));
}

void CacheHasher::finish() {
    digest.getHash(new_hash);
}

bool CacheHasher::matches() {
    return memcmp(new_hash, existing_hash, sizeof(new_hash)) == 0;
}

void CacheHasher::commit() {
    auto cache_hashfile = fopen(hash_file.c_str(), "wb");
    if (cache_hashfile) {
        fwrite(new_hash, 1, sizeof(new_hash), cache_hashfile);
        fclose(cache_hashfile);
    }
}


Timer::Timer(): start(std::chrono::steady_clock::now()) {}

std::chrono::milliseconds Timer::elapsed() {
    auto delta = std::chrono::steady_clock::now() - start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(delta);
}
