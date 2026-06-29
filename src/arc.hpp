#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <optional>
#include <vector>

#include "utils.hpp"

class ArcArchive {
public:
    std::map<istring, std::vector<uint8_t>> files;

    static std::optional<ArcArchive> from_stream(std::istream &stream);
    void add_or_replace(istring const& name, std::vector<uint8_t> data);
    bool save(const char* path);
};
