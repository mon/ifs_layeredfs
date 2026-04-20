#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "utils.hpp"

struct ArcFile {
    std::vector<uint8_t> packed_data;
    uint32_t unpacked_size;
};

class ArcArchive {
public:
    std::map<std::string, ArcFile, CaseInsensitiveCompare> files;

    static std::optional<ArcArchive> from_stream(std::istream &stream);
    void add_or_replace(std::string const& name, std::vector<uint8_t> data);
    bool save(const char* path);
};
