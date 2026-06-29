#pragma once

#include <windows.h>

#include <stdint.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include "3rd_party/md5.h"

#include "fmt/format.h"

// every in-game internal path uses ascii chars, so this is fine
inline char toupper_ascii(char c) {
    if (c >= 'a' && c <= 'z')
        c -= 0x20;
    return c;
}

struct char_traits_insensitive : public std::char_traits<char> {
    static bool eq(char c1, char c2) { return toupper_ascii(c1) == toupper_ascii(c2); }
    static bool ne(char c1, char c2) { return toupper_ascii(c1) != toupper_ascii(c2); }
    static bool lt(char c1, char c2) { return toupper_ascii(c1) < toupper_ascii(c2); }
    static int compare(const char* s1, const char* s2, size_t n) { return _memicmp(s1, s2, n); }

    static const char* find(const char* s, std::size_t n, char a) {
        const auto ua{toupper_ascii(a)};
        while (n-- != 0) {
            if (toupper_ascii(*s) == ua)
                return s;
            s++;
        }
        return nullptr;
    }
};

using istring_view = std::basic_string_view<char, char_traits_insensitive>;

class istring : public std::basic_string<char, char_traits_insensitive> {
    using base = std::basic_string<char, char_traits_insensitive>;

    using base::base;

  public:
    // needed so the inherited methods (substr etc) can be cast back
    istring(base&& s)
        : base(std::move(s)) {}

    istring(std::string&& s)
        : base(std::move(reinterpret_cast<base&&>(s))) {}
    istring(std::string_view s)
        : base(s.data(), s.size()) {}

    operator std::filesystem::path() const {
        return std::filesystem::path(data(), data() + size());
    }

    // this should be a deliberate choice
    inline std::string downcast_string() const { return std::string(data(), size()); }
    inline const std::string& downcast_ref() const {
        const istring& self = *this;
        return reinterpret_cast<const std::string&>(self);
    }

    // joining paths is common enough

    istring operator/(const char* component) const {
        return istring(fmt::format("{}/{}", downcast_ref(), component));
    }
    istring operator/(std::string_view component) const {
        return istring(fmt::format("{}/{}", downcast_ref(), component));
    }
    istring operator/(istring_view component) const {
        return istring(fmt::format("{}/{}", downcast_ref(), component));
    }

    // as is replacing original names with their modded equivalents

    void replace_suffix(istring_view from, istring_view to) {
        if (!ends_with(from))
            return;

        replace(end() - from.size(), end(), to);
    }

    void replace_suffix_foreach_path_component(istring_view from, istring_view to) {
        // requirement to use the nice views thing
        assert(from.size() == to.size());

        for (auto el : std::views::split(*this, std::string_view("/"))) {
            istring_view segment(el);
            if (!segment.ends_with(from))
                continue;

            std::copy(to.begin(), to.end(), el.end() - from.size());
        }
    }

    void replace_all(istring_view from, istring_view to) {
        size_t off;
        while ((off = find(from)) != npos)
            replace(off, from.size(), to);
    }
};

// needed when an istring is inside another container
template <>
struct fmt::formatter<istring> : fmt::formatter<std::string_view> {
    auto format(const istring& s, format_context& ctx) const {
        return fmt::formatter<std::string_view>::format({s.data(), s.size()}, ctx);
    }
};

template <>
struct std::hash<istring> {
    std::size_t operator()(const istring& s) const noexcept {
        // a probably terrible attempt to somewhat-copy murmurhash
        constexpr size_t m = 0x5bd1e995;
        size_t h           = s.size();
        for (auto c : s) {
            h ^= toupper_ascii(c);
            h *= m;
            h ^= h >> 15;
        }
        return h;
    }
};

// the given path:
// -  must be known to exist
// - must start with "/" or "./"
// - must not end with "/"
std::string path_to_actual_case(std::string path);
std::vector<istring> folders_in_folder(const std::filesystem::path& root);
bool mkdir_p(const std::filesystem::path& path);
std::filesystem::file_time_type file_time(const std::filesystem::path& path);
bool write_bytes(const std::filesystem::path& path, std::span<const char> bytes);
inline bool write_bytes(const std::filesystem::path& path, std::span<const uint8_t> bytes) {
    return write_bytes(path, {reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

// Hashes the names and timestamps of input files into a rebuilt output.
// Invalidates on DLL timestamp change, input timestamp change, or input change
class CacheHasher {
  public:
    CacheHasher(std::filesystem::path hash_file);
    // add a path and its timestamp to the hash. Should not be called after `finish`
    void add(istring_view path);
    // complete the hashing op
    void finish();
    // check if the hashfile matches
    bool matches();
    // write out an updated hashfile. Should be called after `finish`
    void commit();

  private:
    std::filesystem::path hash_file;
    MD5 digest;
    std::array<uint8_t, MD5::HashBytes> existing_hash;
    std::array<uint8_t, MD5::HashBytes> new_hash;
};

class Timer {
  public:
    Timer();
    std::chrono::milliseconds elapsed();

  private:
    std::chrono::steady_clock::time_point start;
};

class ScopeGuard {
  public:
    explicit ScopeGuard(std::function<void()>&& cb)
        : cb(std::move(cb)) {}
    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard(ScopeGuard&&)                 = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&)      = delete;

    ~ScopeGuard() {
        if (cb)
            cb();
    }

    void cancel() { cb = nullptr; }

  private:
    std::function<void()> cb;
};
