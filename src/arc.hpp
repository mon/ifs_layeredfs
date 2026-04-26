#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "utils.hpp"

#pragma pack(push, 1)
struct ArcHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t filecount;
    uint32_t compression;
};

struct ArcEntry {
    uint32_t str_offset;
    uint32_t file_offset;
    uint32_t unpacked_size;
    uint32_t packed_size;
};
#pragma pack(pop)

constexpr uint32_t ARC_MAGIC             = 0x19751120;
constexpr uint32_t ARC_VERSION           = 1;
constexpr uint32_t ARC_COMPRESSION_NONE  = 0;
constexpr uint32_t ARC_COMPRESSION_AVSLZ = 2;

// Holds inner-file bytes uncompressed. We always emit uncompressed arcs from
// repacks so the ramfs demangler can find inner .ifs files at known offsets.
class ArcArchive {
public:
    std::map<std::string, std::vector<uint8_t>, CaseInsensitiveCompare> files;

    static std::optional<ArcArchive> from_stream(std::istream &stream);
    void add_or_replace(std::string const& name, std::vector<uint8_t> data);
    bool save(const char* path);
};
