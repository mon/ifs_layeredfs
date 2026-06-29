#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdarg.h>
#include <windows.h>

#include "utils.hpp"
#include "hook.h"

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
        if(_stricmp(segment, ffd.cFileName) == 0)
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

std::vector<istring> folders_in_folder(const std::filesystem::path &root) {
    std::vector<istring> results;

    std::error_code err;
    for (const auto& child : std::filesystem::directory_iterator(root, err)) {
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

bool write_bytes(const std::filesystem::path &path, std::span<const char> bytes) {
    std::ofstream f(path, std::ios::binary);
    if(!f) return false;

    f.write(bytes.data(), bytes.size());

    return true;
}

CacheHasher::CacheHasher(std::filesystem::path _hash_file): hash_file(std::move(_hash_file)) {
    // always hash the DLL time
    digest.add(&dll_time, sizeof(dll_time));

    std::ifstream cache_hashfile(hash_file.c_str(), std::ios::binary);
    if (cache_hashfile) {
        cache_hashfile.read(reinterpret_cast<char*>(existing_hash.data()), MD5::HashBytes);
    }
}

void CacheHasher::add(istring_view path) {
    digest.add(path.data(), path.size());

    auto ts = file_time(path);
    digest.add(&ts, sizeof(ts));
}

void CacheHasher::finish() {
    digest.getHash(new_hash.data());
}

bool CacheHasher::matches() {
    return new_hash == existing_hash;
}

void CacheHasher::commit() {
    write_bytes(hash_file, new_hash);
}


Timer::Timer(): start(std::chrono::steady_clock::now()) {}

std::chrono::milliseconds Timer::elapsed() {
    auto delta = std::chrono::steady_clock::now() - start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(delta);
}
