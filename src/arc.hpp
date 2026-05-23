#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "utils.hpp"

class ArcArchive {
public:
    std::map<std::string, std::vector<uint8_t>, CaseInsensitiveCompare> files;

    static std::optional<ArcArchive> from_stream(std::istream &stream);
    void add_or_replace(std::string const& name, std::vector<uint8_t> data);
    bool save(const char* path);
};
